#pragma once
#ifndef RELASTIC_TAB_H
#define RELASTIC_TAB_H

#include "../glwidget/relasticwidget.h"
#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QPushButton>
#include <QLabel>
#include <QGroupBox>
#include <QFormLayout>
#include <QSlider>
#include <QRadioButton>
#include <QCheckBox>
#include <QStackedWidget>
#include <QDoubleSpinBox>
#include <QButtonGroup>

// Create Relastic tab
inline QWidget* createRelasticTab(RelasticGLWidget* glWidget) {
    QWidget *tab = new QWidget;
    QHBoxLayout *layout = new QHBoxLayout(tab);
    layout->addWidget(glWidget);
    return tab;
}

// 创建Relastic渲染模式选择组（与basic_tab.h相同）
inline QGroupBox* createRelasticRenderingModeGroup(RelasticGLWidget* glWidget) {
    QGroupBox *group = new QGroupBox("Rendering Mode");
    QVBoxLayout *layout = new QVBoxLayout(group);
    
    QRadioButton *solidRadio = new QRadioButton("Solid (Blinn-Phong)");
    solidRadio->setChecked(false); // 修改为不选中
    
    // 添加Flat Shading单选按钮
    QRadioButton *flatRadio = new QRadioButton("Flat Shading");
    flatRadio->setChecked(true); // 修改为选中
    
    layout->addWidget(solidRadio);
    layout->addWidget(flatRadio);
    
    // 连接渲染模式信号
    QObject::connect(solidRadio, &QRadioButton::clicked, [glWidget]() {
        glWidget->currentRenderMode = RelasticGLWidget::BlinnPhong;
        glWidget->update();
    });
    
    QObject::connect(flatRadio, &QRadioButton::clicked, [glWidget]() {
        glWidget->currentRenderMode = RelasticGLWidget::FlatShading;
        glWidget->update();
    });
    
    return group;
}

// Create velocity control group
inline QGroupBox* createVelocityControlGroup(RelasticGLWidget* glWidget) {
    QGroupBox *group = new QGroupBox("Relative Velocity Control (in units of c)");
    QFormLayout *layout = new QFormLayout(group);
    
    // X-direction velocity slider
    QSlider *xSlider = new QSlider(Qt::Horizontal);
    xSlider->setRange(0, 99); // 0-0.99
    xSlider->setValue(0);
    xSlider->setStyleSheet("QSlider::groove:horizontal { height: 8px; background: #505050; }"
                          "QSlider::handle:horizontal { width: 20px; background: #42a2da; }");
    
    QLabel *xValueLabel = new QLabel("0.00");
    xValueLabel->setFixedWidth(40);
    xValueLabel->setAlignment(Qt::AlignCenter);
    
    QHBoxLayout *xLayout = new QHBoxLayout();
    xLayout->addWidget(new QLabel("X:"));
    xLayout->addWidget(xSlider);
    xLayout->addWidget(xValueLabel);
    
    // Y-direction velocity slider
    QSlider *ySlider = new QSlider(Qt::Horizontal);
    ySlider->setRange(0, 99);
    ySlider->setValue(0);
    ySlider->setStyleSheet("QSlider::groove:horizontal { height: 8px; background: #505050; }"
                          "QSlider::handle:horizontal { width: 20px; background: #42a2da; }");
    
    QLabel *yValueLabel = new QLabel("0.00");
    yValueLabel->setFixedWidth(40);
    yValueLabel->setAlignment(Qt::AlignCenter);
    
    QHBoxLayout *yLayout = new QHBoxLayout();
    yLayout->addWidget(new QLabel("Y:"));
    yLayout->addWidget(ySlider);
    yLayout->addWidget(yValueLabel);
    
    // Z-direction velocity slider
    QSlider *zSlider = new QSlider(Qt::Horizontal);
    zSlider->setRange(0, 99);
    zSlider->setValue(0);
    zSlider->setStyleSheet("QSlider::groove:horizontal { height: 8px; background: #505050; }"
                          "QSlider::handle:horizontal { width: 20px; background: #42a2da; }");
    
    QLabel *zValueLabel = new QLabel("0.00");
    zValueLabel->setFixedWidth(40);
    zValueLabel->setAlignment(Qt::AlignCenter);
    
    QHBoxLayout *zLayout = new QHBoxLayout();
    zLayout->addWidget(new QLabel("Z:"));
    zLayout->addWidget(zSlider);
    zLayout->addWidget(zValueLabel);
    
    // Total speed display
    QLabel *totalSpeedLabel = new QLabel("Total Speed: 0.00c");
    totalSpeedLabel->setAlignment(Qt::AlignCenter);
    totalSpeedLabel->setStyleSheet("font-weight: bold; color: #42a2da;");
    
    // Lorentz factor display
    QLabel *gammaLabel = new QLabel("Lorentz Factor γ: 1.00");
    gammaLabel->setAlignment(Qt::AlignCenter);
    gammaLabel->setStyleSheet("font-weight: bold; color: #ff6666;");
    
    layout->addRow(xLayout);
    layout->addRow(yLayout);
    layout->addRow(zLayout);
    layout->addRow(totalSpeedLabel);
    layout->addRow(gammaLabel);
    
    // Connect slider signals
    auto updateVelocity = [glWidget, xSlider, ySlider, zSlider, 
                          xValueLabel, yValueLabel, zValueLabel,
                          totalSpeedLabel, gammaLabel]() {
        float vx = xSlider->value() / 100.0f;
        float vy = ySlider->value() / 100.0f;
        float vz = zSlider->value() / 100.0f;
        
        xValueLabel->setText(QString::number(vx, 'f', 2));
        yValueLabel->setText(QString::number(vy, 'f', 2));
        zValueLabel->setText(QString::number(vz, 'f', 2));
        
        QVector3D velocity(vx, vy, vz);
        glWidget->setVelocity(velocity);
        
        // Calculate total speed
        float totalSpeed = velocity.length();
        totalSpeedLabel->setText(QString("Total Speed: %1c").arg(totalSpeed, 0, 'f', 2));
        
        // Calculate Lorentz factor
        float gamma = 1.0f / sqrt(1.0f - totalSpeed * totalSpeed);
        if (totalSpeed >= 0.999f) gamma = 1000.0f;
        gammaLabel->setText(QString("Lorentz Factor γ: %1").arg(gamma, 0, 'f', 2));
    };
    
    QObject::connect(xSlider, &QSlider::valueChanged, updateVelocity);
    QObject::connect(ySlider, &QSlider::valueChanged, updateVelocity);
    QObject::connect(zSlider, &QSlider::valueChanged, updateVelocity);
    
    return group;
}

// Create visualization mode selection group
inline QGroupBox* createVisualizationModeGroup(RelasticGLWidget* glWidget) {
    QGroupBox *group = new QGroupBox("Visualization Mode");
    QVBoxLayout *layout = new QVBoxLayout(group);
    
    // Create radio button group
    QButtonGroup *modeGroup = new QButtonGroup(group);
    
    QRadioButton *originalRadio = new QRadioButton("Original Model");
    QRadioButton *measuredRadio = new QRadioButton("Measured Appearance");
    QRadioButton *visualRadio = new QRadioButton("Visual Appearance");
    
    originalRadio->setChecked(true);
    
    modeGroup->addButton(originalRadio, 0);
    modeGroup->addButton(measuredRadio, 1);
    modeGroup->addButton(visualRadio, 2);
    
    layout->addWidget(originalRadio);
    layout->addWidget(measuredRadio);
    layout->addWidget(visualRadio);
    
    // Connect signals
    QObject::connect(modeGroup, QOverload<int>::of(&QButtonGroup::buttonClicked),
        [glWidget](int id) {
            glWidget->setVisualizationMode(id);
        });
    
    return group;
}

// Create Relastic显示选项组（与basic_tab.h相同）
inline QGroupBox* createRelasticDisplayOptionsGroup(RelasticGLWidget* glWidget) {
    QGroupBox *group = new QGroupBox("Display Options");
    QVBoxLayout *layout = new QVBoxLayout(group);
    
    QCheckBox *wireframeCheckbox = new QCheckBox("Show Wireframe Overlay");
    wireframeCheckbox->setStyleSheet("color: white;");
    wireframeCheckbox->setChecked(true); // 默认选中
    QObject::connect(wireframeCheckbox, &QCheckBox::stateChanged, [glWidget](int state) {
        glWidget->setShowWireframeOverlay(state == Qt::Checked);
    });
    
    QCheckBox *faceCheckbox = new QCheckBox("Hide Faces");
    faceCheckbox->setStyleSheet("color: white;");
    QObject::connect(faceCheckbox, &QCheckBox::stateChanged, [glWidget](int state) {
        glWidget->setHideFaces(state == Qt::Checked);
    });
    
    layout->addWidget(wireframeCheckbox);
    layout->addWidget(faceCheckbox);
    return group;
}

// Create information display group
inline QGroupBox* createRelasticInfoGroup(QLabel** infoLabel) {
    QGroupBox *infoGroup = new QGroupBox("Relativity Information");
    QVBoxLayout *infoLayout = new QVBoxLayout(infoGroup);
    
    // Create information label
    QLabel *label = new QLabel("No model loaded\nUsing Lorentz transformation to visualize relativistic effects");
    label->setAlignment(Qt::AlignCenter);
    label->setFixedHeight(80);
    label->setStyleSheet("background-color: #3A3A3A; color: white; border-radius: 5px; padding: 5px; font-size: 14px;");
    label->setWordWrap(true);
    
    // Assign to infoLabel pointer if provided
    if (infoLabel) {
        *infoLabel = label;
    }
    
    infoLayout->addWidget(label);
    return infoGroup;
}

// 创建导出变换模型按钮
inline QPushButton* createExportTransformedButton(RelasticGLWidget* glWidget, QWidget* mainWindow) {
    QPushButton *exportButton = new QPushButton("Export Transformed OBJ");
    exportButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #4CAF50;"
        "   color: white;"
        "   border: none;"
        "   padding: 10px 20px;"
        "   font-size: 16px;"
        "   border-radius: 5px;"
        "   font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: #45a049; }"
        "QPushButton:disabled { background-color: #666666; color: #999999; }"
    );
    
    // 初始状态：如果没有加载模型，禁用按钮
    exportButton->setEnabled(false);
    
    // 当模型加载状态改变时更新按钮状态
    // 注意：这里需要RelasticGLWidget有modelLoadedChanged信号
    // 如果基类没有这个信号，我们可以使用其他方式
    // 这里我们假设RelasticGLWidget有modelLoadedChanged信号
    QObject::connect(glWidget, &RelasticGLWidget::modelLoadedChanged, exportButton, &QPushButton::setEnabled);
    
    QObject::connect(exportButton, &QPushButton::clicked, [glWidget, mainWindow]() {
        if (!glWidget->modelLoaded) {
            QMessageBox::warning(mainWindow, "No Model", "Please load a model first!");
            return;
        }
        
        // 获取当前可视化模式
        int mode = glWidget->getVisualizationMode();
        QString modeStr;
        
        if (mode == 0) modeStr = "original";
        else if (mode == 1) modeStr = "measured";
        else if (mode == 2) modeStr = "visual";
        
        // 获取速度信息
        QVector3D velocity = glWidget->getVelocity();
        QString velocityStr = QString("_v%1_%2_%3")
            .arg(velocity.x(), 0, 'f', 2)
            .arg(velocity.y(), 0, 'f', 2)
            .arg(velocity.z(), 0, 'f', 2);
        
        // 替换特殊字符
        velocityStr.replace('.', 'p');
        velocityStr.replace('-', 'm');
        
        // 建议文件名
        QString suggestedName = QString("transformed_%1%2.obj").arg(modeStr).arg(velocityStr);
        
        QString filePath = QFileDialog::getSaveFileName(
            mainWindow, 
            "Save Transformed OBJ File",
            suggestedName,
            "OBJ Files (*.obj)"
        );
        
        if (!filePath.isEmpty()) {
            bool success = glWidget->exportTransformedOBJ(filePath);
            if (success) {
                QMessageBox::information(mainWindow, "Export Successful", 
                    QString("Transformed model exported to:\n%1\n\nMode: %2\nVelocity: (%3, %4, %5)c")
                        .arg(filePath)
                        .arg(modeStr)
                        .arg(velocity.x(), 0, 'f', 2)
                        .arg(velocity.y(), 0, 'f', 2)
                        .arg(velocity.z(), 0, 'f', 2));
            } else {
                QMessageBox::critical(mainWindow, "Export Failed", 
                    "Failed to export transformed model. Please check the file path and permissions.");
            }
        }
    });
    
    return exportButton;
}

// Create Relastic control panel
inline QWidget* createRelasticControlPanel(RelasticGLWidget* glWidget, QLabel* infoLabel, QWidget* mainWindow) {
    QWidget *panel = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(panel);
    
    // OBJ file load button
    QPushButton *loadButton = new QPushButton("Load OBJ File");
    loadButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #505050;"
        "   color: white;"
        "   border: none;"
        "   padding: 10px 20px;"
        "   font-size: 16px;"
        "   border-radius: 5px;"
        "}"
        "QPushButton:hover { background-color: #606060; }"
    );
    QObject::connect(loadButton, &QPushButton::clicked, [glWidget, infoLabel, mainWindow]() {
        QString filePath = QFileDialog::getOpenFileName(
            mainWindow, "Open OBJ File", "", "OBJ Files (*.obj)");
        
        if (!filePath.isEmpty()) {
            glWidget->loadOBJ(filePath);
            infoLabel->setText(QString("Model loaded: %1\nReady for relativistic transformation").arg(QFileInfo(filePath).fileName()));
            mainWindow->setWindowTitle(QString("OBJ Viewer - %1 (Relativity)").arg(QFileInfo(filePath).fileName()));
        }
    });
    
    layout->addWidget(loadButton);
    layout->addWidget(createRelasticInfoGroup(&infoLabel));
    
    // 添加渲染模式组（与basic_tab.h相同）
    layout->addWidget(createRelasticRenderingModeGroup(glWidget));
    
    // 添加显示选项组（与basic_tab.h相同）
    layout->addWidget(createRelasticDisplayOptionsGroup(glWidget));
    
    layout->addWidget(createVelocityControlGroup(glWidget));
    layout->addWidget(createVisualizationModeGroup(glWidget));
    
    // 添加导出变换模型按钮
    layout->addWidget(createExportTransformedButton(glWidget, mainWindow));
    
    // View control buttons
    QPushButton *resetButton = new QPushButton("Reset View");
    resetButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #505050;"
        "   color: white;"
        "   border: none;"
        "   padding: 10px 20px;"
        "   font-size: 16px;"
        "   border-radius: 5px;"
        "}"
        "QPushButton:hover { background-color: #606060; }"
    );
    QObject::connect(resetButton, &QPushButton::clicked, [glWidget]() {
        glWidget->resetView();
    });
    
    QPushButton *centerButton = new QPushButton("Center View");
    centerButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #505050;"
        "   color: white;"
        "   border: none;"
        "   padding: 10px 20px;"
        "   font-size: 16px;"
        "   border-radius: 5px;"
        "}"
        "QPushButton:hover { background-color: #606060; }"
    );
    QObject::connect(centerButton, &QPushButton::clicked, [glWidget]() {
        glWidget->centerView();
    });
    
    layout->addWidget(resetButton);
    layout->addWidget(centerButton);
    
    // Relativity effect explanation
    QGroupBox *explanationGroup = new QGroupBox("Relativity Effects Explanation");
    QVBoxLayout *explanationLayout = new QVBoxLayout(explanationGroup);
    
    QLabel *explanationLabel = new QLabel(
        "• Measured Appearance: Calculate object shape measured by stationary observer based on Lorentz transformation\n"
        "• Visual Appearance: Apply light cone intersection method after Lorentz transformation\n"
        "• Velocity is in units of light speed (c), range [0, 0.99)\n"
        "• When velocity approaches light speed, Lorentz contraction occurs in the direction of motion\n"
        "• Lorentz factor γ = 1/√(1-v²/c²) indicates the degree of time dilation and length contraction\n"
        "• Export button saves the transformed vertices according to current visualization mode"
    );
    explanationLabel->setWordWrap(true);
    explanationLabel->setStyleSheet("color: #CCCCCC; font-size: 12px;");
    
    explanationLayout->addWidget(explanationLabel);
    layout->addWidget(explanationGroup);
    
    layout->addStretch();
    return panel;
}

#endif // RELASTIC_TAB_H