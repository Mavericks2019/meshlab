#include "interlocking/printableinterlockalgorithm.h"

#include <QCoreApplication>

#include <iostream>
#include <numeric>

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
        || finalSnapshot.partitionedModel.faces.isEmpty()
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
    return 0;
}
