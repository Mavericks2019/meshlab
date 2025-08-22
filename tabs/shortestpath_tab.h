#ifndef SHORTESTPATH_TAB_H
#define SHORTESTPATH_TAB_H

#include "../glwidget/shortestpathglwidget.h"
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
#include <QDateTime>

// 创建最短路径标签页
QWidget* createShortestPathTab(ShortestPathGLWidget* glWidget) {
    QWidget *tab = new QWidget;
    QHBoxLayout *layout = new QHBoxLayout(tab);
    layout->addWidget(glWidget);
    return tab;
}

// 创建OBJ文件加载按钮（最短路径版）
QWidget* createShortestPathModelLoadButton(ShortestPathGLWidget* glWidget, QLabel* infoLabel, QWidget* mainWindow) {
    QPushButton *button = new QPushButton("Load OBJ File (Shortest Path)");
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
            infoLabel->setText("Model loaded (Shortest Path): " + QFileInfo(filePath).fileName());
            mainWindow->setWindowTitle("OBJ Viewer - " + QFileInfo(filePath).fileName() + " (Shortest Path)");
        }
    });
    return button;
}

// 创建最短路径计算按钮
QWidget* createShortestPathCalculateButton(ShortestPathGLWidget* glWidget) {
    QPushButton *button = new QPushButton("Calculate Shortest Path");
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
        glWidget->calculateShortestPath();
    });
    return button;
}

// 创建清除选择按钮
QWidget* createClearSelectionButton(ShortestPathGLWidget* glWidget) {
    QPushButton *button = new QPushButton("Clear Selection");
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
        glWidget->clearSelectedPoints();
    });
    return button;
}

// 创建保存拾取图像按钮
QWidget* createSavePickingImageButton(ShortestPathGLWidget* glWidget) {
    QPushButton *button = new QPushButton("Save Picking Image");
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
        // 生成带时间戳的文件名
        QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
        QString defaultFileName = QString("picking_image_%1.png").arg(timestamp);
        
        // 获取保存路径
        QString filePath = QFileDialog::getSaveFileName(
            nullptr, "Save Picking Image", defaultFileName, "PNG Images (*.png)");
        
        if (!filePath.isEmpty()) {
            glWidget->savePickingImage(filePath);
            QMessageBox::information(nullptr, "Success", 
                QString("Picking image saved to:\n%1").arg(filePath));
        }
    });
    return button;
}

// 创建最短路径控制面板
QWidget* createShortestPathControlPanel(ShortestPathGLWidget* glWidget, QLabel* infoLabel, QWidget* mainWindow) {
    QWidget *panel = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(panel);
    
    // 添加控件组
    layout->addWidget(createShortestPathModelLoadButton(glWidget, infoLabel, mainWindow));
    layout->addWidget(createClearSelectionButton(glWidget));
    layout->addWidget(createShortestPathCalculateButton(glWidget));
    layout->addWidget(createSavePickingImageButton(glWidget));  // 新增按钮
    
    // 添加基本渲染模式组
    QGroupBox *renderingGroup = new QGroupBox("Rendering Mode");
    QVBoxLayout *renderingLayout = new QVBoxLayout(renderingGroup);
    
    QRadioButton *solidRadio = new QRadioButton("Solid (Blinn-Phong)");
    solidRadio->setChecked(false);
    
    QRadioButton *flatRadio = new QRadioButton("Flat Shading");
    flatRadio->setChecked(true);
    
    renderingLayout->addWidget(solidRadio);
    renderingLayout->addWidget(flatRadio);
    
    QObject::connect(solidRadio, &QRadioButton::clicked, [glWidget]() {
        glWidget->currentRenderMode = BaseGLWidget::BlinnPhong;
        glWidget->update();
    });
    
    QObject::connect(flatRadio, &QRadioButton::clicked, [glWidget]() {
        glWidget->currentRenderMode = BaseGLWidget::FlatShading;
        glWidget->update();
    });
    
    layout->addWidget(renderingGroup);
    
    // 添加显示选项组
    QGroupBox *displayGroup = new QGroupBox("Display Options");
    QVBoxLayout *displayLayout = new QVBoxLayout(displayGroup);
    
    QCheckBox *wireframeCheckbox = new QCheckBox("Show Wireframe Overlay");
    wireframeCheckbox->setStyleSheet("color: white;");
    wireframeCheckbox->setChecked(true);
    QObject::connect(wireframeCheckbox, &QCheckBox::stateChanged, [glWidget](int state) {
        glWidget->setShowWireframeOverlay(state == Qt::Checked);
    });
    
    QCheckBox *faceCheckbox = new QCheckBox("Hide Faces");
    faceCheckbox->setStyleSheet("color: white;");
    QObject::connect(faceCheckbox, &QCheckBox::stateChanged, [glWidget](int state) {
        glWidget->setHideFaces(state == Qt::Checked);
    });
    
    displayLayout->addWidget(wireframeCheckbox);
    displayLayout->addWidget(faceCheckbox);
    layout->addWidget(displayGroup);
    
    // 视图控制按钮
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

#endif // SHORTESTPATH_TAB_H