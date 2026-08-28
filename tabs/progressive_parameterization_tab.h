#pragma once

#include "../glwidget/progressiveparameterizationwidget.h"

#include <QCoreApplication>
#include <QCheckBox>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QVBoxLayout>

inline QWidget* createProgressiveParameterizationTab(ProgressiveParameterizationWidget* widget)
{
    QWidget* tab = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout(tab);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(widget);
    return tab;
}

inline QString progressiveSamplePath()
{
    const QString sourcePath = QDir::current().filePath("models/progressive_sample.obj");
    if (QFileInfo::exists(sourcePath)) return sourcePath;
    return QDir(QCoreApplication::applicationDirPath()).filePath("models/progressive_sample.obj");
}

inline QWidget* createProgressiveParameterizationControlPanel(
    ProgressiveParameterizationWidget* widget, QWidget* mainWindow)
{
    QWidget* panel = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout(panel);
    layout->setAlignment(Qt::AlignTop);

    QLabel* fileLabel = new QLabel("No mesh loaded", panel);
    fileLabel->setWordWrap(true);
    QLabel* statusLabel = new QLabel("Idle", panel);
    statusLabel->setWordWrap(true);
    statusLabel->setMinimumHeight(72);
    statusLabel->setStyleSheet("background: #30343a; color: white; padding: 8px;");

    QGroupBox* inputGroup = new QGroupBox("Input Mesh", panel);
    QVBoxLayout* inputLayout = new QVBoxLayout(inputGroup);
    QPushButton* loadButton = new QPushButton("Load OBJ", inputGroup);
    QPushButton* sampleButton = new QPushButton("Load Sample", inputGroup);
    inputLayout->addWidget(loadButton);
    inputLayout->addWidget(sampleButton);
    inputLayout->addWidget(fileLabel);

    QGroupBox* runGroup = new QGroupBox("Iteration", panel);
    QVBoxLayout* runLayout = new QVBoxLayout(runGroup);
    QPushButton* runButton = new QPushButton("Run", runGroup);
    QPushButton* pauseButton = new QPushButton("Pause", runGroup);
    QPushButton* stepButton = new QPushButton("Step", runGroup);
    QPushButton* resetButton = new QPushButton("Reset", runGroup);
    runLayout->addWidget(runButton);
    runLayout->addWidget(pauseButton);
    runLayout->addWidget(stepButton);
    runLayout->addWidget(resetButton);

    QGroupBox* renderingGroup = new QGroupBox("Rendering Mode", panel);
    QVBoxLayout* renderingLayout = new QVBoxLayout(renderingGroup);
    QRadioButton* blinnPhongRadio = new QRadioButton("Solid (Blinn-Phong)", renderingGroup);
    QRadioButton* flatShadingRadio = new QRadioButton("Flat Shading", renderingGroup);
    flatShadingRadio->setChecked(true);
    renderingLayout->addWidget(blinnPhongRadio);
    renderingLayout->addWidget(flatShadingRadio);

    QGroupBox* displayGroup = new QGroupBox("Display Options", panel);
    QVBoxLayout* displayLayout = new QVBoxLayout(displayGroup);
    QCheckBox* facesCheckbox = new QCheckBox("Show Faces", displayGroup);
    QCheckBox* wireframeCheckbox = new QCheckBox("Show Wireframe Overlay", displayGroup);
    facesCheckbox->setChecked(true);
    wireframeCheckbox->setChecked(true);
    displayLayout->addWidget(facesCheckbox);
    displayLayout->addWidget(wireframeCheckbox);

    QGroupBox* viewGroup = new QGroupBox("View", panel);
    QVBoxLayout* viewLayout = new QVBoxLayout(viewGroup);
    QPushButton* resetViewsButton = new QPushButton("Reset Views", viewGroup);
    QPushButton* centerViewsButton = new QPushButton("Center Views", viewGroup);
    viewLayout->addWidget(resetViewsButton);
    viewLayout->addWidget(centerViewsButton);

    QPushButton* saveButton = new QPushButton("Export Mp OBJ", panel);
    saveButton->setEnabled(false);

    auto loadPath = [widget, fileLabel](const QString& path) {
        if (path.isEmpty()) return;
        fileLabel->setText(QFileInfo(path).fileName());
        widget->loadAndStart(path, false);
    };
    QObject::connect(loadButton, &QPushButton::clicked, panel, [mainWindow, loadPath]() {
        loadPath(QFileDialog::getOpenFileName(mainWindow, "Open triangular mesh", QString(), "Mesh files (*.obj *.off *.ply)"));
    });
    QObject::connect(sampleButton, &QPushButton::clicked, panel, [mainWindow, loadPath]() {
        const QString path = progressiveSamplePath();
        if (!QFileInfo::exists(path)) QMessageBox::warning(mainWindow, "Sample", "Bundled sample mesh was not found.");
        else loadPath(path);
    });
    QObject::connect(runButton, &QPushButton::clicked, panel, [widget, loadPath]() {
        if (widget->isRunning()) widget->resume();
        else if (!widget->inputPath().isEmpty()) widget->loadAndStart(widget->inputPath(), true);
        else loadPath(progressiveSamplePath());
    });
    QObject::connect(pauseButton, &QPushButton::clicked, widget, &ProgressiveParameterizationWidget::pause);
    QObject::connect(stepButton, &QPushButton::clicked, panel, [widget, loadPath]() {
        if (widget->isRunning()) widget->step();
        else if (!widget->inputPath().isEmpty()) widget->loadAndStart(widget->inputPath(), false);
        else loadPath(progressiveSamplePath());
    });
    QObject::connect(resetButton, &QPushButton::clicked, widget, &ProgressiveParameterizationWidget::reset);
    QObject::connect(blinnPhongRadio, &QRadioButton::toggled, panel, [widget](bool checked) {
        if (checked) widget->setRenderMode(BaseGLWidget::BlinnPhong);
    });
    QObject::connect(flatShadingRadio, &QRadioButton::toggled, panel, [widget](bool checked) {
        if (checked) widget->setRenderMode(BaseGLWidget::FlatShading);
    });
    QObject::connect(facesCheckbox, &QCheckBox::toggled, widget, &ProgressiveParameterizationWidget::setFacesVisible);
    QObject::connect(wireframeCheckbox, &QCheckBox::toggled, widget, &ProgressiveParameterizationWidget::setWireframeVisible);
    QObject::connect(resetViewsButton, &QPushButton::clicked, widget, &ProgressiveParameterizationWidget::resetViews);
    QObject::connect(centerViewsButton, &QPushButton::clicked, widget, &ProgressiveParameterizationWidget::centerViews);
    QObject::connect(widget, &ProgressiveParameterizationWidget::statusChanged, statusLabel, &QLabel::setText);
    QObject::connect(widget, &ProgressiveParameterizationWidget::statusChanged, panel, [saveButton](const QString& text) {
        saveButton->setEnabled(!text.startsWith("Error") && text != "Idle");
    });
    QObject::connect(saveButton, &QPushButton::clicked, panel, [widget, mainWindow]() {
        const QString path = QFileDialog::getSaveFileName(mainWindow, "Export parameterization", "parameterized.obj", "OBJ files (*.obj)");
        if (!path.isEmpty() && !widget->saveParameterized(path))
            QMessageBox::warning(mainWindow, "Export", "Could not write the parameterized mesh.");
    });

    layout->addWidget(inputGroup);
    layout->addWidget(runGroup);
    layout->addWidget(renderingGroup);
    layout->addWidget(displayGroup);
    layout->addWidget(viewGroup);
    layout->addWidget(statusLabel);
    layout->addWidget(saveButton);
    layout->addStretch();
    return panel;
}
