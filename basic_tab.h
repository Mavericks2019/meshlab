#ifndef BASIC_TAB_H
#define BASIC_TAB_H

#include "glwidget/baseglwidget.h"
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

// 创建基础标签页
QWidget* createBasicTab(BaseGLWidget* glWidget) {
    QWidget *tab = new QWidget;
    QHBoxLayout *layout = new QHBoxLayout(tab);
    layout->addWidget(glWidget);
    return tab;
}

// 创建OBJ文件加载按钮（基础版）
QWidget* createBasicModelLoadButton(BaseGLWidget* glWidget, QLabel* infoLabel, QWidget* mainWindow) {
    QPushButton *button = new QPushButton("Load OBJ File");
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
            infoLabel->setText("Model loaded: " + QFileInfo(filePath).fileName());
            mainWindow->setWindowTitle("OBJ Viewer - " + QFileInfo(filePath).fileName());
        }
    });
    return button;
}

// 创建基础渲染模式选择组（只有Blinn-Phong）
QGroupBox* createBasicRenderingModeGroup(BaseGLWidget* glWidget) {
    QGroupBox *group = new QGroupBox("Rendering Mode");
    QVBoxLayout *layout = new QVBoxLayout(group);
    
    QRadioButton *solidRadio = new QRadioButton("Solid (Blinn-Phong)");
    solidRadio->setChecked(true);
    
    // 添加Flat Shading单选按钮
    QRadioButton *flatRadio = new QRadioButton("Flat Shading");
    
    layout->addWidget(solidRadio);
    layout->addWidget(flatRadio);
    
    // 连接渲染模式信号
    QObject::connect(solidRadio, &QRadioButton::clicked, [glWidget]() {
        glWidget->currentRenderMode = BaseGLWidget::BlinnPhong;
        glWidget->update();
    });
    
    QObject::connect(flatRadio, &QRadioButton::clicked, [glWidget]() {
        glWidget->currentRenderMode = BaseGLWidget::FlatShading;
        glWidget->update();
    });
    
    return group;
}

// 创建基础显示选项组
QGroupBox* createBasicDisplayOptionsGroup(BaseGLWidget* glWidget) {
    QGroupBox *group = new QGroupBox("Display Options");
    QVBoxLayout *layout = new QVBoxLayout(group);
    
    QCheckBox *wireframeCheckbox = new QCheckBox("Show Wireframe Overlay");
    wireframeCheckbox->setStyleSheet("color: white;");
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

// 创建基础模型控制面板
QWidget* createBasicControlPanel(BaseGLWidget* glWidget, QLabel* infoLabel, QWidget* mainWindow) {
    QWidget *panel = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(panel);
    
    // 添加控件组
    layout->addWidget(createBasicModelLoadButton(glWidget, infoLabel, mainWindow));
    layout->addWidget(createBasicRenderingModeGroup(glWidget));
    layout->addWidget(createBasicDisplayOptionsGroup(glWidget));
    
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
    
    // 自适应视图按钮
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
    layout->addWidget(centerButton);
    
    layout->addStretch();
    return panel;
}

#endif // BASIC_TAB_H