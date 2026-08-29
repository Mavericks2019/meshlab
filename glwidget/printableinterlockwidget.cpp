#include "printableinterlockwidget.h"

#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QSplitter>
#include <QVBoxLayout>

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
    originalViewport_ = new SteadyDissectionViewport(this);
    partitionViewport_ = new SteadyDissectionViewport(this);
    originalViewport_->setShowWireframeOverlay(false);
    partitionViewport_->setShowWireframeOverlay(false);

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

    auto createPanel = [this](QLabel** nameLabel, SteadyDissectionViewport* viewport,
                              const QString& title) {
        QWidget* panel = new QWidget(this);
        QVBoxLayout* layout = new QVBoxLayout(panel);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
        *nameLabel = new QLabel(title, panel);
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
    splitter->addWidget(createPanel(&originalNameLabel_, originalViewport_, "Original model"));
    splitter->addWidget(createPanel(&partitionNameLabel_, partitionViewport_,
                                    "Interlocking parts"));
    splitter->setSizes({600, 600});

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(header);
    layout->addWidget(splitter, 1);
}

PrintableInterlockWidget::~PrintableInterlockWidget()
{
    reset();
}

void PrintableInterlockWidget::loadAndStart(
    const QString& meshPath, const PrintableInterlockParameters& parameters,
    bool continuous)
{
    reset();
    meshPath_ = meshPath;
    lastParameters_ = parameters;
    const QString name = QFileInfo(meshPath).fileName();
    originalNameLabel_->setText("Original model  |  " + name);
    partitionNameLabel_->setText("Interlocking parts  |  " + name);
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
    if (worker_) {
        worker_->requestStop();
        worker_->wait();
        worker_->deleteLater();
        worker_ = nullptr;
    }
    originalViewport_->clearModel();
    partitionViewport_->clearModel();
    originalNameLabel_->setText("Original model");
    partitionNameLabel_->setText("Interlocking parts");
    phaseLabel_->setText("Select one watertight triangle mesh");
    metricsLabel_->clear();
    emit statusChanged("Ready");
    emit runningChanged(false);
}

void PrintableInterlockWidget::setWireframeVisible(bool visible)
{
    originalViewport_->setShowWireframeOverlay(visible);
    partitionViewport_->setShowWireframeOverlay(visible);
}

void PrintableInterlockWidget::setExplosion(float amount)
{
    partitionViewport_->setExplosion(amount);
}

void PrintableInterlockWidget::resetViews()
{
    originalViewport_->resetView();
    partitionViewport_->resetView();
}

bool PrintableInterlockWidget::isRunning() const
{
    return worker_ && worker_->isRunning();
}

void PrintableInterlockWidget::applySnapshot(const PrintableInterlockSnapshot& snapshot)
{
    originalViewport_->setModelData(snapshot.originalModel, snapshot.requestedPieces);
    partitionViewport_->setModelData(snapshot.partitionedModel, snapshot.requestedPieces);
    phaseLabel_->setText(snapshot.phase);
    metricsLabel_->setText(QString("%1 / %2 pieces  |  %3^3 voxels")
        .arg(snapshot.completedPieces).arg(snapshot.requestedPieces).arg(snapshot.resolution));
    emit statusChanged(QString(
        "%1\nPieces %2 / %3\nOccupied voxels %4 | internal %5 | boundary %6\n"
        "Boundary attached %7 / %6\nTiny/disconnected local cells %8 / %9\n"
        "Interlocking sequence: %10")
        .arg(snapshot.phase)
        .arg(snapshot.completedPieces)
        .arg(snapshot.requestedPieces)
        .arg(snapshot.occupiedVoxels)
        .arg(snapshot.internalVoxels)
        .arg(snapshot.boundaryVoxels)
        .arg(snapshot.attachedBoundaryVoxels)
        .arg(snapshot.tinyVoxels)
        .arg(snapshot.disconnectedVoxels)
        .arg(snapshot.interlocking ? "satisfied" : "pending"));
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
