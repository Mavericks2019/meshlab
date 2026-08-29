#include "dchartswidget.h"

#include <QColor>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QSet>
#include <QVBoxLayout>

#include <algorithm>
#include <cmath>

DChartsViewport::DChartsViewport(QWidget* parent)
    : BaseGLWidget(parent),
      colorVbo_(QOpenGLBuffer::VertexBuffer),
      seamVbo_(QOpenGLBuffer::VertexBuffer),
      seedVbo_(QOpenGLBuffer::VertexBuffer)
{
    bgColor = QColor("#20242a");
    setWireframeColor(QVector4D(0.08f, 0.09f, 0.11f, 0.38f));
    setShowWireframeOverlay(false);
    setMinimumSize(480, 360);
}

DChartsViewport::~DChartsViewport()
{
    if (context()) {
        makeCurrent();
        colorVbo_.destroy();
        seamVbo_.destroy();
        seedVbo_.destroy();
        overlayVao_.destroy();
        chartProgram_.removeAllShaders();
        doneCurrent();
    }
}

void DChartsViewport::initializeGL()
{
    BaseGLWidget::initializeGL();
    colorVbo_.create();
    seamVbo_.create();
    seedVbo_.create();
    overlayVao_.create();
    chartProgram_.addShaderFromSourceFile(
        QOpenGLShader::Vertex, ":/glwidget/shaders/progressive_component.vert");
    chartProgram_.addShaderFromSourceFile(
        QOpenGLShader::Fragment, ":/glwidget/shaders/progressive_component.frag");
    chartProgram_.link();
    glReady_ = true;
    if (modelLoaded)
        updateBuffersFromOpenMesh();
}

QVector3D DChartsViewport::chartColor(int chart) const
{
    static const QVector<QVector3D> palette = {
        {0.31f, 0.55f, 0.76f}, {0.95f, 0.56f, 0.17f}, {0.88f, 0.34f, 0.35f},
        {0.46f, 0.72f, 0.70f}, {0.35f, 0.63f, 0.31f}, {0.93f, 0.79f, 0.28f},
        {0.69f, 0.48f, 0.63f}, {1.00f, 0.62f, 0.66f}, {0.61f, 0.46f, 0.37f},
        {0.40f, 0.72f, 0.89f}, {0.75f, 0.43f, 0.20f}, {0.49f, 0.44f, 0.78f}
    };
    if (chart < 0)
        return QVector3D(0.28f, 0.30f, 0.33f);
    if (chart < palette.size())
        return palette[chart];
    const int hue = (chart * 137 + 23) % 360;
    const QColor color = QColor::fromHsv(hue, 168, 222);
    return QVector3D(float(color.redF()), float(color.greenF()), float(color.blueF()));
}

void DChartsViewport::setSnapshot(const DChartsSnapshot& snapshot)
{
    const bool initializeView = !modelLoaded;
    clearMeshData();
    faceColors_.clear();
    seamPositions_.clear();
    seedPositions_.clear();
    if (snapshot.mesh.vertices.isEmpty() || snapshot.mesh.faces.isEmpty()) {
        update();
        return;
    }

    QVector3D minimum = snapshot.mesh.vertices.front();
    QVector3D maximum = minimum;
    for (const QVector3D& point : snapshot.mesh.vertices) {
        minimum.setX((std::min)(minimum.x(), point.x()));
        minimum.setY((std::min)(minimum.y(), point.y()));
        minimum.setZ((std::min)(minimum.z(), point.z()));
        maximum.setX((std::max)(maximum.x(), point.x()));
        maximum.setY((std::max)(maximum.y(), point.y()));
        maximum.setZ((std::max)(maximum.z(), point.z()));
    }
    const QVector3D center = 0.5f * (minimum + maximum);
    const QVector3D extent = maximum - minimum;
    const float scale = 2.0f / (std::max)({extent.x(), extent.y(), extent.z(), 1e-6f});
    auto normalized = [&](unsigned index) {
        return (snapshot.mesh.vertices[int(index)] - center) * scale;
    };

    QSet<int> seedFaces;
    for (const int seed : snapshot.seedFaces)
        seedFaces.insert(seed);

    const int faceCount = snapshot.mesh.faces.size() / 3;
    for (int face = 0; face < faceCount; ++face) {
        std::vector<Mesh::VertexHandle> handles;
        handles.reserve(3);
        QVector3D centroid;
        const int chart = face < snapshot.faceCharts.size()
            ? snapshot.faceCharts[face] : -1;
        const QVector3D color = chartColor(chart);
        for (int corner = 0; corner < 3; ++corner) {
            const unsigned sourceIndex = snapshot.mesh.faces[face * 3 + corner];
            const QVector3D point = normalized(sourceIndex);
            handles.push_back(openMesh.add_vertex(Mesh::Point(point.x(), point.y(), point.z())));
            faceColors_.push_back(color);
            centroid += point;
        }
        openMesh.add_face(handles);
        if (seedFaces.contains(face))
            seedPositions_.push_back(centroid / 3.0f);
    }

    for (int i = 0; i + 1 < snapshot.seamEdges.size(); i += 2) {
        const unsigned a = snapshot.seamEdges[i];
        const unsigned b = snapshot.seamEdges[i + 1];
        if (a < unsigned(snapshot.mesh.vertices.size())
            && b < unsigned(snapshot.mesh.vertices.size())) {
            seamPositions_.push_back(normalized(a));
            seamPositions_.push_back(normalized(b));
        }
    }

    openMesh.request_face_normals();
    openMesh.request_vertex_normals();
    openMesh.update_normals();
    prepareFaceIndices();
    prepareEdgeIndices();
    modelLoaded = true;
    if (initializeView) {
        rotation = QQuaternion::fromEulerAngles(-16.0f, 28.0f, 0.0f);
        zoom = 1.0f;
        modelCenter = QVector3D();
        viewDistance = 4.0f;
        viewScale = 1.0f;
        initialRotation = rotation;
        initialZoom = zoom;
        initialModelCenter = modelCenter;
        initialViewDistance = viewDistance;
        initialViewScale = viewScale;
    }

    if (glReady_) {
        makeCurrent();
        updateBuffersFromOpenMesh();
        doneCurrent();
    }
    update();
}

void DChartsViewport::clearSnapshot()
{
    clearMeshData();
    faceColors_.clear();
    seamPositions_.clear();
    seedPositions_.clear();
    update();
}

void DChartsViewport::setSeamsVisible(bool visible)
{
    seamsVisible_ = visible;
    update();
}

void DChartsViewport::setSeedsVisible(bool visible)
{
    seedsVisible_ = visible;
    update();
}

void DChartsViewport::updateBuffersFromOpenMesh()
{
    BaseGLWidget::updateBuffersFromOpenMesh();
    if (!glReady_)
        return;

    vao.bind();
    if (chartProgram_.isLinked() && faceColors_.size() == int(openMesh.n_vertices())) {
        chartProgram_.bind();
        vbo.bind();
        const int positionLocation = chartProgram_.attributeLocation("aPos");
        if (positionLocation >= 0) {
            chartProgram_.enableAttributeArray(positionLocation);
            chartProgram_.setAttributeBuffer(
                positionLocation, GL_FLOAT, 0, 3, 3 * sizeof(float));
        }
        colorVbo_.bind();
        colorVbo_.allocate(faceColors_.constData(), faceColors_.size() * int(sizeof(QVector3D)));
        const int colorLocation = chartProgram_.attributeLocation("aComponentColor");
        if (colorLocation >= 0) {
            chartProgram_.enableAttributeArray(colorLocation);
            chartProgram_.setAttributeBuffer(
                colorLocation, GL_FLOAT, 0, 3, sizeof(QVector3D));
        }
        colorVbo_.release();
        chartProgram_.release();
    }
    vao.release();

    seamVbo_.bind();
    seamVbo_.allocate(seamPositions_.constData(), seamPositions_.size() * int(sizeof(QVector3D)));
    seamVbo_.release();
    seedVbo_.bind();
    seedVbo_.allocate(seedPositions_.constData(), seedPositions_.size() * int(sizeof(QVector3D)));
    seedVbo_.release();
}

void DChartsViewport::drawOverlay(const QMatrix4x4& model, const QMatrix4x4& view,
                                  const QMatrix4x4& projection, QOpenGLBuffer& buffer,
                                  int vertexCount, GLenum primitive, const QVector4D& color,
                                  float widthOrSize)
{
    if (vertexCount <= 0)
        return;
    GLint previousDepthFunction = GL_LESS;
    GLboolean previousDepthMask = GL_TRUE;
    glGetIntegerv(GL_DEPTH_FUNC, &previousDepthFunction);
    glGetBooleanv(GL_DEPTH_WRITEMASK, &previousDepthMask);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_FALSE);
    if (primitive == GL_POINTS)
        glPointSize(widthOrSize);
    else
        glLineWidth(widthOrSize);

    wireframeProgram.bind();
    overlayVao_.bind();
    buffer.bind();
    const int positionLocation = wireframeProgram.attributeLocation("aPos");
    wireframeProgram.enableAttributeArray(positionLocation);
    wireframeProgram.setAttributeBuffer(
        positionLocation, GL_FLOAT, 0, 3, sizeof(QVector3D));
    wireframeProgram.setUniformValue("model", model);
    wireframeProgram.setUniformValue("view", view);
    wireframeProgram.setUniformValue("projection", projection);
    wireframeProgram.setUniformValue("lineColor", color);
    glDrawArrays(primitive, 0, vertexCount);
    wireframeProgram.disableAttributeArray(positionLocation);
    buffer.release();
    overlayVao_.release();
    wireframeProgram.release();

    glDepthMask(previousDepthMask);
    glDepthFunc(previousDepthFunction);
}

void DChartsViewport::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    if (!modelLoaded || openMesh.n_vertices() == 0)
        return;

    QMatrix4x4 model;
    QMatrix4x4 view;
    QMatrix4x4 projection;
    model.rotate(rotation);
    model.scale(zoom);
    view.lookAt(QVector3D(0.0f, 0.0f, viewDistance * viewScale), modelCenter,
                QVector3D(0.0f, 1.0f, 0.0f));
    projection.perspective(45.0f, width() / float((std::max)(height(), 1)), 0.1f, 100.0f);

    GLint oldPolygonMode[2];
    glGetIntegerv(GL_POLYGON_MODE, oldPolygonMode);
    if (!hideFaces) {
        if (showWireframeOverlay) {
            glEnable(GL_POLYGON_OFFSET_FILL);
            glPolygonOffset(1.0f, 1.0f);
        }
        chartProgram_.bind();
        vao.bind();
        faceEbo.bind();
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        chartProgram_.setUniformValue("model", model);
        chartProgram_.setUniformValue("view", view);
        chartProgram_.setUniformValue("projection", projection);
        glDrawElements(GL_TRIANGLES, GLsizei(faces.size()), GL_UNSIGNED_INT, nullptr);
        faceEbo.release();
        vao.release();
        chartProgram_.release();
        if (showWireframeOverlay) {
            glDisable(GL_POLYGON_OFFSET_FILL);
            drawWireframeOverlay(model, view, projection);
        }
    } else {
        drawWireframe(model, view, projection);
    }

    if (seamsVisible_) {
        drawOverlay(model, view, projection, seamVbo_, seamPositions_.size(), GL_LINES,
                    QVector4D(0.03f, 0.035f, 0.045f, 1.0f), 3.0f);
    }
    if (seedsVisible_) {
        drawOverlay(model, view, projection, seedVbo_, seedPositions_.size(), GL_POINTS,
                    QVector4D(0.98f, 0.98f, 0.95f, 1.0f), 9.0f);
    }

    if (showAxis)
        drawXYZAxis(view, projection);
    glPolygonMode(GL_FRONT, oldPolygonMode[0]);
    glPolygonMode(GL_BACK, oldPolygonMode[1]);
}

DChartsWorker::DChartsWorker(const QString& path, const DChartsParameters& parameters,
                             QObject* parent)
    : QThread(parent), path_(path), parameters_(parameters)
{
}

DChartsWorker::~DChartsWorker()
{
    requestStop();
    wait();
}

void DChartsWorker::setContinuous(bool continuous)
{
    QMutexLocker lock(&mutex_);
    continuous_ = continuous;
    if (continuous)
        waitCondition_.wakeAll();
}

void DChartsWorker::requestStep()
{
    QMutexLocker lock(&mutex_);
    continuous_ = false;
    ++stepBudget_;
    waitCondition_.wakeAll();
}

void DChartsWorker::requestStop()
{
    stopRequested_.store(true);
    QMutexLocker lock(&mutex_);
    waitCondition_.wakeAll();
}

bool DChartsWorker::waitAfterSnapshot()
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
        msleep(110);
    return keepRunning;
}

void DChartsWorker::run()
{
    QString error;
    const bool succeeded = DChartsAlgorithm::run(
        path_, parameters_,
        [this](const DChartsSnapshot& snapshot) {
            if (stopRequested_.load())
                return false;
            emit snapshotReady(snapshot);
            return snapshot.complete ? !stopRequested_.load() : waitAfterSnapshot();
        },
        &error);
    if (!succeeded && !stopRequested_.load())
        emit failed(error.isEmpty() ? "D-Charts segmentation failed." : error);
}

DChartsWidget::DChartsWidget(QWidget* parent)
    : QWidget(parent)
{
    qRegisterMetaType<DChartsSnapshot>("DChartsSnapshot");
    viewport_ = new DChartsViewport(this);

    phaseLabel_ = new QLabel("No mesh loaded", this);
    phaseLabel_->setStyleSheet("font-size: 15px; font-weight: 600; color: #f4f6f8;");
    metricsLabel_ = new QLabel("", this);
    metricsLabel_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    metricsLabel_->setStyleSheet("color: #aeb7c2;");

    QFrame* header = new QFrame(this);
    header->setStyleSheet("QFrame { background: #2b3037; border-bottom: 1px solid #404751; }");
    QHBoxLayout* headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(14, 9, 14, 9);
    headerLayout->addWidget(phaseLabel_, 1);
    headerLayout->addWidget(metricsLabel_);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(header);
    layout->addWidget(viewport_, 1);
}

DChartsWidget::~DChartsWidget()
{
    reset();
}

void DChartsWidget::loadAndStart(const QString& path,
                                 const DChartsParameters& parameters,
                                 bool continuous)
{
    reset();
    inputPath_ = path;
    lastParameters_ = parameters;
    viewport_->clearSnapshot();
    phaseLabel_->setText("Loading mesh");
    metricsLabel_->clear();
    worker_ = new DChartsWorker(path, parameters, this);
    connect(worker_, &DChartsWorker::snapshotReady,
            this, &DChartsWidget::applySnapshot);
    connect(worker_, &DChartsWorker::failed,
            this, &DChartsWidget::handleFailure);
    connect(worker_, &QThread::finished,
            this, &DChartsWidget::handleFinished);
    worker_->setContinuous(continuous);
    emit runningChanged(true);
    emit statusChanged("Loading and triangulating the input mesh...");
    worker_->start();
}

void DChartsWidget::pause()
{
    if (worker_)
        worker_->setContinuous(false);
}

void DChartsWidget::resume()
{
    if (worker_)
        worker_->setContinuous(true);
}

void DChartsWidget::step()
{
    if (worker_)
        worker_->requestStep();
    else if (!inputPath_.isEmpty())
        loadAndStart(inputPath_, lastParameters_, false);
}

void DChartsWidget::reset()
{
    if (worker_) {
        worker_->requestStop();
        worker_->wait();
        worker_->deleteLater();
        worker_ = nullptr;
    }
    viewport_->clearSnapshot();
    phaseLabel_->setText("No mesh loaded");
    metricsLabel_->clear();
    emit statusChanged("Ready");
    emit runningChanged(false);
}

void DChartsWidget::setWireframeVisible(bool visible)
{
    viewport_->setShowWireframeOverlay(visible);
}

void DChartsWidget::setSeamsVisible(bool visible)
{
    viewport_->setSeamsVisible(visible);
}

void DChartsWidget::setSeedsVisible(bool visible)
{
    viewport_->setSeedsVisible(visible);
}

void DChartsWidget::resetView()
{
    viewport_->resetView();
}

void DChartsWidget::centerView()
{
    viewport_->centerView();
}

bool DChartsWidget::isRunning() const
{
    return worker_ && worker_->isRunning();
}

void DChartsWidget::applySnapshot(const DChartsSnapshot& snapshot)
{
    viewport_->setSnapshot(snapshot);
    phaseLabel_->setText(snapshot.phase);
    metricsLabel_->setText(QString("Iteration %1  |  %2 charts  |  %3 / %4 faces")
        .arg(snapshot.iteration)
        .arg(snapshot.chartCount)
        .arg(snapshot.assignedFaces)
        .arg(snapshot.assignedFaces + snapshot.unassignedFaces));
    emit statusChanged(QString(
        "%1\nIteration %2 | Charts %3\nAssigned %4 | Unassigned %5\nChanged %6% | mean F %7 | max F %8")
        .arg(snapshot.phase)
        .arg(snapshot.iteration)
        .arg(snapshot.chartCount)
        .arg(snapshot.assignedFaces)
        .arg(snapshot.unassignedFaces)
        .arg(snapshot.reassignedRatio * 100.0, 0, 'f', 2)
        .arg(snapshot.meanFittingError, 0, 'g', 5)
        .arg(snapshot.maxFittingError, 0, 'g', 5));
    emit snapshotChanged(snapshot);
}

void DChartsWidget::handleFailure(const QString& message)
{
    phaseLabel_->setText("Segmentation failed");
    metricsLabel_->clear();
    emit statusChanged("Error: " + message);
}

void DChartsWidget::handleFinished()
{
    DChartsWorker* finishedWorker = qobject_cast<DChartsWorker*>(sender());
    if (finishedWorker && worker_ == finishedWorker) {
        worker_->deleteLater();
        worker_ = nullptr;
    }
    emit runningChanged(false);
}
