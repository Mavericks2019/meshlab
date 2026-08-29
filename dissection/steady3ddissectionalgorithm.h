#pragma once

#include <QVector>
#include <QVector3D>
#include <QString>
#include <QMetaType>

#include <functional>

struct SteadyDissectionParameters {
    int resolution = 16;
    int pieceCount = 10;
    int candidateLimit = 10;
    unsigned int randomSeed = 7;
    double maxBoundaryDeletionRatio = 0.35;
    bool enforceGeneralizedInterlocking = true;
};

struct SteadyDissectionMeshData {
    QVector<QVector3D> vertices;
    QVector<unsigned int> faces;
    QVector<int> facePieces;
};

struct SteadyDissectionSnapshot {
    SteadyDissectionMeshData firstForm;
    SteadyDissectionMeshData secondForm;
    QVector<int> firstPieceVoxelCounts;
    QVector<int> secondPieceVoxelCounts;
    QVector<int> firstExtractionDirections;
    QVector<int> secondExtractionDirections;
    QString phase;
    int resolution = 0;
    int requestedPieces = 0;
    int completedPieces = 0;
    int firstInputVoxels = 0;
    int secondInputVoxels = 0;
    int firstDeletedVoxels = 0;
    int secondDeletedVoxels = 0;
    bool congruentPieces = false;
    bool generalizedInterlocking = false;
    bool complete = false;
};

Q_DECLARE_METATYPE(SteadyDissectionSnapshot)

class Steady3DDissectionAlgorithm {
public:
    using SnapshotCallback = std::function<bool(const SteadyDissectionSnapshot&)>;

    static bool run(const QString& firstPath,
                    const QString& secondPath,
                    const SteadyDissectionParameters& parameters,
                    const SnapshotCallback& callback,
                    QString* errorMessage);
};
