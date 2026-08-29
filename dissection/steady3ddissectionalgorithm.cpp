#include "steady3ddissectionalgorithm.h"

#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Polygon_mesh_processing/measure.h>
#include <CGAL/Polygon_mesh_processing/connected_components.h>
#include <CGAL/Polygon_mesh_processing/self_intersections.h>
#include <CGAL/Polygon_mesh_processing/shape_predicates.h>
#include <CGAL/Side_of_triangle_mesh.h>
#include <CGAL/Surface_mesh.h>
#include <CGAL/boost/graph/IO/polygon_mesh_io.h>
#include <CGAL/boost/graph/helpers.h>

#include <QFileInfo>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <numeric>
#include <queue>
#include <random>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace {
namespace PMP = CGAL::Polygon_mesh_processing;
using Kernel = CGAL::Exact_predicates_inexact_constructions_kernel;
using Point = Kernel::Point_3;
using SurfaceMesh = CGAL::Surface_mesh<Point>;

constexpr int kEmpty = -2;
constexpr int kRemaining = -1;

struct Coord {
    int x = 0;
    int y = 0;
    int z = 0;

    bool operator==(const Coord& other) const
    {
        return x == other.x && y == other.y && z == other.z;
    }
    bool operator!=(const Coord& other) const { return !(*this == other); }
    Coord operator+(const Coord& other) const { return {x + other.x, y + other.y, z + other.z}; }
    Coord operator-(const Coord& other) const { return {x - other.x, y - other.y, z - other.z}; }
    Coord operator*(int scalar) const { return {x * scalar, y * scalar, z * scalar}; }
};

struct CoordHash {
    std::size_t operator()(const Coord& value) const
    {
        const std::uint64_t x = std::uint32_t(value.x * 73856093);
        const std::uint64_t y = std::uint32_t(value.y * 19349663);
        const std::uint64_t z = std::uint32_t(value.z * 83492791);
        return std::size_t(x ^ (y << 1U) ^ (z << 2U));
    }
};

const std::array<Coord, 6> kDirections{{
    {1, 0, 0}, {-1, 0, 0}, {0, 1, 0},
    {0, -1, 0}, {0, 0, 1}, {0, 0, -1}
}};

struct Rotation {
    int value[3][3]{};

    Coord apply(const Coord& point) const
    {
        return {
            value[0][0] * point.x + value[0][1] * point.y + value[0][2] * point.z,
            value[1][0] * point.x + value[1][1] * point.y + value[1][2] * point.z,
            value[2][0] * point.x + value[2][1] * point.y + value[2][2] * point.z
        };
    }

    Coord inverseApply(const Coord& point) const
    {
        return {
            value[0][0] * point.x + value[1][0] * point.y + value[2][0] * point.z,
            value[0][1] * point.x + value[1][1] * point.y + value[2][1] * point.z,
            value[0][2] * point.x + value[1][2] * point.y + value[2][2] * point.z
        };
    }
};

int permutationSign(const std::array<int, 3>& permutation)
{
    int inversions = 0;
    for (int i = 0; i < 3; ++i)
        for (int j = i + 1; j < 3; ++j)
            inversions += permutation[i] > permutation[j] ? 1 : 0;
    return inversions % 2 == 0 ? 1 : -1;
}

std::vector<Rotation> cubeRotations()
{
    std::vector<Rotation> result;
    std::array<int, 3> permutation{{0, 1, 2}};
    do {
        const int parity = permutationSign(permutation);
        for (int sx : {-1, 1}) {
            for (int sy : {-1, 1}) {
                for (int sz : {-1, 1}) {
                    if (parity * sx * sy * sz != 1)
                        continue;
                    Rotation rotation;
                    rotation.value[0][permutation[0]] = sx;
                    rotation.value[1][permutation[1]] = sy;
                    rotation.value[2][permutation[2]] = sz;
                    result.push_back(rotation);
                }
            }
        }
    } while (std::next_permutation(permutation.begin(), permutation.end()));
    return result;
}

Rotation identityRotation()
{
    Rotation rotation;
    rotation.value[0][0] = 1;
    rotation.value[1][1] = 1;
    rotation.value[2][2] = 1;
    return rotation;
}

struct Grid {
    int resolution = 0;
    std::vector<int> labels;
    std::vector<unsigned char> originalBoundary;
    int inputVoxelCount = 0;

    explicit Grid(int size = 0)
        : resolution(size), labels(size * size * size, kEmpty),
          originalBoundary(size * size * size, 0)
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

    Coord coord(int indexValue) const
    {
        Coord point;
        point.x = indexValue % resolution;
        indexValue /= resolution;
        point.y = indexValue % resolution;
        point.z = indexValue / resolution;
        return point;
    }

    int label(const Coord& point) const
    {
        return contains(point) ? labels[index(point)] : kEmpty;
    }

    bool isRemaining(const Coord& point) const { return label(point) == kRemaining; }

    int remainingCount() const
    {
        return int(std::count(labels.begin(), labels.end(), kRemaining));
    }

    int deletedCount() const
    {
        return inputVoxelCount - int(std::count_if(labels.begin(), labels.end(),
            [](int labelValue) { return labelValue != kEmpty; }));
    }

    int deletedBoundaryCount() const
    {
        int result = 0;
        for (int i = 0; i < int(labels.size()); ++i)
            if (labels[i] == kEmpty && originalBoundary[i])
                ++result;
        return result;
    }
};

bool hasSameRemainingCells(const Grid& first, const Grid& second)
{
    if (first.resolution != second.resolution
        || first.labels.size() != second.labels.size())
        return false;
    for (int index = 0; index < int(first.labels.size()); ++index) {
        if ((first.labels[index] == kRemaining) != (second.labels[index] == kRemaining))
            return false;
    }
    return true;
}

struct LoadedMesh {
    SurfaceMesh mesh;
    Point center;
    double volume = 0.0;
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
    if (!CGAL::is_triangle_mesh(result->mesh)) {
        *error = QString("%1 is not a triangle-only mesh.").arg(QFileInfo(path).fileName());
        return false;
    }
    for (SurfaceMesh::Face_index face : result->mesh.faces()) {
        if (PMP::is_degenerate_triangle_face(face, result->mesh)) {
            *error = QString("%1 contains degenerate triangles. Repair the mesh before voxelization.")
                         .arg(QFileInfo(path).fileName());
            return false;
        }
    }
    if (!CGAL::is_closed(result->mesh)) {
        *error = QString("%1 is open. Steady 3D dissection needs a watertight surface that bounds a solid volume.")
                     .arg(QFileInfo(path).fileName());
        return false;
    }
    auto componentMap = result->mesh.add_property_map<SurfaceMesh::Face_index, std::size_t>(
        "f:steady_dissection_component", 0).first;
    const std::size_t componentCount = PMP::connected_components(
        result->mesh, componentMap);
    result->mesh.remove_property_map(componentMap);
    if (componentCount != 1) {
        *error = QString("%1 contains multiple disconnected shells. Provide one connected watertight solid.")
                     .arg(QFileInfo(path).fileName());
        return false;
    }
    if (PMP::does_self_intersect(result->mesh)) {
        *error = QString("%1 contains self-intersections. Repair the mesh before voxelization.")
                     .arg(QFileInfo(path).fileName());
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
    result->volume = std::abs(CGAL::to_double(PMP::volume(result->mesh)));
    if (!(result->volume > 1e-12) || !std::isfinite(result->volume)) {
        *error = QString("%1 does not enclose a usable non-zero volume.")
                     .arg(QFileInfo(path).fileName());
        return false;
    }
    return true;
}

void normalizeMesh(LoadedMesh* loaded, double scale)
{
    for (SurfaceMesh::Vertex_index vertex : loaded->mesh.vertices()) {
        const Point point = loaded->mesh.point(vertex);
        loaded->mesh.point(vertex) = Point(
            (CGAL::to_double(point.x()) - CGAL::to_double(loaded->center.x())) * scale,
            (CGAL::to_double(point.y()) - CGAL::to_double(loaded->center.y())) * scale,
            (CGAL::to_double(point.z()) - CGAL::to_double(loaded->center.z())) * scale);
    }
    loaded->center = Point(0.0, 0.0, 0.0);
    loaded->volume *= scale * scale * scale;
}

double maximumExtent(const SurfaceMesh& mesh)
{
    CGAL::Bbox_3 box;
    bool first = true;
    for (SurfaceMesh::Vertex_index vertex : mesh.vertices()) {
        const CGAL::Bbox_3 pointBox = mesh.point(vertex).bbox();
        box = first ? pointBox : box + pointBox;
        first = false;
    }
    return (std::max)({box.xmax() - box.xmin(), box.ymax() - box.ymin(),
                       box.zmax() - box.zmin()});
}

bool voxelize(const SurfaceMesh& mesh, int resolution, double cellSize, Grid* grid,
              QString* error)
{
    *grid = Grid(resolution);
    CGAL::Side_of_triangle_mesh<SurfaceMesh, Kernel> side(mesh);
    const double half = 0.5 * double(resolution) * cellSize;
    for (int z = 0; z < resolution; ++z) {
        for (int y = 0; y < resolution; ++y) {
            for (int x = 0; x < resolution; ++x) {
                const Point sample(-half + (x + 0.5) * cellSize,
                                   -half + (y + 0.5) * cellSize,
                                   -half + (z + 0.5) * cellSize);
                const CGAL::Bounded_side location = side(sample);
                if (location != CGAL::ON_UNBOUNDED_SIDE)
                    grid->labels[grid->index({x, y, z})] = kRemaining;
            }
        }
    }
    grid->inputVoxelCount = grid->remainingCount();
    if (grid->inputVoxelCount == 0) {
        *error = "Voxelization produced no occupied cells. Increase the resolution or repair the mesh.";
        return false;
    }
    for (int index = 0; index < int(grid->labels.size()); ++index) {
        if (grid->labels[index] != kRemaining)
            continue;
        const Coord point = grid->coord(index);
        for (const Coord& direction : kDirections) {
            if (!grid->isRemaining(point + direction)) {
                grid->originalBoundary[index] = 1;
                break;
            }
        }
    }
    return true;
}

bool rayClear(const Grid& grid, const Coord& point, int direction,
              const std::unordered_set<int>* virtualDeleted = nullptr)
{
    Coord cursor = point + kDirections[direction];
    while (grid.contains(cursor)) {
        const int index = grid.index(cursor);
        if (grid.labels[index] == kRemaining
            && (!virtualDeleted || virtualDeleted->count(index) == 0))
            return false;
        cursor = cursor + kDirections[direction];
    }
    return true;
}

int remainingNeighborCount(const Grid& grid, const Coord& point)
{
    int result = 0;
    for (const Coord& direction : kDirections)
        result += grid.isRemaining(point + direction) ? 1 : 0;
    return result;
}

struct Seed {
    Coord point;
    std::vector<int> directions;
    int neighbors = 0;
};

std::vector<Seed> findSeeds(const Grid& grid, int limit)
{
    std::vector<Seed> seeds;
    for (int index = 0; index < int(grid.labels.size()); ++index) {
        if (grid.labels[index] != kRemaining)
            continue;
        Seed seed;
        seed.point = grid.coord(index);
        seed.neighbors = remainingNeighborCount(grid, seed.point);
        for (int direction = 0; direction < 6; ++direction)
            if (rayClear(grid, seed.point, direction))
                seed.directions.push_back(direction);
        if (!seed.directions.empty())
            seeds.push_back(seed);
    }
    std::stable_sort(seeds.begin(), seeds.end(), [](const Seed& left, const Seed& right) {
        if (left.neighbors != right.neighbors)
            return left.neighbors > right.neighbors;
        if (left.directions.size() != right.directions.size())
            return left.directions.size() < right.directions.size();
        if (left.point.z != right.point.z)
            return left.point.z > right.point.z;
        if (left.point.y != right.point.y)
            return left.point.y < right.point.y;
        return left.point.x < right.point.x;
    });
    if (int(seeds.size()) > limit)
        seeds.resize(limit);
    return seeds;
}

int localCompatibility(const Grid& first, const Grid& second,
                       const Coord& firstSeed, const Coord& secondSeed,
                       const Rotation& rotation)
{
    int common = 0;
    int mismatch = 0;
    for (int z = -2; z <= 2; ++z) {
        for (int y = -2; y <= 2; ++y) {
            for (int x = -2; x <= 2; ++x) {
                if (std::abs(x) + std::abs(y) + std::abs(z) > 3)
                    continue;
                const Coord offset{x, y, z};
                const bool firstOccupied = first.isRemaining(firstSeed + offset);
                const bool secondOccupied = second.isRemaining(secondSeed + rotation.apply(offset));
                common += firstOccupied && secondOccupied ? 1 : 0;
                mismatch += firstOccupied != secondOccupied ? 1 : 0;
            }
        }
    }
    return common * 4 - mismatch;
}

struct SeedPair {
    Coord first;
    Coord second;
    int firstDirection = 0;
    int secondDirection = 0;
    Rotation rotation;
    int score = 0;
};

std::vector<SeedPair> pairSeeds(const Grid& first, const Grid& second,
                                int candidateLimit, std::mt19937& generator)
{
    const std::vector<Seed> firstSeeds = findSeeds(first, 15);
    const std::vector<Seed> secondSeeds = findSeeds(second, 15);
    const std::vector<Rotation> rotations = cubeRotations();
    std::vector<SeedPair> pairs;
    for (const Seed& firstSeed : firstSeeds) {
        for (const Seed& secondSeed : secondSeeds) {
            for (const Rotation& rotation : rotations) {
                const int compatibility = localCompatibility(
                    first, second, firstSeed.point, secondSeed.point, rotation);
                for (int firstDirection : firstSeed.directions) {
                    for (int secondDirection : secondSeed.directions) {
                        SeedPair pair;
                        pair.first = firstSeed.point;
                        pair.second = secondSeed.point;
                        pair.firstDirection = firstDirection;
                        pair.secondDirection = secondDirection;
                        pair.rotation = rotation;
                        pair.score = compatibility + firstSeed.neighbors + secondSeed.neighbors;
                        pairs.push_back(pair);
                    }
                }
            }
        }
    }
    std::shuffle(pairs.begin(), pairs.end(), generator);
    std::stable_sort(pairs.begin(), pairs.end(), [](const SeedPair& left, const SeedPair& right) {
        return left.score > right.score;
    });
    const int keep = (std::max)(candidateLimit * 5, candidateLimit);
    if (int(pairs.size()) > keep)
        pairs.resize(keep);
    return pairs;
}

std::vector<SeedPair> pairExactSeeds(const Grid& first, const Grid& second,
                                     int candidateLimit, std::mt19937& generator)
{
    const int seedLimit = (std::max)(15, candidateLimit * 2);
    const std::vector<Seed> seeds = findSeeds(first, seedLimit);
    const Rotation identity = identityRotation();
    std::vector<SeedPair> pairs;
    for (const Seed& seed : seeds) {
        if (!second.isRemaining(seed.point))
            continue;
        for (int direction : seed.directions) {
            if (!rayClear(second, seed.point, direction))
                continue;
            SeedPair pair;
            pair.first = seed.point;
            pair.second = seed.point;
            pair.firstDirection = direction;
            pair.secondDirection = direction;
            pair.rotation = identity;
            pair.score = localCompatibility(first, second, seed.point, seed.point, identity)
                + 2 * seed.neighbors;
            pairs.push_back(pair);
        }
    }
    std::shuffle(pairs.begin(), pairs.end(), generator);
    std::stable_sort(pairs.begin(), pairs.end(), [](const SeedPair& left,
                                                    const SeedPair& right) {
        return left.score > right.score;
    });
    const int keep = (std::max)(candidateLimit * 5, candidateLimit);
    if (int(pairs.size()) > keep)
        pairs.resize(keep);
    return pairs;
}

struct PieceCandidate {
    std::unordered_set<Coord, CoordHash> offsets;
    std::unordered_set<int> deleteFirst;
    std::unordered_set<int> deleteSecond;
    SeedPair seedPair;
    double score = std::numeric_limits<double>::infinity();
    bool steady = false;
};

bool mapsToRemaining(const Grid& first, const Grid& second, const SeedPair& pair,
                     const Coord& offset)
{
    return first.isRemaining(pair.first + offset)
        && second.isRemaining(pair.second + pair.rotation.apply(offset));
}

bool addSweepClosure(const Grid& first, const Grid& second,
                     PieceCandidate* candidate, int maximumSize)
{
    bool changed = true;
    while (changed) {
        changed = false;
        std::vector<Coord> current(candidate->offsets.begin(), candidate->offsets.end());
        std::unordered_set<Coord, CoordHash> requiredFirst;
        std::unordered_set<Coord, CoordHash> requiredSecond;

        for (const Coord& offset : current) {
            Coord cursor = candidate->seedPair.first + offset;
            while (first.contains(cursor)) {
                const int index = first.index(cursor);
                if (first.labels[index] == kRemaining
                    && candidate->deleteFirst.count(index) == 0) {
                    requiredFirst.insert(cursor - candidate->seedPair.first);
                }
                cursor = cursor + kDirections[candidate->seedPair.firstDirection];
            }

            cursor = candidate->seedPair.second + candidate->seedPair.rotation.apply(offset);
            while (second.contains(cursor)) {
                const int index = second.index(cursor);
                if (second.labels[index] == kRemaining
                    && candidate->deleteSecond.count(index) == 0) {
                    requiredSecond.insert(candidate->seedPair.rotation.inverseApply(
                        cursor - candidate->seedPair.second));
                }
                cursor = cursor + kDirections[candidate->seedPair.secondDirection];
            }
        }

        std::unordered_set<Coord, CoordHash> allRequired = requiredFirst;
        allRequired.insert(requiredSecond.begin(), requiredSecond.end());
        for (const Coord& offset : allRequired) {
            const bool neededFirst = requiredFirst.count(offset) != 0;
            const bool neededSecond = requiredSecond.count(offset) != 0;
            const Coord firstPoint = candidate->seedPair.first + offset;
            const Coord secondPoint = candidate->seedPair.second
                + candidate->seedPair.rotation.apply(offset);
            const bool presentFirst = first.isRemaining(firstPoint)
                && candidate->deleteFirst.count(first.index(firstPoint)) == 0;
            const bool presentSecond = second.isRemaining(secondPoint)
                && candidate->deleteSecond.count(second.index(secondPoint)) == 0;

            if (neededFirst && neededSecond && presentFirst && presentSecond) {
                if (candidate->offsets.insert(offset).second)
                    changed = true;
            } else {
                if (neededFirst && presentFirst) {
                    const int index = first.index(firstPoint);
                    if (candidate->offsets.count(offset) != 0)
                        return false;
                    changed = candidate->deleteFirst.insert(index).second || changed;
                }
                if (neededSecond && presentSecond) {
                    const int index = second.index(secondPoint);
                    if (candidate->offsets.count(offset) != 0)
                        return false;
                    changed = candidate->deleteSecond.insert(index).second || changed;
                }
            }
        }
        if (int(candidate->offsets.size()) > maximumSize)
            return false;
    }
    return true;
}

std::vector<Coord> frontierOffsets(const Grid& first, const Grid& second,
                                   const PieceCandidate& candidate)
{
    std::unordered_set<Coord, CoordHash> frontier;
    for (const Coord& offset : candidate.offsets) {
        for (const Coord& direction : kDirections) {
            const Coord next = offset + direction;
            if (candidate.offsets.count(next) == 0
                && mapsToRemaining(first, second, candidate.seedPair, next)) {
                const Coord firstPoint = candidate.seedPair.first + next;
                const Coord secondPoint = candidate.seedPair.second
                    + candidate.seedPair.rotation.apply(next);
                if (candidate.deleteFirst.count(first.index(firstPoint)) == 0
                    && candidate.deleteSecond.count(second.index(secondPoint)) == 0)
                    frontier.insert(next);
            }
        }
    }
    return std::vector<Coord>(frontier.begin(), frontier.end());
}

int countVirtualRemaining(const Grid& grid,
                          const std::unordered_set<Coord, CoordHash>& offsets,
                          const Coord& seed, const Rotation* rotation,
                          const std::unordered_set<int>& deleted)
{
    int result = grid.remainingCount() - int(deleted.size());
    for (const Coord& offset : offsets) {
        const Coord point = seed + (rotation ? rotation->apply(offset) : offset);
        if (grid.isRemaining(point) && deleted.count(grid.index(point)) == 0)
            --result;
    }
    return result;
}

bool virtualRemainderConnected(const Grid& grid,
                               const std::unordered_set<Coord, CoordHash>& offsets,
                               const Coord& seed, const Rotation* rotation,
                               const std::unordered_set<int>& deleted)
{
    const int expected = countVirtualRemaining(grid, offsets, seed, rotation, deleted);
    if (expected <= 1)
        return true;
    auto removed = [&](int index) {
        if (deleted.count(index) != 0)
            return true;
        const Coord point = grid.coord(index);
        const Coord local = rotation ? rotation->inverseApply(point - seed) : point - seed;
        return offsets.count(local) != 0;
    };

    int start = -1;
    for (int index = 0; index < int(grid.labels.size()); ++index) {
        if (grid.labels[index] == kRemaining && !removed(index)) {
            start = index;
            break;
        }
    }
    if (start < 0)
        return expected == 0;

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
                && !removed(nextIndex)) {
                visited[nextIndex] = 1;
                queue.push(nextIndex);
            }
        }
    }
    return reached == expected;
}

std::array<bool, 6> blockingDirections(
    const Grid& grid, const std::unordered_set<Coord, CoordHash>& offsets,
    const Coord& seed, const Rotation* rotation, const std::unordered_set<int>& deleted)
{
    std::array<bool, 6> blocked{{false, false, false, false, false, false}};
    for (const Coord& offset : offsets) {
        const Coord point = seed + (rotation ? rotation->apply(offset) : offset);
        for (int direction = 0; direction < 6; ++direction) {
            const Coord neighbor = point + kDirections[direction];
            if (!grid.isRemaining(neighbor))
                continue;
            const int index = grid.index(neighbor);
            if (deleted.count(index) != 0)
                continue;
            const Coord local = rotation
                ? rotation->inverseApply(neighbor - seed) : neighbor - seed;
            if (offsets.count(local) == 0)
                blocked[direction] = true;
        }
    }
    return blocked;
}

bool candidateIsValid(const Grid& first, const Grid& second,
                      PieceCandidate* candidate, int minimumSize,
                      int maximumSize, double maxBoundaryDeletionRatio,
                      bool requireSteady)
{
    const int size = int(candidate->offsets.size());
    if (size < minimumSize || size > maximumSize)
        return false;
    if (!virtualRemainderConnected(first, candidate->offsets, candidate->seedPair.first,
                                   nullptr, candidate->deleteFirst)
        || !virtualRemainderConnected(second, candidate->offsets, candidate->seedPair.second,
                                      &candidate->seedPair.rotation,
                                      candidate->deleteSecond))
        return false;

    int boundaryDeleted = 0;
    for (int index : candidate->deleteFirst)
        boundaryDeleted += first.originalBoundary[index] ? 1 : 0;
    for (int index : candidate->deleteSecond)
        boundaryDeleted += second.originalBoundary[index] ? 1 : 0;
    const int boundaryCount = int(std::count(first.originalBoundary.begin(),
                                             first.originalBoundary.end(), 1))
        + int(std::count(second.originalBoundary.begin(), second.originalBoundary.end(), 1));
    if (boundaryCount > 0
        && double(boundaryDeleted) / double(boundaryCount) > maxBoundaryDeletionRatio)
        return false;

    const auto firstBlocked = blockingDirections(
        first, candidate->offsets, candidate->seedPair.first, nullptr,
        candidate->deleteFirst);
    const auto secondBlocked = blockingDirections(
        second, candidate->offsets, candidate->seedPair.second,
        &candidate->seedPair.rotation, candidate->deleteSecond);
    bool steady = true;
    for (int direction = 0; direction < 6; ++direction) {
        if (direction != candidate->seedPair.firstDirection)
            steady = steady && firstBlocked[direction];
        if (direction != candidate->seedPair.secondDirection)
            steady = steady && secondBlocked[direction];
    }
    candidate->steady = steady;
    if (requireSteady && !steady)
        return false;

    const double target = 0.5 * double(minimumSize + maximumSize);
    candidate->score = std::abs(double(size) - target)
        + 5.0 * double(candidate->deleteFirst.size() + candidate->deleteSecond.size())
        + (steady ? 0.0 : 1000.0);
    return true;
}

bool growPiece(const Grid& first, const Grid& second, const SeedPair& pair,
               int targetSize, const SteadyDissectionParameters& parameters,
               std::mt19937& generator, PieceCandidate* result)
{
    const int minimumSize = (std::max)(1, int(std::floor(0.6 * targetSize)));
    const int maximumSize = (std::max)(minimumSize, int(std::ceil(1.4 * targetSize)));
    PieceCandidate candidate;
    candidate.seedPair = pair;
    candidate.offsets.insert({0, 0, 0});
    if (!addSweepClosure(first, second, &candidate, maximumSize))
        return false;

    PieceCandidate best;
    bool found = false;
    while (int(candidate.offsets.size()) <= maximumSize) {
        PieceCandidate checked = candidate;
        if (candidateIsValid(first, second, &checked, minimumSize, maximumSize,
                             parameters.maxBoundaryDeletionRatio,
                             parameters.enforceGeneralizedInterlocking)) {
            if (!found || checked.score < best.score) {
                best = std::move(checked);
                found = true;
            }
            if (int(candidate.offsets.size()) >= targetSize && best.steady)
                break;
        }

        std::vector<Coord> frontier = frontierOffsets(first, second, candidate);
        if (frontier.empty())
            break;
        std::shuffle(frontier.begin(), frontier.end(), generator);
        std::stable_sort(frontier.begin(), frontier.end(), [&](const Coord& left, const Coord& right) {
            const Coord leftFirst = pair.first + left;
            const Coord leftSecond = pair.second + pair.rotation.apply(left);
            const Coord rightFirst = pair.first + right;
            const Coord rightSecond = pair.second + pair.rotation.apply(right);
            const int leftNeighbors = remainingNeighborCount(first, leftFirst)
                + remainingNeighborCount(second, leftSecond);
            const int rightNeighbors = remainingNeighborCount(first, rightFirst)
                + remainingNeighborCount(second, rightSecond);
            return leftNeighbors < rightNeighbors;
        });

        bool added = false;
        const int trials = (std::min)(int(frontier.size()), 24);
        for (int i = 0; i < trials; ++i) {
            PieceCandidate next = candidate;
            next.offsets.insert(frontier[i]);
            if (addSweepClosure(first, second, &next, maximumSize)) {
                candidate = std::move(next);
                added = true;
                break;
            }
        }
        if (!added)
            break;
    }
    if (!found)
        return false;
    *result = std::move(best);
    return true;
}

void applyCandidate(Grid* first, Grid* second, const PieceCandidate& candidate,
                    int pieceIndex)
{
    for (int index : candidate.deleteFirst)
        first->labels[index] = kEmpty;
    for (int index : candidate.deleteSecond)
        second->labels[index] = kEmpty;
    for (const Coord& offset : candidate.offsets) {
        first->labels[first->index(candidate.seedPair.first + offset)] = pieceIndex;
        second->labels[second->index(candidate.seedPair.second
            + candidate.seedPair.rotation.apply(offset))] = pieceIndex;
    }
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

std::unordered_set<int> largestConnectedSubset(const Grid& grid,
                                                const std::unordered_set<int>& subset)
{
    std::unordered_set<int> remaining = subset;
    std::unordered_set<int> best;
    while (!remaining.empty()) {
        std::unordered_set<int> component;
        std::queue<int> queue;
        const int start = *remaining.begin();
        queue.push(start);
        remaining.erase(start);
        component.insert(start);
        while (!queue.empty()) {
            const Coord point = grid.coord(queue.front());
            queue.pop();
            for (const Coord& direction : kDirections) {
                const Coord next = point + direction;
                if (!grid.contains(next))
                    continue;
                const int nextIndex = grid.index(next);
                auto iterator = remaining.find(nextIndex);
                if (iterator != remaining.end()) {
                    remaining.erase(iterator);
                    component.insert(nextIndex);
                    queue.push(nextIndex);
                }
            }
        }
        if (component.size() > best.size())
            best = std::move(component);
    }
    return best;
}

bool constructLastPiece(Grid* first, Grid* second, int pieceIndex,
                        const SteadyDissectionParameters& parameters,
                        const QVector<int>& firstDirections,
                        const QVector<int>& secondDirections,
                        bool requireExactCover,
                        int* firstDirection, int* secondDirection)
{
    std::vector<Coord> firstRemaining;
    std::vector<Coord> secondRemaining;
    Coord firstSum;
    Coord secondSum;
    for (int index = 0; index < int(first->labels.size()); ++index) {
        if (first->labels[index] == kRemaining) {
            const Coord point = first->coord(index);
            firstRemaining.push_back(point);
            firstSum = firstSum + point;
        }
        if (second->labels[index] == kRemaining) {
            const Coord point = second->coord(index);
            secondRemaining.push_back(point);
            secondSum = secondSum + point;
        }
    }
    if (firstRemaining.empty() || secondRemaining.empty())
        return false;

    struct LastPieceCandidate {
        std::unordered_set<int> firstCells;
        Rotation rotation;
        Coord translation;
    };
    const std::vector<Rotation> rotations = cubeRotations();
    std::vector<LastPieceCandidate> candidates;
    constexpr int kLastCandidateLimit = 128;
    for (const Rotation& rotation : rotations) {
        const Coord rotatedSum = rotation.apply(firstSum);
        const Coord centerTranslation{
            int(std::lround(double(secondSum.x) / secondRemaining.size()
                - double(rotatedSum.x) / firstRemaining.size())),
            int(std::lround(double(secondSum.y) / secondRemaining.size()
                - double(rotatedSum.y) / firstRemaining.size())),
            int(std::lround(double(secondSum.z) / secondRemaining.size()
                - double(rotatedSum.z) / firstRemaining.size()))
        };
        for (int dz = -2; dz <= 2; ++dz) {
            for (int dy = -2; dy <= 2; ++dy) {
                for (int dx = -2; dx <= 2; ++dx) {
                    const Coord translation = centerTranslation + Coord{dx, dy, dz};
                    std::unordered_set<int> common;
                    for (const Coord& point : firstRemaining) {
                        const Coord mapped = rotation.apply(point) + translation;
                        if (second->isRemaining(mapped))
                            common.insert(first->index(point));
                    }
                    common = largestConnectedSubset(*first, common);
                    if (!common.empty()
                        && (int(candidates.size()) < kLastCandidateLimit
                            || common.size() > candidates.back().firstCells.size())) {
                        candidates.push_back({std::move(common), rotation, translation});
                        std::stable_sort(candidates.begin(), candidates.end(),
                            [](const LastPieceCandidate& left, const LastPieceCandidate& right) {
                                return left.firstCells.size() > right.firstCells.size();
                            });
                        if (int(candidates.size()) > kLastCandidateLimit)
                            candidates.pop_back();
                    }
                }
            }
        }
    }
    if (candidates.empty())
        return false;

    for (const LastPieceCandidate& candidate : candidates) {
        if (requireExactCover
            && (candidate.firstCells.size() != firstRemaining.size()
                || candidate.firstCells.size() != secondRemaining.size()))
            continue;
        Grid trialFirst = *first;
        Grid trialSecond = *second;
        std::unordered_set<int> matchedSecond;
        for (int index : candidate.firstCells) {
            const Coord mapped = candidate.rotation.apply(first->coord(index))
                + candidate.translation;
            matchedSecond.insert(second->index(mapped));
        }
        for (int index = 0; index < int(trialFirst.labels.size()); ++index) {
            if (trialFirst.labels[index] == kRemaining)
                trialFirst.labels[index] = candidate.firstCells.count(index)
                    ? pieceIndex : kEmpty;
            if (trialSecond.labels[index] == kRemaining)
                trialSecond.labels[index] = matchedSecond.count(index)
                    ? pieceIndex : kEmpty;
        }

        const int totalBoundary = int(std::count(trialFirst.originalBoundary.begin(),
                                                 trialFirst.originalBoundary.end(), 1))
            + int(std::count(trialSecond.originalBoundary.begin(),
                             trialSecond.originalBoundary.end(), 1));
        const int deletedBoundary = trialFirst.deletedBoundaryCount()
            + trialSecond.deletedBoundaryCount();
        if (requireExactCover
            && (trialFirst.deletedCount() != 0 || trialSecond.deletedCount() != 0))
            continue;
        if (totalBoundary > 0 && double(deletedBoundary) / totalBoundary
            > parameters.maxBoundaryDeletionRatio)
            continue;

        QVector<int> finalFirstDirections = firstDirections;
        QVector<int> finalSecondDirections = secondDirections;
        finalFirstDirections.push_back(-1);
        finalSecondDirections.push_back(-1);
        if (parameters.enforceGeneralizedInterlocking
            && (!localModelSatisfied(trialFirst, finalFirstDirections)
                || !localModelSatisfied(trialSecond, finalSecondDirections)))
            continue;

        *first = std::move(trialFirst);
        *second = std::move(trialSecond);
        *firstDirection = -1;
        *secondDirection = -1;
        return true;
    }
    return false;
}

void appendQuad(SteadyDissectionMeshData* mesh,
                const std::array<QVector3D, 4>& corners, int piece)
{
    const unsigned int base = unsigned(mesh->vertices.size());
    for (const QVector3D& corner : corners)
        mesh->vertices.push_back(corner);
    mesh->faces << base << base + 1U << base + 2U
                << base << base + 2U << base + 3U;
    mesh->facePieces << piece << piece;
}

SteadyDissectionMeshData buildMesh(const Grid& grid)
{
    SteadyDissectionMeshData mesh;
    const float half = 0.5f * float(grid.resolution);
    const float scale = 2.0f / float(grid.resolution);
    auto point = [&](float x, float y, float z) {
        return QVector3D((x - half) * scale, (y - half) * scale, (z - half) * scale);
    };

    for (int index = 0; index < int(grid.labels.size()); ++index) {
        const int label = grid.labels[index];
        if (label == kEmpty)
            continue;
        const Coord cell = grid.coord(index);
        const float x = float(cell.x);
        const float y = float(cell.y);
        const float z = float(cell.z);
        for (int direction = 0; direction < 6; ++direction) {
            const int neighborLabel = grid.label(cell + kDirections[direction]);
            if (neighborLabel == label)
                continue;
            switch (direction) {
            case 0:
                appendQuad(&mesh, {{point(x + 1, y, z), point(x + 1, y + 1, z),
                                    point(x + 1, y + 1, z + 1), point(x + 1, y, z + 1)}}, label);
                break;
            case 1:
                appendQuad(&mesh, {{point(x, y, z + 1), point(x, y + 1, z + 1),
                                    point(x, y + 1, z), point(x, y, z)}}, label);
                break;
            case 2:
                appendQuad(&mesh, {{point(x, y + 1, z + 1), point(x + 1, y + 1, z + 1),
                                    point(x + 1, y + 1, z), point(x, y + 1, z)}}, label);
                break;
            case 3:
                appendQuad(&mesh, {{point(x, y, z), point(x + 1, y, z),
                                    point(x + 1, y, z + 1), point(x, y, z + 1)}}, label);
                break;
            case 4:
                appendQuad(&mesh, {{point(x + 1, y, z + 1), point(x + 1, y + 1, z + 1),
                                    point(x, y + 1, z + 1), point(x, y, z + 1)}}, label);
                break;
            default:
                appendQuad(&mesh, {{point(x, y, z), point(x, y + 1, z),
                                    point(x + 1, y + 1, z), point(x + 1, y, z)}}, label);
                break;
            }
        }
    }
    return mesh;
}

QVector<int> pieceCounts(const Grid& grid, int completedPieces)
{
    QVector<int> counts(completedPieces, 0);
    for (int label : grid.labels)
        if (label >= 0 && label < counts.size())
            ++counts[label];
    return counts;
}

SteadyDissectionSnapshot makeSnapshot(
    const Grid& first, const Grid& second, const SteadyDissectionParameters& parameters,
    const QVector<int>& firstDirections, const QVector<int>& secondDirections,
    const QString& phase, bool complete, bool generalizedInterlocking)
{
    SteadyDissectionSnapshot snapshot;
    snapshot.firstForm = buildMesh(first);
    snapshot.secondForm = buildMesh(second);
    snapshot.phase = phase;
    snapshot.resolution = parameters.resolution;
    snapshot.requestedPieces = parameters.pieceCount;
    snapshot.completedPieces = firstDirections.size();
    snapshot.firstInputVoxels = first.inputVoxelCount;
    snapshot.secondInputVoxels = second.inputVoxelCount;
    snapshot.firstDeletedVoxels = first.deletedCount();
    snapshot.secondDeletedVoxels = second.deletedCount();
    snapshot.firstExtractionDirections = firstDirections;
    snapshot.secondExtractionDirections = secondDirections;
    snapshot.firstPieceVoxelCounts = pieceCounts(first, snapshot.completedPieces);
    snapshot.secondPieceVoxelCounts = pieceCounts(second, snapshot.completedPieces);
    snapshot.congruentPieces = snapshot.firstPieceVoxelCounts == snapshot.secondPieceVoxelCounts;
    snapshot.generalizedInterlocking = generalizedInterlocking;
    snapshot.complete = complete;
    return snapshot;
}

class Engine {
public:
    Engine(const SteadyDissectionParameters& parameters,
           const Steady3DDissectionAlgorithm::SnapshotCallback& callback)
        : parameters_(parameters), callback_(callback), generator_(parameters.randomSeed)
    {
    }

    bool execute(const QString& firstPath, const QString& secondPath, QString* error)
    {
        if (parameters_.resolution < 8 || parameters_.resolution > 35) {
            *error = "Voxel resolution must be between 8 and 35.";
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

        LoadedMesh firstMesh;
        LoadedMesh secondMesh;
        if (!loadAndValidateMesh(firstPath, &firstMesh, error)
            || !loadAndValidateMesh(secondPath, &secondMesh, error))
            return false;

        normalizeMesh(&firstMesh, 1.0);
        normalizeMesh(&secondMesh, std::cbrt(firstMesh.volume / secondMesh.volume));
        const double extent = (std::max)(maximumExtent(firstMesh.mesh),
                                         maximumExtent(secondMesh.mesh));
        const double cellSize = extent / double(parameters_.resolution - 2);
        if (!voxelize(firstMesh.mesh, parameters_.resolution, cellSize, &first_, error)
            || !voxelize(secondMesh.mesh, parameters_.resolution, cellSize, &second_, error))
            return false;

        exactCover_ = hasSameRemainingCells(first_, second_);

        if ((std::min)(first_.inputVoxelCount, second_.inputVoxelCount)
            < parameters_.pieceCount * 2) {
            *error = "The voxelized models contain too few cells for the requested piece count.";
            return false;
        }
        if (!publish("Voxelized equal-volume input meshes", false, false))
            return true;

        const Grid initialFirst = first_;
        const Grid initialSecond = second_;
        QVector<PieceCandidate> solution;
        Grid solvedFirst;
        Grid solvedSecond;
        QVector<int> solvedFirstDirections;
        QVector<int> solvedSecondDirections;
        nodeBudget_ = (std::max)(300, parameters_.candidateLimit
            * parameters_.pieceCount * 12);
        nodesExplored_ = 0;
        if (!searchSolution(0, initialFirst, initialSecond, {}, {}, &solution,
                            &solvedFirst, &solvedSecond,
                            &solvedFirstDirections, &solvedSecondDirections)) {
            *error = QString("Construction-tree search exhausted %1 states without a complete "
                             "solution. Try more candidates, more pieces, a lower resolution, "
                             "or disable the strict interlocking check.")
                         .arg(nodesExplored_);
            return false;
        }

        first_ = initialFirst;
        second_ = initialSecond;
        firstDirections_.clear();
        secondDirections_.clear();
        for (int piece = 0; piece < solution.size(); ++piece) {
            applyCandidate(&first_, &second_, solution[piece], piece);
            firstDirections_.push_back(solution[piece].seedPair.firstDirection);
            secondDirections_.push_back(solution[piece].seedPair.secondDirection);
            const bool locallyInterlocking = localModelSatisfied(first_, firstDirections_)
                && localModelSatisfied(second_, secondDirections_);
            if (!publish(QString("Co-expanded and stabilized piece %1 of %2")
                             .arg(piece + 1).arg(parameters_.pieceCount),
                         false, locallyInterlocking))
                return true;
        }

        first_ = std::move(solvedFirst);
        second_ = std::move(solvedSecond);
        firstDirections_ = std::move(solvedFirstDirections);
        secondDirections_ = std::move(solvedSecondDirections);

        const bool finalInterlocking = localModelSatisfied(first_, firstDirections_)
            && localModelSatisfied(second_, secondDirections_);
        if (parameters_.enforceGeneralizedInterlocking && !finalInterlocking) {
            *error = "The final compatibility cleanup removed a required blocking voxel. "
                     "Try more candidates, more pieces, or a higher voxel resolution.";
            return false;
        }

        const bool congruent = pieceCounts(first_, parameters_.pieceCount)
            == pieceCounts(second_, parameters_.pieceCount);
        if (!congruent) {
            *error = "Internal error: paired pieces lost voxel-count correspondence.";
            return false;
        }
        if (exactCover_ && (first_.deletedCount() != 0 || second_.deletedCount() != 0)) {
            *error = "Internal error: exact-cover construction deleted input voxels.";
            return false;
        }
        return publish(exactCover_ ? "Complete exact common dissection"
                                   : "Complete common dissection", true,
                       finalInterlocking);
    }

private:
    bool searchSolution(int piece, const Grid& first, const Grid& second,
                        const QVector<int>& firstDirections,
                        const QVector<int>& secondDirections,
                        QVector<PieceCandidate>* sequence,
                        Grid* solvedFirst, Grid* solvedSecond,
                        QVector<int>* solvedFirstDirections,
                        QVector<int>* solvedSecondDirections)
    {
        if (nodesExplored_ >= nodeBudget_)
            return false;
        if (piece == parameters_.pieceCount - 1) {
            Grid finalFirst = first;
            Grid finalSecond = second;
            int firstDirection = -1;
            int secondDirection = -1;
            ++nodesExplored_;
            if (!constructLastPiece(&finalFirst, &finalSecond, piece, parameters_,
                                    firstDirections, secondDirections,
                                    exactCover_,
                                    &firstDirection, &secondDirection))
                return false;
            *solvedFirst = std::move(finalFirst);
            *solvedSecond = std::move(finalSecond);
            *solvedFirstDirections = firstDirections;
            *solvedSecondDirections = secondDirections;
            solvedFirstDirections->push_back(firstDirection);
            solvedSecondDirections->push_back(secondDirection);
            return true;
        }

        const int piecesLeft = parameters_.pieceCount - piece;
        const int target = (std::max)(1,
            (std::min)(first.remainingCount(), second.remainingCount()) / piecesLeft);
        const std::vector<SeedPair> pairs = exactCover_
            ? pairExactSeeds(first, second, parameters_.candidateLimit, generator_)
            : pairSeeds(first, second, parameters_.candidateLimit, generator_);
        std::vector<PieceCandidate> candidates;
        for (const SeedPair& pair : pairs) {
            PieceCandidate candidate;
            if (!growPiece(first, second, pair, target, parameters_, generator_, &candidate))
                continue;
            if (exactCover_
                && (!candidate.deleteFirst.empty() || !candidate.deleteSecond.empty()))
                continue;
            Grid trialFirst = first;
            Grid trialSecond = second;
            applyCandidate(&trialFirst, &trialSecond, candidate, piece);
            QVector<int> trialFirstDirections = firstDirections;
            QVector<int> trialSecondDirections = secondDirections;
            trialFirstDirections.push_back(candidate.seedPair.firstDirection);
            trialSecondDirections.push_back(candidate.seedPair.secondDirection);
            const bool preservesBlocking = localModelSatisfied(
                trialFirst, trialFirstDirections)
                && localModelSatisfied(trialSecond, trialSecondDirections);
            if (!parameters_.enforceGeneralizedInterlocking || preservesBlocking)
                candidates.push_back(std::move(candidate));
        }
        std::stable_sort(candidates.begin(), candidates.end(),
            [](const PieceCandidate& left, const PieceCandidate& right) {
                return left.score < right.score;
            });
        if (int(candidates.size()) > parameters_.candidateLimit)
            candidates.resize(parameters_.candidateLimit);

        for (const PieceCandidate& candidate : candidates) {
            if (++nodesExplored_ > nodeBudget_)
                return false;
            Grid trialFirst = first;
            Grid trialSecond = second;
            applyCandidate(&trialFirst, &trialSecond, candidate, piece);
            QVector<int> trialFirstDirections = firstDirections;
            QVector<int> trialSecondDirections = secondDirections;
            trialFirstDirections.push_back(candidate.seedPair.firstDirection);
            trialSecondDirections.push_back(candidate.seedPair.secondDirection);
            sequence->push_back(candidate);
            if (searchSolution(piece + 1, trialFirst, trialSecond,
                               trialFirstDirections, trialSecondDirections,
                               sequence, solvedFirst, solvedSecond,
                               solvedFirstDirections, solvedSecondDirections))
                return true;
            sequence->pop_back();
        }
        return false;
    }

    bool publish(const QString& phase, bool complete, bool generalizedInterlocking)
    {
        if (!callback_)
            return true;
        return callback_(makeSnapshot(first_, second_, parameters_, firstDirections_,
                                      secondDirections_, phase, complete,
                                      generalizedInterlocking));
    }

    SteadyDissectionParameters parameters_;
    Steady3DDissectionAlgorithm::SnapshotCallback callback_;
    std::mt19937 generator_;
    Grid first_;
    Grid second_;
    QVector<int> firstDirections_;
    QVector<int> secondDirections_;
    int nodesExplored_ = 0;
    int nodeBudget_ = 0;
    bool exactCover_ = false;
};
} // namespace

bool Steady3DDissectionAlgorithm::run(
    const QString& firstPath, const QString& secondPath,
    const SteadyDissectionParameters& parameters,
    const SnapshotCallback& callback, QString* errorMessage)
{
    QString localError;
    Engine engine(parameters, callback);
    const bool result = engine.execute(firstPath, secondPath, &localError);
    if (errorMessage)
        *errorMessage = localError;
    return result;
}
