#pragma once

#include "glwidget/printableinterlockwidget.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QCoreApplication>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QStyle>
#include <QTimer>
#include <QVBoxLayout>

#include <algorithm>

inline QWidget* createPrintableInterlockTab(PrintableInterlockWidget* widget)
{
    QWidget* tab = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout(tab);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(widget);
    return tab;
}

inline QString printableInterlockSamplePath(const QString& fileName)
{
    const QString appCandidate = QCoreApplication::applicationDirPath()
        + "/../../models/" + fileName;
    if (QFileInfo::exists(appCandidate))
        return QFileInfo(appCandidate).absoluteFilePath();
    return QFileInfo(QCoreApplication::applicationDirPath()
                     + "/../../../models/" + fileName).absoluteFilePath();
}

inline QWidget* createPrintableInterlockControlPanel(
    PrintableInterlockWidget* widget, QWidget* mainWindow)
{
    QWidget* panel = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout(panel);
    layout->setContentsMargins(9, 9, 9, 9);
    layout->setSpacing(9);

    QGroupBox* inputGroup = new QGroupBox("Input Solid", panel);
    QHBoxLayout* inputLayout = new QHBoxLayout(inputGroup);
    QLabel* inputLabel = new QLabel("Model", inputGroup);
    QLineEdit* meshPath = new QLineEdit(inputGroup);
    meshPath->setReadOnly(true);
    meshPath->setPlaceholderText("No mesh selected");
    QPushButton* loadButton = new QPushButton(inputGroup);
    loadButton->setIcon(panel->style()->standardIcon(QStyle::SP_DirOpenIcon));
    loadButton->setToolTip(
        "Select one connected, watertight, non-self-intersecting triangle mesh");
    loadButton->setFixedWidth(34);
    inputLayout->addWidget(inputLabel);
    inputLayout->addWidget(meshPath, 1);
    inputLayout->addWidget(loadButton);

    QPushButton* sampleButton = new QPushButton("Load sample model", panel);
    sampleButton->setIcon(panel->style()->standardIcon(QStyle::SP_FileIcon));

    QGroupBox* parametersGroup = new QGroupBox("2015 Paper Parameters", panel);
    QFormLayout* parametersLayout = new QFormLayout(parametersGroup);
    QSpinBox* resolution = new QSpinBox(parametersGroup);
    resolution->setRange(8, 100);
    resolution->setValue(50);
    resolution->setSuffix(" cells");
    QSpinBox* samples = new QSpinBox(parametersGroup);
    samples->setRange(2, 6);
    samples->setValue(4);
    samples->setToolTip("Local samples per voxel edge");
    QSpinBox* pieceCount = new QSpinBox(parametersGroup);
    pieceCount->setRange(3, 30);
    pieceCount->setValue(6);
    QSpinBox* candidateLimit = new QSpinBox(parametersGroup);
    candidateLimit->setRange(1, 40);
    candidateLimit->setValue(20);
    QSpinBox* randomSeed = new QSpinBox(parametersGroup);
    randomSeed->setRange(0, 999999);
    randomSeed->setValue(7);
    QDoubleSpinBox* internalThreshold = new QDoubleSpinBox(parametersGroup);
    internalThreshold->setDecimals(1);
    internalThreshold->setRange(10.0, 100.0);
    internalThreshold->setValue(50.0);
    internalThreshold->setSuffix(" %");
    QDoubleSpinBox* faceContact = new QDoubleSpinBox(parametersGroup);
    faceContact->setDecimals(1);
    faceContact->setRange(0.0, 25.0);
    faceContact->setValue(5.0);
    faceContact->setSuffix(" %");
    QDoubleSpinBox* maximumExtent = new QDoubleSpinBox(parametersGroup);
    maximumExtent->setDecimals(1);
    maximumExtent->setRange(50.0, 100.0);
    maximumExtent->setValue(100.0);
    maximumExtent->setSuffix(" %");
    QCheckBox* refineSeams = new QCheckBox("Refine salient cutting seams", parametersGroup);
    refineSeams->setChecked(true);
    QCheckBox* strictInterlocking = new QCheckBox(
        "Enforce interlocking sequence", parametersGroup);
    strictInterlocking->setChecked(true);
    parametersLayout->addRow("Voxel resolution", resolution);
    parametersLayout->addRow("Samples / voxel edge", samples);
    parametersLayout->addRow("Pieces", pieceCount);
    parametersLayout->addRow("Candidates", candidateLimit);
    parametersLayout->addRow("Random seed", randomSeed);
    parametersLayout->addRow("Internal volume", internalThreshold);
    parametersLayout->addRow("Face connection", faceContact);
    parametersLayout->addRow("Max part extent", maximumExtent);
    parametersLayout->addRow(refineSeams);
    parametersLayout->addRow(strictInterlocking);

    QGroupBox* iterationGroup = new QGroupBox("Construction", panel);
    QHBoxLayout* iterationLayout = new QHBoxLayout(iterationGroup);
    QPushButton* runButton = new QPushButton(iterationGroup);
    runButton->setIcon(panel->style()->standardIcon(QStyle::SP_MediaPlay));
    runButton->setToolTip("Run all remaining construction stages");
    QPushButton* pauseButton = new QPushButton(iterationGroup);
    pauseButton->setIcon(panel->style()->standardIcon(QStyle::SP_MediaPause));
    pauseButton->setToolTip("Pause after the current stage");
    QPushButton* stepButton = new QPushButton(iterationGroup);
    stepButton->setIcon(panel->style()->standardIcon(QStyle::SP_MediaSkipForward));
    stepButton->setToolTip("Advance exactly one construction stage");
    QPushButton* resetButton = new QPushButton(iterationGroup);
    resetButton->setIcon(panel->style()->standardIcon(QStyle::SP_BrowserReload));
    resetButton->setToolTip("Stop and reset");
    for (QPushButton* button : {runButton, pauseButton, stepButton, resetButton}) {
        button->setFixedHeight(34);
        iterationLayout->addWidget(button);
    }

    QGroupBox* displayGroup = new QGroupBox("Display", panel);
    QVBoxLayout* displayLayout = new QVBoxLayout(displayGroup);
    QHBoxLayout* geometryModeLayout = new QHBoxLayout;
    QPushButton* voxelModeButton = new QPushButton("Voxel solids (D)", displayGroup);
    QPushButton* surfaceModeButton = new QPushButton("Surface cut (E)", displayGroup);
    voxelModeButton->setObjectName("printableVoxelMode");
    surfaceModeButton->setObjectName("printableSurfaceMode");
    QButtonGroup* geometryMode = new QButtonGroup(displayGroup);
    geometryMode->setExclusive(true);
    for (QPushButton* button : {voxelModeButton, surfaceModeButton}) {
        button->setCheckable(true);
        button->setFixedHeight(32);
        button->setStyleSheet(
            "QPushButton:checked { background: #397a69; color: white; border: 1px solid #69a894; }");
        geometryMode->addButton(button);
        geometryModeLayout->addWidget(button);
    }
    surfaceModeButton->setChecked(true);
    QCheckBox* wireframe = new QCheckBox("Show surface feature and voxel edges", displayGroup);
    wireframe->setObjectName("printableWireframeToggle");
    wireframe->setChecked(false);
    QFormLayout* explosionLayout = new QFormLayout;
    QSlider* explosion = new QSlider(Qt::Horizontal, displayGroup);
    explosion->setObjectName("printablePartSpacing");
    explosion->setRange(0, 150);
    explosion->setValue(10);
    explosionLayout->addRow("Part spacing", explosion);
    QPushButton* resetViewsButton = new QPushButton("Reset both views", displayGroup);
    resetViewsButton->setIcon(panel->style()->standardIcon(QStyle::SP_DesktopIcon));
    displayLayout->addLayout(geometryModeLayout);
    displayLayout->addWidget(wireframe);
    displayLayout->addLayout(explosionLayout);
    displayLayout->addWidget(resetViewsButton);

    QGroupBox* assemblyGroup = new QGroupBox("Assembly Process", panel);
    QVBoxLayout* assemblyLayout = new QVBoxLayout(assemblyGroup);
    QHBoxLayout* assemblyButtons = new QHBoxLayout;
    QPushButton* previousAssemblyButton = new QPushButton(assemblyGroup);
    previousAssemblyButton->setIcon(
        panel->style()->standardIcon(QStyle::SP_MediaSkipBackward));
    previousAssemblyButton->setToolTip("Previous assembly step");
    QPushButton* playAssemblyButton = new QPushButton(assemblyGroup);
    playAssemblyButton->setIcon(panel->style()->standardIcon(QStyle::SP_MediaPlay));
    playAssemblyButton->setToolTip("Play assembly");
    QPushButton* nextAssemblyButton = new QPushButton(assemblyGroup);
    nextAssemblyButton->setIcon(
        panel->style()->standardIcon(QStyle::SP_MediaSkipForward));
    nextAssemblyButton->setToolTip("Next assembly step");
    for (QPushButton* button : {
             previousAssemblyButton, playAssemblyButton, nextAssemblyButton}) {
        button->setEnabled(false);
        button->setFixedHeight(32);
        assemblyButtons->addWidget(button);
    }
    QSlider* assemblyProgress = new QSlider(Qt::Horizontal, assemblyGroup);
    assemblyProgress->setObjectName("printableAssemblyProgress");
    assemblyProgress->setRange(0, 0);
    assemblyProgress->setEnabled(false);
    QLabel* assemblyStepLabel = new QLabel("Assembly 0 / 0", assemblyGroup);
    assemblyStepLabel->setAlignment(Qt::AlignCenter);
    QTimer* assemblyTimer = new QTimer(assemblyGroup);
    assemblyTimer->setInterval(35);
    assemblyLayout->addLayout(assemblyButtons);
    assemblyLayout->addWidget(assemblyProgress);
    assemblyLayout->addWidget(assemblyStepLabel);

    QGroupBox* outputGroup = new QGroupBox("Printable Output", panel);
    QVBoxLayout* outputLayout = new QVBoxLayout(outputGroup);
    QPushButton* exportButton = new QPushButton("Export displayed OBJ parts", outputGroup);
    exportButton->setIcon(panel->style()->standardIcon(QStyle::SP_DialogSaveButton));
    exportButton->setEnabled(false);
    outputLayout->addWidget(exportButton);

    QLabel* statusLabel = new QLabel("Ready", panel);
    statusLabel->setWordWrap(true);
    statusLabel->setMinimumHeight(164);
    statusLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    statusLabel->setStyleSheet(
        "background: #2b3037; color: #f2f4f6; padding: 9px; border: 1px solid #424953;");

    auto parameters = [=]() {
        PrintableInterlockParameters result;
        result.resolution = resolution->value();
        result.samplesPerVoxel = samples->value();
        result.pieceCount = pieceCount->value();
        result.candidateLimit = candidateLimit->value();
        result.randomSeed = unsigned(randomSeed->value());
        result.internalVolumeThreshold = internalThreshold->value() / 100.0;
        result.minimumFaceContact = faceContact->value() / 100.0;
        result.maxPartExtentRatio = maximumExtent->value() / 100.0;
        result.refineSalientSeams = refineSeams->isChecked();
        result.enforceInterlocking = strictInterlocking->isChecked();
        return result;
    };
    auto start = [=](bool continuous) {
        if (meshPath->text().isEmpty()) {
            QMessageBox::warning(mainWindow, "Printable Interlocking Parts",
                                 "Select one input mesh first.");
            return;
        }
        widget->loadAndStart(meshPath->text(), parameters(), continuous);
    };
    auto loadPreview = [=](const QString& path) {
        QString error;
        if (!widget->loadPreview(path, &error)) {
            QMessageBox::warning(mainWindow, "Printable Interlocking Parts", error);
            return false;
        }
        meshPath->setText(path);
        return true;
    };

    QObject::connect(loadButton, &QPushButton::clicked, panel, [=]() {
        const QString path = QFileDialog::getOpenFileName(
            mainWindow, "Open watertight triangle mesh", QString(),
            "Triangle meshes (*.obj *.off *.ply)");
        if (!path.isEmpty())
            loadPreview(path);
    });
    QObject::connect(sampleButton, &QPushButton::clicked, panel, [=]() {
        const QString samplePath = printableInterlockSamplePath("rock.obj");
        if (!QFileInfo::exists(samplePath)) {
            QMessageBox::warning(mainWindow, "Printable Interlocking Parts",
                                 "The bundled sample mesh was not found.");
            return;
        }
        if (!loadPreview(samplePath))
            return;
        resolution->setValue(50);
        samples->setValue(4);
        pieceCount->setValue(6);
        candidateLimit->setValue(20);
        internalThreshold->setValue(50.0);
        faceContact->setValue(5.0);
        maximumExtent->setValue(100.0);
    });
    QObject::connect(runButton, &QPushButton::clicked, panel, [=]() {
        if (widget->isRunning())
            widget->resume();
        else
            start(true);
    });
    QObject::connect(pauseButton, &QPushButton::clicked,
                     widget, &PrintableInterlockWidget::pause);
    QObject::connect(stepButton, &QPushButton::clicked, panel, [=]() {
        if (widget->isRunning())
            widget->step();
        else
            start(false);
    });
    QObject::connect(resetButton, &QPushButton::clicked,
                     widget, &PrintableInterlockWidget::reset);
    QObject::connect(wireframe, &QCheckBox::toggled,
                     widget, &PrintableInterlockWidget::setWireframeVisible);
    QObject::connect(voxelModeButton, &QPushButton::toggled, panel, [=](bool checked) {
        if (checked)
            widget->setSurfaceClippedMode(false);
    });
    QObject::connect(surfaceModeButton, &QPushButton::toggled, panel, [=](bool checked) {
        if (checked)
            widget->setSurfaceClippedMode(true);
    });
    QObject::connect(explosion, &QSlider::valueChanged, panel, [=](int value) {
        widget->setExplosion(float(value) / 100.0f);
    });
    auto stopAssemblyPlayback = [=]() {
        assemblyTimer->stop();
        playAssemblyButton->setIcon(
            panel->style()->standardIcon(QStyle::SP_MediaPlay));
        playAssemblyButton->setToolTip("Play assembly");
    };
    auto clearPartSpacing = [=]() {
        if (explosion->value() != 0)
            explosion->setValue(0);
    };
    QObject::connect(assemblyProgress, &QSlider::sliderPressed,
                     panel, clearPartSpacing);
    QObject::connect(assemblyProgress, &QSlider::valueChanged, panel, [=](int value) {
        widget->setAssemblyProgress(float(value) / 100.0f);
        const int steps = assemblyProgress->maximum() / 100;
        assemblyStepLabel->setText(QString("Assembly %1 / %2")
            .arg(float(value) / 100.0f, 0, 'f', value % 100 == 0 ? 0 : 1)
            .arg(steps));
    });
    QObject::connect(previousAssemblyButton, &QPushButton::clicked, panel, [=]() {
        stopAssemblyPlayback();
        clearPartSpacing();
        const int value = assemblyProgress->value();
        assemblyProgress->setValue(value % 100 == 0
            ? (std::max)(0, value - 100) : value - value % 100);
    });
    QObject::connect(nextAssemblyButton, &QPushButton::clicked, panel, [=]() {
        stopAssemblyPlayback();
        clearPartSpacing();
        const int value = assemblyProgress->value();
        assemblyProgress->setValue((std::min)(assemblyProgress->maximum(),
            value % 100 == 0 ? value + 100 : value + (100 - value % 100)));
    });
    QObject::connect(playAssemblyButton, &QPushButton::clicked, panel, [=]() {
        if (assemblyTimer->isActive()) {
            stopAssemblyPlayback();
            return;
        }
        clearPartSpacing();
        if (assemblyProgress->value() >= assemblyProgress->maximum())
            assemblyProgress->setValue(0);
        assemblyTimer->start();
        playAssemblyButton->setIcon(
            panel->style()->standardIcon(QStyle::SP_MediaPause));
        playAssemblyButton->setToolTip("Pause assembly");
    });
    QObject::connect(assemblyTimer, &QTimer::timeout, panel, [=]() {
        const int next = assemblyProgress->value() + 4;
        if (next >= assemblyProgress->maximum()) {
            assemblyProgress->setValue(assemblyProgress->maximum());
            stopAssemblyPlayback();
        } else {
            assemblyProgress->setValue(next);
        }
    });
    QObject::connect(resetViewsButton, &QPushButton::clicked,
                     widget, &PrintableInterlockWidget::resetViews);
    QObject::connect(exportButton, &QPushButton::clicked, panel, [=]() {
        const QString directory = QFileDialog::getExistingDirectory(
            mainWindow, "Export watertight interlocking parts");
        if (directory.isEmpty())
            return;
        QString error;
        if (!widget->exportPrintableParts(directory, &error)) {
            QMessageBox::warning(mainWindow, "Printable Interlocking Parts", error);
            return;
        }
        QMessageBox::information(
            mainWindow, "Printable Interlocking Parts",
            QString("Exported %1 watertight OBJ parts to:\n%2")
                .arg(pieceCount->value()).arg(directory));
    });
    QObject::connect(widget, &PrintableInterlockWidget::snapshotChanged,
                     exportButton, [=](const PrintableInterlockSnapshot& snapshot) {
        exportButton->setEnabled(snapshot.complete
            && snapshot.voxelWatertightParts == snapshot.requestedPieces
            && snapshot.watertightParts == snapshot.requestedPieces);
    });
    QObject::connect(widget, &PrintableInterlockWidget::snapshotChanged,
                     assemblyGroup, [=](const PrintableInterlockSnapshot& snapshot) {
        if (!snapshot.complete) {
            stopAssemblyPlayback();
            assemblyProgress->setRange(0, 0);
            assemblyProgress->setEnabled(false);
            previousAssemblyButton->setEnabled(false);
            playAssemblyButton->setEnabled(false);
            nextAssemblyButton->setEnabled(false);
            assemblyStepLabel->setText("Assembly 0 / 0");
            return;
        }
        stopAssemblyPlayback();
        const int steps = (std::max)(0, snapshot.extractionDirections.size() - 1);
        const bool available = snapshot.interlocking && steps > 0;
        assemblyProgress->setRange(0, steps * 100);
        assemblyProgress->setValue(steps * 100);
        assemblyProgress->setEnabled(available);
        previousAssemblyButton->setEnabled(available);
        playAssemblyButton->setEnabled(available);
        nextAssemblyButton->setEnabled(available);
        assemblyStepLabel->setText(QString("Assembly %1 / %1").arg(steps));
    });
    QObject::connect(widget, &PrintableInterlockWidget::runningChanged,
                     assemblyGroup, [=](bool running) {
        if (!running)
            return;
        stopAssemblyPlayback();
        assemblyProgress->setRange(0, 0);
        assemblyProgress->setEnabled(false);
        previousAssemblyButton->setEnabled(false);
        playAssemblyButton->setEnabled(false);
        nextAssemblyButton->setEnabled(false);
        assemblyStepLabel->setText("Assembly 0 / 0");
    });
    QObject::connect(widget, &PrintableInterlockWidget::statusChanged,
                     statusLabel, &QLabel::setText);

    layout->addWidget(inputGroup);
    layout->addWidget(sampleButton);
    layout->addWidget(parametersGroup);
    layout->addWidget(iterationGroup);
    layout->addWidget(displayGroup);
    layout->addWidget(assemblyGroup);
    layout->addWidget(outputGroup);
    layout->addWidget(statusLabel);
    layout->addStretch();
    return panel;
}
