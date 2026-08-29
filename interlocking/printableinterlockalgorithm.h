#pragma once

#include "dissection/steady3ddissectionalgorithm.h"

#include <QMetaType>
#include <QString>
#include <QVector>

#include <functional>

struct PrintableInterlockParameters {
    int resolution = 16;
    int samplesPerVoxel = 4;
    int pieceCount = 8;
    int candidateLimit = 20;
    unsigned int randomSeed = 7;
    double internalVolumeThreshold = 0.5;
    double minimumFaceContact = 0.05;
    double maxPartExtentRatio = 1.0;
    bool refineSalientSeams = true;
    bool enforceInterlocking = true;
};

struct PrintableInterlockSnapshot {
    SteadyDissectionMeshData originalModel;
    SteadyDissectionMeshData partitionedModel;
    QVector<int> pieceVoxelCounts;
    QVector<int> extractionDirections;
    QString phase;
    int resolution = 0;
    int requestedPieces = 0;
    int completedPieces = 0;
    int occupiedVoxels = 0;
    int internalVoxels = 0;
    int boundaryVoxels = 0;
    int attachedBoundaryVoxels = 0;
    int tinyVoxels = 0;
    int disconnectedVoxels = 0;
    bool interlocking = false;
    bool complete = false;
};

Q_DECLARE_METATYPE(PrintableInterlockSnapshot)

class PrintableInterlockAlgorithm {
public:
    using SnapshotCallback = std::function<bool(const PrintableInterlockSnapshot&)>;

    static bool run(const QString& meshPath,
                    const PrintableInterlockParameters& parameters,
                    const SnapshotCallback& callback,
                    QString* errorMessage);
};
