// relativistic_tab.h
#pragma once
#ifndef RELATIVISTIC_TAB_H
#define RELATIVISTIC_TAB_H

#include "../glwidget/relativisticwidget.h"
#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QPushButton>
#include <QLabel>
#include <QGroupBox>
#include <QRadioButton>
#include <QCheckBox>
#include <QSlider>
#include <QDoubleSpinBox>

// 创建Relativistic标签页
inline QWidget* createRelativisticTab(RelativisticGLWidget* glWidget) {
    QWidget *tab = new QWidget;
    QHBoxLayout *layout = new QHBoxLayout(tab);
    layout->addWidget(glWidget);
    return tab;
}

// 创建OBJ文件加载按钮（Relativistic版）
inline QWidget* createRelativisticModelLoadButton(RelativisticGLWidget* glWidget, QLabel* infoLabel, QWidget* mainWindow) {
    QPushButton *button = new QPushButton("Load OBJ File (Relativistic)");
    button->setStyleSheet(
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
    QObject::connect(button, &QPushButton::clicked, [glWidget, infoLabel, mainWindow]() {
        QString filePath = QFileDialog::getOpenFileName(
            mainWindow, "Open OBJ File", "", "OBJ Files (*.obj)");
        
        if (!filePath.isEmpty()) {
            glWidget->loadOBJ(filePath);
            infoLabel->setText("Model loaded (Relativistic): " + QFileInfo(filePath).fileName());
            mainWindow->setWindowTitle("OBJ Viewer - " + QFileInfo(filePath).fileName() + " (Relativistic)");
        }
    });
    return button;
}

// 创建Relativistic渲染模式选择组
inline QGroupBox* createRelativisticRenderingModeGroup(RelativisticGLWidget* glWidget) {
    QGroupBox *group = new QGroupBox("Rendering Mode");
    QVBoxLayout *layout = new QVBoxLayout(group);
    
    QRadioButton *flatRadio = new QRadioButton("Flat Shading");
    flatRadio->setChecked(true); // 默认选中
    
    QRadioButton *blinnPhongRadio = new QRadioButton("Blinn-Phong");
    
    QRadioButton *wireframeRadio = new QRadioButton("Wireframe Only");
    
    QRadioButton *facesRadio = new QRadioButton("Faces Only");
    
    layout->addWidget(flatRadio);
    layout->addWidget(blinnPhongRadio);
    layout->addWidget(wireframeRadio);
    layout->addWidget(facesRadio);
    
    // 连接渲染模式信号
    QObject::connect(flatRadio, &QRadioButton::clicked, [glWidget]() {
        glWidget->setRenderMode(RelativisticGLWidget::FlatShading);
    });
    
    QObject::connect(blinnPhongRadio, &QRadioButton::clicked, [glWidget]() {
        glWidget->setRenderMode(RelativisticGLWidget::BlinnPhong);
    });
    
    QObject::connect(wireframeRadio, &QRadioButton::clicked, [glWidget]() {
        glWidget->setRenderMode(RelativisticGLWidget::WireframeOnly);
    });
    
    QObject::connect(facesRadio, &QRadioButton::clicked, [glWidget]() {
        glWidget->setRenderMode(RelativisticGLWidget::FacesOnly);
    });
    
    return group;
}

// 创建Relativistic显示选项组
inline QGroupBox* createRelativisticDisplayOptionsGroup(RelativisticGLWidget* glWidget) {
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

// 创建Relativistic变换模式选择组
inline QGroupBox* createRelativisticTransformModeGroup(RelativisticGLWidget* glWidget) {
    QGroupBox *group = new QGroupBox("Transform Mode");
    QVBoxLayout *layout = new QVBoxLayout(group);
    
    QRadioButton *noTransformRadio = new QRadioButton("No Transform");
    noTransformRadio->setChecked(true); // 默认选中
    
    QRadioButton *lorentzOnlyRadio = new QRadioButton("Lorentz Only");
    
    QRadioButton *lorentzLightConeRadio = new QRadioButton("Lorentz + Light Cone");
    
    layout->addWidget(noTransformRadio);
    layout->addWidget(lorentzOnlyRadio);
    layout->addWidget(lorentzLightConeRadio);
    
    // 连接变换模式信号
    QObject::connect(noTransformRadio, &QRadioButton::clicked, [glWidget]() {
        glWidget->setTransformMode(RelativisticGLWidget::NoTransform);
    });
    
    QObject::connect(lorentzOnlyRadio, &QRadioButton::clicked, [glWidget]() {
        glWidget->setTransformMode(RelativisticGLWidget::LorentzOnly);
    });
    
    QObject::connect(lorentzLightConeRadio, &QRadioButton::clicked, [glWidget]() {
        glWidget->setTransformMode(RelativisticGLWidget::LorentzAndLightCone);
    });
    
    return group;
}

// 创建速度控制组
inline QGroupBox* createVelocityControlGroup(RelativisticGLWidget* glWidget, QLabel* velocityLabel) {
    QGroupBox *group = new QGroupBox("Velocity Control (X-direction)");
    QVBoxLayout *layout = new QVBoxLayout(group);
    
    // 速度标签
    layout->addWidget(velocityLabel);
    
    // 滑动条
    QSlider *velocitySlider = new QSlider(Qt::Horizontal);
    velocitySlider->setRange(0, 100); // 0-100% 光速
    velocitySlider->setValue(0);
    velocitySlider->setTickPosition(QSlider::TicksBelow);
    velocitySlider->setTickInterval(10);
    
    // 数值显示
    QDoubleSpinBox *velocitySpinBox = new QDoubleSpinBox();
    velocitySpinBox->setRange(0.0, 1.0);
    velocitySpinBox->setSingleStep(0.01);
    velocitySpinBox->setValue(0.0);
    velocitySpinBox->setDecimals(2);
    velocitySpinBox->setSuffix(" c");
    
    // 水平布局：滑动条和数值框
    QHBoxLayout *sliderLayout = new QHBoxLayout();
    sliderLayout->addWidget(velocitySlider);
    sliderLayout->addWidget(velocitySpinBox);
    layout->addLayout(sliderLayout);
    
    // 标签显示速度信息
    QLabel *infoLabel = new QLabel("Speed: 0.00 c (0% of light speed)");
    infoLabel->setStyleSheet("color: #FFA500; font-weight: bold;");
    layout->addWidget(infoLabel);
    
    // 连接信号
    // 滑动条 -> 数值框
    QObject::connect(velocitySlider, &QSlider::valueChanged, [velocitySpinBox, infoLabel](int value) {
        float speed = value / 100.0f;
        velocitySpinBox->setValue(speed);
        infoLabel->setText(QString("Speed: %1 c (%2% of light speed)")
                          .arg(speed, 0, 'f', 2)
                          .arg(value));
    });
    
    // 数值框 -> 滑动条
    QObject::connect(velocitySpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                     [velocitySlider, infoLabel](double value) {
        int sliderValue = static_cast<int>(value * 100);
        velocitySlider->setValue(sliderValue);
        infoLabel->setText(QString("Speed: %1 c (%2% of light speed)")
                          .arg(value, 0, 'f', 2)
                          .arg(sliderValue));
    });
    
    // 数值框 -> GLWidget
    QObject::connect(velocitySpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                     [glWidget](double value) {
        glWidget->setVx(static_cast<float>(value));
    });
    
    // GLWidget速度变化 -> 更新UI
    QObject::connect(glWidget, &RelativisticGLWidget::velocityChanged,
                     [velocitySpinBox, velocitySlider, infoLabel](float vx) {
        velocitySpinBox->setValue(vx);
        velocitySlider->setValue(static_cast<int>(vx * 100));
        infoLabel->setText(QString("Speed: %1 c (%2% of light speed)")
                          .arg(vx, 0, 'f', 2)
                          .arg(static_cast<int>(vx * 100)));
    });
    
    return group;
}

// 创建Relativistic控制面板
inline QWidget* createRelativisticControlPanel(RelativisticGLWidget* glWidget, QLabel* infoLabel, QWidget* mainWindow) {
    QWidget *panel = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(panel);
    
    // 添加加载按钮
    layout->addWidget(createRelativisticModelLoadButton(glWidget, infoLabel, mainWindow));
    
    // 添加渲染模式选择组
    layout->addWidget(createRelativisticRenderingModeGroup(glWidget));
    
    // 添加显示选项组
    layout->addWidget(createRelativisticDisplayOptionsGroup(glWidget));
    
    // 添加变换模式选择组
    layout->addWidget(createRelativisticTransformModeGroup(glWidget));
    
    // 添加速度控制组
    QLabel *velocityLabel = new QLabel("Adjust X-direction velocity (0 to 1.0 c)");
    velocityLabel->setStyleSheet("color: white;");
    layout->addWidget(createVelocityControlGroup(glWidget, velocityLabel));
    
    // 视图重置按钮
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
    layout->addWidget(resetButton);
    
    // 描述标签
    QLabel *descriptionLabel = new QLabel(
        "<b>Relativistic View</b><br><br>"
        "Camera at (0,0,0) looking forward<br>"
        "Perspective projection with adaptive FOV<br>"
        "Model scaled to fit 15×15×15 box<br>"
        "Positioned at left center (-15,0,-30)<br>"
        "Mouse dragging disabled for rotation<br>"
        "Use arrow keys for rotation, +/- for zoom<br><br>"
        "<b>Relativistic Transformations:</b><br>"
        "• Lorentz Only: Length contraction along velocity<br>"
        "• Lorentz + Light Cone: Two-step transformation<br>"
        "• Light speed c = 1.0 (normalized)<br>"
        "• Speed automatically clamped to ≤ c"
    );
    descriptionLabel->setStyleSheet("color: white; background-color: #3A3A3A; padding: 10px; border-radius: 5px;");
    descriptionLabel->setWordWrap(true);
    layout->addWidget(descriptionLabel);
    
    layout->addStretch();
    return panel;
}

#endif // RELATIVISTIC_TAB_H