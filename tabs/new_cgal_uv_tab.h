// tabs/new_cgal_uv_tab.h
#pragma once
#ifndef NEW_CGAL_UV_TAB_H
#define NEW_CGAL_UV_TAB_H

#include "../glwidget/cgalglwidget.h"
#include "../glwidget/uvparamwidget.h"
#include "../glwidget/arapglwidget.h"  // 新增：包含ARAPGLWidget
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
#include <QProgressDialog>
#include <QElapsedTimer>
#include <QTimer>        // 添加QTimer头文件
#include <QEventLoop>    // 添加QEventLoop头文件

// 创建新CGAL-UV视图标签页 - 修改为使用ARAPGLWidget
inline QWidget* createNewCGALUVTab(ARAPGLWidget* leftWidget, UVParamWidget* rightWidget) {
    QWidget *tab = new QWidget;
    QHBoxLayout *layout = new QHBoxLayout(tab);
    
    // 创建分割器
    QSplitter *splitter = new QSplitter(Qt::Horizontal, tab);
    
    // 添加左视图（ARAPGLWidget）
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

// 创建左视图（ARAP）加载按钮
inline QWidget* createARAPViewLoadButton(ARAPGLWidget* arapWidget, QLabel* infoLabel, QWidget* mainWindow) {
    QPushButton *button = new QPushButton("Load OBJ to ARAP View");
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
    QObject::connect(button, &QPushButton::clicked, [arapWidget, infoLabel, mainWindow]() {
        QString filePath = QFileDialog::getOpenFileName(
            mainWindow, "Open OBJ File for ARAP View", "", "OBJ Files (*.obj)");
        
        if (!filePath.isEmpty()) {
            arapWidget->loadOBJ(filePath);
            QString fileName = QFileInfo(filePath).fileName();
            infoLabel->setText("ARAP View: " + fileName);
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

// 创建ARAP参数化按钮
inline QWidget* createARAPParameterizationButton(ARAPGLWidget* arapWidget, UVParamWidget* uvWidget, 
                                                QLabel* leftInfoLabel, QLabel* rightInfoLabel) {
    QPushButton *button = new QPushButton("Perform ARAP Parameterization");
    button->setStyleSheet(
        "QPushButton {"
        "   background-color: #4CAF50;"
        "   color: white;"
        "   border: none;"
        "   padding: 12px 24px;"
        "   font-size: 16px;"
        "   font-weight: bold;"
        "   border-radius: 5px;"
        "}"
        "QPushButton:hover { background-color: #45a049; }"
        "QPushButton:disabled { background-color: #808080; color: #CCCCCC; }"
    );
    
    // 初始禁用按钮，直到模型加载
    button->setEnabled(false);
    
    // 监听模型加载状态
    // 注意：这里需要实际连接到模型加载的信号，简化处理：通过检查按钮使能状态
    QObject::connect(arapWidget, &ARAPGLWidget::destroyed, button, [button]() {
        button->setEnabled(false);
    });
    
    QObject::connect(button, &QPushButton::clicked, [arapWidget, uvWidget, leftInfoLabel, rightInfoLabel, button]() {
        if (!arapWidget->canPerformARAP()) {
            QMessageBox::warning(nullptr, "Cannot Perform ARAP", 
                               "Please load a valid mesh first.");
            return;
        }
        
        // 显示进度对话框
        QProgressDialog progress("Performing ARAP Parameterization...", "Cancel", 0, 100, nullptr);
        progress.setWindowModality(Qt::WindowModal);
        progress.setMinimumDuration(0);
        progress.setValue(0);
        
        // 模拟进度更新（实际进度应该在ARAP函数中更新）
        QTimer progressTimer;
        int progressValue = 0;
        QObject::connect(&progressTimer, &QTimer::timeout, [&progress, &progressValue]() {
            if (progressValue < 90) {
                progressValue += 10;
                progress.setValue(progressValue);
            }
        });
        
        progressTimer.start(100);
        
        // 直接执行ARAP参数化
        bool success = arapWidget->performARAPParameterization();
        
        progressTimer.stop();
        progress.setValue(100);
        
        if (success) {
            leftInfoLabel->setText("ARAP View: Parameterized mesh");
            rightInfoLabel->setText("UV View: Ready for parameterization result");
            
            QMessageBox::information(nullptr, "Success", 
                                   "ARAP parameterization completed successfully.");
        }
    });
    
    // 定期检查是否可以执行ARAP
    QTimer *checkTimer = new QTimer(button);
    QObject::connect(checkTimer, &QTimer::timeout, [arapWidget, button]() {
        button->setEnabled(arapWidget->canPerformARAP());
    });
    checkTimer->start(1000); // 每秒检查一次
    
    return button;
}

// 创建ARAP视图显示控制组
inline QGroupBox* createARAPViewDisplayGroup(ARAPGLWidget* arapWidget) {
    QGroupBox *group = new QGroupBox("ARAP View Options");
    group->setStyleSheet("QGroupBox { color: white; font-weight: bold; }");
    QVBoxLayout *layout = new QVBoxLayout(group);
    
    // 显示坐标轴复选框
    QCheckBox *axisCheckbox = new QCheckBox("Show Axis");
    axisCheckbox->setStyleSheet("color: white;");
    axisCheckbox->setChecked(false);
    QObject::connect(axisCheckbox, &QCheckBox::stateChanged, [arapWidget](int state) {
        arapWidget->setShowAxis(state == Qt::Checked);
    });
    layout->addWidget(axisCheckbox);
    
    // 显示线框叠加复选框
    QCheckBox *wireframeCheckbox = new QCheckBox("Show Wireframe Overlay");
    wireframeCheckbox->setStyleSheet("color: white;");
    wireframeCheckbox->setChecked(true);
    QObject::connect(wireframeCheckbox, &QCheckBox::stateChanged, [arapWidget](int state) {
        arapWidget->setShowWireframeOverlay(state == Qt::Checked);
    });
    layout->addWidget(wireframeCheckbox);
    
    // 隐藏面复选框
    QCheckBox *hideFacesCheckbox = new QCheckBox("Hide Faces (Wireframe Only)");
    hideFacesCheckbox->setStyleSheet("color: white;");
    hideFacesCheckbox->setChecked(false);
    QObject::connect(hideFacesCheckbox, &QCheckBox::stateChanged, [arapWidget](int state) {
        arapWidget->setHideFaces(state == Qt::Checked);
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
        [arapWidget](int index) {
            arapWidget->currentRenderMode = static_cast<ARAPGLWidget::RenderMode>(index);
            arapWidget->update();
        });
    layout->addWidget(renderModeCombo);
    
    // ARAP参数化选项
    QLabel *arapOptionsLabel = new QLabel("ARAP Options:");
    arapOptionsLabel->setStyleSheet("color: white; font-weight: bold; margin-top: 10px;");
    layout->addWidget(arapOptionsLabel);
    
    // 使用固定边界复选框
    QCheckBox *fixedBorderCheckbox = new QCheckBox("Use Fixed Border");
    fixedBorderCheckbox->setStyleSheet("color: white;");
    fixedBorderCheckbox->setChecked(true);
    QObject::connect(fixedBorderCheckbox, &QCheckBox::stateChanged, [arapWidget](int state) {
        arapWidget->setUseFixedBorder(state == Qt::Checked);
    });
    layout->addWidget(fixedBorderCheckbox);
    
    // 使用迭代求解器复选框
    QCheckBox *iterativeSolverCheckbox = new QCheckBox("Use Iterative Solver");
    iterativeSolverCheckbox->setStyleSheet("color: white;");
    iterativeSolverCheckbox->setChecked(true);
    QObject::connect(iterativeSolverCheckbox, &QCheckBox::stateChanged, [arapWidget](int state) {
        arapWidget->setUseIterativeSolver(state == Qt::Checked);
    });
    layout->addWidget(iterativeSolverCheckbox);
    
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
inline QGroupBox* createViewControlGroup(ARAPGLWidget* arapWidget, UVParamWidget* uvWidget) {
    QGroupBox *group = new QGroupBox("View Controls");
    group->setStyleSheet("QGroupBox { color: white; font-weight: bold; }");
    QVBoxLayout *layout = new QVBoxLayout(group);
    
    // 重置ARAP视图按钮
    QPushButton *resetARAPButton = new QPushButton("Reset ARAP View");
    resetARAPButton->setStyleSheet(
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
    QObject::connect(resetARAPButton, &QPushButton::clicked, [arapWidget]() {
        arapWidget->resetView();
    });
    layout->addWidget(resetARAPButton);
    
    // 居中ARAP视图按钮
    QPushButton *centerARAPButton = new QPushButton("Center ARAP View");
    centerARAPButton->setStyleSheet(
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
    QObject::connect(centerARAPButton, &QPushButton::clicked, [arapWidget]() {
        arapWidget->centerView();
    });
    layout->addWidget(centerARAPButton);
    
    // 清除参数化按钮
    QPushButton *clearParamButton = new QPushButton("Clear Parameterization");
    clearParamButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #FF5722;"
        "   color: white;"
        "   border: none;"
        "   padding: 8px 15px;"
        "   font-size: 14px;"
        "   border-radius: 5px;"
        "}"
        "QPushButton:hover { background-color: #FF7043; }"
        "QPushButton:disabled { background-color: #808080; color: #CCCCCC; }"
    );
    QObject::connect(clearParamButton, &QPushButton::clicked, [arapWidget]() {
        arapWidget->clearParameterization();
    });
    layout->addWidget(clearParamButton);
    
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
inline QWidget* createNewCGALUVControlPanel(ARAPGLWidget* arapWidget, UVParamWidget* uvWidget, 
                                           QLabel* leftInfoLabel, QLabel* rightInfoLabel, 
                                           QWidget* mainWindow) {
    QWidget *panel = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(panel);
    
    // 添加ARAP参数化按钮
    layout->addWidget(createARAPParameterizationButton(arapWidget, uvWidget, leftInfoLabel, rightInfoLabel));
    
    // 添加视图控制按钮组
    layout->addWidget(createViewControlGroup(arapWidget, uvWidget));
    
    // 添加ARAP视图显示控制组
    layout->addWidget(createARAPViewDisplayGroup(arapWidget));
    
    // 添加UV视图显示控制组
    layout->addWidget(createUVViewDisplayGroup(uvWidget));
    
    // 添加左视图加载按钮
    layout->addWidget(createARAPViewLoadButton(arapWidget, leftInfoLabel, mainWindow));
    
    // 添加右视图加载按钮
    layout->addWidget(createUVViewLoadButton(uvWidget, rightInfoLabel, mainWindow));
    
    layout->addStretch();
    return panel;
}

#endif // NEW_CGAL_UV_TAB_H