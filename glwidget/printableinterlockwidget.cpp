#include "printableinterlockwidget.h"

#include <QDir>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLocale>
#include <QSaveFile>
#include <QStringList>
#include <QTextStream>
#include <QVBoxLayout>

#include <algorithm>
#include <utility>

namespace {
QString snapshotDetail(const PrintableInterlockSnapshot& snapshot)
{
    return QString(
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
        .arg(snapshot.watertightParts);
}

QString processStageLabel(int stage)
{
    static const QStringList labels = {
        "(A) Input watertight model",
        "(B) Voxelization and shape analysis",
        "(C) Initial interlocking parts",
        "(D) Boundary voxel attachment",
        "(E) Surface-cut final parts"
    };
    return labels[(std::clamp)(stage, 0, labels.size() - 1)];
}

int failureStageFromMessage(const QString& message, int fallback)
{
    const QString lower = message.toLower();
    if (lower.contains("interlocking core"))
        return 2;
    if (lower.contains("boundary voxel")
        || lower.contains("material-bearing")
        || lower.contains("disassembly sequence")
        || lower.contains("refinement broke")
        || lower.contains("final partition")
        || lower.contains("voxel solid")) {
        return 3;
    }
    if (lower.contains("boolean") || lower.contains("intersection")
        || lower.contains("surface-cut") || lower.contains("labeled solid")
        || lower.contains("printable triangles")) {
        return 4;
    }
    if (lower.contains("voxelization") || lower.contains("input mesh")
        || lower.contains("mesh file") || lower.contains("triangle-only")
        || lower.contains("watertight") || lower.contains("self-intersect")
        || lower.contains("structurally strong internal")) {
        return 1;
    }
    return (std::clamp)(fallback, 1, 4);
}
} // namespace

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
            return snapshot.complete || snapshot.voxelComplete
                ? !stopRequested_.load() : waitAfterSnapshot();
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
    clearProcessHistory();
    partitionViewport_->setPieceOffsets({});
    partitionViewport_->setModelData(previewModel_, 0);
    partitionNameLabel_->setText("Input model  |  " + QFileInfo(meshPath_).fileName());
    phaseLabel_->setText("Input model loaded");
    metricsLabel_->setText(QString("%1 triangles")
        .arg(previewModel_.faces.size() / 3));
    emit statusChanged("Input model loaded. Run all stages or advance one stage at a time.");
    recordProcessFrame(0, processStageLabel(0), "Input model loaded.",
                       previewModel_, 0);
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
    clearProcessHistory();
    if (!previewModel_.faces.isEmpty()) {
        recordProcessFrame(0, processStageLabel(0), "Input model loaded.",
                           previewModel_, 0);
    }
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
    clearProcessHistory();
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
        recordProcessFrame(0, processStageLabel(0),
                           "Construction reset to the input model.",
                           previewModel_, 0);
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
    if (lastSnapshot_.voxelComplete)
        showFinalResult();
    partitionViewport_->setExplosion(amount);
}

void PrintableInterlockWidget::setAssemblyProgress(float progress)
{
    showFinalResult();
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
    if (surfaceClippedMode_ && lastSnapshot_.voxelComplete
        && !lastSnapshot_.complete) {
        surfaceClippedMode_ = false;
    }
    partitionViewport_->setFeatureWireframeOnly(surfaceClippedMode_);
    if (!lastSnapshot_.voxelComplete)
        return;
    showFinalResult();
    updateAssemblyOffsets();
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
    const bool selectedResultComplete = surfaceClippedMode_
        ? lastSnapshot_.complete : lastSnapshot_.voxelComplete;
    if (!selectedResultComplete || watertightParts != lastSnapshot_.requestedPieces
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
    const bool firstAssemblySnapshot = snapshot.voxelComplete
        && !lastSnapshot_.voxelComplete;
    lastSnapshot_ = snapshot;
    recordSnapshotFrame(snapshot);
    if (snapshot.voxelComplete && !snapshot.complete)
        surfaceClippedMode_ = false;
    partitionViewport_->setFeatureWireframeOnly(surfaceClippedMode_);
    if (firstAssemblySnapshot)
        assemblyProgress_ = float(assemblyStepCount());
    if (!viewingProcessFrame_) {
        const SteadyDissectionMeshData& displayedModel = snapshot.voxelComplete
            && (!surfaceClippedMode_ || !snapshot.complete)
                ? snapshot.voxelizedModel : snapshot.partitionedModel;
        partitionViewport_->setModelData(displayedModel, snapshot.requestedPieces);
    }
    if (snapshot.voxelComplete)
        updateAssemblyOffsets();
    if (!viewingProcessFrame_) {
        if (snapshot.voxelComplete) {
            partitionNameLabel_->setText(
                QString(surfaceClippedMode_ ? "Surface-cut solids  |  "
                                            : "Voxel solids  |  ")
                + QFileInfo(meshPath_).fileName());
        } else {
            partitionNameLabel_->setText(
                "Current construction stage  |  " + QFileInfo(meshPath_).fileName());
        }
        phaseLabel_->setText(snapshot.phase);
        metricsLabel_->setText(QString("%1 / %2 pieces  |  %3^3 voxels")
            .arg(snapshot.completedPieces).arg(snapshot.requestedPieces)
            .arg(snapshot.resolution));
    }
    emit statusChanged(snapshotDetail(snapshot));
    emit snapshotChanged(snapshot);
}

void PrintableInterlockWidget::handleFailure(const QString& message)
{
    int failedStage = 1;
    SteadyDissectionMeshData failedModel = previewModel_;
    int failedPieceCount = 0;
    if (!processFrames_.isEmpty()) {
        const ProcessFrame& latest = processFrames_.back();
        failedStage = latest.stage >= 4 ? 4 : latest.stage + 1;
        failedModel = latest.model;
        failedPieceCount = latest.pieceCount;
    }
    failedStage = failureStageFromMessage(message, failedStage);
    const QString detail = "Error: " + message;
    recordProcessFrame(failedStage,
                       processStageLabel(failedStage) + " - failed",
                       detail, failedModel, failedPieceCount, true);
    if (lastSnapshot_.voxelComplete) {
        emit statusChanged(
            "Surface cut error: " + message
            + "\nVoxel interlocking succeeded. Use the assembly controls to inspect it.");
    } else {
        emit statusChanged("Error: " + message);
    }
}

void PrintableInterlockWidget::clearProcessHistory()
{
    processFrames_.clear();
    analysisInternalModel_ = SteadyDissectionMeshData();
    analysisOriginalModel_ = SteadyDissectionMeshData();
    currentProcessFrame_ = -1;
    voxelAnalysisView_ = 1;
    analysisInternalCount_ = 0;
    analysisBoundaryCount_ = 0;
    analysisOccupiedCount_ = 0;
    viewingProcessFrame_ = false;
    emit processHistoryChanged(0, -1, -1, "No construction history", false);
    emit voxelAnalysisViewChanged(false, voxelAnalysisView_);
}

void PrintableInterlockWidget::recordProcessFrame(
    int stage, const QString& label, const QString& detail,
    const SteadyDissectionMeshData& model, int pieceCount, bool failed)
{
    if (failed) {
        for (int i = processFrames_.size() - 1; i >= 0; --i) {
            if (processFrames_[i].stage > stage)
                processFrames_.removeAt(i);
        }
    }
    int index = -1;
    for (int i = 0; i < processFrames_.size(); ++i) {
        if (processFrames_[i].stage == stage) {
            index = i;
            break;
        }
    }
    ProcessFrame frame;
    frame.stage = stage;
    frame.label = label;
    frame.detail = detail;
    frame.model = model;
    frame.pieceCount = pieceCount;
    frame.failed = failed;
    if (index < 0) {
        processFrames_.push_back(std::move(frame));
        index = processFrames_.size() - 1;
    } else {
        processFrames_[index] = std::move(frame);
    }
    showProcessFrame(index);
}

void PrintableInterlockWidget::recordSnapshotFrame(
    const PrintableInterlockSnapshot& snapshot)
{
    const QString detail = snapshotDetail(snapshot);
    if (snapshot.voxelComplete) {
        recordProcessFrame(3, processStageLabel(3), detail,
                           snapshot.voxelizedModel, snapshot.requestedPieces);
        recordProcessFrame(4,
                           snapshot.complete ? processStageLabel(4)
                                             : processStageLabel(4) + " - failed",
                           detail,
                           snapshot.complete ? snapshot.partitionedModel
                                             : snapshot.voxelizedModel,
                           snapshot.requestedPieces, !snapshot.complete);
        return;
    }
    if (snapshot.phase.startsWith("Voxelized")) {
        analysisInternalModel_ = snapshot.internalVoxelModel;
        analysisOriginalModel_ = snapshot.originalModel;
        analysisInternalCount_ = snapshot.internalVoxels;
        analysisBoundaryCount_ = snapshot.boundaryVoxels;
        analysisOccupiedCount_ = snapshot.occupiedVoxels;
        recordProcessFrame(1, processStageLabel(1), detail,
                           snapshot.partitionedModel, snapshot.requestedPieces);
    } else if (snapshot.phase.startsWith("Generated initial")) {
        recordProcessFrame(2, processStageLabel(2), detail,
                           snapshot.partitionedModel, snapshot.requestedPieces);
    } else if (snapshot.phase.startsWith("Attached boundary")
               || snapshot.phase.startsWith("Preparing watertight")) {
        recordProcessFrame(3, processStageLabel(3), detail,
                           snapshot.partitionedModel, snapshot.requestedPieces);
    } else if (snapshot.phase.startsWith("Intersecting")
               || snapshot.phase.startsWith("Cutting")) {
        recordProcessFrame(4, processStageLabel(4) + " - running", detail,
                           snapshot.partitionedModel, snapshot.requestedPieces);
    }
}

void PrintableInterlockWidget::showProcessFrame(int index)
{
    if (index < 0 || index >= processFrames_.size())
        return;
    currentProcessFrame_ = index;
    viewingProcessFrame_ = true;
    const ProcessFrame& frame = processFrames_[index];
    partitionViewport_->setPieceOffsets({});
    if (frame.stage == 1)
        updateVoxelAnalysisView(frame);
    else
        partitionViewport_->setModelData(frame.model, frame.pieceCount);
    phaseLabel_->setText(frame.label);
    if (frame.stage != 1) {
        partitionNameLabel_->setText(
            "Construction replay  |  " + QFileInfo(meshPath_).fileName());
        metricsLabel_->setText(QString("Stage %1 / 5").arg(frame.stage + 1));
    }
    emit statusChanged(frame.detail);
    emit processHistoryChanged(processFrames_.size(), index, frame.stage,
                               frame.label, frame.failed);
    emit voxelAnalysisViewChanged(frame.stage == 1, voxelAnalysisView_);
}

void PrintableInterlockWidget::setVoxelAnalysisView(int mode)
{
    voxelAnalysisView_ = (std::clamp)(mode, 0, 2);
    const bool available = viewingProcessFrame_
        && currentProcessFrame_ >= 0
        && currentProcessFrame_ < processFrames_.size()
        && processFrames_[currentProcessFrame_].stage == 1;
    if (available)
        updateVoxelAnalysisView(processFrames_[currentProcessFrame_]);
    emit voxelAnalysisViewChanged(available, voxelAnalysisView_);
}

void PrintableInterlockWidget::updateVoxelAnalysisView(const ProcessFrame& frame)
{
    static const QStringList modeNames = {
        "Internal voxels", "All occupied voxels", "Surface + internal voxels"
    };
    if (voxelAnalysisView_ == 1) {
        partitionViewport_->setModelData(frame.model, frame.pieceCount);
    } else {
        partitionViewport_->setModelData(analysisInternalModel_, frame.pieceCount);
        if (voxelAnalysisView_ == 2) {
            partitionViewport_->setOverlayModelData(
                analysisOriginalModel_, QVector3D(0.30f, 0.38f, 0.46f), 0.32f);
        }
    }
    partitionNameLabel_->setText(
        QString("Stage B - %1  |  %2")
            .arg(modeNames[voxelAnalysisView_], QFileInfo(meshPath_).fileName()));
    metricsLabel_->setText(
        QString("%1 occupied | %2 internal | %3 boundary")
            .arg(analysisOccupiedCount_)
            .arg(analysisInternalCount_)
            .arg(analysisBoundaryCount_));
}

void PrintableInterlockWidget::showFinalResult()
{
    if (!lastSnapshot_.voxelComplete)
        return;
    viewingProcessFrame_ = false;
    emit voxelAnalysisViewChanged(false, voxelAnalysisView_);
    partitionViewport_->setModelData(
        surfaceClippedMode_ && lastSnapshot_.complete
            ? lastSnapshot_.partitionedModel : lastSnapshot_.voxelizedModel,
        lastSnapshot_.requestedPieces);
    partitionNameLabel_->setText(
        QString(surfaceClippedMode_ && lastSnapshot_.complete
                    ? "Surface-cut solids  |  " : "Voxel solids  |  ")
        + QFileInfo(meshPath_).fileName());
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
