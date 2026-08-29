#pragma once

#include "../glwidget/atlasparameterizationwidget.h"

#include <QCheckBox>
#include <QCoreApplication>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QGroupBox>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QToolButton>
#include <QVBoxLayout>

inline QWidget* createAtlasParameterizationTab(AtlasParameterizationWidget* widget)
{
    QWidget* tab = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout(tab);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(widget);
    return tab;
}

inline QString atlasSamplePath()
{
    const QString sourcePath = QDir::current().filePath("models/Input.obj");
    if (QFileInfo::exists(sourcePath))
        return sourcePath;
    return QDir(QCoreApplication::applicationDirPath()).filePath("models/Input.obj");
}

inline QWidget* createAtlasParameterizationControlPanel(
    AtlasParameterizationWidget* widget, QWidget* mainWindow)
{
    QWidget* panel = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout(panel);
    layout->setAlignment(Qt::AlignTop);

    QLabel* fileLabel = new QLabel("No mesh loaded", panel);
    fileLabel->setWordWrap(true);
    QLabel* statusLabel = new QLabel("Idle", panel);
    statusLabel->setWordWrap(true);
    statusLabel->setMinimumHeight(84);
    statusLabel->setStyleSheet("background: #30343a; color: white; padding: 8px;");

    QGroupBox* inputGroup = new QGroupBox("Input Atlas Mesh", panel);
    QVBoxLayout* inputLayout = new QVBoxLayout(inputGroup);
    QPushButton* loadButton = new QPushButton("Load OBJ with UV", inputGroup);
    QPushButton* sampleButton = new QPushButton("Load Sample", inputGroup);
    inputLayout->addWidget(loadButton);
    inputLayout->addWidget(sampleButton);
    inputLayout->addWidget(fileLabel);

    QGroupBox* pipelineGroup = new QGroupBox("Packing Pipeline", panel);
    QVBoxLayout* pipelineLayout = new QVBoxLayout(pipelineGroup);
    QPushButton* runAllButton = new QPushButton("Run All", pipelineGroup);
    QPushButton* pauseButton = new QPushButton("Pause", pipelineGroup);
    QPushButton* nextButton = new QPushButton("Next Step", pipelineGroup);
    QPushButton* restartButton = new QPushButton("Restart", pipelineGroup);
    pipelineLayout->addWidget(runAllButton);
    pipelineLayout->addWidget(pauseButton);
    pipelineLayout->addWidget(nextButton);
    pipelineLayout->addWidget(restartButton);

    QGroupBox* renderingGroup = new QGroupBox("Rendering Mode", panel);
    QVBoxLayout* renderingLayout = new QVBoxLayout(renderingGroup);
    QRadioButton* solidRadio = new QRadioButton("Solid (Blinn-Phong)", renderingGroup);
    QRadioButton* flatRadio = new QRadioButton("Flat Shading", renderingGroup);
    flatRadio->setChecked(true);
    renderingLayout->addWidget(solidRadio);
    renderingLayout->addWidget(flatRadio);

    QGroupBox* displayGroup = new QGroupBox("Display Options", panel);
    QVBoxLayout* displayLayout = new QVBoxLayout(displayGroup);
    QCheckBox* facesCheckbox = new QCheckBox("Show Faces", displayGroup);
    QCheckBox* wireframeCheckbox = new QCheckBox("Show Wireframe Overlay", displayGroup);
    QCheckBox* checkerboardCheckbox =
        new QCheckBox("Show Checkerboard Texture (3D + UV)", displayGroup);
    QCheckBox* componentsCheckbox =
        new QCheckBox("Color Connected Components (3D + UV)", displayGroup);
    facesCheckbox->setChecked(true);
    wireframeCheckbox->setChecked(false);
    checkerboardCheckbox->setChecked(false);
    componentsCheckbox->setChecked(true);
    displayLayout->addWidget(facesCheckbox);
    displayLayout->addWidget(wireframeCheckbox);
    displayLayout->addWidget(checkerboardCheckbox);
    displayLayout->addWidget(componentsCheckbox);

    QGroupBox* viewGroup = new QGroupBox("View", panel);
    QVBoxLayout* viewLayout = new QVBoxLayout(viewGroup);
    QPushButton* resetViewsButton = new QPushButton("Reset Views", viewGroup);
    QPushButton* centerViewsButton = new QPushButton("Center Views", viewGroup);
    viewLayout->addWidget(resetViewsButton);
    viewLayout->addWidget(centerViewsButton);

    QPushButton* exportButton = new QPushButton("Export Atlas OBJ", panel);
    exportButton->setEnabled(false);

    auto loadPath = [widget, fileLabel](const QString& path) {
        if (path.isEmpty())
            return;
        fileLabel->setText(QFileInfo(path).fileName());
        widget->loadAndStart(path, false);
    };

    QObject::connect(loadButton, &QPushButton::clicked, panel, [mainWindow, loadPath]() {
        loadPath(QFileDialog::getOpenFileName(
            mainWindow, "Open textured triangular mesh", QString(), "OBJ files (*.obj)"));
    });
    QObject::connect(sampleButton, &QPushButton::clicked, panel, [mainWindow, loadPath]() {
        const QString path = atlasSamplePath();
        if (!QFileInfo::exists(path))
            QMessageBox::warning(mainWindow, "Sample", "Bundled AAAtlas sample was not found.");
        else
            loadPath(path);
    });
    QObject::connect(runAllButton, &QPushButton::clicked, panel, [widget, loadPath]() {
        if (widget->inputPath().isEmpty())
            loadPath(atlasSamplePath());
        widget->runAll();
    });
    QObject::connect(pauseButton, &QPushButton::clicked,
                     widget, &AtlasParameterizationWidget::pause);
    QObject::connect(nextButton, &QPushButton::clicked, panel, [widget, loadPath]() {
        if (widget->inputPath().isEmpty())
            loadPath(atlasSamplePath());
        widget->step();
    });
    QObject::connect(restartButton, &QPushButton::clicked,
                     widget, &AtlasParameterizationWidget::restart);
    QObject::connect(solidRadio, &QRadioButton::toggled, panel, [widget](bool checked) {
        if (checked)
            widget->setRenderMode(BaseGLWidget::BlinnPhong);
    });
    QObject::connect(flatRadio, &QRadioButton::toggled, panel, [widget](bool checked) {
        if (checked)
            widget->setRenderMode(BaseGLWidget::FlatShading);
    });
    QObject::connect(facesCheckbox, &QCheckBox::toggled,
                     widget, &AtlasParameterizationWidget::setFacesVisible);
    QObject::connect(wireframeCheckbox, &QCheckBox::toggled,
                     widget, &AtlasParameterizationWidget::setWireframeVisible);
    QObject::connect(checkerboardCheckbox, &QCheckBox::toggled, panel,
                     [widget, componentsCheckbox](bool checked) {
        if (checked)
            componentsCheckbox->setChecked(false);
        widget->setCheckerboardVisible(checked);
    });
    QObject::connect(componentsCheckbox, &QCheckBox::toggled, panel,
                     [widget, checkerboardCheckbox](bool checked) {
        if (checked)
            checkerboardCheckbox->setChecked(false);
        widget->setComponentColorsVisible(checked);
    });
    QObject::connect(resetViewsButton, &QPushButton::clicked,
                     widget, &AtlasParameterizationWidget::resetViews);
    QObject::connect(centerViewsButton, &QPushButton::clicked,
                     widget, &AtlasParameterizationWidget::centerViews);
    QObject::connect(widget, &AtlasParameterizationWidget::statusChanged,
                     statusLabel, &QLabel::setText);
    QObject::connect(widget, &AtlasParameterizationWidget::resultAvailable,
                     exportButton, &QPushButton::setEnabled);
    QObject::connect(exportButton, &QPushButton::clicked, panel, [widget, mainWindow]() {
        const QString path = QFileDialog::getSaveFileName(
            mainWindow, "Export UV atlas", "atlas_result.obj", "OBJ files (*.obj)");
        if (!path.isEmpty() && !widget->saveParameterized(path))
            QMessageBox::warning(mainWindow, "Export", "Could not write the atlas mesh.");
    });

    widget->setCheckerboardVisible(checkerboardCheckbox->isChecked());
    widget->setComponentColorsVisible(componentsCheckbox->isChecked());
    widget->setWireframeVisible(wireframeCheckbox->isChecked());

    layout->addWidget(inputGroup);
    layout->addWidget(pipelineGroup);
    layout->addWidget(renderingGroup);
    layout->addWidget(displayGroup);
    layout->addWidget(viewGroup);
    layout->addWidget(statusLabel);
    layout->addWidget(exportButton);
    layout->addStretch();
    return panel;
}
