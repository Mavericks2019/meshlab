#include "dissection/steady3ddissectionalgorithm.h"

#include <QCoreApplication>

#include <iostream>

int main(int argc, char** argv)
{
    QCoreApplication application(argc, argv);
    if (argc < 3) {
        std::cerr << "usage: steady_dissection_smoke <first-mesh> <second-mesh>\n";
        return 2;
    }

    SteadyDissectionParameters parameters;
    parameters.resolution = 10;
    parameters.pieceCount = 4;
    parameters.candidateLimit = 20;
    parameters.maxBoundaryDeletionRatio = 0.8;
    parameters.enforceGeneralizedInterlocking = true;
    if (argc > 3)
        parameters.resolution = QString::fromLocal8Bit(argv[3]).toInt();
    if (argc > 4)
        parameters.pieceCount = QString::fromLocal8Bit(argv[4]).toInt();
    if (argc > 5)
        parameters.candidateLimit = QString::fromLocal8Bit(argv[5]).toInt();
    const bool expectZeroDeletion = argc > 6
        && QString::fromLocal8Bit(argv[6]) == "--expect-zero-deletion";

    SteadyDissectionSnapshot finalSnapshot;
    int snapshotCount = 0;
    QString error;
    const bool succeeded = Steady3DDissectionAlgorithm::run(
        QString::fromLocal8Bit(argv[1]), QString::fromLocal8Bit(argv[2]), parameters,
        [&](const SteadyDissectionSnapshot& snapshot) {
            ++snapshotCount;
            finalSnapshot = snapshot;
            std::cout << snapshot.phase.toStdString()
                      << " pieces=" << snapshot.completedPieces
                      << " voxels=" << snapshot.firstInputVoxels
                      << '/' << snapshot.secondInputVoxels
                      << " deleted=" << snapshot.firstDeletedVoxels
                      << '/' << snapshot.secondDeletedVoxels << '\n';
            return true;
        },
        &error);

    if (!succeeded) {
        std::cerr << error.toStdString() << '\n';
        return 3;
    }
    if (!finalSnapshot.complete || !finalSnapshot.congruentPieces
        || !finalSnapshot.generalizedInterlocking
        || finalSnapshot.completedPieces != parameters.pieceCount
        || finalSnapshot.firstPieceVoxelCounts != finalSnapshot.secondPieceVoxelCounts
        || finalSnapshot.firstForm.faces.isEmpty() || finalSnapshot.secondForm.faces.isEmpty()
        || snapshotCount < parameters.pieceCount + 1) {
        std::cerr << "The co-dissection did not produce complete congruent colored forms.\n";
        return 4;
    }
    for (int count : finalSnapshot.firstPieceVoxelCounts) {
        if (count <= 0) {
            std::cerr << "The result contains an empty piece.\n";
            return 5;
        }
    }
    if (expectZeroDeletion
        && (finalSnapshot.firstDeletedVoxels != 0
            || finalSnapshot.secondDeletedVoxels != 0)) {
        std::cerr << "Expected an exact cover, but the result deleted input voxels.\n";
        return 6;
    }
    return 0;
}
