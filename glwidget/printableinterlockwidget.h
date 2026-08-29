#pragma once

#include "glwidget/steady3ddissectionwidget.h"
#include "interlocking/printableinterlockalgorithm.h"

#include <QMutex>
#include <QThread>
#include <QWaitCondition>

#include <atomic>

class QLabel;

class PrintableInterlockWorker : public QThread {
    Q_OBJECT
public:
    PrintableInterlockWorker(const QString& meshPath,
                             const PrintableInterlockParameters& parameters,
                             QObject* parent = nullptr);
    ~PrintableInterlockWorker() override;

    void setContinuous(bool continuous);
    void requestStep();
    void requestStop();

signals:
    void snapshotReady(const PrintableInterlockSnapshot& snapshot);
    void failed(const QString& message);

protected:
    void run() override;

private:
    bool waitAfterSnapshot();

    QString meshPath_;
    PrintableInterlockParameters parameters_;
    QMutex mutex_;
    QWaitCondition waitCondition_;
    bool continuous_ = false;
    int stepBudget_ = 0;
    std::atomic_bool stopRequested_{false};
};

class PrintableInterlockWidget : public QWidget {
    Q_OBJECT
public:
    explicit PrintableInterlockWidget(QWidget* parent = nullptr);
    ~PrintableInterlockWidget() override;

    void loadAndStart(const QString& meshPath,
                      const PrintableInterlockParameters& parameters,
                      bool continuous);
    void pause();
    void resume();
    void step();
    void reset();
    void setWireframeVisible(bool visible);
    void setExplosion(float amount);
    void resetViews();
    bool isRunning() const;

signals:
    void statusChanged(const QString& text);
    void snapshotChanged(const PrintableInterlockSnapshot& snapshot);
    void runningChanged(bool running);

private slots:
    void applySnapshot(const PrintableInterlockSnapshot& snapshot);
    void handleFailure(const QString& message);
    void handleFinished();

private:
    SteadyDissectionViewport* originalViewport_ = nullptr;
    SteadyDissectionViewport* partitionViewport_ = nullptr;
    QLabel* originalNameLabel_ = nullptr;
    QLabel* partitionNameLabel_ = nullptr;
    QLabel* phaseLabel_ = nullptr;
    QLabel* metricsLabel_ = nullptr;
    PrintableInterlockWorker* worker_ = nullptr;
    QString meshPath_;
    PrintableInterlockParameters lastParameters_;
};
