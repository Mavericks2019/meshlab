#pragma once

#include "../glwidget/progressiveparameterizationwidget.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
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
    layout->addWidget(statusLabel);
    layout->addWidget(saveButton);
    layout->addStretch();
    return panel;
}
