#pragma once

#include "dissection/steady3ddissectionalgorithm.h"

#include <QMetaType>
#include <QString>
#include <QVector>

#include <functional>

enum class PrintableSurfaceCutMethod {
    PerPartBoolean,
    GlobalLabeledCut
};

struct PrintableInterlockParameters {
    int resolution = 15;
    int samplesPerVoxel = 4;
    int pieceCount = 8;
    int candidateLimit = 20;
    unsigned int randomSeed = 7;
    double internalVolumeThreshold = 0.5;
    double minimumFaceContact = 0.05;
    double maxPartExtentRatio = 1.0;
    PrintableSurfaceCutMethod surfaceCutMethod =
        PrintableSurfaceCutMethod::PerPartBoolean;
    bool refineSalientSeams = true;
    bool enforceInterlocking = true;
};

struct PrintableInterlockSnapshot {
    SteadyDissectionMeshData originalModel;
    SteadyDissectionMeshData voxelizedModel;
    SteadyDissectionMeshData partitionedModel;
    QVector<SteadyDissectionMeshData> voxelParts;
    QVector<SteadyDissectionMeshData> printableParts;
    QVector<int> pieceVoxelCounts;
    QVector<int> extractionDirections;
    QVector3D sourceCenter;
    QString phase;
    double sourceUnitsPerNormalizedUnit = 1.0;
    int resolution = 0;
    int requestedPieces = 0;
    int completedPieces = 0;
    int occupiedVoxels = 0;
    int internalVoxels = 0;
    int boundaryVoxels = 0;
    int attachedBoundaryVoxels = 0;
    int tinyVoxels = 0;
    int disconnectedVoxels = 0;
    int voxelWatertightParts = 0;
    int watertightParts = 0;
    bool interlocking = false;
    bool voxelComplete = false;
    bool complete = false;
};

Q_DECLARE_METATYPE(PrintableInterlockSnapshot)

class PrintableInterlockAlgorithm {
public:
    using SnapshotCallback = std::function<bool(const PrintableInterlockSnapshot&)>;

    static bool loadPreview(const QString& meshPath,
                            SteadyDissectionMeshData* preview,
                            QString* errorMessage);
    static bool run(const QString& meshPath,
                    const PrintableInterlockParameters& parameters,
                    const SnapshotCallback& callback,
                    QString* errorMessage);
};
