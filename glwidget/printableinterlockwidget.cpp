#include "printableinterlockwidget.h"

#include <QDir>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLocale>
#include <QSaveFile>
#include <QTextStream>
#include <QVBoxLayout>

#include <algorithm>
#include <utility>

PrintableInterlockWorker::PrintableInterlockWorker(
    const QString& meshPath, const PrintableInterlockParameters& parameters,
    QObject* parent)
    : QThread(parent), meshPath_(meshPath), parameters_(parameters)
{
}

PrintableInterlockWorker::~PrintableInterlockWorker()
{
    requestStop();
    wait();
}

void PrintableInterlockWorker::setContinuous(bool continuous)
{
    QMutexLocker lock(&mutex_);
    continuous_ = continuous;
    if (continuous)
        waitCondition_.wakeAll();
}

void PrintableInterlockWorker::requestStep()
{
    QMutexLocker lock(&mutex_);
    continuous_ = false;
    ++stepBudget_;
    waitCondition_.wakeAll();
}

void PrintableInterlockWorker::requestStop()
{
    stopRequested_.store(true);
    QMutexLocker lock(&mutex_);
    waitCondition_.wakeAll();
}

bool PrintableInterlockWorker::waitAfterSnapshot()
{
    QMutexLocker lock(&mutex_);
    while (!stopRequested_.load() && !continuous_ && stepBudget_ == 0)
        waitCondition_.wait(&mutex_);
    if (stepBudget_ > 0)
        --stepBudget_;
    const bool keepRunning = !stopRequested_.load();
    const bool continuous = continuous_;
    lock.unlock();
    if (continuous && keepRunning)
        msleep(140);
    return keepRunning;
}

void PrintableInterlockWorker::run()
{
    QString error;
    const bool succeeded = PrintableInterlockAlgorithm::run(
        meshPath_, parameters_,
        [this](const PrintableInterlockSnapshot& snapshot) {
            if (stopRequested_.load())
                return false;
            emit snapshotReady(snapshot);
            return snapshot.complete ? !stopRequested_.load() : waitAfterSnapshot();
        },
        &error);
    if (!succeeded && !stopRequested_.load())
        emit failed(error.isEmpty() ? "Printable interlocking construction failed." : error);
}

PrintableInterlockWidget::PrintableInterlockWidget(QWidget* parent)
    : QWidget(parent)
{
    qRegisterMetaType<PrintableInterlockSnapshot>("PrintableInterlockSnapshot");
    partitionViewport_ = new SteadyDissectionViewport(this);
    partitionViewport_->setShowWireframeOverlay(false);
    partitionViewport_->setFeatureWireframeOnly(surfaceClippedMode_);

    phaseLabel_ = new QLabel("Select one watertight triangle mesh", this);
    phaseLabel_->setStyleSheet("font-size: 15px; font-weight: 600; color: #f4f6f8;");
    metricsLabel_ = new QLabel(this);
    metricsLabel_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    metricsLabel_->setStyleSheet("color: #aeb7c2;");

    QFrame* header = new QFrame(this);
    header->setStyleSheet("QFrame { background: #2b3037; border-bottom: 1px solid #404751; }");
    QHBoxLayout* headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(14, 9, 14, 9);
    headerLayout->addWidget(phaseLabel_, 1);
    headerLayout->addWidget(metricsLabel_);

    QWidget* viewportPanel = new QWidget(this);
    QVBoxLayout* viewportLayout = new QVBoxLayout(viewportPanel);
    viewportLayout->setContentsMargins(0, 0, 0, 0);
    viewportLayout->setSpacing(0);
    partitionNameLabel_ = new QLabel("Model preview", viewportPanel);
    partitionNameLabel_->setAlignment(Qt::AlignCenter);
    partitionNameLabel_->setMinimumHeight(34);
    partitionNameLabel_->setStyleSheet(
        "background: #272c32; color: #d8dde3; padding: 6px; border-bottom: 1px solid #3c434c;");
    viewportLayout->addWidget(partitionNameLabel_);
    viewportLayout->addWidget(partitionViewport_, 1);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(header);
    layout->addWidget(viewportPanel, 1);
}

PrintableInterlockWidget::~PrintableInterlockWidget()
{
    reset();
}

void PrintableInterlockWidget::stopWorker()
{
    if (!worker_)
        return;
    worker_->requestStop();
    worker_->wait();
    worker_->deleteLater();
    worker_ = nullptr;
    emit runningChanged(false);
}

bool PrintableInterlockWidget::loadPreview(
    const QString& meshPath, QString* errorMessage)
{
    stopWorker();
    SteadyDissectionMeshData preview;
    if (!PrintableInterlockAlgorithm::loadPreview(meshPath, &preview, errorMessage))
        return false;

    meshPath_ = meshPath;
    previewModel_ = std::move(preview);
    lastSnapshot_ = PrintableInterlockSnapshot();
    assemblyProgress_ = 0.0f;
    partitionViewport_->setPieceOffsets({});
    partitionViewport_->setModelData(previewModel_, 0);
    partitionNameLabel_->setText("Input model  |  " + QFileInfo(meshPath_).fileName());
    phaseLabel_->setText("Input model loaded");
    metricsLabel_->setText(QString("%1 triangles")
        .arg(previewModel_.faces.size() / 3));
    emit statusChanged("Input model loaded. Run all stages or advance one stage at a time.");
    emit snapshotChanged(lastSnapshot_);
    emit runningChanged(false);
    return true;
}

void PrintableInterlockWidget::loadAndStart(
    const QString& meshPath, const PrintableInterlockParameters& parameters,
    bool continuous)
{
    stopWorker();
    meshPath_ = meshPath;
    lastParameters_ = parameters;
    lastSnapshot_ = PrintableInterlockSnapshot();
    assemblyProgress_ = 0.0f;
    partitionViewport_->setPieceOffsets({});
    const QString name = QFileInfo(meshPath).fileName();
    partitionNameLabel_->setText("Input model  |  " + name);
    phaseLabel_->setText("Validating and analyzing local voxel shape");
    metricsLabel_->clear();
    worker_ = new PrintableInterlockWorker(meshPath, parameters, this);
    connect(worker_, &PrintableInterlockWorker::snapshotReady,
            this, &PrintableInterlockWidget::applySnapshot);
    connect(worker_, &PrintableInterlockWorker::failed,
            this, &PrintableInterlockWidget::handleFailure);
    connect(worker_, &QThread::finished,
            this, &PrintableInterlockWidget::handleFinished);
    worker_->setContinuous(continuous);
    emit snapshotChanged(lastSnapshot_);
    emit runningChanged(true);
    emit statusChanged("Validating the solid and sampling local shape inside each voxel...");
    worker_->start();
}

void PrintableInterlockWidget::pause()
{
    if (worker_)
        worker_->setContinuous(false);
}

void PrintableInterlockWidget::resume()
{
    if (worker_)
        worker_->setContinuous(true);
}

void PrintableInterlockWidget::step()
{
    if (worker_)
        worker_->requestStep();
    else if (!meshPath_.isEmpty())
        loadAndStart(meshPath_, lastParameters_, false);
}

void PrintableInterlockWidget::reset()
{
    stopWorker();
    metricsLabel_->clear();
    lastSnapshot_ = PrintableInterlockSnapshot();
    assemblyProgress_ = 0.0f;
    partitionViewport_->setPieceOffsets({});
    if (previewModel_.faces.isEmpty()) {
        partitionViewport_->clearModel();
        partitionNameLabel_->setText("Model preview");
        phaseLabel_->setText("Select one watertight triangle mesh");
        emit statusChanged("Ready");
    } else {
        partitionViewport_->setModelData(previewModel_, 0);
        partitionNameLabel_->setText(
            "Input model  |  " + QFileInfo(meshPath_).fileName());
        phaseLabel_->setText("Input model loaded");
        metricsLabel_->setText(QString("%1 triangles")
            .arg(previewModel_.faces.size() / 3));
        emit statusChanged("Construction reset to the input model.");
    }
    emit snapshotChanged(lastSnapshot_);
    emit runningChanged(false);
}

void PrintableInterlockWidget::setWireframeVisible(bool visible)
{
    partitionViewport_->setShowWireframeOverlay(visible);
}

void PrintableInterlockWidget::setExplosion(float amount)
{
    partitionViewport_->setExplosion(amount);
}

void PrintableInterlockWidget::setAssemblyProgress(float progress)
{
    const int steps = assemblyStepCount();
    assemblyProgress_ = (std::clamp)(progress, 0.0f, float(steps));
    updateAssemblyOffsets();
}

int PrintableInterlockWidget::assemblyStepCount() const
{
    return (std::max)(0, lastSnapshot_.extractionDirections.size() - 1);
}

void PrintableInterlockWidget::updateAssemblyOffsets()
{
    const int pieceCount = lastSnapshot_.extractionDirections.size();
    QVector<QVector3D> offsets(pieceCount, QVector3D());
    const int steps = assemblyStepCount();
    static const QVector<QVector3D> directions = {
        {1.0f, 0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f}, {0.0f, -1.0f, 0.0f},
        {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, -1.0f}
    };
    constexpr float kSeparatedDistance = 2.5f;
    for (int piece = 0; piece < steps; ++piece) {
        const int direction = lastSnapshot_.extractionDirections[piece];
        if (direction < 0 || direction >= directions.size())
            continue;
        // Extraction is piece 0 first; assembly inserts the pieces in reverse order.
        const float assemblyOrder = float(steps - piece);
        const float separated = (std::clamp)(assemblyOrder - assemblyProgress_, 0.0f, 1.0f);
        offsets[piece] = directions[direction] * (kSeparatedDistance * separated);
    }
    partitionViewport_->setPieceOffsets(offsets);
}

void PrintableInterlockWidget::setSurfaceClippedMode(bool surfaceClipped)
{
    surfaceClippedMode_ = surfaceClipped;
    partitionViewport_->setFeatureWireframeOnly(surfaceClippedMode_);
    if (!lastSnapshot_.complete)
        return;
    partitionViewport_->setModelData(
        surfaceClippedMode_ ? lastSnapshot_.partitionedModel
                            : lastSnapshot_.voxelizedModel,
        lastSnapshot_.requestedPieces);
    partitionNameLabel_->setText(
        QString(surfaceClippedMode_ ? "Surface-cut solids  |  " : "Voxel solids  |  ")
        + QFileInfo(meshPath_).fileName());
}

void PrintableInterlockWidget::resetViews()
{
    partitionViewport_->resetView();
}

bool PrintableInterlockWidget::exportPrintableParts(
    const QString& directory, QString* errorMessage) const
{
    const QVector<SteadyDissectionMeshData>& parts = surfaceClippedMode_
        ? lastSnapshot_.printableParts : lastSnapshot_.voxelParts;
    const int watertightParts = surfaceClippedMode_
        ? lastSnapshot_.watertightParts : lastSnapshot_.voxelWatertightParts;
    if (!lastSnapshot_.complete || watertightParts != lastSnapshot_.requestedPieces
        || parts.size() != lastSnapshot_.requestedPieces) {
        if (errorMessage)
            *errorMessage = "Run the construction until all watertight parts are complete.";
        return false;
    }
    QDir outputDirectory(directory);
    if (!outputDirectory.exists()) {
        if (errorMessage)
            *errorMessage = "The selected output directory does not exist.";
        return false;
    }

    const QString baseName = QFileInfo(meshPath_).completeBaseName();
    const QString modeName = surfaceClippedMode_ ? "surface" : "voxel";
    for (int piece = 0; piece < parts.size(); ++piece) {
        const SteadyDissectionMeshData& mesh = parts[piece];
        const QString fileName = QString("%1_%2_part_%3.obj")
            .arg(baseName.isEmpty() ? "interlocking" : baseName)
            .arg(modeName)
            .arg(piece + 1, 2, 10, QChar('0'));
        QSaveFile file(outputDirectory.filePath(fileName));
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            if (errorMessage)
                *errorMessage = QString("Could not create %1.").arg(fileName);
            return false;
        }
        QTextStream stream(&file);
        stream.setLocale(QLocale::c());
        stream.setRealNumberNotation(QTextStream::FixedNotation);
        stream.setRealNumberPrecision(9);
        stream << "# Watertight printable interlocking " << modeName << " part\n";
        stream << "o " << baseName << "_part_" << piece + 1 << "\n";
        const float sourceScale = float(lastSnapshot_.sourceUnitsPerNormalizedUnit);
        for (const QVector3D& normalized : mesh.vertices) {
            const QVector3D point = lastSnapshot_.sourceCenter + normalized * sourceScale;
            stream << "v " << point.x() << ' ' << point.y() << ' ' << point.z() << "\n";
        }
        for (int index = 0; index + 2 < mesh.faces.size(); index += 3) {
            stream << "f " << mesh.faces[index] + 1U << ' '
                   << mesh.faces[index + 1] + 1U << ' '
                   << mesh.faces[index + 2] + 1U << "\n";
        }
        if (stream.status() != QTextStream::Ok || !file.commit()) {
            if (errorMessage)
                *errorMessage = QString("Could not finish writing %1.").arg(fileName);
            return false;
        }
    }
    return true;
}

bool PrintableInterlockWidget::isRunning() const
{
    return worker_ && worker_->isRunning();
}

void PrintableInterlockWidget::applySnapshot(const PrintableInterlockSnapshot& snapshot)
{
    const bool firstCompleteSnapshot = snapshot.complete && !lastSnapshot_.complete;
    lastSnapshot_ = snapshot;
    if (firstCompleteSnapshot)
        assemblyProgress_ = float(assemblyStepCount());
    const SteadyDissectionMeshData& displayedModel = snapshot.complete
        && !surfaceClippedMode_ ? snapshot.voxelizedModel : snapshot.partitionedModel;
    partitionViewport_->setModelData(displayedModel, snapshot.requestedPieces);
    if (snapshot.complete)
        updateAssemblyOffsets();
    if (snapshot.complete) {
        partitionNameLabel_->setText(
            QString(surfaceClippedMode_ ? "Surface-cut solids  |  " : "Voxel solids  |  ")
            + QFileInfo(meshPath_).fileName());
    } else {
        partitionNameLabel_->setText(
            "Current construction stage  |  " + QFileInfo(meshPath_).fileName());
    }
    phaseLabel_->setText(snapshot.phase);
    metricsLabel_->setText(QString("%1 / %2 pieces  |  %3^3 voxels")
        .arg(snapshot.completedPieces).arg(snapshot.requestedPieces).arg(snapshot.resolution));
    emit statusChanged(QString(
        "%1\nPieces %2 / %3\nOccupied voxels %4 | internal %5 | boundary %6\n"
        "Boundary attached %7 / %6\nTiny/disconnected local cells %8 / %9\n"
        "Interlocking sequence: %10\nWatertight voxel/surface parts: %11 / %12 / %3")
        .arg(snapshot.phase)
        .arg(snapshot.completedPieces)
        .arg(snapshot.requestedPieces)
        .arg(snapshot.occupiedVoxels)
        .arg(snapshot.internalVoxels)
        .arg(snapshot.boundaryVoxels)
        .arg(snapshot.attachedBoundaryVoxels)
        .arg(snapshot.tinyVoxels)
        .arg(snapshot.disconnectedVoxels)
        .arg(snapshot.interlocking ? "satisfied" : "pending")
        .arg(snapshot.voxelWatertightParts)
        .arg(snapshot.watertightParts));
    emit snapshotChanged(snapshot);
}

void PrintableInterlockWidget::handleFailure(const QString& message)
{
    phaseLabel_->setText("Printable interlocking construction failed");
    metricsLabel_->clear();
    emit statusChanged("Error: " + message);
}

void PrintableInterlockWidget::handleFinished()
{
    PrintableInterlockWorker* finishedWorker =
        qobject_cast<PrintableInterlockWorker*>(sender());
    if (finishedWorker && worker_ == finishedWorker) {
        worker_->deleteLater();
        worker_ = nullptr;
    }
    emit runningChanged(false);
}
