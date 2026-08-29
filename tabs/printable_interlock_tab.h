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
#include <QVBoxLayout>

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
    resolution->setRange(8, 35);
    resolution->setValue(14);
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
    runButton->setToolTip("Run or resume");
    QPushButton* pauseButton = new QPushButton(iterationGroup);
    pauseButton->setIcon(panel->style()->standardIcon(QStyle::SP_MediaPause));
    pauseButton->setToolTip("Pause after the current stage");
    QPushButton* stepButton = new QPushButton(iterationGroup);
    stepButton->setIcon(panel->style()->standardIcon(QStyle::SP_MediaSkipForward));
    stepButton->setToolTip("Advance one construction stage");
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
    QCheckBox* wireframe = new QCheckBox("Show triangle and voxel edges", displayGroup);
    wireframe->setChecked(false);
    QFormLayout* explosionLayout = new QFormLayout;
    QSlider* explosion = new QSlider(Qt::Horizontal, displayGroup);
    explosion->setRange(0, 45);
    explosion->setValue(10);
    explosionLayout->addRow("Part spacing", explosion);
    QPushButton* resetViewsButton = new QPushButton("Reset both views", displayGroup);
    resetViewsButton->setIcon(panel->style()->standardIcon(QStyle::SP_DesktopIcon));
    displayLayout->addLayout(geometryModeLayout);
    displayLayout->addWidget(wireframe);
    displayLayout->addLayout(explosionLayout);
    displayLayout->addWidget(resetViewsButton);

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

    QObject::connect(loadButton, &QPushButton::clicked, panel, [=]() {
        const QString path = QFileDialog::getOpenFileName(
            mainWindow, "Open watertight triangle mesh", QString(),
            "Triangle meshes (*.obj *.off *.ply)");
        if (!path.isEmpty())
            meshPath->setText(path);
    });
    QObject::connect(sampleButton, &QPushButton::clicked, panel, [=]() {
        const QString samplePath = printableInterlockSamplePath("rock.obj");
        if (!QFileInfo::exists(samplePath)) {
            QMessageBox::warning(mainWindow, "Printable Interlocking Parts",
                                 "The bundled sample mesh was not found.");
            return;
        }
        meshPath->setText(samplePath);
        resolution->setValue(14);
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
    QObject::connect(widget, &PrintableInterlockWidget::statusChanged,
                     statusLabel, &QLabel::setText);

    layout->addWidget(inputGroup);
    layout->addWidget(sampleButton);
    layout->addWidget(parametersGroup);
    layout->addWidget(iterationGroup);
    layout->addWidget(displayGroup);
    layout->addWidget(outputGroup);
    layout->addWidget(statusLabel);
    layout->addStretch();
    return panel;
}
