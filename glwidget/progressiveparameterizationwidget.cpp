#include "progressiveparameterizationwidget.h"

#include "progressive/reference/Parafun.h"

#include <QFile>
#include <QHBoxLayout>
#include <QLabel>
#include <QTextStream>
#include <QVBoxLayout>
#include <OpenMesh/Core/IO/MeshIO.hh>
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace {
ProgressiveMeshData connectedMesh(const Parafun& solver, bool planar)
{
    ProgressiveMeshData data;
    data.vertices.reserve(solver.V_N);
    for (int i = 0; i < solver.V_N; ++i) {
        if (planar) {
            data.vertices.push_back(QVector3D(
                float(solver.originmesh_area_sqrt * solver.position_of_mesh(i)),
                float(solver.originmesh_area_sqrt * solver.position_of_mesh(i + solver.V_N)), 0.0f));
        } else {
            const auto p = solver.mesh.point(solver.mesh.vertex_handle(i));
            data.vertices.push_back(QVector3D(float(p[0]), float(p[1]), float(p[2])));
        }
    }
    data.faces.reserve(solver.F_N * 3);
    for (int i = 0; i < solver.F_N; ++i) {
        data.faces << unsigned(solver.F0[i]) << unsigned(solver.F1[i]) << unsigned(solver.F2[i]);
    }
    return data;
}

ProgressiveMeshData referenceMesh(const Parafun& solver)
{
    ProgressiveMeshData data;
    data.vertices.reserve(solver.F_N * 3);
    data.faces.reserve(solver.F_N * 3);
    const double* uv = solver.position_of_mesh.data();
    for (int i = 0; i < solver.F_N; ++i) {
        const double p00 = solver.update_p00[i], p01 = solver.update_p01[i];
        const double p10 = solver.update_p10[i], p11 = solver.update_p11[i];
        const double det = p00 * p11 - p01 * p10;
        if (!std::isfinite(det) || std::abs(det) < 1e-14)
            throw std::runtime_error("Degenerate progressive reference triangle.");
        double rx[3] = {0.0, p11 / det, -p01 / det};
        double ry[3] = {0.0, -p10 / det, p00 / det};
        double qx[3] = {uv[solver.F0[i]], uv[solver.F1[i]], uv[solver.F2[i]]};
        double qy[3] = {uv[solver.F0[i] + solver.V_N], uv[solver.F1[i] + solver.V_N], uv[solver.F2[i] + solver.V_N]};
        const double rcx = (rx[0] + rx[1] + rx[2]) / 3.0;
        const double rcy = (ry[0] + ry[1] + ry[2]) / 3.0;
        const double qcx = (qx[0] + qx[1] + qx[2]) / 3.0;
        const double qcy = (qy[0] + qy[1] + qy[2]) / 3.0;
        double dot = 0.0, cross = 0.0;
        for (int j = 0; j < 3; ++j) {
            const double x = rx[j] - rcx, y = ry[j] - rcy;
            dot += x * (qx[j] - qcx) + y * (qy[j] - qcy);
            cross += x * (qy[j] - qcy) - y * (qx[j] - qcx);
        }
        const double norm = std::hypot(dot, cross);
        const double c = norm > 0.0 ? dot / norm : 1.0;
        const double s = norm > 0.0 ? cross / norm : 0.0;
        const unsigned base = unsigned(data.vertices.size());
        for (int j = 0; j < 3; ++j) {
            const double x = rx[j] - rcx, y = ry[j] - rcy;
            data.vertices.push_back(QVector3D(
                float(solver.originmesh_area_sqrt * (qcx + c * x - s * y)),
                float(solver.originmesh_area_sqrt * (qcy + s * x + c * y)), 0.0f));
        }
        data.faces << base << base + 1 << base + 2;
    }
    return data;
}

ProgressiveSnapshot makeSnapshot(Parafun& solver, const char* phase, int iteration)
{
    ProgressiveSnapshot snapshot;
    snapshot.source = connectedMesh(solver, false);
    snapshot.reference = referenceMesh(solver);
    snapshot.parameterized = connectedMesh(solver, true);
    snapshot.phase = QString::fromLatin1(phase);
    snapshot.iteration = iteration;
    snapshot.interpolation = solver.Intp_T_Min;
    snapshot.energy = solver.energy_area;
    solver.calc_gradient_norm(solver.position_of_mesh);
    snapshot.gradientNorm = solver.g_norm;
    return snapshot;
}
}

ProgressiveMeshViewport::ProgressiveMeshViewport(QWidget* parent) : BaseGLWidget(parent)
{
    setShowWireframeOverlay(true);
    setSurfaceColor(QVector3D(0.70f, 0.76f, 0.82f));
    setMinimumSize(220, 220);
}

void ProgressiveMeshViewport::initializeGL()
{
    BaseGLWidget::initializeGL();
    glReady_ = true;
}

void ProgressiveMeshViewport::setMeshData(const ProgressiveMeshData& data, bool planar)
{
    clearMeshData();
    QVector<Mesh::VertexHandle> handles;
    handles.reserve(data.vertices.size());
    for (const QVector3D& p : data.vertices)
        handles.push_back(openMesh.add_vertex(Mesh::Point(p.x(), p.y(), p.z())));
    for (int i = 0; i + 2 < data.faces.size(); i += 3) {
        std::vector<Mesh::VertexHandle> face = {handles[int(data.faces[i])], handles[int(data.faces[i + 1])], handles[int(data.faces[i + 2])]};
        openMesh.add_face(face);
    }
    if (openMesh.n_vertices() == 0) {
        update();
        return;
    }
    Mesh::Point min, max;
    computeBoundingBox(min, max);
    const Mesh::Point size = max - min;
    const double maxSize = (std::max)({double(size[0]), double(size[1]), double(size[2]), 1e-6});
    centerAndScaleMesh((min + max) * 0.5, float(maxSize));
    openMesh.request_face_normals();
    openMesh.request_vertex_normals();
    openMesh.update_normals();
    prepareFaceIndices();
    prepareEdgeIndices();
    modelLoaded = true;
    rotation = planar ? QQuaternion() : QQuaternion::fromEulerAngles(-18.0f, 28.0f, 0.0f);
    zoom = 1.0f;
    modelCenter = QVector3D(0.0f, 0.0f, 0.0f);
    viewDistance = 4.0f;
    viewScale = 1.0f;
    initialRotation = rotation;
    initialZoom = zoom;
    initialModelCenter = modelCenter;
    initialViewDistance = viewDistance;
    initialViewScale = viewScale;

    // Snapshots arrive after initializeGL in the normal UI flow, so upload the
    // newly rebuilt OpenMesh explicitly instead of leaving stale/empty buffers.
    if (glReady_) {
        makeCurrent();
        updateBuffersFromOpenMesh();
        doneCurrent();
    }
    update();
}

ProgressiveWorker::ProgressiveWorker(const QString& path, QObject* parent) : QThread(parent), path_(path) {}
ProgressiveWorker::~ProgressiveWorker() { requestStop(); wait(); }

void ProgressiveWorker::setContinuous(bool continuous)
{
    QMutexLocker lock(&mutex_);
    continuous_ = continuous;
    if (continuous) waitCondition_.wakeAll();
}

void ProgressiveWorker::requestStep()
{
    QMutexLocker lock(&mutex_);
    continuous_ = false;
    ++stepBudget_;
    waitCondition_.wakeAll();
}

void ProgressiveWorker::requestStop()
{
    stopRequested_.store(true);
    QMutexLocker lock(&mutex_);
    waitCondition_.wakeAll();
}

bool ProgressiveWorker::waitAfterSnapshot()
{
    QMutexLocker lock(&mutex_);
    while (!stopRequested_.load() && !continuous_ && stepBudget_ == 0)
        waitCondition_.wait(&mutex_);
    if (stepBudget_ > 0) --stepBudget_;
    return !stopRequested_.load();
}

void ProgressiveWorker::run()
{
    try {
        Parafun solver(path_.toStdString());
        for (auto face : solver.mesh.faces()) {
            int count = 0;
            for (auto ignored : solver.mesh.fv_range(face)) { Q_UNUSED(ignored); ++count; }
            if (count != 3) throw std::runtime_error("Progressive parameterization requires a triangular mesh.");
        }
        bool hasBoundary = false;
        for (auto halfedge : solver.mesh.halfedges()) hasBoundary = hasBoundary || solver.mesh.is_boundary(halfedge);
        if (!hasBoundary) throw std::runtime_error("Progressive parameterization requires an open mesh with a boundary.");
        solver.set_stop_callback([this]() { return stopRequested_.load(); });
        solver.set_iteration_callback([this](Parafun& state, const char* phase, int iteration) {
            emit snapshotReady(makeSnapshot(state, phase, iteration));
            waitAfterSnapshot();
        });
        solver.run_bpe();
    } catch (const std::exception& error) {
        if (!stopRequested_.load()) emit failed(QString::fromLocal8Bit(error.what()));
    }
}

ProgressiveParameterizationWidget::ProgressiveParameterizationWidget(QWidget* parent) : QWidget(parent)
{
    qRegisterMetaType<ProgressiveSnapshot>("ProgressiveSnapshot");
    sourceView_ = new ProgressiveMeshViewport(this);
    referenceView_ = new ProgressiveMeshViewport(this);
    parameterizedView_ = new ProgressiveMeshViewport(this);
    auto makePane = [this](const QString& title, ProgressiveMeshViewport* view) {
        QWidget* pane = new QWidget(this);
        QVBoxLayout* layout = new QVBoxLayout(pane);
        layout->setContentsMargins(3, 3, 3, 3);
        QLabel* label = new QLabel(title, pane);
        label->setAlignment(Qt::AlignCenter);
        label->setStyleSheet("font-weight: 600; padding: 6px; background: #30343a; color: white;");
        layout->addWidget(label);
        layout->addWidget(view, 1);
        return pane;
    };
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(4);
    layout->addWidget(makePane("M - Original 3D", sourceView_), 1);
    layout->addWidget(makePane("Mr - Progressive Reference", referenceView_), 1);
    layout->addWidget(makePane("Mp - Current Parameterization", parameterizedView_), 1);
}

ProgressiveParameterizationWidget::~ProgressiveParameterizationWidget() { reset(); }

void ProgressiveParameterizationWidget::loadAndStart(const QString& path, bool continuous)
{
    reset();
    inputPath_ = path;
    sourceView_->loadOBJ(path);
    referenceView_->setMeshData(ProgressiveMeshData(), true);
    parameterizedView_->setMeshData(ProgressiveMeshData(), true);
    worker_ = new ProgressiveWorker(path, this);
    connect(worker_, &ProgressiveWorker::snapshotReady, this, &ProgressiveParameterizationWidget::applySnapshot);
    connect(worker_, &ProgressiveWorker::failed, this, &ProgressiveParameterizationWidget::handleFailure);
    connect(worker_, &QThread::finished, this, &ProgressiveParameterizationWidget::handleFinished);
    worker_->setContinuous(continuous);
    emit runningChanged(true);
    emit statusChanged("Initializing Tutte parameterization...");
    worker_->start();
}

void ProgressiveParameterizationWidget::pause() { if (worker_) worker_->setContinuous(false); }
void ProgressiveParameterizationWidget::resume() { if (worker_) worker_->setContinuous(true); }
void ProgressiveParameterizationWidget::step() { if (worker_) worker_->requestStep(); else if (!inputPath_.isEmpty()) loadAndStart(inputPath_, false); }
void ProgressiveParameterizationWidget::reset()
{
    if (!worker_) return;
    worker_->requestStop();
    worker_->wait();
    worker_->deleteLater();
    worker_ = nullptr;
    emit runningChanged(false);
}

void ProgressiveParameterizationWidget::setRenderMode(BaseGLWidget::RenderMode mode)
{
    for (ProgressiveMeshViewport* view : {sourceView_, referenceView_, parameterizedView_}) {
        view->currentRenderMode = mode;
        view->update();
    }
}

void ProgressiveParameterizationWidget::setWireframeVisible(bool visible)
{
    for (ProgressiveMeshViewport* view : {sourceView_, referenceView_, parameterizedView_})
        view->setShowWireframeOverlay(visible);
}

void ProgressiveParameterizationWidget::setFacesVisible(bool visible)
{
    for (ProgressiveMeshViewport* view : {sourceView_, referenceView_, parameterizedView_})
        view->setHideFaces(!visible);
}

void ProgressiveParameterizationWidget::resetViews()
{
    for (ProgressiveMeshViewport* view : {sourceView_, referenceView_, parameterizedView_})
        view->resetView();
}

void ProgressiveParameterizationWidget::centerViews()
{
    for (ProgressiveMeshViewport* view : {sourceView_, referenceView_, parameterizedView_})
        view->centerView();
}

bool ProgressiveParameterizationWidget::isRunning() const { return worker_ && worker_->isRunning(); }

void ProgressiveParameterizationWidget::applySnapshot(const ProgressiveSnapshot& snapshot)
{
    lastSnapshot_ = snapshot;
    sourceView_->setMeshData(snapshot.source, false);
    referenceView_->setMeshData(snapshot.reference, true);
    parameterizedView_->setMeshData(snapshot.parameterized, true);
    emit statusChanged(QString("%1 | iter %2 | t %3 | E %4 | |g| %5")
        .arg(snapshot.phase).arg(snapshot.iteration).arg(snapshot.interpolation, 0, 'f', 5)
        .arg(snapshot.energy, 0, 'g', 8).arg(snapshot.gradientNorm, 0, 'g', 5));
}

void ProgressiveParameterizationWidget::handleFailure(const QString& message) { emit statusChanged("Error: " + message); }
void ProgressiveParameterizationWidget::handleFinished() { emit runningChanged(false); }

bool ProgressiveParameterizationWidget::saveParameterized(const QString& path) const
{
    if (lastSnapshot_.parameterized.vertices.isEmpty()) return false;
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return false;
    QTextStream out(&file);
    for (const QVector3D& p : lastSnapshot_.parameterized.vertices)
        out << "v " << p.x() << ' ' << p.y() << ' ' << p.z() << '\n';
    for (int i = 0; i + 2 < lastSnapshot_.parameterized.faces.size(); i += 3)
        out << "f " << lastSnapshot_.parameterized.faces[i] + 1 << ' ' << lastSnapshot_.parameterized.faces[i + 1] + 1 << ' ' << lastSnapshot_.parameterized.faces[i + 2] + 1 << '\n';
    return true;
}
