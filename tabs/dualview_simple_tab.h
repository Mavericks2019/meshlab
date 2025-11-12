// dualview_simple_tab.h
#ifndef DUALVIEW_SIMPLE_TAB_H
#define DUALVIEW_SIMPLE_TAB_H

#include "../glwidget/baseglwidget.h"
#include "../glwidget/uvparamwidget.h"
#include "basic_tab.h"
#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QPushButton>
#include <QLabel>
#include <QGroupBox>
#include <QTabWidget>
#include <QFileDialog>
#include <QFileInfo>

// 创建简单双视图标签页
QWidget* createDualViewSimpleTab(BaseGLWidget* leftWidget, UVParamWidget* rightWidget) {
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

// 创建简单双视图控制面板
QWidget* createDualViewSimpleControlPanel(BaseGLWidget* leftWidget, UVParamWidget* rightWidget, QLabel* leftInfoLabel, QLabel* rightInfoLabel, QWidget* mainWindow) {
    QWidget *panel = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(panel);
    
    // 创建标签页来分隔左右视图的控制
    QTabWidget *controlTabs = new QTabWidget;
    
    // 左侧视图控制面板 (使用BaseGLWidget的控制)
    QWidget *leftControlPanel = createBasicControlPanel(leftWidget, leftInfoLabel, mainWindow);
    controlTabs->addTab(leftControlPanel, "Left View Controls");
    
    // 右侧视图控制面板 - 简化版本，只显示基本信息
    QWidget *rightControlPanel = new QWidget;
    QVBoxLayout *rightLayout = new QVBoxLayout(rightControlPanel);
    rightLayout->setAlignment(Qt::AlignTop);
    
    // 右侧视图信息显示
    QGroupBox *rightInfoGroup = new QGroupBox("Right View Information");
    QVBoxLayout *rightInfoLayout = new QVBoxLayout(rightInfoGroup);
    rightInfoLayout->addWidget(rightInfoLabel);
    rightLayout->addWidget(rightInfoGroup);
    
    // 添加右侧视图重置按钮
    QPushButton *resetRightButton = new QPushButton("Reset Right View");
    resetRightButton->setStyleSheet(
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
    QObject::connect(resetRightButton, &QPushButton::clicked, [rightWidget]() {
        // UVParamWidget 没有 resetView 方法，这里可以添加或忽略
        rightWidget->update();
    });
    rightLayout->addWidget(resetRightButton);
    
    rightLayout->addStretch();
    
    controlTabs->addTab(rightControlPanel, "Right View Controls");
    
    layout->addWidget(controlTabs);
    
    // 添加同步加载按钮
    QPushButton *syncLoadButton = new QPushButton("Load OBJ File (Left View Only)");
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
            // 只加载到左侧视图
            leftWidget->loadOBJ(filePath);
            leftInfoLabel->setText("Model loaded (Left View): " + QFileInfo(filePath).fileName());
            
            // 右侧视图保持白色正方形
            rightInfoLabel->setText("White Square View - Ready for extension");
            
            mainWindow->setWindowTitle("OBJ Viewer - " + QFileInfo(filePath).fileName() + " (Simple Dual View)");
        }
    });
    layout->addWidget(syncLoadButton);
    
    layout->addStretch();
    return panel;
}

#endif // DUALVIEW_SIMPLE_TAB_H