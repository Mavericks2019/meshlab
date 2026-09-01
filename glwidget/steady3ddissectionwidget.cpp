#include "steady3ddissectionwidget.h"

#include <QColor>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QSplitter>
#include <QVBoxLayout>

#include <algorithm>
#include <array>
#include <map>

SteadyDissectionViewport::SteadyDissectionViewport(QWidget* parent)
    : BaseGLWidget(parent),
      colorVbo_(QOpenGLBuffer::VertexBuffer),
      overlayVbo_(QOpenGLBuffer::VertexBuffer),
      overlayEbo_(QOpenGLBuffer::IndexBuffer)
{
    bgColor = QColor("#20242a");
    setWireframeColor(QVector4D(0.035f, 0.04f, 0.05f, 0.55f));
    setShowWireframeOverlay(true);
    setMinimumSize(360, 320);
}

SteadyDissectionViewport::~SteadyDissectionViewport()
{
    if (context()) {
        makeCurrent();
        colorVbo_.destroy();
        overlayVao_.destroy();
        overlayVbo_.destroy();
        overlayEbo_.destroy();
        pieceProgram_.removeAllShaders();
        overlayProgram_.removeAllShaders();
        doneCurrent();
    }
}

void SteadyDissectionViewport::initializeGL()
{
    BaseGLWidget::initializeGL();
    colorVbo_.create();
    overlayVao_.create();
    overlayVbo_.create();
    overlayEbo_.create();
    pieceProgram_.addShaderFromSourceFile(
        QOpenGLShader::Vertex, ":/glwidget/shaders/progressive_component.vert");
    pieceProgram_.addShaderFromSourceFile(
        QOpenGLShader::Fragment, ":/glwidget/shaders/progressive_component.frag");
    pieceProgram_.link();
    overlayProgram_.addShaderFromSourceFile(
        QOpenGLShader::Vertex, ":/glwidget/shaders/transparent_overlay.vert");
    overlayProgram_.addShaderFromSourceFile(
        QOpenGLShader::Fragment, ":/glwidget/shaders/transparent_overlay.frag");
    overlayProgram_.link();
    glReady_ = true;
    if (!modelData_.faces.isEmpty())
        rebuildMesh();
    if (!overlayModelData_.faces.isEmpty())
        rebuildOverlayBuffers();
}

QVector3D SteadyDissectionViewport::pieceColor(int piece) const
{
    static const QVector<QVector3D> palette = {
        {0.91f, 0.35f, 0.36f}, {0.30f, 0.57f, 0.82f}, {0.42f, 0.72f, 0.47f},
        {0.95f, 0.68f, 0.25f}, {0.66f, 0.48f, 0.78f}, {0.33f, 0.74f, 0.73f},
        {0.95f, 0.49f, 0.67f}, {0.68f, 0.63f, 0.32f}, {0.49f, 0.47f, 0.83f},
        {0.88f, 0.55f, 0.31f}, {0.37f, 0.66f, 0.89f}, {0.72f, 0.42f, 0.43f},
        {0.52f, 0.75f, 0.34f}, {0.84f, 0.48f, 0.78f}, {0.91f, 0.78f, 0.31f}
    };
    if (piece < 0)
        return QVector3D(0.58f, 0.61f, 0.65f);
    if (piece < palette.size())
        return palette[piece];
    const QColor color = QColor::fromHsv((piece * 137 + 19) % 360, 170, 225);
    return QVector3D(float(color.redF()), float(color.greenF()), float(color.blueF()));
}

void SteadyDissectionViewport::setModelData(const SteadyDissectionMeshData& data,
                                            int pieceCount)
{
    clearOverlayModelData();
    modelData_ = data;
    pieceCount_ = pieceCount;
    rebuildMesh();
}

void SteadyDissectionViewport::setOverlayModelData(
    const SteadyDissectionMeshData& data, const QVector3D& color, float opacity)
{
    overlayModelData_ = data;
    overlayColor_ = color;
    overlayOpacity_ = (std::clamp)(opacity, 0.0f, 1.0f);
    if (glReady_) {
        makeCurrent();
        rebuildOverlayBuffers();
        doneCurrent();
    }
    update();
}

void SteadyDissectionViewport::clearOverlayModelData()
{
    overlayModelData_ = SteadyDissectionMeshData();
    update();
}

void SteadyDissectionViewport::clearModel()
{
    modelData_ = SteadyDissectionMeshData();
    pieceCount_ = 0;
    pieceOffsets_.clear();
    clearOverlayModelData();
    clearMeshData();
    faceColors_.clear();
    update();
}

void SteadyDissectionViewport::setExplosion(float amount)
{
    explosion_ = (std::max)(0.0f, amount);
    if (!modelData_.faces.isEmpty())
        rebuildMesh();
}

void SteadyDissectionViewport::setPieceOffsets(const QVector<QVector3D>& offsets)
{
    pieceOffsets_ = offsets;
    if (!modelData_.faces.isEmpty())
        rebuildMesh();
}

void SteadyDissectionViewport::setFeatureWireframeOnly(bool enabled)
{
    if (featureWireframeOnly_ == enabled)
        return;
    featureWireframeOnly_ = enabled;
    if (!modelLoaded)
        return;
    rebuildWireframeEdges();
    if (glReady_) {
        makeCurrent();
        updateBuffersFromOpenMesh();
        doneCurrent();
    }
    update();
}

void SteadyDissectionViewport::rebuildMesh()
{
    const bool initializeView = !modelLoaded;
    clearMeshData();
    faceColors_.clear();
    if (modelData_.vertices.isEmpty() || modelData_.faces.isEmpty()) {
        update();
        return;
    }

    QVector<QVector3D> pieceCenters((std::max)(pieceCount_, 0), QVector3D());
    QVector<int> pieceSamples(pieceCenters.size(), 0);
    QVector3D globalCenter;
    int globalSamples = 0;
    const int faceCount = modelData_.faces.size() / 3;
    for (int face = 0; face < faceCount; ++face) {
        const int piece = face < modelData_.facePieces.size()
            ? modelData_.facePieces[face] : -1;
        for (int corner = 0; corner < 3; ++corner) {
            const QVector3D point = modelData_.vertices[
                int(modelData_.faces[face * 3 + corner])];
            globalCenter += point;
            ++globalSamples;
            if (piece >= 0 && piece < pieceCenters.size()) {
                pieceCenters[piece] += point;
                ++pieceSamples[piece];
            }
        }
    }
    if (globalSamples > 0)
        globalCenter /= float(globalSamples);
    for (int piece = 0; piece < pieceCenters.size(); ++piece)
        if (pieceSamples[piece] > 0)
            pieceCenters[piece] /= float(pieceSamples[piece]);

    std::map<std::pair<unsigned int, int>, Mesh::VertexHandle> vertexHandles;
    for (int face = 0; face < faceCount; ++face) {
        const int piece = face < modelData_.facePieces.size()
            ? modelData_.facePieces[face] : -1;
        QVector3D offset = piece >= 0 && piece < pieceCenters.size()
            ? (pieceCenters[piece] - globalCenter) * explosion_ : QVector3D();
        if (piece >= 0 && piece < pieceOffsets_.size())
            offset += pieceOffsets_[piece];
        std::array<unsigned int, 3> sourceVertices;
        bool validFace = true;
        for (int corner = 0; corner < 3; ++corner) {
            sourceVertices[corner] = modelData_.faces[face * 3 + corner];
            if (sourceVertices[corner] >= unsigned(modelData_.vertices.size()))
                validFace = false;
        }
        if (!validFace)
            continue;
        std::vector<Mesh::VertexHandle> handles;
        handles.reserve(3);
        for (int corner = 0; corner < 3; ++corner) {
            const auto key = std::make_pair(sourceVertices[corner], piece);
            auto found = vertexHandles.find(key);
            if (found == vertexHandles.end()) {
                const QVector3D point = modelData_.vertices[int(sourceVertices[corner])] + offset;
                const Mesh::VertexHandle handle = openMesh.add_vertex(
                    Mesh::Point(point.x(), point.y(), point.z()));
                vertexHandles.emplace(key, handle);
                if (faceColors_.size() <= handle.idx())
                    faceColors_.resize(handle.idx() + 1);
                faceColors_[handle.idx()] = pieceColor(piece);
                handles.push_back(handle);
            } else {
                handles.push_back(found->second);
            }
        }
        openMesh.add_face(handles);
    }

    openMesh.request_face_normals();
    openMesh.request_vertex_normals();
    openMesh.update_normals();
    prepareFaceIndices();
    rebuildWireframeEdges();
    modelLoaded = true;
    if (initializeView) {
        rotation = QQuaternion::fromEulerAngles(-18.0f, 30.0f, 0.0f);
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

void SteadyDissectionViewport::rebuildWireframeEdges()
{
    if (!featureWireframeOnly_) {
        prepareEdgeIndices();
        return;
    }

    edges.clear();
    constexpr double kCoplanarNormalDot = 0.9999;
    for (Mesh::EdgeHandle edge : openMesh.edges()) {
        const Mesh::HalfedgeHandle first = openMesh.halfedge_handle(edge, 0);
        const Mesh::HalfedgeHandle second = openMesh.halfedge_handle(edge, 1);
        bool visible = openMesh.is_boundary(first) || openMesh.is_boundary(second);
        if (!visible) {
            const Mesh::FaceHandle firstFace = openMesh.face_handle(first);
            const Mesh::FaceHandle secondFace = openMesh.face_handle(second);
            const Mesh::Normal firstNormal = openMesh.normal(firstFace);
            const Mesh::Normal secondNormal = openMesh.normal(secondFace);
            const double normalDot = firstNormal[0] * secondNormal[0]
                + firstNormal[1] * secondNormal[1]
                + firstNormal[2] * secondNormal[2];
            visible = normalDot < kCoplanarNormalDot;
        }
        if (!visible)
            continue;
        edges.push_back(unsigned(openMesh.from_vertex_handle(first).idx()));
        edges.push_back(unsigned(openMesh.to_vertex_handle(first).idx()));
    }
}

void SteadyDissectionViewport::updateBuffersFromOpenMesh()
{
    BaseGLWidget::updateBuffersFromOpenMesh();
    if (!glReady_)
        return;

    vao.bind();
    if (pieceProgram_.isLinked() && faceColors_.size() == int(openMesh.n_vertices())) {
        pieceProgram_.bind();
        vbo.bind();
        const int positionLocation = pieceProgram_.attributeLocation("aPos");
        if (positionLocation >= 0) {
            pieceProgram_.enableAttributeArray(positionLocation);
            pieceProgram_.setAttributeBuffer(
                positionLocation, GL_FLOAT, 0, 3, 3 * sizeof(float));
        }
        colorVbo_.bind();
        colorVbo_.allocate(faceColors_.constData(),
                           faceColors_.size() * int(sizeof(QVector3D)));
        const int colorLocation = pieceProgram_.attributeLocation("aComponentColor");
        if (colorLocation >= 0) {
            pieceProgram_.enableAttributeArray(colorLocation);
            pieceProgram_.setAttributeBuffer(
                colorLocation, GL_FLOAT, 0, 3, sizeof(QVector3D));
        }
        colorVbo_.release();
        pieceProgram_.release();
    }
    vao.release();
}

void SteadyDissectionViewport::rebuildOverlayBuffers()
{
    if (!glReady_ || !overlayProgram_.isLinked()
        || overlayModelData_.vertices.isEmpty()
        || overlayModelData_.faces.isEmpty()) {
        return;
    }
    overlayVao_.bind();
    overlayProgram_.bind();
    overlayVbo_.bind();
    overlayVbo_.allocate(overlayModelData_.vertices.constData(),
                         overlayModelData_.vertices.size() * int(sizeof(QVector3D)));
    const int positionLocation = overlayProgram_.attributeLocation("aPos");
    if (positionLocation >= 0) {
        overlayProgram_.enableAttributeArray(positionLocation);
        overlayProgram_.setAttributeBuffer(
            positionLocation, GL_FLOAT, 0, 3, sizeof(QVector3D));
    }
    overlayEbo_.bind();
    overlayEbo_.allocate(overlayModelData_.faces.constData(),
                         overlayModelData_.faces.size() * int(sizeof(unsigned int)));
    overlayVbo_.release();
    overlayProgram_.release();
    overlayVao_.release();
}

void SteadyDissectionViewport::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    const bool hasMainModel = modelLoaded && openMesh.n_vertices() > 0;
    const bool hasOverlay = overlayProgram_.isLinked()
        && !overlayModelData_.vertices.isEmpty()
        && !overlayModelData_.faces.isEmpty();
    if (!hasMainModel && !hasOverlay)
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
    if (hasMainModel && !hideFaces) {
        if (showWireframeOverlay) {
            glEnable(GL_POLYGON_OFFSET_FILL);
            glPolygonOffset(1.0f, 1.0f);
        }
        pieceProgram_.bind();
        vao.bind();
        faceEbo.bind();
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        pieceProgram_.setUniformValue("model", model);
        pieceProgram_.setUniformValue("view", view);
        pieceProgram_.setUniformValue("projection", projection);
        glDrawElements(GL_TRIANGLES, GLsizei(faces.size()), GL_UNSIGNED_INT, nullptr);
        faceEbo.release();
        vao.release();
        pieceProgram_.release();
        if (showWireframeOverlay) {
            glDisable(GL_POLYGON_OFFSET_FILL);
            drawWireframeOverlay(model, view, projection);
        }
    } else if (hasMainModel) {
        drawWireframe(model, view, projection);
    }
    if (hasOverlay) {
        const GLboolean blendWasEnabled = glIsEnabled(GL_BLEND);
        const GLboolean cullWasEnabled = glIsEnabled(GL_CULL_FACE);
        GLboolean depthMask = GL_TRUE;
        glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMask);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glDepthMask(GL_FALSE);
        overlayProgram_.bind();
        overlayVao_.bind();
        overlayEbo_.bind();
        overlayProgram_.setUniformValue("model", model);
        overlayProgram_.setUniformValue("view", view);
        overlayProgram_.setUniformValue("projection", projection);
        overlayProgram_.setUniformValue(
            "overlayColor", QVector4D(overlayColor_, overlayOpacity_));
        glDrawElements(GL_TRIANGLES, GLsizei(overlayModelData_.faces.size()),
                       GL_UNSIGNED_INT, nullptr);
        overlayEbo_.release();
        overlayVao_.release();
        overlayProgram_.release();
        glDepthMask(depthMask);
        if (!cullWasEnabled)
            glDisable(GL_CULL_FACE);
        if (!blendWasEnabled)
            glDisable(GL_BLEND);
    }
    if (showAxis)
        drawXYZAxis(view, projection);
    glPolygonMode(GL_FRONT, oldPolygonMode[0]);
    glPolygonMode(GL_BACK, oldPolygonMode[1]);
}

SteadyDissectionWorker::SteadyDissectionWorker(
    const QString& firstPath, const QString& secondPath,
    const SteadyDissectionParameters& parameters, QObject* parent)
    : QThread(parent), firstPath_(firstPath), secondPath_(secondPath),
      parameters_(parameters)
{
}

SteadyDissectionWorker::~SteadyDissectionWorker()
{
    requestStop();
    wait();
}

void SteadyDissectionWorker::setContinuous(bool continuous)
{
    QMutexLocker lock(&mutex_);
    continuous_ = continuous;
    if (continuous)
        waitCondition_.wakeAll();
}

void SteadyDissectionWorker::requestStep()
{
    QMutexLocker lock(&mutex_);
    continuous_ = false;
    ++stepBudget_;
    waitCondition_.wakeAll();
}

void SteadyDissectionWorker::requestStop()
{
    stopRequested_.store(true);
    QMutexLocker lock(&mutex_);
    waitCondition_.wakeAll();
}

bool SteadyDissectionWorker::waitAfterSnapshot()
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
        msleep(120);
    return keepRunning;
}

void SteadyDissectionWorker::run()
{
    QString error;
    const bool succeeded = Steady3DDissectionAlgorithm::run(
        firstPath_, secondPath_, parameters_,
        [this](const SteadyDissectionSnapshot& snapshot) {
            if (stopRequested_.load())
                return false;
            emit snapshotReady(snapshot);
            return snapshot.complete ? !stopRequested_.load() : waitAfterSnapshot();
        },
        &error);
    if (!succeeded && !stopRequested_.load())
        emit failed(error.isEmpty() ? "Steady 3D dissection failed." : error);
}

Steady3DDissectionWidget::Steady3DDissectionWidget(QWidget* parent)
    : QWidget(parent)
{
    qRegisterMetaType<SteadyDissectionSnapshot>("SteadyDissectionSnapshot");
    firstViewport_ = new SteadyDissectionViewport(this);
    secondViewport_ = new SteadyDissectionViewport(this);

    phaseLabel_ = new QLabel("Select two watertight triangle meshes", this);
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

    auto createFormPanel = [this](QLabel** nameLabel, SteadyDissectionViewport* viewport,
                                  const QString& placeholder) {
        QWidget* panel = new QWidget(this);
        QVBoxLayout* layout = new QVBoxLayout(panel);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
        *nameLabel = new QLabel(placeholder, panel);
        (*nameLabel)->setAlignment(Qt::AlignCenter);
        (*nameLabel)->setMinimumHeight(34);
        (*nameLabel)->setStyleSheet(
            "background: #272c32; color: #d8dde3; padding: 6px; border-bottom: 1px solid #3c434c;");
        layout->addWidget(*nameLabel);
        layout->addWidget(viewport, 1);
        return panel;
    };

    QSplitter* splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setChildrenCollapsible(false);
    splitter->setHandleWidth(5);
    splitter->addWidget(createFormPanel(&firstNameLabel_, firstViewport_, "Form A"));
    splitter->addWidget(createFormPanel(&secondNameLabel_, secondViewport_, "Form B"));
    splitter->setSizes({600, 600});

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(header);
    layout->addWidget(splitter, 1);
}

Steady3DDissectionWidget::~Steady3DDissectionWidget()
{
    reset();
}

void Steady3DDissectionWidget::loadAndStart(
    const QString& firstPath, const QString& secondPath,
    const SteadyDissectionParameters& parameters, bool continuous)
{
    reset();
    firstPath_ = firstPath;
    secondPath_ = secondPath;
    lastParameters_ = parameters;
    firstNameLabel_->setText("Form A  |  " + QFileInfo(firstPath).fileName());
    secondNameLabel_->setText("Form B  |  " + QFileInfo(secondPath).fileName());
    phaseLabel_->setText("Validating and voxelizing meshes");
    metricsLabel_->clear();
    worker_ = new SteadyDissectionWorker(firstPath, secondPath, parameters, this);
    connect(worker_, &SteadyDissectionWorker::snapshotReady,
            this, &Steady3DDissectionWidget::applySnapshot);
    connect(worker_, &SteadyDissectionWorker::failed,
            this, &Steady3DDissectionWidget::handleFailure);
    connect(worker_, &QThread::finished,
            this, &Steady3DDissectionWidget::handleFinished);
    worker_->setContinuous(continuous);
    emit runningChanged(true);
    emit statusChanged("Validating closed triangle meshes and building voxel grids...");
    worker_->start();
}

void Steady3DDissectionWidget::pause()
{
    if (worker_)
        worker_->setContinuous(false);
}

void Steady3DDissectionWidget::resume()
{
    if (worker_)
        worker_->setContinuous(true);
}

void Steady3DDissectionWidget::step()
{
    if (worker_)
        worker_->requestStep();
    else if (!firstPath_.isEmpty() && !secondPath_.isEmpty())
        loadAndStart(firstPath_, secondPath_, lastParameters_, false);
}

void Steady3DDissectionWidget::reset()
{
    if (worker_) {
        worker_->requestStop();
        worker_->wait();
        worker_->deleteLater();
        worker_ = nullptr;
    }
    firstViewport_->clearModel();
    secondViewport_->clearModel();
    firstNameLabel_->setText("Form A");
    secondNameLabel_->setText("Form B");
    phaseLabel_->setText("Select two watertight triangle meshes");
    metricsLabel_->clear();
    emit statusChanged("Ready");
    emit runningChanged(false);
}

void Steady3DDissectionWidget::setWireframeVisible(bool visible)
{
    firstViewport_->setShowWireframeOverlay(visible);
    secondViewport_->setShowWireframeOverlay(visible);
}

void Steady3DDissectionWidget::setExplosion(float amount)
{
    firstViewport_->setExplosion(amount);
    secondViewport_->setExplosion(amount);
}

void Steady3DDissectionWidget::resetViews()
{
    firstViewport_->resetView();
    secondViewport_->resetView();
}

bool Steady3DDissectionWidget::isRunning() const
{
    return worker_ && worker_->isRunning();
}

void Steady3DDissectionWidget::applySnapshot(const SteadyDissectionSnapshot& snapshot)
{
    firstViewport_->setModelData(snapshot.firstForm, snapshot.requestedPieces);
    secondViewport_->setModelData(snapshot.secondForm, snapshot.requestedPieces);
    phaseLabel_->setText(snapshot.phase);
    metricsLabel_->setText(QString("%1 / %2 pieces  |  %3^3 voxels")
        .arg(snapshot.completedPieces).arg(snapshot.requestedPieces).arg(snapshot.resolution));
    emit statusChanged(QString(
        "%1\nPieces %2 / %3 | paired %4\nInput voxels %5 / %6\nDeleted voxels %7 / %8\nGeneralized interlocking: %9")
        .arg(snapshot.phase)
        .arg(snapshot.completedPieces)
        .arg(snapshot.requestedPieces)
        .arg(snapshot.congruentPieces ? "yes" : "pending")
        .arg(snapshot.firstInputVoxels)
        .arg(snapshot.secondInputVoxels)
        .arg(snapshot.firstDeletedVoxels)
        .arg(snapshot.secondDeletedVoxels)
        .arg(snapshot.generalizedInterlocking ? "satisfied" : "pending"));
    emit snapshotChanged(snapshot);
}

void Steady3DDissectionWidget::handleFailure(const QString& message)
{
    phaseLabel_->setText("Dissection failed");
    metricsLabel_->clear();
    emit statusChanged("Error: " + message);
}

void Steady3DDissectionWidget::handleFinished()
{
    SteadyDissectionWorker* finishedWorker = qobject_cast<SteadyDissectionWorker*>(sender());
    if (finishedWorker && worker_ == finishedWorker) {
        worker_->deleteLater();
        worker_ = nullptr;
    }
    emit runningChanged(false);
}
