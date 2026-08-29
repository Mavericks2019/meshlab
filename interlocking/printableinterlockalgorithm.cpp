#include "printableinterlockalgorithm.h"

#include <CGAL/Exact_predicates_exact_constructions_kernel.h>
#include <CGAL/Polygon_mesh_processing/connected_components.h>
#include <CGAL/Polygon_mesh_processing/corefinement.h>
#include <CGAL/Polygon_mesh_processing/measure.h>
#include <CGAL/Polygon_mesh_processing/orientation.h>
#include <CGAL/Polygon_mesh_processing/orient_polygon_soup.h>
#include <CGAL/Polygon_mesh_processing/orient_polygon_soup_extension.h>
#include <CGAL/Polygon_mesh_processing/polygon_soup_to_polygon_mesh.h>
#include <CGAL/Polygon_mesh_processing/repair.h>
#include <CGAL/Polygon_mesh_processing/self_intersections.h>
#include <CGAL/Polygon_mesh_processing/shape_predicates.h>
#include <CGAL/Polygon_mesh_processing/stitch_borders.h>
#include <CGAL/Side_of_triangle_mesh.h>
#include <CGAL/Surface_mesh.h>
#include <CGAL/boost/graph/IO/polygon_mesh_io.h>
#include <CGAL/boost/graph/helpers.h>
#include <CGAL/boost/graph/iterator.h>

#include <QFileInfo>
#include <QVector3D>

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <map>
#include <numeric>
#include <queue>
#include <random>
#include <tuple>
#include <unordered_set>
#include <vector>

namespace {
namespace PMP = CGAL::Polygon_mesh_processing;
using Kernel = CGAL::Exact_predicates_exact_constructions_kernel;
using Point = Kernel::Point_3;
using SurfaceMesh = CGAL::Surface_mesh<Point>;

constexpr int kEmpty = -3;
constexpr int kBoundary = -2;
constexpr int kRemaining = -1;

struct Coord {
    int x = 0;
    int y = 0;
    int z = 0;

    Coord operator+(const Coord& other) const
    {
        return {x + other.x, y + other.y, z + other.z};
    }
};

const std::array<Coord, 6> kDirections{{
    {1, 0, 0}, {-1, 0, 0}, {0, 1, 0},
    {0, -1, 0}, {0, 0, 1}, {0, 0, -1}
}};

struct CellInfo {
    double volume = 0.0;
    std::array<double, 6> faceArea{{0.0, 0.0, 0.0, 0.0, 0.0, 0.0}};
    double saliency = 0.0;
    bool occupied = false;
    bool full = false;
    bool connected = true;
    bool internal = false;
};

struct Grid {
    int resolution = 0;
    double cellSize = 0.0;
    double origin = 0.0;
    std::vector<int> labels;
    std::vector<CellInfo> cells;

    explicit Grid(int size = 0)
        : resolution(size), labels(size * size * size, kEmpty),
          cells(size * size * size)
    {
    }

    bool contains(const Coord& point) const
    {
        return point.x >= 0 && point.y >= 0 && point.z >= 0
            && point.x < resolution && point.y < resolution && point.z < resolution;
    }

    int index(const Coord& point) const
    {
        return (point.z * resolution + point.y) * resolution + point.x;
    }

    Coord coord(int value) const
    {
        Coord point;
        point.x = value % resolution;
        value /= resolution;
        point.y = value % resolution;
        point.z = value / resolution;
        return point;
    }

    int label(const Coord& point) const
    {
        return contains(point) ? labels[index(point)] : kEmpty;
    }

    int remainingCount() const
    {
        return int(std::count(labels.begin(), labels.end(), kRemaining));
    }
};

struct LoadedMesh {
    SurfaceMesh mesh;
    Point center;
    QVector3D sourceCenter;
    double extent = 0.0;
    double volume = 0.0;
    double sourceUnitsPerNormalizedUnit = 1.0;
};

bool loadAndValidateMesh(const QString& path, LoadedMesh* result, QString* error)
{
    if (!QFileInfo::exists(path)) {
        *error = QString("Mesh file does not exist: %1").arg(path);
        return false;
    }
    if (!CGAL::IO::read_polygon_mesh(path.toStdString(), result->mesh)
        || result->mesh.is_empty()) {
        *error = QString("Could not read mesh: %1").arg(path);
        return false;
    }
    const QString name = QFileInfo(path).fileName();
    if (!CGAL::is_triangle_mesh(result->mesh)) {
        *error = QString("%1 is not a triangle-only mesh.").arg(name);
        return false;
    }
    for (SurfaceMesh::Face_index face : result->mesh.faces()) {
        if (PMP::is_degenerate_triangle_face(face, result->mesh)) {
            *error = QString("%1 contains degenerate triangles.").arg(name);
            return false;
        }
    }
    if (!CGAL::is_closed(result->mesh)) {
        *error = QString("%1 is open. Provide one watertight solid.").arg(name);
        return false;
    }
    auto componentMap = result->mesh.add_property_map<SurfaceMesh::Face_index, std::size_t>(
        "f:printable_interlock_component", 0).first;
    const std::size_t componentCount = PMP::connected_components(
        result->mesh, componentMap);
    result->mesh.remove_property_map(componentMap);
    if (componentCount != 1) {
        *error = QString("%1 has multiple disconnected shells. Provide one connected solid.")
                     .arg(name);
        return false;
    }
    if (PMP::does_self_intersect(result->mesh)) {
        *error = QString("%1 contains self-intersections.").arg(name);
        return false;
    }

    CGAL::Bbox_3 box;
    bool first = true;
    for (SurfaceMesh::Vertex_index vertex : result->mesh.vertices()) {
        const CGAL::Bbox_3 pointBox = result->mesh.point(vertex).bbox();
        box = first ? pointBox : box + pointBox;
        first = false;
    }
    result->center = Point(0.5 * (box.xmin() + box.xmax()),
                           0.5 * (box.ymin() + box.ymax()),
                           0.5 * (box.zmin() + box.zmax()));
    result->sourceCenter = QVector3D(float(CGAL::to_double(result->center.x())),
                                     float(CGAL::to_double(result->center.y())),
                                     float(CGAL::to_double(result->center.z())));
    result->extent = (std::max)({box.xmax() - box.xmin(), box.ymax() - box.ymin(),
                                 box.zmax() - box.zmin()});
    result->volume = std::abs(CGAL::to_double(PMP::volume(result->mesh)));
    if (!(result->extent > 1e-12) || !(result->volume > 1e-12)
        || !std::isfinite(result->extent) || !std::isfinite(result->volume)) {
        *error = QString("%1 does not enclose a usable non-zero volume.").arg(name);
        return false;
    }

    const double scale = 2.0 / result->extent;
    result->sourceUnitsPerNormalizedUnit = 1.0 / scale;
    for (SurfaceMesh::Vertex_index vertex : result->mesh.vertices()) {
        const Point point = result->mesh.point(vertex);
        result->mesh.point(vertex) = Point(
            (CGAL::to_double(point.x()) - CGAL::to_double(result->center.x())) * scale,
            (CGAL::to_double(point.y()) - CGAL::to_double(result->center.y())) * scale,
            (CGAL::to_double(point.z()) - CGAL::to_double(result->center.z())) * scale);
    }
    result->center = Point(0.0, 0.0, 0.0);
    result->extent = 2.0;
    result->volume *= scale * scale * scale;
    PMP::orient_to_bound_a_volume(result->mesh);
    return true;
}

bool samplesConnected(const std::vector<unsigned char>& inside, int side)
{
    const int totalInside = int(std::count(inside.begin(), inside.end(), 1));
    if (totalInside <= 1)
        return true;
    int start = -1;
    for (int i = 0; i < int(inside.size()); ++i) {
        if (inside[i]) {
            start = i;
            break;
        }
    }
    std::vector<unsigned char> visited(inside.size(), 0);
    std::queue<int> queue;
    queue.push(start);
    visited[start] = 1;
    int reached = 0;
    while (!queue.empty()) {
        const int value = queue.front();
        queue.pop();
        ++reached;
        const int x = value % side;
        const int y = (value / side) % side;
        const int z = value / (side * side);
        for (const Coord& direction : kDirections) {
            const int nx = x + direction.x;
            const int ny = y + direction.y;
            const int nz = z + direction.z;
            if (nx < 0 || ny < 0 || nz < 0 || nx >= side || ny >= side || nz >= side)
                continue;
            const int next = (nz * side + ny) * side + nx;
            if (inside[next] && !visited[next]) {
                visited[next] = 1;
                queue.push(next);
            }
        }
    }
    return reached == totalInside;
}

int localSampleIndex(int x, int y, int z, int side)
{
    return (z * side + y) * side + x;
}

bool voxelizeAndAnalyze(const SurfaceMesh& mesh,
                        const PrintableInterlockParameters& parameters,
                        Grid* grid, int* tinyCount, int* disconnectedCount,
                        QString* error)
{
    *grid = Grid(parameters.resolution);
    grid->cellSize = 2.0 / double(parameters.resolution - 2);
    grid->origin = -0.5 * double(parameters.resolution) * grid->cellSize;
    const int subdivisions = parameters.samplesPerVoxel;
    const int side = subdivisions + 1;
    const int sampleCount = side * side * side;
    CGAL::Side_of_triangle_mesh<SurfaceMesh, Kernel> classify(mesh);
    *tinyCount = 0;
    *disconnectedCount = 0;

    for (int z = 0; z < grid->resolution; ++z) {
        for (int y = 0; y < grid->resolution; ++y) {
            for (int x = 0; x < grid->resolution; ++x) {
                const Coord cell{x, y, z};
                const int cellIndex = grid->index(cell);
                std::vector<unsigned char> inside(sampleCount, 0);
                int count = 0;
                for (int sz = 0; sz < side; ++sz) {
                    for (int sy = 0; sy < side; ++sy) {
                        for (int sx = 0; sx < side; ++sx) {
                            const Point sample(
                                grid->origin + (x + double(sx) / subdivisions) * grid->cellSize,
                                grid->origin + (y + double(sy) / subdivisions) * grid->cellSize,
                                grid->origin + (z + double(sz) / subdivisions) * grid->cellSize);
                            const bool isInside = classify(sample) != CGAL::ON_UNBOUNDED_SIDE;
                            inside[localSampleIndex(sx, sy, sz, side)] = isInside ? 1 : 0;
                            count += isInside ? 1 : 0;
                        }
                    }
                }
                CellInfo& info = grid->cells[cellIndex];
                info.volume = double(count) / sampleCount;
                info.occupied = count > 0;
                info.full = count == sampleCount;
                info.connected = samplesConnected(inside, side);
                if (!info.occupied)
                    continue;
                grid->labels[cellIndex] = kBoundary;
                if (info.volume <= 0.01)
                    ++*tinyCount;
                if (!info.connected)
                    ++*disconnectedCount;

                for (int direction = 0; direction < 6; ++direction) {
                    int faceInside = 0;
                    for (int a = 0; a < side; ++a) {
                        for (int b = 0; b < side; ++b) {
                            int sx = a;
                            int sy = b;
                            int sz = 0;
                            if (direction < 2) {
                                sx = direction == 0 ? subdivisions : 0;
                                sy = a;
                                sz = b;
                            } else if (direction < 4) {
                                sx = a;
                                sy = direction == 2 ? subdivisions : 0;
                                sz = b;
                            } else {
                                sx = a;
                                sy = b;
                                sz = direction == 4 ? subdivisions : 0;
                            }
                            faceInside += inside[localSampleIndex(sx, sy, sz, side)] ? 1 : 0;
                        }
                    }
                    info.faceArea[direction] = double(faceInside) / double(side * side);
                }
            }
        }
    }

    const int occupiedCount = int(std::count_if(grid->cells.begin(), grid->cells.end(),
        [](const CellInfo& info) { return info.occupied; }));
    if (occupiedCount == 0) {
        *error = "Voxelization produced no occupied cells. Increase the resolution.";
        return false;
    }

    for (int index = 0; index < int(grid->cells.size()); ++index) {
        CellInfo& info = grid->cells[index];
        if (!info.occupied)
            continue;
        const Coord point = grid->coord(index);
        bool hasFullNeighbor = false;
        bool hasWeakPositiveContact = false;
        double strongestContact = 0.0;
        double variation = 0.0;
        int occupiedNeighbors = 0;
        for (int direction = 0; direction < 6; ++direction) {
            const Coord neighborPoint = point + kDirections[direction];
            if (!grid->contains(neighborPoint))
                continue;
            const CellInfo& neighbor = grid->cells[grid->index(neighborPoint)];
            if (!neighbor.occupied)
                continue;
            ++occupiedNeighbors;
            hasFullNeighbor = hasFullNeighbor || neighbor.full;
            const double contact = (std::min)(info.faceArea[direction],
                                              neighbor.faceArea[direction ^ 1]);
            strongestContact = (std::max)(strongestContact, contact);
            hasWeakPositiveContact = hasWeakPositiveContact
                || (contact > 0.0 && contact < parameters.minimumFaceContact);
            variation += std::abs(info.volume - neighbor.volume);
        }
        info.internal = info.full
            || (info.connected && info.volume >= parameters.internalVolumeThreshold
                && hasFullNeighbor && strongestContact >= 0.25
                && !hasWeakPositiveContact);
        info.saliency = (1.0 - info.volume) + variation / (std::max)(1, occupiedNeighbors);
    }

    std::unordered_set<int> unvisited;
    for (int index = 0; index < int(grid->cells.size()); ++index)
        if (grid->cells[index].internal)
            unvisited.insert(index);
    std::unordered_set<int> largest;
    while (!unvisited.empty()) {
        std::unordered_set<int> component;
        std::queue<int> queue;
        const int start = *unvisited.begin();
        unvisited.erase(start);
        component.insert(start);
        queue.push(start);
        while (!queue.empty()) {
            const Coord point = grid->coord(queue.front());
            queue.pop();
            for (const Coord& direction : kDirections) {
                const Coord next = point + direction;
                if (!grid->contains(next))
                    continue;
                const int nextIndex = grid->index(next);
                auto iterator = unvisited.find(nextIndex);
                if (iterator != unvisited.end()) {
                    unvisited.erase(iterator);
                    component.insert(nextIndex);
                    queue.push(nextIndex);
                }
            }
        }
        if (component.size() > largest.size())
            largest = std::move(component);
    }
    for (int index = 0; index < int(grid->cells.size()); ++index) {
        CellInfo& info = grid->cells[index];
        info.internal = largest.count(index) != 0;
        if (info.internal)
            grid->labels[index] = kRemaining;
    }
    if (int(largest.size()) < parameters.pieceCount * 3) {
        *error = QString("Only %1 structurally strong internal voxels remain for %2 parts. "
                         "Increase resolution or lower the internal-volume threshold.")
                     .arg(largest.size()).arg(parameters.pieceCount);
        return false;
    }
    return true;
}

bool rayClear(const Grid& grid, const Coord& point, int direction)
{
    Coord cursor = point + kDirections[direction];
    while (grid.contains(cursor)) {
        if (grid.label(cursor) == kRemaining)
            return false;
        cursor = cursor + kDirections[direction];
    }
    return true;
}

int remainingNeighborCount(const Grid& grid, const Coord& point)
{
    int result = 0;
    for (const Coord& direction : kDirections)
        result += grid.label(point + direction) == kRemaining ? 1 : 0;
    return result;
}

struct Seed {
    int index = -1;
    int direction = -1;
    int neighbors = 0;
};

std::vector<Seed> findSeeds(const Grid& grid, int limit, std::mt19937& generator)
{
    std::vector<Seed> seeds;
    for (int index = 0; index < int(grid.labels.size()); ++index) {
        if (grid.labels[index] != kRemaining)
            continue;
        const Coord point = grid.coord(index);
        const int neighbors = remainingNeighborCount(grid, point);
        for (int direction = 0; direction < 6; ++direction) {
            if (rayClear(grid, point, direction))
                seeds.push_back({index, direction, neighbors});
        }
    }
    std::shuffle(seeds.begin(), seeds.end(), generator);
    std::stable_sort(seeds.begin(), seeds.end(), [](const Seed& left, const Seed& right) {
        return left.neighbors > right.neighbors;
    });
    if (int(seeds.size()) > limit)
        seeds.resize(limit);
    return seeds;
}

bool addSweepClosure(const Grid& grid, int direction,
                     std::unordered_set<int>* cells, int maximumSize)
{
    bool changed = true;
    while (changed) {
        changed = false;
        const std::vector<int> current(cells->begin(), cells->end());
        for (int index : current) {
            Coord cursor = grid.coord(index) + kDirections[direction];
            while (grid.contains(cursor)) {
                if (grid.label(cursor) == kRemaining
                    && cells->insert(grid.index(cursor)).second)
                    changed = true;
                cursor = cursor + kDirections[direction];
            }
        }
        if (int(cells->size()) > maximumSize)
            return false;
    }
    return true;
}

bool remainingConnectedAfter(const Grid& grid, const std::unordered_set<int>& removed)
{
    const int expected = grid.remainingCount() - int(removed.size());
    if (expected <= 1)
        return true;
    int start = -1;
    for (int index = 0; index < int(grid.labels.size()); ++index) {
        if (grid.labels[index] == kRemaining && removed.count(index) == 0) {
            start = index;
            break;
        }
    }
    if (start < 0)
        return false;
    std::vector<unsigned char> visited(grid.labels.size(), 0);
    std::queue<int> queue;
    queue.push(start);
    visited[start] = 1;
    int reached = 0;
    while (!queue.empty()) {
        const int index = queue.front();
        queue.pop();
        ++reached;
        const Coord point = grid.coord(index);
        for (const Coord& direction : kDirections) {
            const Coord next = point + direction;
            if (!grid.contains(next))
                continue;
            const int nextIndex = grid.index(next);
            if (!visited[nextIndex] && grid.labels[nextIndex] == kRemaining
                && removed.count(nextIndex) == 0) {
                visited[nextIndex] = 1;
                queue.push(nextIndex);
            }
        }
    }
    return reached == expected;
}

bool extentAllowed(const Grid& grid, const std::unordered_set<int>& cells,
                   double maximumRatio)
{
    if (maximumRatio >= 0.999 || cells.empty())
        return true;
    Coord minimum{grid.resolution, grid.resolution, grid.resolution};
    Coord maximum{-1, -1, -1};
    for (int index : cells) {
        const Coord point = grid.coord(index);
        minimum.x = (std::min)(minimum.x, point.x);
        minimum.y = (std::min)(minimum.y, point.y);
        minimum.z = (std::min)(minimum.z, point.z);
        maximum.x = (std::max)(maximum.x, point.x);
        maximum.y = (std::max)(maximum.y, point.y);
        maximum.z = (std::max)(maximum.z, point.z);
    }
    const int allowed = (std::max)(1, int(std::floor(maximumRatio * grid.resolution)));
    return maximum.x - minimum.x + 1 <= allowed
        && maximum.y - minimum.y + 1 <= allowed
        && maximum.z - minimum.z + 1 <= allowed;
}

bool candidateSteady(const Grid& grid, const std::unordered_set<int>& cells,
                     int extractionDirection)
{
    std::array<bool, 6> blocked{{false, false, false, false, false, false}};
    for (int index : cells) {
        const Coord point = grid.coord(index);
        for (int direction = 0; direction < 6; ++direction) {
            const Coord neighbor = point + kDirections[direction];
            if (grid.label(neighbor) == kRemaining
                && (!grid.contains(neighbor)
                    || cells.count(grid.index(neighbor)) == 0))
                blocked[direction] = true;
        }
    }
    for (int direction = 0; direction < 6; ++direction)
        if (direction != extractionDirection && !blocked[direction])
            return false;
    return true;
}

struct PartCandidate {
    std::unordered_set<int> cells;
    int direction = -1;
    double score = std::numeric_limits<double>::infinity();
};

std::vector<int> frontierCells(const Grid& grid, const std::unordered_set<int>& cells)
{
    std::unordered_set<int> frontier;
    for (int index : cells) {
        const Coord point = grid.coord(index);
        for (const Coord& direction : kDirections) {
            const Coord next = point + direction;
            if (!grid.contains(next))
                continue;
            const int nextIndex = grid.index(next);
            if (grid.labels[nextIndex] == kRemaining && cells.count(nextIndex) == 0)
                frontier.insert(nextIndex);
        }
    }
    return std::vector<int>(frontier.begin(), frontier.end());
}

bool growCandidate(const Grid& grid, const Seed& seed, int targetSize,
                   const PrintableInterlockParameters& parameters,
                   std::mt19937& generator, PartCandidate* result)
{
    const int minimumSize = (std::max)(1, int(std::floor(0.55 * targetSize)));
    const int maximumSize = (std::max)(minimumSize, int(std::ceil(1.5 * targetSize)));
    std::unordered_set<int> cells{seed.index};
    if (!addSweepClosure(grid, seed.direction, &cells, maximumSize))
        return false;

    PartCandidate best;
    bool found = false;
    while (int(cells.size()) <= maximumSize) {
        if (int(cells.size()) >= minimumSize
            && remainingConnectedAfter(grid, cells)
            && extentAllowed(grid, cells, parameters.maxPartExtentRatio)
            && candidateSteady(grid, cells, seed.direction)) {
            const double score = std::abs(double(cells.size() - targetSize));
            if (!found || score < best.score) {
                best.cells = cells;
                best.direction = seed.direction;
                best.score = score;
                found = true;
            }
            if (int(cells.size()) >= targetSize)
                break;
        }
        std::vector<int> frontier = frontierCells(grid, cells);
        if (frontier.empty())
            break;
        std::shuffle(frontier.begin(), frontier.end(), generator);
        std::stable_sort(frontier.begin(), frontier.end(), [&](int left, int right) {
            return remainingNeighborCount(grid, grid.coord(left))
                < remainingNeighborCount(grid, grid.coord(right));
        });
        bool advanced = false;
        const int trials = (std::min)(24, int(frontier.size()));
        for (int trial = 0; trial < trials; ++trial) {
            std::unordered_set<int> next = cells;
            next.insert(frontier[trial]);
            if (addSweepClosure(grid, seed.direction, &next, maximumSize)) {
                cells = std::move(next);
                advanced = true;
                break;
            }
        }
        if (!advanced)
            break;
    }
    if (!found)
        return false;
    *result = std::move(best);
    return true;
}

bool localModelSatisfied(const Grid& grid, const QVector<int>& extractionDirections)
{
    for (int piece = 0; piece < extractionDirections.size(); ++piece) {
        const int extractionDirection = extractionDirections[piece];
        if (extractionDirection < 0)
            continue;
        std::array<bool, 6> blocked{{false, false, false, false, false, false}};
        for (int index = 0; index < int(grid.labels.size()); ++index) {
            if (grid.labels[index] != piece)
                continue;
            const Coord point = grid.coord(index);
            for (int direction = 0; direction < 6; ++direction) {
                Coord cursor = point + kDirections[direction];
                if (direction == extractionDirection) {
                    while (grid.contains(cursor)) {
                        const int label = grid.label(cursor);
                        if (label == kRemaining || label > piece)
                            return false;
                        cursor = cursor + kDirections[direction];
                    }
                } else {
                    const int label = grid.label(cursor);
                    if (label == kRemaining || label > piece)
                        blocked[direction] = true;
                }
            }
        }
        for (int direction = 0; direction < 6; ++direction)
            if (direction != extractionDirection && !blocked[direction])
                return false;
    }
    return true;
}

class CoreSearch {
public:
    CoreSearch(const PrintableInterlockParameters& parameters, std::mt19937& generator)
        : parameters_(parameters), generator_(generator)
    {
        budget_ = (std::max)(500, parameters.candidateLimit * parameters.pieceCount * 20);
    }

    bool run(const Grid& input, Grid* output, QVector<int>* directions)
    {
        QVector<int> currentDirections;
        return search(0, input, currentDirections, output, directions);
    }

private:
    bool search(int piece, const Grid& input, const QVector<int>& directions,
                Grid* output, QVector<int>* outputDirections)
    {
        if (nodes_ >= budget_)
            return false;
        if (piece == parameters_.pieceCount - 1) {
            ++nodes_;
            Grid trial = input;
            std::unordered_set<int> remaining;
            for (int index = 0; index < int(trial.labels.size()); ++index)
                if (trial.labels[index] == kRemaining)
                    remaining.insert(index);
            if (remaining.empty() || !extentAllowed(trial, remaining,
                                                    parameters_.maxPartExtentRatio))
                return false;
            for (int index : remaining)
                trial.labels[index] = piece;
            QVector<int> finalDirections = directions;
            finalDirections.push_back(-1);
            if (parameters_.enforceInterlocking
                && !localModelSatisfied(trial, finalDirections))
                return false;
            *output = std::move(trial);
            *outputDirections = std::move(finalDirections);
            return true;
        }

        const int piecesLeft = parameters_.pieceCount - piece;
        const int target = (std::max)(1, input.remainingCount() / piecesLeft);
        const std::vector<Seed> seeds = findSeeds(
            input, (std::max)(20, parameters_.candidateLimit * 3), generator_);
        std::vector<PartCandidate> candidates;
        for (const Seed& seed : seeds) {
            PartCandidate candidate;
            if (!growCandidate(input, seed, target, parameters_, generator_, &candidate))
                continue;
            Grid trial = input;
            for (int index : candidate.cells)
                trial.labels[index] = piece;
            QVector<int> trialDirections = directions;
            trialDirections.push_back(candidate.direction);
            if (!parameters_.enforceInterlocking
                || localModelSatisfied(trial, trialDirections))
                candidates.push_back(std::move(candidate));
        }
        std::stable_sort(candidates.begin(), candidates.end(),
            [](const PartCandidate& left, const PartCandidate& right) {
                return left.score < right.score;
            });
        if (int(candidates.size()) > parameters_.candidateLimit)
            candidates.resize(parameters_.candidateLimit);

        for (const PartCandidate& candidate : candidates) {
            if (++nodes_ > budget_)
                return false;
            Grid trial = input;
            for (int index : candidate.cells)
                trial.labels[index] = piece;
            QVector<int> trialDirections = directions;
            trialDirections.push_back(candidate.direction);
            if (search(piece + 1, trial, trialDirections, output, outputDirections))
                return true;
        }
        return false;
    }

    const PrintableInterlockParameters& parameters_;
    std::mt19937& generator_;
    int nodes_ = 0;
    int budget_ = 0;
};

std::unordered_set<int> cellsForPiece(const Grid& grid, int piece)
{
    std::unordered_set<int> result;
    for (int index = 0; index < int(grid.labels.size()); ++index)
        if (grid.labels[index] == piece)
            result.insert(index);
    return result;
}

double connectionToPiece(const Grid& grid, int cellIndex, int piece)
{
    const Coord point = grid.coord(cellIndex);
    double result = 0.0;
    for (int direction = 0; direction < 6; ++direction) {
        const Coord neighbor = point + kDirections[direction];
        if (!grid.contains(neighbor) || grid.label(neighbor) != piece)
            continue;
        const CellInfo& neighborInfo = grid.cells[grid.index(neighbor)];
        result += (std::min)(grid.cells[cellIndex].faceArea[direction],
                            neighborInfo.faceArea[direction ^ 1]);
    }
    return result;
}

bool attachBoundaryVoxels(Grid* grid, const QVector<int>& directions,
                          const PrintableInterlockParameters& parameters,
                          int* attachedCount)
{
    std::unordered_set<int> pending;
    for (int index = 0; index < int(grid->labels.size()); ++index)
        if (grid->labels[index] == kBoundary)
            pending.insert(index);
    *attachedCount = 0;
    double requiredConnection = parameters.minimumFaceContact;
    bool relaxedWeakConnections = false;

    while (!pending.empty()) {
        std::vector<int> order(pending.begin(), pending.end());
        std::stable_sort(order.begin(), order.end(), [&](int left, int right) {
            auto assignedNeighbors = [&](int index) {
                int count = 0;
                const Coord point = grid->coord(index);
                for (const Coord& direction : kDirections)
                    count += grid->label(point + direction) >= 0 ? 1 : 0;
                return count;
            };
            return assignedNeighbors(left) > assignedNeighbors(right);
        });
        int progress = 0;
        for (int index : order) {
            std::unordered_set<int> neighborPieces;
            const Coord point = grid->coord(index);
            for (const Coord& direction : kDirections) {
                const int label = grid->label(point + direction);
                if (label >= 0)
                    neighborPieces.insert(label);
            }
            int bestPiece = -1;
            double bestConnection = -1.0;
            for (int piece : neighborPieces) {
                const double connection = connectionToPiece(*grid, index, piece);
                if (connection <= 0.0 || connection < requiredConnection)
                    continue;
                Grid trial = *grid;
                trial.labels[index] = piece;
                const std::unordered_set<int> pieceCells = cellsForPiece(trial, piece);
                if (!extentAllowed(trial, pieceCells, parameters.maxPartExtentRatio))
                    continue;
                if (parameters.enforceInterlocking
                    && !localModelSatisfied(trial, directions))
                    continue;
                if (connection > bestConnection) {
                    bestConnection = connection;
                    bestPiece = piece;
                }
            }
            if (bestPiece >= 0) {
                grid->labels[index] = bestPiece;
                pending.erase(index);
                ++progress;
                ++*attachedCount;
            }
        }
        if (progress == 0 && requiredConnection > 0.0 && !relaxedWeakConnections) {
            requiredConnection = 0.0;
            relaxedWeakConnections = true;
            continue;
        }
        if (progress == 0)
            return false;
    }
    return true;
}

bool pieceConnected(const Grid& grid, int piece)
{
    int start = -1;
    int expected = 0;
    for (int index = 0; index < int(grid.labels.size()); ++index) {
        if (grid.labels[index] == piece) {
            start = index;
            ++expected;
        }
    }
    if (expected == 0)
        return false;
    std::vector<unsigned char> visited(grid.labels.size(), 0);
    std::queue<int> queue;
    queue.push(start);
    visited[start] = 1;
    int reached = 0;
    while (!queue.empty()) {
        const int index = queue.front();
        queue.pop();
        ++reached;
        const Coord point = grid.coord(index);
        for (const Coord& direction : kDirections) {
            const Coord next = point + direction;
            if (!grid.contains(next))
                continue;
            const int nextIndex = grid.index(next);
            if (!visited[nextIndex] && grid.labels[nextIndex] == piece) {
                visited[nextIndex] = 1;
                queue.push(nextIndex);
            }
        }
    }
    return reached == expected;
}

double localSeamEnergy(const Grid& grid, int index, int proposedLabel)
{
    const Coord point = grid.coord(index);
    double energy = 0.0;
    for (int direction = 0; direction < 6; ++direction) {
        const Coord neighbor = point + kDirections[direction];
        if (!grid.contains(neighbor))
            continue;
        const int neighborIndex = grid.index(neighbor);
        const int neighborLabel = grid.labels[neighborIndex];
        if (neighborLabel >= 0 && neighborLabel != proposedLabel) {
            energy += grid.cells[index].saliency * grid.cells[neighborIndex].saliency
                + connectionToPiece(grid, index, neighborLabel);
        }
    }
    return energy;
}

int refineSalientSeams(Grid* grid, const QVector<int>& directions,
                       const PrintableInterlockParameters& parameters)
{
    int swaps = 0;
    for (int pass = 0; pass < 2; ++pass) {
        std::vector<int> candidates;
        for (int index = 0; index < int(grid->labels.size()); ++index)
            if (grid->cells[index].occupied && !grid->cells[index].internal)
                candidates.push_back(index);
        std::stable_sort(candidates.begin(), candidates.end(), [&](int left, int right) {
            return grid->cells[left].saliency > grid->cells[right].saliency;
        });
        for (int index : candidates) {
            const int oldPiece = grid->labels[index];
            if (oldPiece < 0)
                continue;
            const double oldEnergy = localSeamEnergy(*grid, index, oldPiece);
            int bestPiece = oldPiece;
            double bestEnergy = oldEnergy;
            const Coord point = grid->coord(index);
            std::unordered_set<int> neighborPieces;
            for (const Coord& direction : kDirections) {
                const int label = grid->label(point + direction);
                if (label >= 0 && label != oldPiece)
                    neighborPieces.insert(label);
            }
            for (int piece : neighborPieces) {
                const double energy = localSeamEnergy(*grid, index, piece);
                if (energy + 1e-8 >= bestEnergy)
                    continue;
                Grid trial = *grid;
                trial.labels[index] = piece;
                if (!pieceConnected(trial, oldPiece) || !pieceConnected(trial, piece))
                    continue;
                if (!extentAllowed(trial, cellsForPiece(trial, piece),
                                   parameters.maxPartExtentRatio))
                    continue;
                if (parameters.enforceInterlocking
                    && !localModelSatisfied(trial, directions))
                    continue;
                bestEnergy = energy;
                bestPiece = piece;
            }
            if (bestPiece != oldPiece) {
                grid->labels[index] = bestPiece;
                ++swaps;
            }
        }
    }
    return swaps;
}

void appendTriangle(SteadyDissectionMeshData* output,
                    const QVector3D& a, const QVector3D& b, const QVector3D& c,
                    int piece)
{
    const unsigned int base = unsigned(output->vertices.size());
    output->vertices << a << b << c;
    output->faces << base << base + 1U << base + 2U;
    output->facePieces << piece;
}

void appendQuad(SteadyDissectionMeshData* output,
                const std::array<QVector3D, 4>& corners, int piece)
{
    appendTriangle(output, corners[0], corners[1], corners[2], piece);
    appendTriangle(output, corners[0], corners[2], corners[3], piece);
}

SteadyDissectionMeshData buildOriginalMesh(const SurfaceMesh& mesh)
{
    SteadyDissectionMeshData output;
    for (SurfaceMesh::Face_index face : mesh.faces()) {
        std::array<QVector3D, 3> points;
        int corner = 0;
        for (SurfaceMesh::Vertex_index vertex :
             CGAL::vertices_around_face(mesh.halfedge(face), mesh)) {
            if (corner >= 3)
                break;
            const Point point = mesh.point(vertex);
            points[corner++] = QVector3D(float(CGAL::to_double(point.x())),
                                         float(CGAL::to_double(point.y())),
                                         float(CGAL::to_double(point.z())));
        }
        if (corner == 3)
            appendTriangle(&output, points[0], points[1], points[2], -1);
    }
    return output;
}

std::vector<QVector3D> clipPolygonAxis(const std::vector<QVector3D>& input,
                                      int axis, float boundary, bool keepGreater)
{
    std::vector<QVector3D> output;
    if (input.empty())
        return output;
    auto inside = [&](const QVector3D& point) {
        return keepGreater ? point[axis] >= boundary - 1e-6f
                           : point[axis] <= boundary + 1e-6f;
    };
    QVector3D previous = input.back();
    bool previousInside = inside(previous);
    for (const QVector3D& current : input) {
        const bool currentInside = inside(current);
        if (currentInside != previousInside) {
            const float denominator = current[axis] - previous[axis];
            if (std::abs(denominator) > 1e-8f) {
                const float amount = (boundary - previous[axis]) / denominator;
                output.push_back(previous + amount * (current - previous));
            }
        }
        if (currentInside)
            output.push_back(current);
        previous = current;
        previousInside = currentInside;
    }
    return output;
}

std::vector<QVector3D> clipTriangleToCell(const std::array<QVector3D, 3>& triangle,
                                         const QVector3D& minimum,
                                         const QVector3D& maximum)
{
    std::vector<QVector3D> polygon(triangle.begin(), triangle.end());
    for (int axis = 0; axis < 3 && !polygon.empty(); ++axis) {
        polygon = clipPolygonAxis(polygon, axis, minimum[axis], true);
        polygon = clipPolygonAxis(polygon, axis, maximum[axis], false);
    }
    return polygon;
}

QVector3D gridPoint(const Grid& grid, float x, float y, float z)
{
    return QVector3D(float(grid.origin + x * grid.cellSize),
                     float(grid.origin + y * grid.cellSize),
                     float(grid.origin + z * grid.cellSize));
}

SteadyDissectionMeshData buildPartitionMesh(const SurfaceMesh& mesh, const Grid& grid)
{
    SteadyDissectionMeshData output;
    for (SurfaceMesh::Face_index face : mesh.faces()) {
        std::array<QVector3D, 3> points;
        int corner = 0;
        for (SurfaceMesh::Vertex_index vertex :
             CGAL::vertices_around_face(mesh.halfedge(face), mesh)) {
            if (corner >= 3)
                break;
            const Point point = mesh.point(vertex);
            points[corner] = QVector3D(float(CGAL::to_double(point.x())),
                                       float(CGAL::to_double(point.y())),
                                       float(CGAL::to_double(point.z())));
            ++corner;
        }
        if (corner != 3)
            continue;
        QVector3D triangleMinimum = points[0];
        QVector3D triangleMaximum = points[0];
        for (int i = 1; i < 3; ++i) {
            triangleMinimum.setX((std::min)(triangleMinimum.x(), points[i].x()));
            triangleMinimum.setY((std::min)(triangleMinimum.y(), points[i].y()));
            triangleMinimum.setZ((std::min)(triangleMinimum.z(), points[i].z()));
            triangleMaximum.setX((std::max)(triangleMaximum.x(), points[i].x()));
            triangleMaximum.setY((std::max)(triangleMaximum.y(), points[i].y()));
            triangleMaximum.setZ((std::max)(triangleMaximum.z(), points[i].z()));
        }
        Coord firstCell{
            int(std::floor((triangleMinimum.x() - grid.origin) / grid.cellSize)),
            int(std::floor((triangleMinimum.y() - grid.origin) / grid.cellSize)),
            int(std::floor((triangleMinimum.z() - grid.origin) / grid.cellSize))
        };
        Coord lastCell{
            int(std::floor((triangleMaximum.x() - grid.origin) / grid.cellSize)),
            int(std::floor((triangleMaximum.y() - grid.origin) / grid.cellSize)),
            int(std::floor((triangleMaximum.z() - grid.origin) / grid.cellSize))
        };
        firstCell.x = (std::max)(0, firstCell.x);
        firstCell.y = (std::max)(0, firstCell.y);
        firstCell.z = (std::max)(0, firstCell.z);
        lastCell.x = (std::min)(grid.resolution - 1, lastCell.x);
        lastCell.y = (std::min)(grid.resolution - 1, lastCell.y);
        lastCell.z = (std::min)(grid.resolution - 1, lastCell.z);
        for (int z = firstCell.z; z <= lastCell.z; ++z) {
            for (int y = firstCell.y; y <= lastCell.y; ++y) {
                for (int x = firstCell.x; x <= lastCell.x; ++x) {
                    const int piece = grid.label({x, y, z});
                    if (piece < 0)
                        continue;
                    const QVector3D minimum = gridPoint(grid, float(x), float(y), float(z));
                    const QVector3D maximum = gridPoint(
                        grid, float(x + 1), float(y + 1), float(z + 1));
                    const std::vector<QVector3D> polygon = clipTriangleToCell(
                        points, minimum, maximum);
                    if (polygon.size() < 3)
                        continue;
                    for (int i = 1; i + 1 < int(polygon.size()); ++i) {
                        if (QVector3D::crossProduct(polygon[i] - polygon[0],
                                                   polygon[i + 1] - polygon[0])
                                .lengthSquared() > 1e-12f)
                            appendTriangle(&output, polygon[0], polygon[i], polygon[i + 1], piece);
                    }
                }
            }
        }
    }

    for (int index = 0; index < int(grid.labels.size()); ++index) {
        const int piece = grid.labels[index];
        if (piece < 0)
            continue;
        const Coord cell = grid.coord(index);
        const float x = float(cell.x);
        const float y = float(cell.y);
        const float z = float(cell.z);
        for (int direction = 0; direction < 6; ++direction) {
            const Coord neighborCell = cell + kDirections[direction];
            const int neighborPiece = grid.label(neighborCell);
            if (neighborPiece < 0 || neighborPiece == piece)
                continue;
            const double contact = grid.contains(neighborCell)
                ? (std::min)(grid.cells[index].faceArea[direction],
                             grid.cells[grid.index(neighborCell)].faceArea[direction ^ 1])
                : 0.0;
            // A partial contact ratio does not describe where material lies on the
            // face. Drawing a centered square for it can protrude outside the solid.
            if (contact < 1.0 - 1e-9)
                continue;
            const float halfContact = 0.5f;
            const float low = 0.5f - halfContact;
            const float high = 0.5f + halfContact;
            switch (direction) {
            case 0:
                appendQuad(&output, {{gridPoint(grid, x + 1, y + low, z + low),
                                      gridPoint(grid, x + 1, y + high, z + low),
                                      gridPoint(grid, x + 1, y + high, z + high),
                                      gridPoint(grid, x + 1, y + low, z + high)}}, piece);
                break;
            case 1:
                appendQuad(&output, {{gridPoint(grid, x, y + low, z + high),
                                      gridPoint(grid, x, y + high, z + high),
                                      gridPoint(grid, x, y + high, z + low),
                                      gridPoint(grid, x, y + low, z + low)}}, piece);
                break;
            case 2:
                appendQuad(&output, {{gridPoint(grid, x + low, y + 1, z + high),
                                      gridPoint(grid, x + high, y + 1, z + high),
                                      gridPoint(grid, x + high, y + 1, z + low),
                                      gridPoint(grid, x + low, y + 1, z + low)}}, piece);
                break;
            case 3:
                appendQuad(&output, {{gridPoint(grid, x + low, y, z + low),
                                      gridPoint(grid, x + high, y, z + low),
                                      gridPoint(grid, x + high, y, z + high),
                                      gridPoint(grid, x + low, y, z + high)}}, piece);
                break;
            case 4:
                appendQuad(&output, {{gridPoint(grid, x + high, y + low, z + 1),
                                      gridPoint(grid, x + high, y + high, z + 1),
                                      gridPoint(grid, x + low, y + high, z + 1),
                                      gridPoint(grid, x + low, y + low, z + 1)}}, piece);
                break;
            default:
                appendQuad(&output, {{gridPoint(grid, x + low, y + low, z),
                                      gridPoint(grid, x + low, y + high, z),
                                      gridPoint(grid, x + high, y + high, z),
                                      gridPoint(grid, x + high, y + low, z)}}, piece);
                break;
            }
        }
    }
    return output;
}

bool buildVoxelSolid(const Grid& grid, int piece, SurfaceMesh* output, QString* error)
{
    output->clear();
    const int vertexSide = grid.resolution + 1;
    std::vector<int> vertices(vertexSide * vertexSide * vertexSide, -1);
    std::vector<Point> points;
    std::vector<std::array<std::size_t, 3>> triangles;
    auto vertexAt = [&](const Coord& point) {
        const int key = (point.z * vertexSide + point.y) * vertexSide + point.x;
        int& vertex = vertices[key];
        if (vertex < 0) {
            vertex = int(points.size());
            points.emplace_back(
                grid.origin + point.x * grid.cellSize,
                grid.origin + point.y * grid.cellSize,
                grid.origin + point.z * grid.cellSize);
        }
        return std::size_t(vertex);
    };
    auto addQuad = [&](const std::array<Coord, 4>& corners) {
        const std::size_t a = vertexAt(corners[0]);
        const std::size_t b = vertexAt(corners[1]);
        const std::size_t c = vertexAt(corners[2]);
        const std::size_t d = vertexAt(corners[3]);
        triangles.push_back({{a, b, c}});
        triangles.push_back({{a, c, d}});
        return true;
    };

    bool valid = true;
    for (int index = 0; index < int(grid.labels.size()) && valid; ++index) {
        if (grid.labels[index] != piece)
            continue;
        const Coord cell = grid.coord(index);
        const int x = cell.x;
        const int y = cell.y;
        const int z = cell.z;
        for (int direction = 0; direction < 6 && valid; ++direction) {
            if (grid.label(cell + kDirections[direction]) == piece)
                continue;
            switch (direction) {
            case 0:
                valid = addQuad({{{x + 1, y, z}, {x + 1, y + 1, z},
                                  {x + 1, y + 1, z + 1}, {x + 1, y, z + 1}}});
                break;
            case 1:
                valid = addQuad({{{x, y, z + 1}, {x, y + 1, z + 1},
                                  {x, y + 1, z}, {x, y, z}}});
                break;
            case 2:
                valid = addQuad({{{x, y + 1, z + 1}, {x + 1, y + 1, z + 1},
                                  {x + 1, y + 1, z}, {x, y + 1, z}}});
                break;
            case 3:
                valid = addQuad({{{x, y, z}, {x + 1, y, z},
                                  {x + 1, y, z + 1}, {x, y, z + 1}}});
                break;
            case 4:
                valid = addQuad({{{x + 1, y, z + 1}, {x + 1, y + 1, z + 1},
                                  {x, y + 1, z + 1}, {x, y, z + 1}}});
                break;
            default:
                valid = addQuad({{{x, y, z}, {x, y + 1, z},
                                  {x + 1, y + 1, z}, {x + 1, y, z}}});
                break;
            }
        }
    }

    if (!valid || triangles.empty()) {
        *error = QString("Part %1 does not form a manifold closed voxel solid.").arg(piece + 1);
        return false;
    }

    const bool soupWasManifold = PMP::orient_polygon_soup(points, triangles);
    PMP::duplicate_non_manifold_edges_in_polygon_soup(points, triangles);
    PMP::polygon_soup_to_polygon_mesh(points, triangles, *output);
    PMP::stitch_borders(*output);
    int singularGroupCount = 0;
    auto insetVertices = [&](const std::unordered_set<int>& marked, double amount) {
        std::vector<std::pair<SurfaceMesh::Vertex_index, Kernel::Vector_3>> displacements;
        for (SurfaceMesh::Vertex_index vertex : output->vertices()) {
            if (!marked.count(vertex.idx()))
                continue;
            Kernel::Vector_3 towardUmbrella = CGAL::NULL_VECTOR;
            int neighborCount = 0;
            for (SurfaceMesh::Vertex_index neighbor :
                 CGAL::vertices_around_target(output->halfedge(vertex), *output)) {
                towardUmbrella = towardUmbrella + (output->point(neighbor) - output->point(vertex));
                ++neighborCount;
            }
            if (neighborCount > 0)
                towardUmbrella = towardUmbrella / neighborCount;
            const double length = std::sqrt(CGAL::to_double(towardUmbrella.squared_length()));
            if (length > 1e-12)
                displacements.push_back({vertex, towardUmbrella * (amount / length)});
        }
        for (const auto& displacement : displacements)
            output->point(displacement.first) = output->point(displacement.first)
                + displacement.second;
    };
    if (!soupWasManifold) {
        std::map<std::tuple<double, double, double>,
                 std::vector<SurfaceMesh::Vertex_index>> coincidentVertices;
        for (SurfaceMesh::Vertex_index vertex : output->vertices()) {
            const Point point = output->point(vertex);
            coincidentVertices[{CGAL::to_double(point.x()), CGAL::to_double(point.y()),
                                CGAL::to_double(point.z())}].push_back(vertex);
        }
        const double inset = grid.cellSize * 0.02;
        std::unordered_set<int> singularVertices;
        for (const auto& entry : coincidentVertices) {
            if (entry.second.size() < 2)
                continue;
            ++singularGroupCount;
            for (SurfaceMesh::Vertex_index vertex : entry.second)
                singularVertices.insert(vertex.idx());
        }
        insetVertices(singularVertices, inset);
    }

    if (output->is_empty() || !CGAL::is_triangle_mesh(*output)
        || !CGAL::is_closed(*output)) {
        *error = QString("Part %1 does not form a closed voxel solid "
                         "(empty: %2; triangles: %3; closed: %4; soup manifold: %5).")
                     .arg(piece + 1)
                     .arg(output->is_empty() ? "yes" : "no")
                     .arg(CGAL::is_triangle_mesh(*output) ? "yes" : "no")
                     .arg(CGAL::is_closed(*output) ? "yes" : "no")
                     .arg(soupWasManifold ? "yes" : "no");
        return false;
    }
    const bool selfIntersecting = PMP::does_self_intersect(*output);
    const bool boundsVolume = !selfIntersecting && PMP::does_bound_a_volume(*output);
    if (selfIntersecting || !boundsVolume) {
        *error = QString("Part %1 has an invalid voxel boundary (%2). "
                         "Soup manifold: %3; singular groups: %4.")
                     .arg(piece + 1)
                     .arg(selfIntersecting ? "self-intersection" : "invalid orientation")
                     .arg(soupWasManifold ? "yes" : "no")
                     .arg(singularGroupCount);
        return false;
    }
    auto componentMap = output->add_property_map<
        SurfaceMesh::Face_index, std::size_t>("f:voxel_component", 0).first;
    const std::size_t componentCount = PMP::connected_components(*output, componentMap);
    output->remove_property_map(componentMap);
    if (componentCount != 1) {
        *error = QString("Part %1 contains %2 disconnected voxel shells. "
                         "Try a different seed or a higher voxel resolution.")
                     .arg(piece + 1).arg(componentCount);
        return false;
    }
    PMP::orient_to_bound_a_volume(*output);
    return true;
}

SteadyDissectionMeshData surfaceMeshData(const SurfaceMesh& mesh, int piece)
{
    SteadyDissectionMeshData output;
    std::unordered_map<int, unsigned int> vertexMap;
    vertexMap.reserve(mesh.number_of_vertices());
    for (SurfaceMesh::Vertex_index vertex : mesh.vertices()) {
        const Point point = mesh.point(vertex);
        const unsigned int index = unsigned(output.vertices.size());
        vertexMap.emplace(vertex.idx(), index);
        output.vertices << QVector3D(float(CGAL::to_double(point.x())),
                                     float(CGAL::to_double(point.y())),
                                     float(CGAL::to_double(point.z())));
    }
    for (SurfaceMesh::Face_index face : mesh.faces()) {
        int corners = 0;
        for (SurfaceMesh::Vertex_index vertex :
             CGAL::vertices_around_face(mesh.halfedge(face), mesh)) {
            output.faces << vertexMap.at(vertex.idx());
            ++corners;
        }
        if (corners != 3) {
            output.faces.resize(output.faces.size() - corners);
            continue;
        }
        output.facePieces << piece;
    }
    return output;
}

void appendMeshData(SteadyDissectionMeshData* destination,
                    const SteadyDissectionMeshData& source)
{
    const unsigned int offset = unsigned(destination->vertices.size());
    destination->vertices += source.vertices;
    destination->facePieces += source.facePieces;
    destination->faces.reserve(destination->faces.size() + source.faces.size());
    for (unsigned int vertex : source.faces)
        destination->faces << offset + vertex;
}

bool buildPrintableSolidParts(const SurfaceMesh& original, const Grid& grid,
                              int pieceCount,
                              QVector<SteadyDissectionMeshData>* voxelParts,
                              SteadyDissectionMeshData* voxelCombined,
                              QVector<SteadyDissectionMeshData>* clippedParts,
                              SteadyDissectionMeshData* clippedCombined,
                              QString* error)
{
    voxelParts->clear();
    clippedParts->clear();
    *voxelCombined = SteadyDissectionMeshData();
    *clippedCombined = SteadyDissectionMeshData();
    voxelParts->reserve(pieceCount);
    clippedParts->reserve(pieceCount);

    for (int piece = 0; piece < pieceCount; ++piece) {
        SurfaceMesh voxelSolid;
        if (!buildVoxelSolid(grid, piece, &voxelSolid, error))
            return false;
        SteadyDissectionMeshData voxelData = surfaceMeshData(voxelSolid, piece);
        appendMeshData(voxelCombined, voxelData);
        voxelParts->push_back(std::move(voxelData));

        SurfaceMesh source = original;
        SurfaceMesh intersection;
        bool computed = false;
        try {
            computed = PMP::corefine_and_compute_intersection(
                source, voxelSolid, intersection);
        } catch (const std::exception& exception) {
            *error = QString("Boolean intersection failed for part %1: %2")
                         .arg(piece + 1).arg(exception.what());
            return false;
        }
        PMP::remove_isolated_vertices(intersection);
        if (!computed || intersection.is_empty() || !CGAL::is_triangle_mesh(intersection)
            || !CGAL::is_closed(intersection)) {
            *error = QString("Part %1 did not produce a closed solid intersection.")
                         .arg(piece + 1);
            return false;
        }
        const bool selfIntersecting = PMP::does_self_intersect(intersection);
        const bool boundsVolume = !selfIntersecting && PMP::does_bound_a_volume(intersection);
        if (selfIntersecting || !boundsVolume) {
            *error = QString("Part %1 intersection is not a printable watertight volume (%2).")
                         .arg(piece + 1)
                         .arg(selfIntersecting ? "self-intersection" : "invalid orientation");
            return false;
        }
        auto componentMap = intersection.add_property_map<
            SurfaceMesh::Face_index, std::size_t>("f:printable_component", 0).first;
        const std::size_t componentCount = PMP::connected_components(
            intersection, componentMap);
        intersection.remove_property_map(componentMap);
        if (componentCount != 1) {
            *error = QString("Part %1 intersects the original solid in %2 disconnected volumes. "
                             "Try a different seed or a higher voxel resolution.")
                         .arg(piece + 1).arg(componentCount);
            return false;
        }
        PMP::orient_to_bound_a_volume(intersection);
        SteadyDissectionMeshData data = surfaceMeshData(intersection, piece);
        if (data.faces.isEmpty()) {
            *error = QString("Part %1 produced no printable triangles.").arg(piece + 1);
            return false;
        }
        appendMeshData(clippedCombined, data);
        clippedParts->push_back(std::move(data));
    }
    return true;
}

SteadyDissectionMeshData buildAnalysisMesh(const Grid& grid)
{
    SteadyDissectionMeshData output;
    for (int index = 0; index < int(grid.labels.size()); ++index) {
        if (!grid.cells[index].occupied)
            continue;
        const int piece = grid.cells[index].internal ? 2 : -1;
        const Coord cell = grid.coord(index);
        const float x = float(cell.x);
        const float y = float(cell.y);
        const float z = float(cell.z);
        for (int direction = 0; direction < 6; ++direction) {
            const Coord neighbor = cell + kDirections[direction];
            if (grid.contains(neighbor) && grid.cells[grid.index(neighbor)].occupied)
                continue;
            switch (direction) {
            case 0:
                appendQuad(&output, {{gridPoint(grid, x + 1, y, z), gridPoint(grid, x + 1, y + 1, z),
                                      gridPoint(grid, x + 1, y + 1, z + 1), gridPoint(grid, x + 1, y, z + 1)}}, piece);
                break;
            case 1:
                appendQuad(&output, {{gridPoint(grid, x, y, z + 1), gridPoint(grid, x, y + 1, z + 1),
                                      gridPoint(grid, x, y + 1, z), gridPoint(grid, x, y, z)}}, piece);
                break;
            case 2:
                appendQuad(&output, {{gridPoint(grid, x, y + 1, z + 1), gridPoint(grid, x + 1, y + 1, z + 1),
                                      gridPoint(grid, x + 1, y + 1, z), gridPoint(grid, x, y + 1, z)}}, piece);
                break;
            case 3:
                appendQuad(&output, {{gridPoint(grid, x, y, z), gridPoint(grid, x + 1, y, z),
                                      gridPoint(grid, x + 1, y, z + 1), gridPoint(grid, x, y, z + 1)}}, piece);
                break;
            case 4:
                appendQuad(&output, {{gridPoint(grid, x + 1, y, z + 1), gridPoint(grid, x + 1, y + 1, z + 1),
                                      gridPoint(grid, x, y + 1, z + 1), gridPoint(grid, x, y, z + 1)}}, piece);
                break;
            default:
                appendQuad(&output, {{gridPoint(grid, x, y, z), gridPoint(grid, x, y + 1, z),
                                      gridPoint(grid, x + 1, y + 1, z), gridPoint(grid, x + 1, y, z)}}, piece);
                break;
            }
        }
    }
    return output;
}

QVector<int> pieceCounts(const Grid& grid, int pieceCount)
{
    QVector<int> counts(pieceCount, 0);
    for (int label : grid.labels)
        if (label >= 0 && label < pieceCount)
            ++counts[label];
    return counts;
}

class Engine {
public:
    Engine(const PrintableInterlockParameters& parameters,
           const PrintableInterlockAlgorithm::SnapshotCallback& callback)
        : parameters_(parameters), callback_(callback), generator_(parameters.randomSeed)
    {
    }

    bool execute(const QString& meshPath, QString* error)
    {
        if (parameters_.resolution < 8 || parameters_.resolution > 35) {
            *error = "Voxel resolution must be between 8 and 35.";
            return false;
        }
        if (parameters_.samplesPerVoxel < 2 || parameters_.samplesPerVoxel > 6) {
            *error = "Samples per voxel must be between 2 and 6.";
            return false;
        }
        if (parameters_.pieceCount < 3 || parameters_.pieceCount > 30) {
            *error = "Piece count must be between 3 and 30.";
            return false;
        }
        if (parameters_.candidateLimit < 1 || parameters_.candidateLimit > 40) {
            *error = "Candidate limit must be between 1 and 40.";
            return false;
        }

        if (!loadAndValidateMesh(meshPath, &mesh_, error))
            return false;
        original_ = buildOriginalMesh(mesh_.mesh);
        if (!voxelizeAndAnalyze(mesh_.mesh, parameters_, &grid_, &tinyCount_,
                                &disconnectedCount_, error))
            return false;
        occupiedCount_ = int(std::count_if(grid_.cells.begin(), grid_.cells.end(),
            [](const CellInfo& info) { return info.occupied; }));
        internalCount_ = int(std::count_if(grid_.cells.begin(), grid_.cells.end(),
            [](const CellInfo& info) { return info.internal; }));
        boundaryCount_ = occupiedCount_ - internalCount_;
        if (!publish("Voxelized model and analyzed local shape", false,
                     buildAnalysisMesh(grid_), false))
            return true;

        CoreSearch search(parameters_, generator_);
        Grid coreResult;
        if (!search.run(grid_, &coreResult, &directions_)) {
            *error = "Could not construct an interlocking core. Try more candidates, fewer parts, "
                     "a lower internal-volume threshold, or a different random seed.";
            return false;
        }
        grid_ = std::move(coreResult);
        if (!publish("Generated initial interlocking parts from internal voxels", false,
                     buildPartitionMesh(mesh_.mesh, grid_),
                     localModelSatisfied(grid_, directions_)))
            return true;

        if (!attachBoundaryVoxels(&grid_, directions_, parameters_, &attachedBoundaryCount_)) {
            *error = "Some boundary voxels cannot be attached without blocking the disassembly "
                     "sequence. Try more candidates, a higher resolution, or a different seed.";
            return false;
        }
        if (!publish("Attached boundary voxels by shape connection strength", false,
                     buildPartitionMesh(mesh_.mesh, grid_),
                     localModelSatisfied(grid_, directions_)))
            return true;

        if (parameters_.refineSalientSeams)
            seamSwapCount_ = refineSalientSeams(&grid_, directions_, parameters_);
        bool interlocking = localModelSatisfied(grid_, directions_);
        if (parameters_.enforceInterlocking && !interlocking) {
            *error = "Boundary or seam refinement broke the interlocking sequence.";
            return false;
        }
        for (int index = 0; index < int(grid_.cells.size()); ++index) {
            if (grid_.cells[index].occupied && grid_.labels[index] < 0) {
                *error = "Internal error: the final partition did not cover every occupied voxel.";
                return false;
            }
        }
        if (!publish("Preparing watertight voxel solids", false,
                     buildPartitionMesh(mesh_.mesh, grid_), interlocking))
            return true;
        const SteadyDissectionMeshData preview = buildPartitionMesh(mesh_.mesh, grid_);
        if (!publish("Computing watertight solid intersections", false,
                     preview, interlocking))
            return true;
        SteadyDissectionMeshData voxelModel;
        SteadyDissectionMeshData printableModel;
        if (!buildPrintableSolidParts(mesh_.mesh, grid_, parameters_.pieceCount,
                                      &voxelParts_, &voxelModel,
                                      &printableParts_, &printableModel, error))
            return false;
        voxelModel_ = std::move(voxelModel);
        return publish(parameters_.refineSalientSeams
                           ? QString("Complete watertight solids (%1 seam swaps)")
                                 .arg(seamSwapCount_)
                           : "Complete watertight interlocking solids",
                       true, printableModel, interlocking);
    }

private:
    bool publish(const QString& phase, bool complete,
                 const SteadyDissectionMeshData& partitioned, bool interlocking)
    {
        if (!callback_)
            return true;
        PrintableInterlockSnapshot snapshot;
        snapshot.originalModel = original_;
        if (complete)
            snapshot.voxelizedModel = voxelModel_;
        snapshot.partitionedModel = partitioned;
        if (complete) {
            snapshot.voxelParts = voxelParts_;
            snapshot.printableParts = printableParts_;
        }
        snapshot.pieceVoxelCounts = pieceCounts(grid_, parameters_.pieceCount);
        snapshot.extractionDirections = directions_;
        snapshot.sourceCenter = mesh_.sourceCenter;
        snapshot.sourceUnitsPerNormalizedUnit = mesh_.sourceUnitsPerNormalizedUnit;
        snapshot.phase = phase;
        snapshot.resolution = parameters_.resolution;
        snapshot.requestedPieces = parameters_.pieceCount;
        snapshot.completedPieces = int(std::count_if(
            snapshot.pieceVoxelCounts.begin(), snapshot.pieceVoxelCounts.end(),
            [](int count) { return count > 0; }));
        snapshot.occupiedVoxels = occupiedCount_;
        snapshot.internalVoxels = internalCount_;
        snapshot.boundaryVoxels = boundaryCount_;
        snapshot.attachedBoundaryVoxels = attachedBoundaryCount_;
        snapshot.tinyVoxels = tinyCount_;
        snapshot.disconnectedVoxels = disconnectedCount_;
        snapshot.voxelWatertightParts = complete ? voxelParts_.size() : 0;
        snapshot.watertightParts = complete ? printableParts_.size() : 0;
        snapshot.interlocking = interlocking;
        snapshot.complete = complete;
        return callback_(snapshot);
    }

    PrintableInterlockParameters parameters_;
    PrintableInterlockAlgorithm::SnapshotCallback callback_;
    std::mt19937 generator_;
    LoadedMesh mesh_;
    Grid grid_;
    SteadyDissectionMeshData original_;
    SteadyDissectionMeshData voxelModel_;
    QVector<int> directions_;
    QVector<SteadyDissectionMeshData> voxelParts_;
    QVector<SteadyDissectionMeshData> printableParts_;
    int occupiedCount_ = 0;
    int internalCount_ = 0;
    int boundaryCount_ = 0;
    int attachedBoundaryCount_ = 0;
    int tinyCount_ = 0;
    int disconnectedCount_ = 0;
    int seamSwapCount_ = 0;
};
} // namespace

bool PrintableInterlockAlgorithm::run(
    const QString& meshPath, const PrintableInterlockParameters& parameters,
    const SnapshotCallback& callback, QString* errorMessage)
{
    QString localError;
    Engine engine(parameters, callback);
    const bool result = engine.execute(meshPath, &localError);
    if (errorMessage)
        *errorMessage = localError;
    return result;
}
