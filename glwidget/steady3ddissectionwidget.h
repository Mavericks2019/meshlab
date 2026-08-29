#pragma once

#include "baseglwidget.h"
#include "dissection/steady3ddissectionalgorithm.h"

#include <QMutex>
#include <QThread>
#include <QWaitCondition>

#include <atomic>

class QLabel;

class SteadyDissectionViewport : public BaseGLWidget {
    Q_OBJECT
public:
    explicit SteadyDissectionViewport(QWidget* parent = nullptr);
    ~SteadyDissectionViewport() override;

    void setModelData(const SteadyDissectionMeshData& data, int pieceCount);
    void clearModel();
    void setExplosion(float amount);

protected:
    void initializeGL() override;
    void paintGL() override;
    void updateBuffersFromOpenMesh() override;

private:
    QVector3D pieceColor(int piece) const;
    void rebuildMesh();

    bool glReady_ = false;
    int pieceCount_ = 0;
    float explosion_ = 0.08f;
    SteadyDissectionMeshData modelData_;
    QVector<QVector3D> faceColors_;
    QOpenGLShaderProgram pieceProgram_;
    QOpenGLBuffer colorVbo_;
};

class SteadyDissectionWorker : public QThread {
    Q_OBJECT
public:
    SteadyDissectionWorker(const QString& firstPath, const QString& secondPath,
                           const SteadyDissectionParameters& parameters,
                           QObject* parent = nullptr);
    ~SteadyDissectionWorker() override;

    void setContinuous(bool continuous);
    void requestStep();
    void requestStop();

signals:
    void snapshotReady(const SteadyDissectionSnapshot& snapshot);
    void failed(const QString& message);

protected:
    void run() override;

private:
    bool waitAfterSnapshot();

    QString firstPath_;
    QString secondPath_;
    SteadyDissectionParameters parameters_;
    QMutex mutex_;
    QWaitCondition waitCondition_;
    bool continuous_ = false;
    int stepBudget_ = 0;
    std::atomic_bool stopRequested_{false};
};

class Steady3DDissectionWidget : public QWidget {
    Q_OBJECT
public:
    explicit Steady3DDissectionWidget(QWidget* parent = nullptr);
    ~Steady3DDissectionWidget() override;

    void loadAndStart(const QString& firstPath, const QString& secondPath,
                      const SteadyDissectionParameters& parameters,
                      bool continuous);
    void pause();
    void resume();
    void step();
    void reset();
    void setWireframeVisible(bool visible);
    void setExplosion(float amount);
    void resetViews();
    bool isRunning() const;
    QString firstPath() const { return firstPath_; }
    QString secondPath() const { return secondPath_; }

signals:
    void statusChanged(const QString& text);
    void snapshotChanged(const SteadyDissectionSnapshot& snapshot);
    void runningChanged(bool running);

private slots:
    void applySnapshot(const SteadyDissectionSnapshot& snapshot);
    void handleFailure(const QString& message);
    void handleFinished();

private:
    SteadyDissectionViewport* firstViewport_ = nullptr;
    SteadyDissectionViewport* secondViewport_ = nullptr;
    QLabel* firstNameLabel_ = nullptr;
    QLabel* secondNameLabel_ = nullptr;
    QLabel* phaseLabel_ = nullptr;
    QLabel* metricsLabel_ = nullptr;
    SteadyDissectionWorker* worker_ = nullptr;
    QString firstPath_;
    QString secondPath_;
    SteadyDissectionParameters lastParameters_;
};
