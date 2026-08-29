#pragma once

#include <QVector>
#include <QVector3D>
#include <QString>

#include <functional>

struct DChartsParameters {
    int initialChartCount = 8;
    int maxIterations = 100;
    double fittingThreshold = 0.2;
    double alpha = 1.0;
    double beta = 0.7;
    double gamma = 0.5;
    double convergenceRatio = 0.05;
    double largeHoleAreaRatio = 0.01;
    int boundaryRelaxationPasses = 5;
    double mergeThreshold = 0.01;
    bool addPartialCuts = true;
};

struct DChartsMeshData {
    QVector<QVector3D> vertices;
    QVector<unsigned int> faces;
};

struct DChartsSnapshot {
    DChartsMeshData mesh;
    QVector<int> faceCharts;
    QVector<int> seedFaces;
    QVector<unsigned int> seamEdges;
    QString phase;
    int iteration = 0;
    int chartCount = 0;
    int assignedFaces = 0;
    int unassignedFaces = 0;
    double reassignedRatio = 0.0;
    double meanFittingError = 0.0;
    double maxFittingError = 0.0;
    bool complete = false;
};

Q_DECLARE_METATYPE(DChartsSnapshot)

class DChartsAlgorithm {
public:
    using SnapshotCallback = std::function<bool(const DChartsSnapshot&)>;

    static bool run(const QString& path,
                    const DChartsParameters& parameters,
                    const SnapshotCallback& callback,
                    QString* errorMessage);
};
