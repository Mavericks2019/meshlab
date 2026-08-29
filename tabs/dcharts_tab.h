#pragma once

#include "../glwidget/dchartswidget.h"

#include <QCheckBox>
#include <QCoreApplication>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QStyle>
#include <QVBoxLayout>

inline QWidget* createDChartsTab(DChartsWidget* widget)
{
    QWidget* tab = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout(tab);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(widget);
    return tab;
}

inline QString dChartsSamplePath()
{
    const QString sourcePath = QDir::current().filePath("models/cow.obj");
    if (QFileInfo::exists(sourcePath))
        return sourcePath;
    return QDir(QCoreApplication::applicationDirPath()).filePath("models/cow.obj");
}

inline QWidget* createDChartsControlPanel(DChartsWidget* widget, QWidget* mainWindow)
{
    QWidget* panel = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout(panel);
    layout->setAlignment(Qt::AlignTop);

    QLabel* fileLabel = new QLabel("No mesh loaded", panel);
    fileLabel->setWordWrap(true);
    QLabel* statusLabel = new QLabel("Idle", panel);
    statusLabel->setWordWrap(true);
    statusLabel->setMinimumHeight(116);
    statusLabel->setStyleSheet(
        "background: #2b3037; color: #f2f4f6; padding: 9px; border: 1px solid #424953;");

    QGroupBox* inputGroup = new QGroupBox("Input Mesh", panel);
    QVBoxLayout* inputLayout = new QVBoxLayout(inputGroup);
    QHBoxLayout* inputButtons = new QHBoxLayout;
    QPushButton* loadButton = new QPushButton("Load", inputGroup);
    loadButton->setIcon(panel->style()->standardIcon(QStyle::SP_DialogOpenButton));
    QPushButton* sampleButton = new QPushButton("Sample", inputGroup);
    sampleButton->setIcon(panel->style()->standardIcon(QStyle::SP_FileIcon));
    inputButtons->addWidget(loadButton);
    inputButtons->addWidget(sampleButton);
    inputLayout->addLayout(inputButtons);
    inputLayout->addWidget(fileLabel);

    QGroupBox* parametersGroup = new QGroupBox("D-Charts Parameters", panel);
    QFormLayout* parametersLayout = new QFormLayout(parametersGroup);
    QSpinBox* chartCount = new QSpinBox(parametersGroup);
    chartCount->setRange(1, 128);
    chartCount->setValue(8);
    QDoubleSpinBox* fittingThreshold = new QDoubleSpinBox(parametersGroup);
    fittingThreshold->setDecimals(4);
    fittingThreshold->setRange(0.0001, 4.0);
    fittingThreshold->setSingleStep(0.02);
    fittingThreshold->setValue(0.2);
    QDoubleSpinBox* alpha = new QDoubleSpinBox(parametersGroup);
    QDoubleSpinBox* beta = new QDoubleSpinBox(parametersGroup);
    QDoubleSpinBox* gamma = new QDoubleSpinBox(parametersGroup);
    for (QDoubleSpinBox* spin : {alpha, beta, gamma}) {
        spin->setDecimals(2);
        spin->setRange(0.0, 4.0);
        spin->setSingleStep(0.1);
    }
    alpha->setValue(1.0);
    beta->setValue(0.7);
    gamma->setValue(0.5);
    QSpinBox* maxIterations = new QSpinBox(parametersGroup);
    maxIterations->setRange(1, 200);
    maxIterations->setValue(100);
    QDoubleSpinBox* mergeThreshold = new QDoubleSpinBox(parametersGroup);
    mergeThreshold->setDecimals(6);
    mergeThreshold->setRange(0.0, 0.5);
    mergeThreshold->setSingleStep(0.001);
    mergeThreshold->setValue(0.01);
    QCheckBox* partialCuts = new QCheckBox("Add darts toward high-error regions", parametersGroup);
    partialCuts->setChecked(true);
    parametersLayout->addRow("Initial charts", chartCount);
    parametersLayout->addRow("Fmax", fittingThreshold);
    parametersLayout->addRow("Alpha", alpha);
    parametersLayout->addRow("Beta", beta);
    parametersLayout->addRow("Gamma", gamma);
    parametersLayout->addRow("Max iterations", maxIterations);
    parametersLayout->addRow("Merge eta", mergeThreshold);
    parametersLayout->addRow(partialCuts);

    QGroupBox* iterationGroup = new QGroupBox("Iteration", panel);
    QHBoxLayout* iterationLayout = new QHBoxLayout(iterationGroup);
    QPushButton* runButton = new QPushButton(iterationGroup);
    runButton->setIcon(panel->style()->standardIcon(QStyle::SP_MediaPlay));
    runButton->setToolTip("Run or resume");
    QPushButton* pauseButton = new QPushButton(iterationGroup);
    pauseButton->setIcon(panel->style()->standardIcon(QStyle::SP_MediaPause));
    pauseButton->setToolTip("Pause after the current stage");
    QPushButton* stepButton = new QPushButton(iterationGroup);
    stepButton->setIcon(panel->style()->standardIcon(QStyle::SP_MediaSkipForward));
    stepButton->setToolTip("Advance one iteration or stage");
    QPushButton* resetButton = new QPushButton(iterationGroup);
    resetButton->setIcon(panel->style()->standardIcon(QStyle::SP_BrowserReload));
    resetButton->setToolTip("Stop and reset the run");
    for (QPushButton* button : {runButton, pauseButton, stepButton, resetButton}) {
        button->setFixedHeight(34);
        iterationLayout->addWidget(button);
    }

    QGroupBox* displayGroup = new QGroupBox("Display", panel);
    QVBoxLayout* displayLayout = new QVBoxLayout(displayGroup);
    QCheckBox* seams = new QCheckBox("Show chart seams", displayGroup);
    QCheckBox* seeds = new QCheckBox("Show seed faces", displayGroup);
    QCheckBox* wireframe = new QCheckBox("Show triangle wireframe", displayGroup);
    seams->setChecked(true);
    seeds->setChecked(true);
    wireframe->setChecked(false);
    QHBoxLayout* viewButtons = new QHBoxLayout;
    QPushButton* resetViewButton = new QPushButton("Reset", displayGroup);
    QPushButton* centerViewButton = new QPushButton("Center", displayGroup);
    viewButtons->addWidget(resetViewButton);
    viewButtons->addWidget(centerViewButton);
    displayLayout->addWidget(seams);
    displayLayout->addWidget(seeds);
    displayLayout->addWidget(wireframe);
    displayLayout->addLayout(viewButtons);

    auto parameters = [=]() {
        DChartsParameters result;
        result.initialChartCount = chartCount->value();
        result.maxIterations = maxIterations->value();
        result.fittingThreshold = fittingThreshold->value();
        result.alpha = alpha->value();
        result.beta = beta->value();
        result.gamma = gamma->value();
        result.mergeThreshold = mergeThreshold->value();
        result.addPartialCuts = partialCuts->isChecked();
        return result;
    };
    auto startPath = [=](const QString& path, bool continuous) {
        if (path.isEmpty())
            return;
        fileLabel->setText(QFileInfo(path).fileName());
        widget->loadAndStart(path, parameters(), continuous);
    };

    QObject::connect(loadButton, &QPushButton::clicked, panel, [=]() {
        startPath(QFileDialog::getOpenFileName(
            mainWindow, "Open mesh for D-Charts", QString(),
            "Mesh files (*.obj *.off *.ply)"), false);
    });
    QObject::connect(sampleButton, &QPushButton::clicked, panel, [=]() {
        const QString path = dChartsSamplePath();
        if (!QFileInfo::exists(path))
            QMessageBox::warning(mainWindow, "D-Charts sample", "The bundled cow mesh was not found.");
        else
            startPath(path, false);
    });
    QObject::connect(runButton, &QPushButton::clicked, panel, [=]() {
        if (widget->isRunning()) {
            widget->resume();
        } else if (!widget->inputPath().isEmpty()) {
            startPath(widget->inputPath(), true);
        } else {
            const QString sample = dChartsSamplePath();
            if (QFileInfo::exists(sample))
                startPath(sample, true);
        }
    });
    QObject::connect(pauseButton, &QPushButton::clicked, widget, &DChartsWidget::pause);
    QObject::connect(stepButton, &QPushButton::clicked, panel, [=]() {
        if (widget->isRunning())
            widget->step();
        else if (!widget->inputPath().isEmpty())
            startPath(widget->inputPath(), false);
        else if (QFileInfo::exists(dChartsSamplePath()))
            startPath(dChartsSamplePath(), false);
    });
    QObject::connect(resetButton, &QPushButton::clicked, widget, &DChartsWidget::reset);
    QObject::connect(seams, &QCheckBox::toggled, widget, &DChartsWidget::setSeamsVisible);
    QObject::connect(seeds, &QCheckBox::toggled, widget, &DChartsWidget::setSeedsVisible);
    QObject::connect(wireframe, &QCheckBox::toggled, widget, &DChartsWidget::setWireframeVisible);
    QObject::connect(resetViewButton, &QPushButton::clicked, widget, &DChartsWidget::resetView);
    QObject::connect(centerViewButton, &QPushButton::clicked, widget, &DChartsWidget::centerView);
    QObject::connect(widget, &DChartsWidget::statusChanged, statusLabel, &QLabel::setText);

    layout->addWidget(inputGroup);
    layout->addWidget(parametersGroup);
    layout->addWidget(iterationGroup);
    layout->addWidget(displayGroup);
    layout->addWidget(statusLabel);
    layout->addStretch();
    return panel;
}
