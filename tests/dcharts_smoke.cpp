#include "dcharts/dchartsalgorithm.h"

#include <QCoreApplication>

#include <iostream>

int main(int argc, char** argv)
{
    QCoreApplication application(argc, argv);
    if (argc < 2) {
        std::cerr << "usage: dcharts_smoke <mesh>\n";
        return 2;
    }

    DChartsParameters parameters;
    parameters.initialChartCount = 8;
    parameters.maxIterations = 30;
    DChartsSnapshot finalSnapshot;
    int snapshotCount = 0;
    QString error;
    const bool succeeded = DChartsAlgorithm::run(
        QString::fromLocal8Bit(argv[1]), parameters,
        [&](const DChartsSnapshot& snapshot) {
            ++snapshotCount;
            finalSnapshot = snapshot;
            std::cout << snapshot.phase.toStdString()
                      << " iter=" << snapshot.iteration
                      << " charts=" << snapshot.chartCount
                      << " assigned=" << snapshot.assignedFaces
                      << " unassigned=" << snapshot.unassignedFaces
                      << " meanF=" << snapshot.meanFittingError << '\n';
            return true;
        },
        &error);

    if (!succeeded) {
        std::cerr << error.toStdString() << '\n';
        return 3;
    }
    if (!finalSnapshot.complete || finalSnapshot.unassignedFaces != 0
        || finalSnapshot.chartCount < 2 || snapshotCount < 5) {
        std::cerr << "D-Charts did not produce a complete multi-chart segmentation.\n";
        return 4;
    }
    return 0;
}
