#pragma once

#include "progressiveparameterizationwidget.h"

#include <QMutex>
#include <QThread>
#include <QWaitCondition>
#include <atomic>

struct AtlasDisplaySnapshot
{
    ProgressiveMeshData source;
    ProgressiveMeshData parameterized;
    QString phase;
    int step = 0;
    double distortion = 0.0;
    bool complete = false;
};
Q_DECLARE_METATYPE(AtlasDisplaySnapshot)

class AtlasWorker : public QThread
{
    Q_OBJECT
public:
    explicit AtlasWorker(const QString& path, QObject* parent = nullptr);
    ~AtlasWorker() override;

    void setContinuous(bool continuous);
    void requestStep();
    void requestStop();

signals:
    void snapshotReady(const AtlasDisplaySnapshot& snapshot);
    void stepStarted(const QString& phase);
    void failed(const QString& message);

protected:
    void run() override;

private:
    bool waitForNextStep();

    QString path_;
    QMutex mutex_;
    QWaitCondition waitCondition_;
    bool continuous_ = false;
    int stepBudget_ = 0;
    std::atomic_bool stopRequested_{false};
};

class AtlasParameterizationWidget : public QWidget
{
    Q_OBJECT
public:
    explicit AtlasParameterizationWidget(QWidget* parent = nullptr);
    ~AtlasParameterizationWidget() override;

    void loadAndStart(const QString& path, bool continuous);
    void runAll();
    void pause();
    void step();
    void restart();
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
    void resultAvailable(bool available);

private slots:
    void applySnapshot(const AtlasDisplaySnapshot& snapshot);
    void handleStepStarted(const QString& phase);
    void handleFailure(const QString& message);
    void handleFinished();

private:
    ProgressiveMeshViewport* sourceView_;
    ProgressiveMeshViewport* parameterizedView_;
    AtlasWorker* worker_ = nullptr;
    QString inputPath_;
    AtlasDisplaySnapshot lastSnapshot_;
};
