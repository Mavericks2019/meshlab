#pragma once

#include "../glwidget/steady3ddissectionwidget.h"

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
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QStyle>
#include <QVBoxLayout>

inline QWidget* createSteady3DDissectionTab(Steady3DDissectionWidget* widget)
{
    QWidget* tab = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout(tab);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(widget);
    return tab;
}

inline QString steadyDissectionSamplePath(const QString& fileName)
{
    const QString sourcePath = QDir::current().filePath("models/" + fileName);
    if (QFileInfo::exists(sourcePath))
        return sourcePath;
    return QDir(QCoreApplication::applicationDirPath()).filePath("models/" + fileName);
}

inline QWidget* createSteady3DDissectionControlPanel(
    Steady3DDissectionWidget* widget, QWidget* mainWindow)
{
    QWidget* panel = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout(panel);
    layout->setAlignment(Qt::AlignTop);

    QGroupBox* inputGroup = new QGroupBox("Input Forms", panel);
    QVBoxLayout* inputLayout = new QVBoxLayout(inputGroup);
    auto createInputRow = [&](const QString& labelText, QLineEdit** pathEdit) {
        QWidget* row = new QWidget(inputGroup);
        QHBoxLayout* rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(0, 0, 0, 0);
        QLabel* label = new QLabel(labelText, row);
        label->setFixedWidth(48);
        *pathEdit = new QLineEdit(row);
        (*pathEdit)->setReadOnly(true);
        (*pathEdit)->setPlaceholderText("No mesh selected");
        QLineEdit* edit = *pathEdit;
        QPushButton* loadButton = new QPushButton(row);
        loadButton->setIcon(panel->style()->standardIcon(QStyle::SP_DialogOpenButton));
        loadButton->setToolTip(
            "Select one connected, watertight, non-self-intersecting triangle mesh");
        loadButton->setFixedWidth(34);
        rowLayout->addWidget(label);
        rowLayout->addWidget(*pathEdit, 1);
        rowLayout->addWidget(loadButton);
        QObject::connect(loadButton, &QPushButton::clicked, panel, [=]() {
            const QString path = QFileDialog::getOpenFileName(
                mainWindow, "Open triangle mesh", QString(),
                "Triangle meshes (*.obj *.off *.ply)");
            if (!path.isEmpty())
                edit->setText(path);
        });
        inputLayout->addWidget(row);
    };

    QLineEdit* firstPath = nullptr;
    QLineEdit* secondPath = nullptr;
    createInputRow("Form A", &firstPath);
    createInputRow("Form B", &secondPath);
    QPushButton* sampleButton = new QPushButton("Load sample pair", inputGroup);
    sampleButton->setIcon(panel->style()->standardIcon(QStyle::SP_FileIcon));
    inputLayout->addWidget(sampleButton);

    QGroupBox* parametersGroup = new QGroupBox("Paper Parameters", panel);
    QFormLayout* parametersLayout = new QFormLayout(parametersGroup);
    QSpinBox* resolution = new QSpinBox(parametersGroup);
    resolution->setRange(8, 35);
    resolution->setValue(16);
    resolution->setSuffix(" cells");
    QSpinBox* pieceCount = new QSpinBox(parametersGroup);
    pieceCount->setRange(3, 30);
    pieceCount->setValue(10);
    QSpinBox* candidateLimit = new QSpinBox(parametersGroup);
    candidateLimit->setRange(1, 40);
    candidateLimit->setValue(10);
    QSpinBox* randomSeed = new QSpinBox(parametersGroup);
    randomSeed->setRange(0, 999999);
    randomSeed->setValue(7);
    QDoubleSpinBox* deletionThreshold = new QDoubleSpinBox(parametersGroup);
    deletionThreshold->setDecimals(1);
    deletionThreshold->setRange(0.0, 100.0);
    deletionThreshold->setValue(35.0);
    deletionThreshold->setSuffix(" %");
    QCheckBox* strictInterlocking = new QCheckBox(
        "Enforce generalized interlocking", parametersGroup);
    strictInterlocking->setChecked(true);
    parametersLayout->addRow("Voxel resolution", resolution);
    parametersLayout->addRow("Pieces", pieceCount);
    parametersLayout->addRow("Candidates", candidateLimit);
    parametersLayout->addRow("Random seed", randomSeed);
    parametersLayout->addRow("Boundary deletion", deletionThreshold);
    parametersLayout->addRow(strictInterlocking);

    QGroupBox* iterationGroup = new QGroupBox("Construction", panel);
    QHBoxLayout* iterationLayout = new QHBoxLayout(iterationGroup);
    QPushButton* runButton = new QPushButton(iterationGroup);
    runButton->setIcon(panel->style()->standardIcon(QStyle::SP_MediaPlay));
    runButton->setToolTip("Run or resume");
    QPushButton* pauseButton = new QPushButton(iterationGroup);
    pauseButton->setIcon(panel->style()->standardIcon(QStyle::SP_MediaPause));
    pauseButton->setToolTip("Pause after the current piece");
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
    QCheckBox* wireframe = new QCheckBox("Show voxel edges", displayGroup);
    wireframe->setChecked(true);
    QFormLayout* explosionLayout = new QFormLayout;
    QSlider* explosion = new QSlider(Qt::Horizontal, displayGroup);
    explosion->setRange(0, 40);
    explosion->setValue(8);
    explosionLayout->addRow("Piece spacing", explosion);
    QPushButton* resetViewsButton = new QPushButton("Reset both views", displayGroup);
    resetViewsButton->setIcon(panel->style()->standardIcon(QStyle::SP_DesktopIcon));
    displayLayout->addWidget(wireframe);
    displayLayout->addLayout(explosionLayout);
    displayLayout->addWidget(resetViewsButton);

    QLabel* statusLabel = new QLabel("Ready", panel);
    statusLabel->setWordWrap(true);
    statusLabel->setMinimumHeight(142);
    statusLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    statusLabel->setStyleSheet(
        "background: #2b3037; color: #f2f4f6; padding: 9px; border: 1px solid #424953;");

    auto parameters = [=]() {
        SteadyDissectionParameters result;
        result.resolution = resolution->value();
        result.pieceCount = pieceCount->value();
        result.candidateLimit = candidateLimit->value();
        result.randomSeed = unsigned(randomSeed->value());
        result.maxBoundaryDeletionRatio = deletionThreshold->value() / 100.0;
        result.enforceGeneralizedInterlocking = strictInterlocking->isChecked();
        return result;
    };
    auto start = [=](bool continuous) {
        if (firstPath->text().isEmpty() || secondPath->text().isEmpty()) {
            QMessageBox::warning(mainWindow, "Steady 3D Dissection",
                                 "Select both input meshes first.");
            return;
        }
        widget->loadAndStart(firstPath->text(), secondPath->text(),
                             parameters(), continuous);
    };

    QObject::connect(sampleButton, &QPushButton::clicked, panel, [=]() {
        const QString first = steadyDissectionSamplePath("cube.obj");
        const QString second = steadyDissectionSamplePath("dissection_cuboid.obj");
        if (!QFileInfo::exists(first) || !QFileInfo::exists(second)) {
            QMessageBox::warning(mainWindow, "Steady 3D Dissection",
                                 "The bundled sample meshes were not found.");
            return;
        }
        firstPath->setText(first);
        secondPath->setText(second);
        resolution->setValue(10);
        pieceCount->setValue(4);
        candidateLimit->setValue(20);
        deletionThreshold->setValue(80.0);
    });
    QObject::connect(runButton, &QPushButton::clicked, panel, [=]() {
        if (widget->isRunning())
            widget->resume();
        else
            start(true);
    });
    QObject::connect(pauseButton, &QPushButton::clicked,
                     widget, &Steady3DDissectionWidget::pause);
    QObject::connect(stepButton, &QPushButton::clicked, panel, [=]() {
        if (widget->isRunning())
            widget->step();
        else
            start(false);
    });
    QObject::connect(resetButton, &QPushButton::clicked,
                     widget, &Steady3DDissectionWidget::reset);
    QObject::connect(wireframe, &QCheckBox::toggled,
                     widget, &Steady3DDissectionWidget::setWireframeVisible);
    QObject::connect(explosion, &QSlider::valueChanged, panel, [=](int value) {
        widget->setExplosion(float(value) / 100.0f);
    });
    QObject::connect(resetViewsButton, &QPushButton::clicked,
                     widget, &Steady3DDissectionWidget::resetViews);
    QObject::connect(widget, &Steady3DDissectionWidget::statusChanged,
                     statusLabel, &QLabel::setText);

    layout->addWidget(inputGroup);
    layout->addWidget(parametersGroup);
    layout->addWidget(iterationGroup);
    layout->addWidget(displayGroup);
    layout->addWidget(statusLabel);
    layout->addStretch();
    return panel;
}
