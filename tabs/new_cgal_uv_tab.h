// tabs/new_cgal_uv_tab.h
#pragma once
#ifndef NEW_CGAL_UV_TAB_H
#define NEW_CGAL_UV_TAB_H

#include "../glwidget/cgalglwidget.h"
#include "../glwidget/uvparamwidget.h"
#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QFileDialog>
#include <QMessageBox>
#include <QPushButton>
#include <QLabel>
#include <QGroupBox>
#include <QCheckBox>
#include <QComboBox>

// 创建新CGAL-UV视图标签页
inline QWidget* createNewCGALUVTab(CGALGLWidget* leftWidget, UVParamWidget* rightWidget) {
    QWidget *tab = new QWidget;
    QHBoxLayout *layout = new QHBoxLayout(tab);
    
    // 创建分割器
    QSplitter *splitter = new QSplitter(Qt::Horizontal, tab);
    
    // 添加左视图（CGALGLWidget）
    splitter->addWidget(leftWidget);
    
    // 添加右视图（UVParamWidget）
    splitter->addWidget(rightWidget);
    
    // 设置初始分割比例（50%-50%）
    splitter->setSizes(QList<int>() << 500 << 500);
    
    // 允许用户调整分割器大小
    splitter->setHandleWidth(5);
    splitter->setStyleSheet(
        "QSplitter::handle {"
        "   background-color: #505050;"
        "}"
        "QSplitter::handle:hover {"
        "   background-color: #606060;"
        "}"
    );
    
    layout->addWidget(splitter);
    return tab;
}

// 创建左视图（CGAL）加载按钮
inline QWidget* createCGALViewLoadButton(CGALGLWidget* cgalWidget, QLabel* infoLabel, QWidget* mainWindow) {
    QPushButton *button = new QPushButton("Load OBJ to CGAL View");
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
    QObject::connect(button, &QPushButton::clicked, [cgalWidget, infoLabel, mainWindow]() {
        QString filePath = QFileDialog::getOpenFileName(
            mainWindow, "Open OBJ File for CGAL View", "", "OBJ Files (*.obj)");
        
        if (!filePath.isEmpty()) {
            cgalWidget->loadOBJ(filePath);
            QString fileName = QFileInfo(filePath).fileName();
            infoLabel->setText("CGAL View: " + fileName);
            mainWindow->setWindowTitle("New CGAL-UV View - " + fileName);
        }
    });
    return button;
}

// 创建右视图（UV）加载按钮
inline QWidget* createUVViewLoadButton(UVParamWidget* uvWidget, QLabel* infoLabel, QWidget* mainWindow) {
    QPushButton *button = new QPushButton("Load OBJ to UV View");
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
    QObject::connect(button, &QPushButton::clicked, [uvWidget, infoLabel, mainWindow]() {
        QString filePath = QFileDialog::getOpenFileName(
            mainWindow, "Open OBJ File for UV View", "", "OBJ Files (*.obj)");
        
        if (!filePath.isEmpty()) {
            uvWidget->loadOBJ(filePath);
            QString fileName = QFileInfo(filePath).fileName();
            bool hasUV = uvWidget->hasUVData();
            infoLabel->setText("UV View: " + fileName + (hasUV ? " (with UV)" : " (no UV)"));
            mainWindow->setWindowTitle("New CGAL-UV View - " + fileName);
        }
    });
    return button;
}

// 创建CGAL视图显示控制组
inline QGroupBox* createCGALViewDisplayGroup(CGALGLWidget* cgalWidget) {
    QGroupBox *group = new QGroupBox("CGAL View Options");
    group->setStyleSheet("QGroupBox { color: white; font-weight: bold; }");
    QVBoxLayout *layout = new QVBoxLayout(group);
    
    // 显示坐标轴复选框
    QCheckBox *axisCheckbox = new QCheckBox("Show Axis");
    axisCheckbox->setStyleSheet("color: white;");
    axisCheckbox->setChecked(false);
    QObject::connect(axisCheckbox, &QCheckBox::stateChanged, [cgalWidget](int state) {
        cgalWidget->setShowAxis(state == Qt::Checked);
    });
    layout->addWidget(axisCheckbox);
    
    // 显示线框叠加复选框
    QCheckBox *wireframeCheckbox = new QCheckBox("Show Wireframe Overlay");
    wireframeCheckbox->setStyleSheet("color: white;");
    wireframeCheckbox->setChecked(true);
    QObject::connect(wireframeCheckbox, &QCheckBox::stateChanged, [cgalWidget](int state) {
        cgalWidget->setShowWireframeOverlay(state == Qt::Checked);
    });
    layout->addWidget(wireframeCheckbox);
    
    // 隐藏面复选框
    QCheckBox *hideFacesCheckbox = new QCheckBox("Hide Faces (Wireframe Only)");
    hideFacesCheckbox->setStyleSheet("color: white;");
    hideFacesCheckbox->setChecked(false);
    QObject::connect(hideFacesCheckbox, &QCheckBox::stateChanged, [cgalWidget](int state) {
        cgalWidget->setHideFaces(state == Qt::Checked);
    });
    layout->addWidget(hideFacesCheckbox);
    
    // 渲染模式下拉框
    QLabel *renderModeLabel = new QLabel("Render Mode:");
    renderModeLabel->setStyleSheet("color: white;");
    layout->addWidget(renderModeLabel);
    
    QComboBox *renderModeCombo = new QComboBox;
    renderModeCombo->addItem("Blinn-Phong Shading");
    renderModeCombo->addItem("Flat Shading");
    renderModeCombo->setCurrentIndex(1); // 默认选择Flat Shading
    renderModeCombo->setStyleSheet(
        "QComboBox {"
        "   background-color: #505050;"
        "   color: white;"
        "   border: 1px solid #606060;"
        "   padding: 5px;"
        "}"
        "QComboBox::drop-down {"
        "   border: none;"
        "}"
        "QComboBox::down-arrow {"
        "   image: none;"
        "   border-left: 4px solid transparent;"
        "   border-right: 4px solid transparent;"
        "   border-top: 5px solid white;"
        "}"
        "QComboBox QAbstractItemView {"
        "   background-color: #505050;"
        "   color: white;"
        "   selection-background-color: #606060;"
        "}"
    );
    QObject::connect(renderModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
        [cgalWidget](int index) {
            cgalWidget->currentRenderMode = static_cast<CGALGLWidget::RenderMode>(index);
            cgalWidget->update();
        });
    layout->addWidget(renderModeCombo);
    
    return group;
}

// 创建UV视图显示控制组
inline QGroupBox* createUVViewDisplayGroup(UVParamWidget* uvWidget) {
    QGroupBox *group = new QGroupBox("UV View Options");
    group->setStyleSheet("QGroupBox { color: white; font-weight: bold; }");
    QVBoxLayout *layout = new QVBoxLayout(group);
    
    // 显示点的复选框
    QCheckBox *pointsCheckbox = new QCheckBox("Show Points");
    pointsCheckbox->setStyleSheet("color: white;");
    pointsCheckbox->setChecked(false);
    QObject::connect(pointsCheckbox, &QCheckBox::stateChanged, [uvWidget](int state) {
        uvWidget->setShowPoints(state == Qt::Checked);
    });
    layout->addWidget(pointsCheckbox);
    
    // 显示线框的复选框
    QCheckBox *wireframeCheckbox = new QCheckBox("Show Wireframe");
    wireframeCheckbox->setStyleSheet("color: white;");
    wireframeCheckbox->setChecked(false);
    QObject::connect(wireframeCheckbox, &QCheckBox::stateChanged, [uvWidget](int state) {
        uvWidget->setShowWireframe(state == Qt::Checked);
    });
    layout->addWidget(wireframeCheckbox);
    
    // 显示面的复选框
    QCheckBox *facesCheckbox = new QCheckBox("Show Faces");
    facesCheckbox->setStyleSheet("color: white;");
    facesCheckbox->setChecked(true);
    QObject::connect(facesCheckbox, &QCheckBox::stateChanged, [uvWidget](int state) {
        uvWidget->setShowFaces(state == Qt::Checked);
    });
    layout->addWidget(facesCheckbox);
    
    // 抗锯齿复选框
    QCheckBox *antialiasingCheckbox = new QCheckBox("Antialiasing");
    antialiasingCheckbox->setStyleSheet("color: white;");
    antialiasingCheckbox->setChecked(true);
    QObject::connect(antialiasingCheckbox, &QCheckBox::stateChanged, [uvWidget](int state) {
        uvWidget->setAntialiasing(state == Qt::Checked);
    });
    layout->addWidget(antialiasingCheckbox);
    
    return group;
}

// 创建视图控制按钮组
inline QGroupBox* createViewControlGroup(CGALGLWidget* cgalWidget, UVParamWidget* uvWidget) {
    QGroupBox *group = new QGroupBox("View Controls");
    group->setStyleSheet("QGroupBox { color: white; font-weight: bold; }");
    QVBoxLayout *layout = new QVBoxLayout(group);
    
    // 重置CGAL视图按钮
    QPushButton *resetCGALButton = new QPushButton("Reset CGAL View");
    resetCGALButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #505050;"
        "   color: white;"
        "   border: none;"
        "   padding: 8px 15px;"
        "   font-size: 14px;"
        "   border-radius: 5px;"
        "}"
        "QPushButton:hover { background-color: #606060; }"
    );
    QObject::connect(resetCGALButton, &QPushButton::clicked, [cgalWidget]() {
        cgalWidget->resetView();
    });
    layout->addWidget(resetCGALButton);
    
    // 居中CGAL视图按钮
    QPushButton *centerCGALButton = new QPushButton("Center CGAL View");
    centerCGALButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #505050;"
        "   color: white;"
        "   border: none;"
        "   padding: 8px 15px;"
        "   font-size: 14px;"
        "   border-radius: 5px;"
        "}"
        "QPushButton:hover { background-color: #606060; }"
    );
    QObject::connect(centerCGALButton, &QPushButton::clicked, [cgalWidget]() {
        cgalWidget->centerView();
    });
    layout->addWidget(centerCGALButton);
    
    // 清除UV视图按钮
    QPushButton *clearUVButton = new QPushButton("Clear UV View");
    clearUVButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #505050;"
        "   color: white;"
        "   border: none;"
        "   padding: 8px 15px;"
        "   font-size: 14px;"
        "   border-radius: 5px;"
        "}"
        "QPushButton:hover { background-color: #606060; }"
    );
    QObject::connect(clearUVButton, &QPushButton::clicked, [uvWidget]() {
        uvWidget->clearData();
    });
    layout->addWidget(clearUVButton);
    
    // 打印UV Mesh信息按钮
    QPushButton *printMeshInfoButton = new QPushButton("Print UV Mesh Info");
    printMeshInfoButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #505050;"
        "   color: white;"
        "   border: none;"
        "   padding: 8px 15px;"
        "   font-size: 14px;"
        "   border-radius: 5px;"
        "}"
        "QPushButton:hover { background-color: #606060; }"
    );
    QObject::connect(printMeshInfoButton, &QPushButton::clicked, [uvWidget]() {
        uvWidget->printMeshInfo();
    });
    layout->addWidget(printMeshInfoButton);
    
    return group;
}

// 创建新CGAL-UV视图控制面板
inline QWidget* createNewCGALUVControlPanel(CGALGLWidget* cgalWidget, UVParamWidget* uvWidget, 
                                           QLabel* leftInfoLabel, QLabel* rightInfoLabel, 
                                           QWidget* mainWindow) {
    QWidget *panel = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(panel);
    
    // 添加视图控制按钮组
    layout->addWidget(createViewControlGroup(cgalWidget, uvWidget));
    
    // 添加CGAL视图显示控制组
    layout->addWidget(createCGALViewDisplayGroup(cgalWidget));
    
    // 添加UV视图显示控制组
    layout->addWidget(createUVViewDisplayGroup(uvWidget));
    
    // 添加左视图加载按钮
    layout->addWidget(createCGALViewLoadButton(cgalWidget, leftInfoLabel, mainWindow));
    
    // 添加右视图加载按钮
    layout->addWidget(createUVViewLoadButton(uvWidget, rightInfoLabel, mainWindow));
    
    layout->addStretch();
    return panel;
}

#endif // NEW_CGAL_UV_TAB_H