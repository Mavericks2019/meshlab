#pragma once

#include "baseglwidget.h"
#include "dcharts/dchartsalgorithm.h"

#include <QMutex>
#include <QThread>
#include <QWaitCondition>

#include <atomic>

class QLabel;

class DChartsViewport : public BaseGLWidget {
    Q_OBJECT
public:
    explicit DChartsViewport(QWidget* parent = nullptr);
    ~DChartsViewport() override;

    void setSnapshot(const DChartsSnapshot& snapshot);
    void clearSnapshot();
    void setSeamsVisible(bool visible);
    void setSeedsVisible(bool visible);

protected:
    void initializeGL() override;
    void paintGL() override;
    void updateBuffersFromOpenMesh() override;

private:
    QVector3D chartColor(int chart) const;
    void drawOverlay(const QMatrix4x4& model, const QMatrix4x4& view,
                     const QMatrix4x4& projection, QOpenGLBuffer& buffer,
                     int vertexCount, GLenum primitive, const QVector4D& color,
                     float widthOrSize);

    bool glReady_ = false;
    bool seamsVisible_ = true;
    bool seedsVisible_ = true;
    QVector<QVector3D> faceColors_;
    QVector<QVector3D> seamPositions_;
    QVector<QVector3D> seedPositions_;
    QOpenGLShaderProgram chartProgram_;
    QOpenGLBuffer colorVbo_;
    QOpenGLBuffer seamVbo_;
    QOpenGLBuffer seedVbo_;
    QOpenGLVertexArrayObject overlayVao_;
};

class DChartsWorker : public QThread {
    Q_OBJECT
public:
    DChartsWorker(const QString& path, const DChartsParameters& parameters,
                  QObject* parent = nullptr);
    ~DChartsWorker() override;

    void setContinuous(bool continuous);
    void requestStep();
    void requestStop();

signals:
    void snapshotReady(const DChartsSnapshot& snapshot);
    void failed(const QString& message);

protected:
    void run() override;

private:
    bool waitAfterSnapshot();

    QString path_;
    DChartsParameters parameters_;
    QMutex mutex_;
    QWaitCondition waitCondition_;
    bool continuous_ = false;
    int stepBudget_ = 0;
    std::atomic_bool stopRequested_{false};
};

class DChartsWidget : public QWidget {
    Q_OBJECT
public:
    explicit DChartsWidget(QWidget* parent = nullptr);
    ~DChartsWidget() override;

    void loadAndStart(const QString& path, const DChartsParameters& parameters,
                      bool continuous);
    void pause();
    void resume();
    void step();
    void reset();
    void setWireframeVisible(bool visible);
    void setSeamsVisible(bool visible);
    void setSeedsVisible(bool visible);
    void resetView();
    void centerView();
    bool isRunning() const;
    QString inputPath() const { return inputPath_; }

signals:
    void statusChanged(const QString& text);
    void snapshotChanged(const DChartsSnapshot& snapshot);
    void runningChanged(bool running);

private slots:
    void applySnapshot(const DChartsSnapshot& snapshot);
    void handleFailure(const QString& message);
    void handleFinished();

private:
    DChartsViewport* viewport_ = nullptr;
    QLabel* phaseLabel_ = nullptr;
    QLabel* metricsLabel_ = nullptr;
    DChartsWorker* worker_ = nullptr;
    QString inputPath_;
    DChartsParameters lastParameters_;
};
