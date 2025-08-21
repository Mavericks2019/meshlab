#ifndef MODEL_TAB_H
#define MODEL_TAB_H

#include "../glwidget/modelglwidget.h"
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

// 创建模型标签页
QWidget* createModelTab(ModelGLWidget* glWidget) {
    QWidget *tab = new QWidget;
    QHBoxLayout *layout = new QHBoxLayout(tab);
    layout->addWidget(glWidget);
    return tab;
}

// 创建OBJ文件加载按钮
QWidget* createModelLoadButton(ModelGLWidget* glWidget, QLabel* infoLabel, QWidget* mainWindow) {
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

// 创建渲染模式选择组
QGroupBox* createRenderingModeGroup(ModelGLWidget* glWidget) {
    QGroupBox *group = new QGroupBox("Rendering Mode");
    QVBoxLayout *layout = new QVBoxLayout(group);
    
    QRadioButton *solidRadio = new QRadioButton("Solid (Blinn-Phong)");
    QRadioButton *flatRadio = new QRadioButton("Flat Shading"); // 添加Flat Shading
    QRadioButton *gaussianRadio = new QRadioButton("Gaussian Curvature");
    QRadioButton *meanRadio = new QRadioButton("Mean Curvature");
    QRadioButton *maxRadio = new QRadioButton("Max Curvature");
    
    solidRadio->setChecked(true);
    
    layout->addWidget(solidRadio);
    layout->addWidget(flatRadio); // 添加Flat Shading
    layout->addWidget(gaussianRadio);
    layout->addWidget(meanRadio);
    layout->addWidget(maxRadio);
    
    // 连接渲染模式信号
    auto connectMode = [glWidget](QRadioButton* radio, ModelGLWidget::RenderMode mode) {
        QObject::connect(radio, &QRadioButton::clicked, [glWidget, mode]() {
            glWidget->setRenderMode(mode);
        });
    };
    
    connectMode(solidRadio, ModelGLWidget::BlinnPhong);
    connectMode(flatRadio, ModelGLWidget::FlatShading); // 连接Flat Shading
    connectMode(gaussianRadio, ModelGLWidget::GaussianCurvature);
    connectMode(meanRadio, ModelGLWidget::MeanCurvature);
    connectMode(maxRadio, ModelGLWidget::MaxCurvature);
    
    return group;
}

// 创建显示选项组
QGroupBox* createDisplayOptionsGroup(ModelGLWidget* glWidget) {
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

// 创建视图重置按钮
QPushButton* createViewResetButton(ModelGLWidget* glWidget) {
    QPushButton *button = new QPushButton("Reset View");
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
    QObject::connect(button, &QPushButton::clicked, [glWidget]() {
        glWidget->resetView();
    });
    return button;
}

// 创建自适应视图按钮
QPushButton* createCenterViewButton(ModelGLWidget* glWidget) {
    QPushButton *button = new QPushButton("Center View");
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
    QObject::connect(button, &QPushButton::clicked, [glWidget]() {
        glWidget->centerView();
    });
    return button;
}

// 创建模型控制面板
QWidget* createModelControlPanel(ModelGLWidget* glWidget, QLabel* infoLabel, QWidget* mainWindow) {
    QWidget *panel = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(panel);
    
    // 添加控件组
    layout->addWidget(createModelLoadButton(glWidget, infoLabel, mainWindow));
    layout->addWidget(createRenderingModeGroup(glWidget));
    layout->addWidget(createDisplayOptionsGroup(glWidget));
    layout->addWidget(createViewResetButton(glWidget));
    layout->addWidget(createCenterViewButton(glWidget));

    layout->addStretch();
    return panel;
}

#endif // MODEL_TAB_H