// basic_tab.h
#ifndef BASIC_TAB_H
#define BASIC_TAB_H

#include "../glwidget/baseglwidget.h"
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

// 创建OpenMesh标签页
QWidget* createBasicTab(BaseGLWidget* glWidget) {
    QWidget *tab = new QWidget;
    QHBoxLayout *layout = new QHBoxLayout(tab);
    layout->addWidget(glWidget);
    return tab;
}

// 创建OBJ文件加载按钮（OpenMesh版）
QWidget* createBasicModelLoadButton(BaseGLWidget* glWidget, QLabel* infoLabel, QWidget* mainWindow) {
    QPushButton *button = new QPushButton("Load OBJ File (OpenMesh)");
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
            infoLabel->setText("Model loaded (OpenMesh): " + QFileInfo(filePath).fileName());
            mainWindow->setWindowTitle("OBJ Viewer - " + QFileInfo(filePath).fileName() + " (OpenMesh)");
        }
    });
    return button;
}

// 创建OpenMesh渲染模式选择组
QGroupBox* createBasicRenderingModeGroup(BaseGLWidget* glWidget) {
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
        glWidget->currentRenderMode = BaseGLWidget::BlinnPhong;
        glWidget->update();
    });
    
    QObject::connect(flatRadio, &QRadioButton::clicked, [glWidget]() {
        glWidget->currentRenderMode = BaseGLWidget::FlatShading;
        glWidget->update();
    });
    
    return group;
}

// 创建OpenMesh显示选项组
QGroupBox* createBasicDisplayOptionsGroup(BaseGLWidget* glWidget) {
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

// 创建OpenMesh模型控制面板
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