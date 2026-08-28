#pragma once

#include "baseglwidget.h"

#include <QMutex>
#include <QThread>
#include <QVector3D>
#include <QWaitCondition>
#include <atomic>

struct ProgressiveMeshData {
    QVector<QVector3D> vertices;
    QVector<unsigned int> faces;
};

struct ProgressiveSnapshot {
    ProgressiveMeshData source;
    ProgressiveMeshData reference;
    ProgressiveMeshData parameterized;
    QString phase;
    int iteration = 0;
    double interpolation = 0.0;
    double energy = 0.0;
    double gradientNorm = 0.0;
};
Q_DECLARE_METATYPE(ProgressiveSnapshot)

class ProgressiveMeshViewport : public BaseGLWidget {
    Q_OBJECT
public:
    explicit ProgressiveMeshViewport(QWidget* parent = nullptr);
    void setMeshData(const ProgressiveMeshData& data, bool planar);

protected:
    void initializeGL() override;

private:
    bool glReady_ = false;
};

class ProgressiveWorker : public QThread {
    Q_OBJECT
public:
    explicit ProgressiveWorker(const QString& path, QObject* parent = nullptr);
    ~ProgressiveWorker() override;

    void setContinuous(bool continuous);
    void requestStep();
    void requestStop();

signals:
    void snapshotReady(const ProgressiveSnapshot& snapshot);
    void failed(const QString& message);

protected:
    void run() override;

private:
    bool waitAfterSnapshot();
    QString path_;
    QMutex mutex_;
    QWaitCondition waitCondition_;
    bool continuous_ = false;
    int stepBudget_ = 0;
    std::atomic_bool stopRequested_{false};
};

class ProgressiveParameterizationWidget : public QWidget {
    Q_OBJECT
public:
    explicit ProgressiveParameterizationWidget(QWidget* parent = nullptr);
    ~ProgressiveParameterizationWidget() override;

    void loadAndStart(const QString& path, bool continuous);
    void pause();
    void resume();
    void step();
    void reset();
    void setRenderMode(BaseGLWidget::RenderMode mode);
    void setWireframeVisible(bool visible);
    void setFacesVisible(bool visible);
    void resetViews();
    void centerViews();
    bool saveParameterized(const QString& path) const;
    bool isRunning() const;
    QString inputPath() const { return inputPath_; }

signals:
    void statusChanged(const QString& text);
    void runningChanged(bool running);

private slots:
    void applySnapshot(const ProgressiveSnapshot& snapshot);
    void handleFailure(const QString& message);
    void handleFinished();

private:
    ProgressiveMeshViewport* sourceView_;
    ProgressiveMeshViewport* referenceView_;
    ProgressiveMeshViewport* parameterizedView_;
    ProgressiveWorker* worker_ = nullptr;
    QString inputPath_;
    ProgressiveSnapshot lastSnapshot_;
};
