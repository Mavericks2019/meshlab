// dualview_extended_tab.h
#ifndef DUALVIEW_EXTENDED_TAB_H
#define DUALVIEW_EXTENDED_TAB_H

#include "../glwidget/baseglwidget.h"
#include "../glwidget/uvparamwidget_extended.h"  // 使用扩展版本
#include "basic_tab.h"
#include "uvparam_tab.h"
#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QPushButton>
#include <QLabel>
#include <QGroupBox>
#include <QCheckBox>
#include <QRadioButton>
#include <QFileDialog>
#include <QFileInfo>
#include <QTabWidget>
#include <QTextEdit>
#include <QMessageBox>
#include <algorithm> // 添加 algorithm 头文件

// 创建扩展双视图标签页
QWidget* createDualViewExtendedTab(BaseGLWidget* leftWidget, UVParamWidgetExtended* rightWidget) {
    QWidget *tab = new QWidget;
    QHBoxLayout *layout = new QHBoxLayout(tab);
    
    // 使用分割器创建左右视图
    QSplitter *splitter = new QSplitter(Qt::Horizontal);
    
    // 添加左右视图
    splitter->addWidget(leftWidget);
    splitter->addWidget(rightWidget);
    
    // 设置初始分割比例
    splitter->setSizes(QList<int>() << 500 << 500);
    
    layout->addWidget(splitter);
    
    return tab;
}

// 创建扩展双视图控制面板
QWidget* createDualViewExtendedControlPanel(BaseGLWidget* leftWidget, UVParamWidgetExtended* rightWidget, QLabel* leftInfoLabel, QLabel* rightInfoLabel, QWidget* mainWindow) {
    QWidget *panel = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(panel);
    
    // 创建标签页来分隔左右视图的控制
    QTabWidget *controlTabs = new QTabWidget;
    
    // 左侧视图控制面板 (使用BaseGLWidget的控制)
    QWidget *leftControlPanel = createBasicControlPanel(leftWidget, leftInfoLabel, mainWindow);
    controlTabs->addTab(leftControlPanel, "Left View Controls");
    
    // 右侧视图控制面板 (使用UVParamWidgetExtended的控制)
    QWidget *rightControlPanel = createUVParamControlPanel(rightWidget, rightInfoLabel, mainWindow);
    controlTabs->addTab(rightControlPanel, "Right View Controls");
    
    layout->addWidget(controlTabs);
    
    // 添加同步加载按钮
    QPushButton *syncLoadButton = new QPushButton("Load OBJ File (Both Views)");
    syncLoadButton->setStyleSheet(
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
    QObject::connect(syncLoadButton, &QPushButton::clicked, [leftWidget, rightWidget, leftInfoLabel, rightInfoLabel, mainWindow]() {
        QString filePath = QFileDialog::getOpenFileName(
            mainWindow, "Open OBJ File", "", "OBJ Files (*.obj)");
        
        if (!filePath.isEmpty()) {
            // 加载到左侧视图
            leftWidget->loadOBJ(filePath);
            leftInfoLabel->setText("Model loaded (Left View): " + QFileInfo(filePath).fileName());
            
            // 加载到右侧视图
            rightWidget->loadOBJ(filePath);
            QString status = rightWidget->hasUVData() ? 
                "Model loaded with UV data (Right View): " : "Model loaded (no UV data, Right View): ";
            rightInfoLabel->setText(status + QFileInfo(filePath).fileName());
            
            mainWindow->setWindowTitle("OBJ Viewer - " + QFileInfo(filePath).fileName() + " (Extended Dual View)");
        }
    });
    layout->addWidget(syncLoadButton);
    
    // 添加初始化按钮
    QPushButton *initButton = new QPushButton("Initialize UV Parameters");
    initButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #5050A0;"
        "   color: white;"
        "   border: none;"
        "   padding: 10px 20px;"
        "   font-size: 16px;"
        "   border-radius: 5px;"
        "}"
        "QPushButton:hover { background-color: #6060B0; }"
    );
    QObject::connect(initButton, &QPushButton::clicked, [rightWidget, rightInfoLabel]() {
        try {
            // 执行初始化
            rightWidget->init();
            // 获取new_mesh和new_para的前10个面数据
            QString output = "Initialization completed.\n\n";
            
            // 获取new_mesh的前10个面
            output += "new_mesh first 10 faces:\n";
            int faceCount = std::min(10, static_cast<int>(rightWidget->getNewMesh().n_faces()));
            for (int i = 0; i < faceCount; i++) {
                auto face = rightWidget->getNewMesh().face_handle(i);
                output += QString("Face %1: ").arg(i);
                // 使用 const 版本的迭代器
                for (auto fv_it = rightWidget->getNewMesh().cfv_begin(face); fv_it != rightWidget->getNewMesh().cfv_end(face); ++fv_it) {
                    output += QString("v%1 ").arg(fv_it->idx());
                }
                output += "\n";
            }
            
            // 获取new_para的前10个面
            output += "\nnew_para first 10 faces:\n";
            faceCount = std::min(10, static_cast<int>(rightWidget->getNewPara().n_faces()));
            for (int i = 0; i < faceCount; i++) {
                auto face = rightWidget->getNewPara().face_handle(i);
                output += QString("Face %1: ").arg(i);
                // 使用 const 版本的迭代器
                for (auto fv_it = rightWidget->getNewPara().cfv_begin(face); fv_it != rightWidget->getNewPara().cfv_end(face); ++fv_it) {
                    output += QString("v%1 ").arg(fv_it->idx());
                }
                output += "\n";
            }
            
            // 显示结果
            QMessageBox::information(nullptr, "Initialization Result", output);
            rightInfoLabel->setText("UV Parameters Initialized");
            
        } catch (const std::exception& e) {
            QMessageBox::critical(nullptr, "Initialization Error", 
                                 QString("Error during initialization: %1").arg(e.what()));
        }
    });
    layout->addWidget(initButton);
    
    layout->addStretch();
    return panel;
}

#endif // DUALVIEW_EXTENDED_TAB_H