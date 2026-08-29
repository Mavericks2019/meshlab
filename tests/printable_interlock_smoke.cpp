#include "interlocking/printableinterlockalgorithm.h"

#include <QCoreApplication>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <numeric>

namespace {
bool isClosedIndexedMesh(const SteadyDissectionMeshData& mesh)
{
    if (mesh.vertices.isEmpty() || mesh.faces.isEmpty() || mesh.faces.size() % 3 != 0)
        return false;
    std::map<std::pair<unsigned int, unsigned int>, int> edgeCounts;
    double signedVolume = 0.0;
    for (int index = 0; index < mesh.faces.size(); index += 3) {
        const unsigned int a = mesh.faces[index];
        const unsigned int b = mesh.faces[index + 1];
        const unsigned int c = mesh.faces[index + 2];
        if (a >= unsigned(mesh.vertices.size()) || b >= unsigned(mesh.vertices.size())
            || c >= unsigned(mesh.vertices.size()) || a == b || b == c || c == a)
            return false;
        for (const auto& edge : {std::pair<unsigned int, unsigned int>{a, b},
                                 {b, c}, {c, a}}) {
            const auto ordered = std::minmax(edge.first, edge.second);
            ++edgeCounts[{ordered.first, ordered.second}];
        }
        signedVolume += QVector3D::dotProduct(
            mesh.vertices[a], QVector3D::crossProduct(mesh.vertices[b], mesh.vertices[c])) / 6.0;
    }
    return std::abs(signedVolume) > 1e-9
        && std::all_of(edgeCounts.begin(), edgeCounts.end(),
                       [](const auto& edge) { return edge.second == 2; });
}
}

int main(int argc, char** argv)
{
    QCoreApplication application(argc, argv);
    if (argc < 2) {
        std::cerr << "usage: printable_interlock_smoke <mesh> [resolution] [pieces] "
                     "[candidates] [minimum-face-contact-percent] [seed]\n";
        return 2;
    }

    PrintableInterlockParameters parameters;
    parameters.resolution = 14;
    parameters.samplesPerVoxel = 4;
    parameters.pieceCount = 6;
    parameters.candidateLimit = 20;
    if (argc > 2)
        parameters.resolution = QString::fromLocal8Bit(argv[2]).toInt();
    if (argc > 3)
        parameters.pieceCount = QString::fromLocal8Bit(argv[3]).toInt();
    if (argc > 4)
        parameters.candidateLimit = QString::fromLocal8Bit(argv[4]).toInt();
    if (argc > 5)
        parameters.minimumFaceContact = QString::fromLocal8Bit(argv[5]).toDouble() / 100.0;
    if (argc > 6)
        parameters.randomSeed = QString::fromLocal8Bit(argv[6]).toUInt();

    PrintableInterlockSnapshot finalSnapshot;
    int snapshotCount = 0;
    QString error;
    const bool succeeded = PrintableInterlockAlgorithm::run(
        QString::fromLocal8Bit(argv[1]), parameters,
        [&](const PrintableInterlockSnapshot& snapshot) {
            ++snapshotCount;
            finalSnapshot = snapshot;
            std::cout << snapshot.phase.toStdString()
                      << " pieces=" << snapshot.completedPieces
                      << " occupied=" << snapshot.occupiedVoxels
                      << " internal=" << snapshot.internalVoxels
                      << " boundary=" << snapshot.boundaryVoxels
                      << " attached=" << snapshot.attachedBoundaryVoxels << '\n';
            return true;
        },
        &error);
    if (!succeeded) {
        std::cerr << error.toStdString() << '\n';
        return 3;
    }

    const int assigned = std::accumulate(finalSnapshot.pieceVoxelCounts.begin(),
                                         finalSnapshot.pieceVoxelCounts.end(), 0);
    if (!finalSnapshot.complete || !finalSnapshot.interlocking
        || finalSnapshot.completedPieces != parameters.pieceCount
        || assigned != finalSnapshot.occupiedVoxels
        || finalSnapshot.attachedBoundaryVoxels != finalSnapshot.boundaryVoxels
        || finalSnapshot.originalModel.faces.isEmpty()
        || finalSnapshot.voxelizedModel.faces.isEmpty()
        || finalSnapshot.partitionedModel.faces.isEmpty()
        || finalSnapshot.voxelWatertightParts != parameters.pieceCount
        || finalSnapshot.watertightParts != parameters.pieceCount
        || finalSnapshot.voxelParts.size() != parameters.pieceCount
        || finalSnapshot.printableParts.size() != parameters.pieceCount
        || snapshotCount < 4) {
        std::cerr << "The single-model partition is incomplete or not interlocking.\n";
        return 4;
    }
    for (int count : finalSnapshot.pieceVoxelCounts) {
        if (count <= 0) {
            std::cerr << "The result contains an empty part.\n";
            return 5;
        }
    }
    for (int piece = 0; piece < parameters.pieceCount; ++piece) {
        if (!isClosedIndexedMesh(finalSnapshot.voxelParts[piece])
            || !isClosedIndexedMesh(finalSnapshot.printableParts[piece])) {
            std::cerr << "A generated voxel or surface-cut part has an open indexed boundary.\n";
            return 6;
        }
    }
    return 0;
}
