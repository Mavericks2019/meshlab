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
        "View space: 100×100×100<br>"
        "Model scaled to fit 20×20×20 box<br>"
        "Positioned at left center (-20,0,0)<br>"
        "Mouse dragging disabled for rotation<br>"
        "Use arrow keys for rotation, +/- for zoom"
    );
    descriptionLabel->setStyleSheet("color: white; background-color: #3A3A3A; padding: 10px; border-radius: 5px;");
    descriptionLabel->setWordWrap(true);
    layout->addWidget(descriptionLabel);
    
    layout->addStretch();
    return panel;
}

#endif // RELATIVISTIC_TAB_H