#include "atlasparameterizationwidget.h"

#include "atlas/atlaspipeline.h"

#include <QFile>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QTextStream>
#include <QVBoxLayout>
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace
{
ProgressiveMeshData toDisplayMesh(const AtlasMeshData& source)
{
    ProgressiveMeshData result;
    result.vertices.reserve(int(source.vertices.size() / 3));
    for (std::size_t i = 0; i + 2 < source.vertices.size(); i += 3)
        result.vertices.push_back(QVector3D(
            source.vertices[i], source.vertices[i + 1], source.vertices[i + 2]));
    result.faces.reserve(int(source.faces.size()));
    for (std::uint32_t index : source.faces)
        result.faces.push_back(unsigned(index));
    return result;
}

AtlasDisplaySnapshot toDisplaySnapshot(const AtlasPipelineSnapshot& source)
{
    AtlasDisplaySnapshot result;
    result.source = toDisplayMesh(source.source);
    result.parameterized = toDisplayMesh(source.parameterized);
    result.phase = QString::fromStdString(source.phase);
    result.step = source.step;
    result.distortion = source.distortion;
    result.complete = source.complete;
    return result;
}
}

AtlasWorker::AtlasWorker(const QString& path, QObject* parent)
    : QThread(parent), path_(path)
{
}

AtlasWorker::~AtlasWorker()
{
    requestStop();
    wait();
}

void AtlasWorker::setContinuous(bool continuous)
{
    QMutexLocker lock(&mutex_);
    continuous_ = continuous;
    if (continuous)
        waitCondition_.wakeAll();
}

void AtlasWorker::requestStep()
{
    QMutexLocker lock(&mutex_);
    continuous_ = false;
    ++stepBudget_;
    waitCondition_.wakeAll();
}

void AtlasWorker::requestStop()
{
    stopRequested_.store(true);
    QMutexLocker lock(&mutex_);
    waitCondition_.wakeAll();
}

bool AtlasWorker::waitForNextStep()
{
    QMutexLocker lock(&mutex_);
    while (!stopRequested_.load() && !continuous_ && stepBudget_ == 0)
        waitCondition_.wait(&mutex_);
    if (stepBudget_ > 0)
        --stepBudget_;
    return !stopRequested_.load();
}

void AtlasWorker::run()
{
    try {
        AtlasPipeline pipeline;
        pipeline.initialize(QFile::encodeName(path_).toStdString());
        emit snapshotReady(toDisplaySnapshot(pipeline.snapshot()));

        while (!pipeline.isComplete()) {
            if (!waitForNextStep())
                return;
            emit stepStarted(QString::fromStdString(pipeline.nextStepName()));
            pipeline.executeNextStep([this](const AtlasPipelineSnapshot& snapshot) {
                if (!stopRequested_.load())
                    emit snapshotReady(toDisplaySnapshot(snapshot));
            });
            emit snapshotReady(toDisplaySnapshot(pipeline.snapshot()));
        }
    } catch (const std::exception& error) {
        if (!stopRequested_.load())
            emit failed(QString::fromLocal8Bit(error.what()));
    }
}

AtlasParameterizationWidget::AtlasParameterizationWidget(QWidget* parent)
    : QWidget(parent)
{
    qRegisterMetaType<AtlasDisplaySnapshot>("AtlasDisplaySnapshot");
    sourceView_ = new ProgressiveMeshViewport(this);
    parameterizedView_ = new ProgressiveMeshViewport(this);
    sourceView_->setSurfaceColor(QVector3D(0.72f, 0.76f, 0.82f));
    parameterizedView_->setSurfaceColor(QVector3D(0.88f, 0.82f, 0.62f));

    auto makePane = [this](const QString& title, ProgressiveMeshViewport* view) {
        QWidget* pane = new QWidget(this);
        QVBoxLayout* paneLayout = new QVBoxLayout(pane);
        paneLayout->setContentsMargins(3, 3, 3, 3);
        QLabel* label = new QLabel(title, pane);
        label->setAlignment(Qt::AlignCenter);
        label->setStyleSheet(
            "font-weight: 600; padding: 6px; background: #30343a; color: white;");
        paneLayout->addWidget(label);
        paneLayout->addWidget(view, 1);
        return pane;
    };

    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(4);
    layout->addWidget(makePane("Original 3D Mesh", sourceView_), 1);
    layout->addWidget(makePane("Current UV Atlas", parameterizedView_), 1);
}

AtlasParameterizationWidget::~AtlasParameterizationWidget()
{
    reset();
}

void AtlasParameterizationWidget::loadAndStart(const QString& path, bool continuous)
{
    reset();
    inputPath_ = path;
    lastSnapshot_ = AtlasDisplaySnapshot();
    sourceView_->setMeshData(ProgressiveMeshData(), false);
    parameterizedView_->setMeshData(ProgressiveMeshData(), true);

    worker_ = new AtlasWorker(path, this);
    connect(worker_, &AtlasWorker::snapshotReady,
            this, &AtlasParameterizationWidget::applySnapshot);
    connect(worker_, &AtlasWorker::stepStarted,
            this, &AtlasParameterizationWidget::handleStepStarted);
    connect(worker_, &AtlasWorker::failed,
            this, &AtlasParameterizationWidget::handleFailure);
    connect(worker_, &QThread::finished,
            this, &AtlasParameterizationWidget::handleFinished);
    worker_->setContinuous(continuous);
    emit runningChanged(true);
    emit resultAvailable(false);
    emit statusChanged("Loading mesh and initial texture atlas...");
    worker_->start();
}

void AtlasParameterizationWidget::runAll()
{
    if (worker_ && worker_->isRunning()) {
        worker_->setContinuous(true);
    } else if (!inputPath_.isEmpty()) {
        loadAndStart(inputPath_, true);
    }
}

void AtlasParameterizationWidget::pause()
{
    if (worker_)
        worker_->setContinuous(false);
}

void AtlasParameterizationWidget::step()
{
    if (worker_ && worker_->isRunning()) {
        worker_->requestStep();
    } else if (!inputPath_.isEmpty()) {
        const QString path = inputPath_;
        loadAndStart(path, false);
        worker_->requestStep();
    }
}

void AtlasParameterizationWidget::restart()
{
    if (!inputPath_.isEmpty())
        loadAndStart(inputPath_, false);
}

void AtlasParameterizationWidget::reset()
{
    if (!worker_)
        return;
    worker_->requestStop();
    worker_->wait();
    worker_->deleteLater();
    worker_ = nullptr;
    emit runningChanged(false);
}

void AtlasParameterizationWidget::setRenderMode(BaseGLWidget::RenderMode mode)
{
    for (ProgressiveMeshViewport* view : {sourceView_, parameterizedView_}) {
        view->currentRenderMode = mode;
        view->update();
    }
}

void AtlasParameterizationWidget::setWireframeVisible(bool visible)
{
    for (ProgressiveMeshViewport* view : {sourceView_, parameterizedView_})
        view->setShowWireframeOverlay(visible);
}

void AtlasParameterizationWidget::setFacesVisible(bool visible)
{
    for (ProgressiveMeshViewport* view : {sourceView_, parameterizedView_})
        view->setHideFaces(!visible);
}

void AtlasParameterizationWidget::setCheckerboardVisible(bool visible)
{
    sourceView_->setCheckerboardVisible(visible);
    parameterizedView_->setCheckerboardVisible(visible);
}

void AtlasParameterizationWidget::resetViews()
{
    sourceView_->resetView();
    parameterizedView_->resetView();
}

void AtlasParameterizationWidget::centerViews()
{
    sourceView_->centerView();
    parameterizedView_->centerView();
}

bool AtlasParameterizationWidget::saveParameterized(const QString& path) const
{
    if (lastSnapshot_.parameterized.vertices.isEmpty())
        return false;
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    QTextStream stream(&file);
    for (const QVector3D& point : lastSnapshot_.parameterized.vertices)
        stream << "v " << point.x() << ' ' << point.y() << ' ' << point.z() << '\n';
    for (int i = 0; i + 2 < lastSnapshot_.parameterized.faces.size(); i += 3) {
        stream << "f "
               << lastSnapshot_.parameterized.faces[i] + 1 << ' '
               << lastSnapshot_.parameterized.faces[i + 1] + 1 << ' '
               << lastSnapshot_.parameterized.faces[i + 2] + 1 << '\n';
    }
    return true;
}

bool AtlasParameterizationWidget::isRunning() const
{
    return worker_ && worker_->isRunning();
}

void AtlasParameterizationWidget::applySnapshot(const AtlasDisplaySnapshot& snapshot)
{
    lastSnapshot_ = snapshot;
    QVector<QVector2D> checkerCoordinates;
    if (!snapshot.parameterized.vertices.isEmpty()) {
        float minX = snapshot.parameterized.vertices.front().x();
        float maxX = minX;
        float minY = snapshot.parameterized.vertices.front().y();
        float maxY = minY;
        for (const QVector3D& vertex : snapshot.parameterized.vertices) {
            minX = (std::min)(minX, vertex.x());
            maxX = (std::max)(maxX, vertex.x());
            minY = (std::min)(minY, vertex.y());
            maxY = (std::max)(maxY, vertex.y());
        }
        const float extent = (std::max)(maxX - minX, maxY - minY);
        const QVector2D origin(0.5f * (minX + maxX), 0.5f * (minY + maxY));
        const float scale = std::isfinite(extent) && extent > 1e-8f ? 1.0f / extent : 1.0f;
        checkerCoordinates.reserve(snapshot.parameterized.vertices.size());
        for (const QVector3D& vertex : snapshot.parameterized.vertices) {
            checkerCoordinates.push_back(
                (QVector2D(vertex.x(), vertex.y()) - origin) * scale
                + QVector2D(0.5f, 0.5f));
        }
    }

    const bool matchingTopology =
        snapshot.source.vertices.size() == checkerCoordinates.size();
    sourceView_->setMeshData(
        snapshot.source, false,
        matchingTopology ? checkerCoordinates : QVector<QVector2D>());
    parameterizedView_->setMeshData(snapshot.parameterized, true, checkerCoordinates);
    emit resultAvailable(!snapshot.parameterized.vertices.isEmpty());
    emit statusChanged(QString("%1 | step %2 | UV V/F %3/%4 | distortion %5")
        .arg(snapshot.phase)
        .arg(snapshot.step)
        .arg(snapshot.parameterized.vertices.size())
        .arg(snapshot.parameterized.faces.size() / 3)
        .arg(snapshot.distortion, 0, 'g', 7));
}

void AtlasParameterizationWidget::handleStepStarted(const QString& phase)
{
    if (phase == "Distortion Reduce") {
        emit statusChanged(
            "Running: Distortion Reduce | each sparse-solver iteration will refresh the atlas...");
        return;
    }
    emit statusChanged("Running: " + phase + "...");
}

void AtlasParameterizationWidget::handleFailure(const QString& message)
{
    emit statusChanged("Error: " + message);
    emit runningChanged(false);
}

void AtlasParameterizationWidget::handleFinished()
{
    emit runningChanged(false);
}
