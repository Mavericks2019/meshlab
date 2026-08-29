#include "dchartsalgorithm.h"

#include "meshutils/my_traits.h"

#include <Eigen/Eigenvalues>
#include <OpenMesh/Core/IO/MeshIO.hh>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <numeric>
#include <queue>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace {
constexpr double kEpsilon = 1e-12;
constexpr double kInfinity = std::numeric_limits<double>::infinity();

struct FaceEdge {
    int v0 = -1;
    int v1 = -1;
    int neighbor = -1;
    double length = 0.0;
};

struct FaceInfo {
    Eigen::Vector3d normal = Eigen::Vector3d::UnitZ();
    Eigen::Vector3d centroid = Eigen::Vector3d::Zero();
    double area = 0.0;
    std::array<int, 3> vertices{{-1, -1, -1}};
    std::array<FaceEdge, 3> edges;
};

struct GlobalEdge {
    int v0 = -1;
    int v1 = -1;
    int face0 = -1;
    int face1 = -1;
    int local0 = -1;
    int local1 = -1;
    double length = 0.0;
};

struct ChartProxy {
    Eigen::Vector3d axis = Eigen::Vector3d::UnitZ();
    double cosine = 1.0;
    int seedFace = -1;
};

struct Candidate {
    double cost = kInfinity;
    int face = -1;
    int chart = -1;
    bool operator>(const Candidate& other) const { return cost > other.cost; }
};

std::uint64_t edgeKey(int a, int b)
{
    const std::uint32_t lo = std::uint32_t((std::min)(a, b));
    const std::uint32_t hi = std::uint32_t((std::max)(a, b));
    return (std::uint64_t(lo) << 32U) | std::uint64_t(hi);
}

class Engine {
public:
    Engine(const DChartsParameters& parameters,
           const DChartsAlgorithm::SnapshotCallback& callback)
        : parameters_(parameters), callback_(callback)
    {
    }

    bool execute(const QString& path, QString* errorMessage)
    {
        if (!load(path, errorMessage))
            return false;

        initializeCharts();
        if (!publish("Seeds", 0, 1.0, false))
            return true;

        QVector<int> previousLabels(faceInfo_.size(), -1);
        double changedRatio = 1.0;
        int iteration = 0;
        for (; iteration < parameters_.maxIterations; ++iteration) {
            labels_ = growCharts(charts_, nullptr);
            compactAndRecomputeCharts();

            int changed = 0;
            for (int face = 0; face < labels_.size(); ++face) {
                if (labels_[face] != previousLabels[face])
                    ++changed;
            }
            changedRatio = labels_.isEmpty() ? 0.0 : double(changed) / double(labels_.size());
            if (!publish("Lloyd growth", iteration + 1, changedRatio, false))
                return true;

            previousLabels = labels_;
            if (iteration > 0 && changedRatio < parameters_.convergenceRatio)
                break;
        }

        if (!publish("Lloyd converged", iteration + 1, changedRatio, false))
            return true;

        fillHoles();
        compactAndRecomputeCharts();
        if (!publish("Hole filling", iteration + 1, 0.0, false))
            return true;

        relaxBoundaries();
        compactAndRecomputeCharts();
        if (!publish("Boundary straightening", iteration + 1, 0.0, false))
            return true;

        mergeCharts();
        compactAndRecomputeCharts();
        if (!publish("Developable merging", iteration + 1, 0.0, false))
            return true;

        if (parameters_.addPartialCuts)
            buildPartialCuts();
        return publish("Complete", iteration + 1, 0.0, true);
    }

private:
    bool load(const QString& path, QString* errorMessage)
    {
        Mesh mesh;
        mesh.request_face_normals();
        mesh.request_vertex_normals();
        if (!OpenMesh::IO::read_mesh(mesh, path.toStdString())) {
            if (errorMessage)
                *errorMessage = QString("Could not read mesh: %1").arg(path);
            return false;
        }
        if (mesh.n_faces() == 0 || mesh.n_vertices() == 0) {
            if (errorMessage)
                *errorMessage = "The input mesh is empty.";
            return false;
        }

        mesh.triangulate();
        mesh.garbage_collection();
        mesh.update_normals();

        mesh_.vertices.reserve(int(mesh.n_vertices()));
        vertexPositions_.reserve(mesh.n_vertices());
        for (const auto vertex : mesh.vertices()) {
            const Mesh::Point point = mesh.point(vertex);
            const Eigen::Vector3d p(point[0], point[1], point[2]);
            vertexPositions_.push_back(p);
            mesh_.vertices.push_back(QVector3D(float(p.x()), float(p.y()), float(p.z())));
        }

        faceInfo_.resize(int(mesh.n_faces()));
        vertexFaces_.resize(int(mesh.n_vertices()));
        std::unordered_map<std::uint64_t, int> edgeMap;

        for (const auto face : mesh.faces()) {
            const int faceIndex = face.idx();
            std::array<int, 3> vertices{{-1, -1, -1}};
            int count = 0;
            for (const auto vertex : mesh.fv_range(face)) {
                if (count < 3)
                    vertices[std::size_t(count)] = vertex.idx();
                ++count;
            }
            if (count != 3) {
                if (errorMessage)
                    *errorMessage = "D-Charts requires a triangular manifold mesh.";
                return false;
            }

            FaceInfo& info = faceInfo_[faceIndex];
            info.vertices = vertices;
            const Eigen::Vector3d& p0 = vertexPositions_[std::size_t(vertices[0])];
            const Eigen::Vector3d& p1 = vertexPositions_[std::size_t(vertices[1])];
            const Eigen::Vector3d& p2 = vertexPositions_[std::size_t(vertices[2])];
            const Eigen::Vector3d cross = (p1 - p0).cross(p2 - p0);
            const double twiceArea = cross.norm();
            if (twiceArea <= kEpsilon) {
                if (errorMessage)
                    *errorMessage = QString("Degenerate triangle at face %1.").arg(faceIndex);
                return false;
            }
            info.area = 0.5 * twiceArea;
            info.normal = cross / twiceArea;
            info.centroid = (p0 + p1 + p2) / 3.0;
            totalArea_ += info.area;

            for (int i = 0; i < 3; ++i) {
                mesh_.faces.push_back(unsigned(vertices[std::size_t(i)]));
                vertexFaces_[std::size_t(vertices[std::size_t(i)])].push_back(faceIndex);
                const int v0 = vertices[std::size_t(i)];
                const int v1 = vertices[std::size_t((i + 1) % 3)];
                const double length = (vertexPositions_[std::size_t(v1)]
                                       - vertexPositions_[std::size_t(v0)]).norm();
                info.edges[std::size_t(i)] = {v0, v1, -1, length};

                const std::uint64_t key = edgeKey(v0, v1);
                const auto found = edgeMap.find(key);
                if (found == edgeMap.end()) {
                    const int edgeIndex = int(globalEdges_.size());
                    edgeMap.emplace(key, edgeIndex);
                    globalEdges_.push_back({v0, v1, faceIndex, -1, i, -1, length});
                } else {
                    GlobalEdge& edge = globalEdges_[std::size_t(found->second)];
                    if (edge.face1 >= 0) {
                        if (errorMessage)
                            *errorMessage = "D-Charts requires a manifold mesh; a non-manifold edge was found.";
                        return false;
                    }
                    edge.face1 = faceIndex;
                    edge.local1 = i;
                }
            }
        }

        for (const GlobalEdge& edge : globalEdges_) {
            if (edge.face1 < 0)
                continue;
            faceInfo_[edge.face0].edges[std::size_t(edge.local0)].neighbor = edge.face1;
            faceInfo_[edge.face1].edges[std::size_t(edge.local1)].neighbor = edge.face0;
        }

        vertexEdges_.resize(vertexPositions_.size());
        for (int edgeIndex = 0; edgeIndex < int(globalEdges_.size()); ++edgeIndex) {
            const GlobalEdge& edge = globalEdges_[std::size_t(edgeIndex)];
            vertexEdges_[std::size_t(edge.v0)].push_back(edgeIndex);
            vertexEdges_[std::size_t(edge.v1)].push_back(edgeIndex);
        }
        labels_.fill(-1, faceInfo_.size());
        return true;
    }

    ChartProxy fitProxy(const QVector<int>& faces) const
    {
        ChartProxy proxy;
        if (faces.isEmpty())
            return proxy;
        if (faces.size() == 1) {
            proxy.axis = faceInfo_[faces.front()].normal;
            proxy.cosine = 1.0;
            return proxy;
        }

        double area = 0.0;
        Eigen::Vector3d mean = Eigen::Vector3d::Zero();
        for (const int face : faces) {
            const double weight = faceInfo_[face].area;
            area += weight;
            mean += weight * faceInfo_[face].normal;
        }
        if (area <= kEpsilon)
            return proxy;
        mean /= area;

        Eigen::Matrix3d covariance = Eigen::Matrix3d::Zero();
        for (const int face : faces) {
            const Eigen::Vector3d delta = faceInfo_[face].normal - mean;
            covariance += faceInfo_[face].area * delta * delta.transpose();
        }
        covariance /= area;

        if (covariance.norm() <= 1e-14 && mean.norm() > kEpsilon) {
            proxy.axis = mean.normalized();
            proxy.cosine = proxy.axis.dot(mean);
        } else {
            Eigen::SelfAdjointEigenSolver<Eigen::Matrix3d> solver(covariance);
            proxy.axis = solver.eigenvectors().col(0).normalized();
            proxy.cosine = proxy.axis.dot(mean);
        }
        if (proxy.cosine < 0.0) {
            proxy.axis = -proxy.axis;
            proxy.cosine = -proxy.cosine;
        }
        proxy.cosine = (std::max)(-1.0, (std::min)(1.0, proxy.cosine));
        return proxy;
    }

    double fittingError(const ChartProxy& chart, int face) const
    {
        const double residual = chart.axis.dot(faceInfo_[face].normal) - chart.cosine;
        return residual * residual;
    }

    QVector<double> faceDistancesFrom(int source, const QVector<char>* allowed = nullptr) const
    {
        QVector<double> distances(faceInfo_.size(), kInfinity);
        if (source < 0 || source >= faceInfo_.size())
            return distances;
        using Entry = std::pair<double, int>;
        std::priority_queue<Entry, std::vector<Entry>, std::greater<Entry>> queue;
        distances[source] = 0.0;
        queue.push({0.0, source});
        while (!queue.empty()) {
            const auto [distance, face] = queue.top();
            queue.pop();
            if (distance != distances[face])
                continue;
            for (const FaceEdge& edge : faceInfo_[face].edges) {
                const int neighbor = edge.neighbor;
                if (neighbor < 0 || (allowed && !(*allowed)[neighbor]))
                    continue;
                const double step = (faceInfo_[face].centroid - faceInfo_[neighbor].centroid).norm();
                const double candidate = distance + step;
                if (candidate < distances[neighbor]) {
                    distances[neighbor] = candidate;
                    queue.push({candidate, neighbor});
                }
            }
        }
        return distances;
    }

    QVector<int> farthestSeeds(int requested) const
    {
        const int count = (std::max)(1, (std::min)(requested, faceInfo_.size()));
        Eigen::Vector3d center = Eigen::Vector3d::Zero();
        for (const FaceInfo& face : faceInfo_)
            center += face.area * face.centroid;
        center /= (std::max)(totalArea_, kEpsilon);

        int start = 0;
        double nearest = kInfinity;
        for (int face = 0; face < faceInfo_.size(); ++face) {
            const double distance = (faceInfo_[face].centroid - center).squaredNorm();
            if (distance < nearest) {
                nearest = distance;
                start = face;
            }
        }
        QVector<double> firstDistances = faceDistancesFrom(start);
        int first = int(std::distance(firstDistances.begin(),
            std::max_element(firstDistances.begin(), firstDistances.end())));

        QVector<int> seeds;
        seeds.reserve(count);
        seeds.push_back(first);
        QVector<double> minDistances = faceDistancesFrom(first);
        while (seeds.size() < count) {
            int next = -1;
            double farthest = -1.0;
            for (int face = 0; face < minDistances.size(); ++face) {
                if (minDistances[face] > farthest && std::isfinite(minDistances[face])) {
                    farthest = minDistances[face];
                    next = face;
                }
            }
            if (next < 0 || seeds.contains(next))
                break;
            seeds.push_back(next);
            const QVector<double> distances = faceDistancesFrom(next);
            for (int face = 0; face < minDistances.size(); ++face)
                minDistances[face] = (std::min)(minDistances[face], distances[face]);
        }
        return seeds;
    }

    void initializeCharts()
    {
        const QVector<int> seeds = farthestSeeds(parameters_.initialChartCount);
        charts_.clear();
        charts_.reserve(seeds.size());
        for (const int seed : seeds) {
            ChartProxy best;
            double bestError = kInfinity;
            for (const int vertex : faceInfo_[seed].vertices) {
                QVector<int> neighborhood;
                for (const int face : vertexFaces_[std::size_t(vertex)])
                    neighborhood.push_back(face);
                ChartProxy candidate = fitProxy(neighborhood);
                const double error = fittingError(candidate, seed);
                if (error < bestError) {
                    bestError = error;
                    best = candidate;
                }
            }
            best.seedFace = seed;
            charts_.push_back(best);
        }
    }

    double assignmentCost(int face, int chartIndex, double distance, double chartArea,
                          const QVector<int>& labels) const
    {
        const double fit = (std::max)(fittingError(charts_[chartIndex], face), kEpsilon);
        const double compactness = (std::max)(
            M_PI * distance * distance / (std::max)(chartArea, kEpsilon), kEpsilon);
        double inner = 0.0;
        double outer = 0.0;
        for (const FaceEdge& edge : faceInfo_[face].edges) {
            if (edge.neighbor >= 0 && labels[edge.neighbor] == chartIndex)
                inner += edge.length;
            else
                outer += edge.length;
        }
        const double boundary = (std::max)(outer / (std::max)(inner, kEpsilon), kEpsilon);
        return std::pow(fit, parameters_.alpha)
            * std::pow(compactness, parameters_.beta)
            * std::pow(boundary, parameters_.gamma);
    }

    QVector<int> growCharts(const QVector<ChartProxy>& proxies, const QVector<char>* allowed)
    {
        const QVector<ChartProxy> savedCharts = charts_;
        charts_ = proxies;
        QVector<int> result(faceInfo_.size(), -1);
        QVector<double> distances(faceInfo_.size(), kInfinity);
        QVector<double> chartAreas(charts_.size(), 0.0);
        std::priority_queue<Candidate, std::vector<Candidate>, std::greater<Candidate>> queue;

        auto pushAround = [&](int face, int chart) {
            for (const FaceEdge& edge : faceInfo_[face].edges) {
                const int neighbor = edge.neighbor;
                if (neighbor < 0 || result[neighbor] >= 0 || (allowed && !(*allowed)[neighbor]))
                    continue;
                double bestDistance = kInfinity;
                for (const FaceEdge& candidateEdge : faceInfo_[neighbor].edges) {
                    const int adjacent = candidateEdge.neighbor;
                    if (adjacent >= 0 && result[adjacent] == chart) {
                        const double step = (faceInfo_[neighbor].centroid
                                           - faceInfo_[adjacent].centroid).norm();
                        bestDistance = (std::min)(bestDistance, distances[adjacent] + step);
                    }
                }
                if (!std::isfinite(bestDistance))
                    continue;
                queue.push({assignmentCost(neighbor, chart, bestDistance,
                                           chartAreas[chart], result), neighbor, chart});
            }
        };

        for (int chart = 0; chart < charts_.size(); ++chart) {
            const int seed = charts_[chart].seedFace;
            if (seed < 0 || seed >= result.size() || result[seed] >= 0
                || (allowed && !(*allowed)[seed]))
                continue;
            result[seed] = chart;
            distances[seed] = 0.0;
            chartAreas[chart] = faceInfo_[seed].area;
        }
        for (int chart = 0; chart < charts_.size(); ++chart) {
            const int seed = charts_[chart].seedFace;
            if (seed >= 0 && seed < result.size() && result[seed] == chart)
                pushAround(seed, chart);
        }

        while (!queue.empty()) {
            const Candidate candidate = queue.top();
            queue.pop();
            if (candidate.face < 0 || result[candidate.face] >= 0)
                continue;
            if (fittingError(charts_[candidate.chart], candidate.face)
                >= parameters_.fittingThreshold)
                continue;

            double bestDistance = kInfinity;
            for (const FaceEdge& edge : faceInfo_[candidate.face].edges) {
                const int neighbor = edge.neighbor;
                if (neighbor >= 0 && result[neighbor] == candidate.chart) {
                    const double step = (faceInfo_[candidate.face].centroid
                                       - faceInfo_[neighbor].centroid).norm();
                    bestDistance = (std::min)(bestDistance, distances[neighbor] + step);
                }
            }
            if (!std::isfinite(bestDistance))
                continue;
            result[candidate.face] = candidate.chart;
            distances[candidate.face] = bestDistance;
            chartAreas[candidate.chart] += faceInfo_[candidate.face].area;
            pushAround(candidate.face, candidate.chart);
        }
        charts_ = savedCharts;
        return result;
    }

    int selectSeed(const QVector<int>& faces, const ChartProxy& proxy) const
    {
        if (faces.isEmpty())
            return -1;
        QVector<int> candidates = faces;
        std::sort(candidates.begin(), candidates.end(), [&](int lhs, int rhs) {
            return fittingError(proxy, lhs) < fittingError(proxy, rhs);
        });
        candidates.resize((std::min)(10, candidates.size()));

        Eigen::Vector3d center = Eigen::Vector3d::Zero();
        double area = 0.0;
        for (const int face : faces) {
            center += faceInfo_[face].area * faceInfo_[face].centroid;
            area += faceInfo_[face].area;
        }
        center /= (std::max)(area, kEpsilon);
        return *std::min_element(candidates.begin(), candidates.end(), [&](int lhs, int rhs) {
            return (faceInfo_[lhs].centroid - center).squaredNorm()
                < (faceInfo_[rhs].centroid - center).squaredNorm();
        });
    }

    void compactAndRecomputeCharts()
    {
        int maxLabel = -1;
        for (const int label : labels_)
            maxLabel = (std::max)(maxLabel, label);
        QVector<QVector<int>> chartFaces(maxLabel + 1);
        for (int face = 0; face < labels_.size(); ++face) {
            if (labels_[face] >= 0)
                chartFaces[labels_[face]].push_back(face);
        }

        QVector<int> remap(chartFaces.size(), -1);
        QVector<ChartProxy> compact;
        for (int old = 0; old < chartFaces.size(); ++old) {
            if (chartFaces[old].isEmpty())
                continue;
            remap[old] = compact.size();
            ChartProxy proxy = fitProxy(chartFaces[old]);
            proxy.seedFace = selectSeed(chartFaces[old], proxy);
            compact.push_back(proxy);
        }
        for (int& label : labels_) {
            if (label >= 0)
                label = remap[label];
        }
        charts_ = compact;
    }

    QVector<QVector<int>> unassignedComponents() const
    {
        QVector<QVector<int>> components;
        QVector<char> visited(faceInfo_.size(), false);
        for (int start = 0; start < labels_.size(); ++start) {
            if (labels_[start] >= 0 || visited[start])
                continue;
            QVector<int> component;
            std::queue<int> queue;
            queue.push(start);
            visited[start] = true;
            while (!queue.empty()) {
                const int face = queue.front();
                queue.pop();
                component.push_back(face);
                for (const FaceEdge& edge : faceInfo_[face].edges) {
                    if (edge.neighbor >= 0 && labels_[edge.neighbor] < 0
                        && !visited[edge.neighbor]) {
                        visited[edge.neighbor] = true;
                        queue.push(edge.neighbor);
                    }
                }
            }
            components.push_back(component);
        }
        return components;
    }

    void fillSmallComponent(const QVector<int>& component)
    {
        struct FillCandidate {
            double distance;
            int face;
            int chart;
            bool operator>(const FillCandidate& other) const { return distance > other.distance; }
        };
        QVector<char> inComponent(faceInfo_.size(), false);
        for (const int face : component)
            inComponent[face] = true;
        std::priority_queue<FillCandidate, std::vector<FillCandidate>,
                            std::greater<FillCandidate>> queue;
        QVector<double> best(faceInfo_.size(), kInfinity);
        for (const int face : component) {
            for (const FaceEdge& edge : faceInfo_[face].edges) {
                if (edge.neighbor >= 0 && labels_[edge.neighbor] >= 0) {
                    const double fit = fittingError(charts_[labels_[edge.neighbor]], face);
                    if (fit < best[face]) {
                        best[face] = fit;
                        queue.push({fit, face, labels_[edge.neighbor]});
                    }
                }
            }
        }
        if (queue.empty()) {
            ChartProxy chart = fitProxy(component);
            chart.seedFace = selectSeed(component, chart);
            const int chartIndex = charts_.size();
            charts_.push_back(chart);
            for (const int face : component)
                labels_[face] = chartIndex;
            return;
        }
        while (!queue.empty()) {
            const FillCandidate candidate = queue.top();
            queue.pop();
            if (labels_[candidate.face] >= 0)
                continue;
            labels_[candidate.face] = candidate.chart;
            for (const FaceEdge& edge : faceInfo_[candidate.face].edges) {
                const int neighbor = edge.neighbor;
                if (neighbor < 0 || !inComponent[neighbor] || labels_[neighbor] >= 0)
                    continue;
                const double distance = candidate.distance + edge.length;
                if (distance < best[neighbor]) {
                    best[neighbor] = distance;
                    queue.push({distance, neighbor, candidate.chart});
                }
            }
        }
    }

    void spawnChartInComponent(const QVector<int>& component)
    {
        ChartProxy chart = fitProxy(component);
        chart.seedFace = selectSeed(component, chart);
        const int chartIndex = charts_.size();
        charts_.push_back(chart);

        QVector<char> allowed(faceInfo_.size(), false);
        for (const int face : component)
            allowed[face] = true;
        QVector<ChartProxy> oneChart{chart};
        QVector<int> local = growCharts(oneChart, &allowed);
        charts_.resize(chartIndex + 1);
        charts_[chartIndex] = chart;
        for (const int face : component) {
            if (local[face] == 0)
                labels_[face] = chartIndex;
        }
    }

    void fillHoles()
    {
        const double largeThreshold = (std::max)(
            totalArea_ * parameters_.largeHoleAreaRatio,
            totalArea_ / (std::max)(1, faceInfo_.size()) * 12.0);
        int guard = faceInfo_.size() + 1;
        while (guard-- > 0) {
            const QVector<QVector<int>> components = unassignedComponents();
            if (components.isEmpty())
                break;
            bool assignedAny = false;
            for (const QVector<int>& component : components) {
                double area = 0.0;
                for (const int face : component)
                    area += faceInfo_[face].area;
                const int before = std::count(labels_.begin(), labels_.end(), -1);
                if (area > largeThreshold)
                    spawnChartInComponent(component);
                else
                    fillSmallComponent(component);
                assignedAny = assignedAny
                    || std::count(labels_.begin(), labels_.end(), -1) < before;
            }
            if (!assignedAny)
                break;
            compactAndRecomputeCharts();
        }
        for (const QVector<int>& component : unassignedComponents())
            fillSmallComponent(component);
    }

    bool wouldDisconnectChart(int face, int chart) const
    {
        QVector<int> sameNeighbors;
        for (const FaceEdge& edge : faceInfo_[face].edges) {
            if (edge.neighbor >= 0 && labels_[edge.neighbor] == chart)
                sameNeighbors.push_back(edge.neighbor);
        }
        if (sameNeighbors.size() <= 1)
            return false;

        std::queue<int> queue;
        std::unordered_set<int> visited;
        queue.push(sameNeighbors.front());
        visited.insert(sameNeighbors.front());
        while (!queue.empty()) {
            const int current = queue.front();
            queue.pop();
            for (const FaceEdge& edge : faceInfo_[current].edges) {
                const int neighbor = edge.neighbor;
                if (neighbor >= 0 && neighbor != face && labels_[neighbor] == chart
                    && visited.insert(neighbor).second)
                    queue.push(neighbor);
            }
        }
        for (const int neighbor : sameNeighbors) {
            if (!visited.count(neighbor))
                return true;
        }
        return false;
    }

    double localBoundaryLength(int face, int proposedLabel) const
    {
        double length = 0.0;
        for (const FaceEdge& edge : faceInfo_[face].edges) {
            if (edge.neighbor < 0 || labels_[edge.neighbor] != proposedLabel)
                length += edge.length;
        }
        return length;
    }

    void relaxBoundaries()
    {
        for (int pass = 0; pass < parameters_.boundaryRelaxationPasses; ++pass) {
            int changed = 0;
            QVector<int> order(faceInfo_.size());
            std::iota(order.begin(), order.end(), 0);
            std::stable_sort(order.begin(), order.end(), [&](int lhs, int rhs) {
                return localBoundaryLength(lhs, labels_[lhs])
                    > localBoundaryLength(rhs, labels_[rhs]);
            });
            for (const int face : order) {
                const int oldChart = labels_[face];
                if (oldChart < 0 || charts_[oldChart].seedFace == face
                    || wouldDisconnectChart(face, oldChart))
                    continue;
                std::set<int> neighborCharts;
                for (const FaceEdge& edge : faceInfo_[face].edges) {
                    if (edge.neighbor >= 0 && labels_[edge.neighbor] >= 0
                        && labels_[edge.neighbor] != oldChart)
                        neighborCharts.insert(labels_[edge.neighbor]);
                }
                const double oldLength = localBoundaryLength(face, oldChart);
                int bestChart = oldChart;
                double bestScore = oldLength;
                for (const int candidate : neighborCharts) {
                    const double fit = fittingError(charts_[candidate], face);
                    if (fit >= parameters_.fittingThreshold)
                        continue;
                    const double newLength = localBoundaryLength(face, candidate);
                    const double score = newLength + 0.05 * std::sqrt(faceInfo_[face].area) * fit;
                    if (newLength + kEpsilon < oldLength && score < bestScore) {
                        bestScore = score;
                        bestChart = candidate;
                    }
                }
                if (bestChart != oldChart) {
                    labels_[face] = bestChart;
                    ++changed;
                }
            }
            if (changed == 0)
                break;
        }
    }

    double cylindricalBoundaryError(int chartA, int chartB) const
    {
        std::set<int> region;
        for (const GlobalEdge& edge : globalEdges_) {
            if (edge.face1 < 0)
                continue;
            const int a = labels_[edge.face0];
            const int b = labels_[edge.face1];
            if (!((a == chartA && b == chartB) || (a == chartB && b == chartA)))
                continue;
            region.insert(edge.face0);
            region.insert(edge.face1);
            for (const int face : {edge.face0, edge.face1}) {
                for (const FaceEdge& neighborEdge : faceInfo_[face].edges) {
                    if (neighborEdge.neighbor >= 0
                        && (labels_[neighborEdge.neighbor] == chartA
                            || labels_[neighborEdge.neighbor] == chartB))
                        region.insert(neighborEdge.neighbor);
                }
            }
        }
        if (region.empty())
            return kInfinity;

        Eigen::Matrix3d moment = Eigen::Matrix3d::Zero();
        double area = 0.0;
        for (const int face : region) {
            const Eigen::Vector3d& normal = faceInfo_[face].normal;
            moment += faceInfo_[face].area * normal * normal.transpose();
            area += faceInfo_[face].area;
        }
        moment /= (std::max)(area, kEpsilon);
        Eigen::SelfAdjointEigenSolver<Eigen::Matrix3d> solver(moment);
        const Eigen::Vector3d axis = solver.eigenvectors().col(0).normalized();
        double error = 0.0;
        for (const int face : region) {
            const double residual = axis.dot(faceInfo_[face].normal);
            error += faceInfo_[face].area * residual * residual;
        }
        return error / (std::max)(area, kEpsilon);
    }

    void mergeCharts()
    {
        int guard = charts_.size() * charts_.size() + 1;
        while (guard-- > 0) {
            std::set<std::pair<int, int>> pairs;
            for (const GlobalEdge& edge : globalEdges_) {
                if (edge.face1 < 0)
                    continue;
                int a = labels_[edge.face0];
                int b = labels_[edge.face1];
                if (a >= 0 && b >= 0 && a != b) {
                    if (a > b)
                        std::swap(a, b);
                    pairs.insert({a, b});
                }
            }
            int mergeA = -1;
            int mergeB = -1;
            double bestError = parameters_.mergeThreshold;
            for (const auto& pair : pairs) {
                const double error = cylindricalBoundaryError(pair.first, pair.second);
                if (error < bestError) {
                    bestError = error;
                    mergeA = pair.first;
                    mergeB = pair.second;
                }
            }
            if (mergeA < 0)
                break;
            for (int& label : labels_) {
                if (label == mergeB)
                    label = mergeA;
            }
            compactAndRecomputeCharts();
        }
    }

    void addVertexPathToCut(int chart, int targetFace,
                            std::unordered_set<std::uint64_t>& cutKeys)
    {
        QVector<char> chartVertex(vertexPositions_.size(), false);
        QVector<char> boundaryVertex(vertexPositions_.size(), false);
        for (int face = 0; face < labels_.size(); ++face) {
            if (labels_[face] != chart)
                continue;
            for (const int vertex : faceInfo_[face].vertices)
                chartVertex[vertex] = true;
            for (const FaceEdge& edge : faceInfo_[face].edges) {
                if (edge.neighbor < 0 || labels_[edge.neighbor] != chart) {
                    boundaryVertex[edge.v0] = true;
                    boundaryVertex[edge.v1] = true;
                }
            }
        }

        using Entry = std::pair<double, int>;
        std::priority_queue<Entry, std::vector<Entry>, std::greater<Entry>> queue;
        QVector<double> distance(vertexPositions_.size(), kInfinity);
        QVector<int> parent(vertexPositions_.size(), -1);
        for (int vertex = 0; vertex < boundaryVertex.size(); ++vertex) {
            if (boundaryVertex[vertex]) {
                distance[vertex] = 0.0;
                queue.push({0.0, vertex});
            }
        }
        while (!queue.empty()) {
            const auto [currentDistance, vertex] = queue.top();
            queue.pop();
            if (currentDistance != distance[vertex])
                continue;
            for (const int edgeIndex : vertexEdges_[std::size_t(vertex)]) {
                const GlobalEdge& edge = globalEdges_[std::size_t(edgeIndex)];
                const int other = edge.v0 == vertex ? edge.v1 : edge.v0;
                if (!chartVertex[other])
                    continue;
                bool belongsToChart = edge.face0 >= 0 && labels_[edge.face0] == chart;
                belongsToChart = belongsToChart
                    || (edge.face1 >= 0 && labels_[edge.face1] == chart);
                if (!belongsToChart)
                    continue;
                const double nextDistance = currentDistance + edge.length;
                if (nextDistance < distance[other]) {
                    distance[other] = nextDistance;
                    parent[other] = vertex;
                    queue.push({nextDistance, other});
                }
            }
        }

        int target = -1;
        double best = kInfinity;
        for (const int vertex : faceInfo_[targetFace].vertices) {
            if (distance[vertex] < best) {
                best = distance[vertex];
                target = vertex;
            }
        }
        while (target >= 0 && parent[target] >= 0) {
            cutKeys.insert(edgeKey(target, parent[target]));
            target = parent[target];
        }
    }

    void buildPartialCuts()
    {
        std::unordered_set<std::uint64_t> cutKeys;
        QVector<char> visited(faceInfo_.size(), false);
        for (int start = 0; start < labels_.size(); ++start) {
            const int chart = labels_[start];
            if (chart < 0 || visited[start]
                || fittingError(charts_[chart], start) <= parameters_.fittingThreshold)
                continue;
            QVector<int> component;
            std::queue<int> queue;
            queue.push(start);
            visited[start] = true;
            while (!queue.empty()) {
                const int face = queue.front();
                queue.pop();
                component.push_back(face);
                for (const FaceEdge& edge : faceInfo_[face].edges) {
                    const int neighbor = edge.neighbor;
                    if (neighbor >= 0 && !visited[neighbor] && labels_[neighbor] == chart
                        && fittingError(charts_[chart], neighbor)
                            > parameters_.fittingThreshold) {
                        visited[neighbor] = true;
                        queue.push(neighbor);
                    }
                }
            }
            const int target = *std::max_element(component.begin(), component.end(),
                [&](int lhs, int rhs) {
                    return fittingError(charts_[chart], lhs)
                        < fittingError(charts_[chart], rhs);
                });
            addVertexPathToCut(chart, target, cutKeys);
        }
        partialCutEdges_.clear();
        for (const GlobalEdge& edge : globalEdges_) {
            if (cutKeys.count(edgeKey(edge.v0, edge.v1))) {
                partialCutEdges_.push_back(unsigned(edge.v0));
                partialCutEdges_.push_back(unsigned(edge.v1));
            }
        }
    }

    QVector<unsigned int> seamEdges() const
    {
        QVector<unsigned int> seams;
        seams.reserve(int(globalEdges_.size()));
        for (const GlobalEdge& edge : globalEdges_) {
            const bool boundary = edge.face1 < 0;
            const bool chartBoundary = edge.face1 >= 0
                && labels_[edge.face0] != labels_[edge.face1];
            if (boundary || chartBoundary) {
                seams.push_back(unsigned(edge.v0));
                seams.push_back(unsigned(edge.v1));
            }
        }
        seams += partialCutEdges_;
        return seams;
    }

    bool publish(const QString& phase, int iteration, double changedRatio, bool complete)
    {
        DChartsSnapshot snapshot;
        snapshot.mesh = mesh_;
        snapshot.faceCharts = labels_;
        snapshot.phase = phase;
        snapshot.iteration = iteration;
        snapshot.chartCount = charts_.size();
        snapshot.reassignedRatio = changedRatio;
        snapshot.complete = complete;
        snapshot.seamEdges = seamEdges();
        snapshot.seedFaces.reserve(charts_.size());
        for (const ChartProxy& chart : charts_)
            snapshot.seedFaces.push_back(chart.seedFace);

        double weightedError = 0.0;
        double assignedArea = 0.0;
        for (int face = 0; face < labels_.size(); ++face) {
            const int chart = labels_[face];
            if (chart < 0) {
                ++snapshot.unassignedFaces;
                continue;
            }
            ++snapshot.assignedFaces;
            const double error = fittingError(charts_[chart], face);
            weightedError += faceInfo_[face].area * error;
            assignedArea += faceInfo_[face].area;
            snapshot.maxFittingError = (std::max)(snapshot.maxFittingError, error);
        }
        snapshot.meanFittingError = weightedError / (std::max)(assignedArea, kEpsilon);
        return !callback_ || callback_(snapshot);
    }

    DChartsParameters parameters_;
    DChartsAlgorithm::SnapshotCallback callback_;
    DChartsMeshData mesh_;
    std::vector<Eigen::Vector3d> vertexPositions_;
    QVector<FaceInfo> faceInfo_;
    std::vector<std::vector<int>> vertexFaces_;
    std::vector<std::vector<int>> vertexEdges_;
    std::vector<GlobalEdge> globalEdges_;
    QVector<ChartProxy> charts_;
    QVector<int> labels_;
    QVector<unsigned int> partialCutEdges_;
    double totalArea_ = 0.0;
};
}

bool DChartsAlgorithm::run(const QString& path,
                           const DChartsParameters& parameters,
                           const SnapshotCallback& callback,
                           QString* errorMessage)
{
    Engine engine(parameters, callback);
    return engine.execute(path, errorMessage);
}
