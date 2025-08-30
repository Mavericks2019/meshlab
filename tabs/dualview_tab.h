// dualview_tab.h
#ifndef DUALVIEW_TAB_H
#define DUALVIEW_TAB_H

#include "../glwidget/baseglwidget.h"
#include "../glwidget/uvparamwidget.h"
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

// 创建双视图标签页
QWidget* createDualViewTab(BaseGLWidget* leftWidget, UVParamWidget* rightWidget) {
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

// 创建双视图控制面板
QWidget* createDualViewControlPanel(BaseGLWidget* leftWidget, UVParamWidget* rightWidget, QLabel* leftInfoLabel, QLabel* rightInfoLabel, QWidget* mainWindow) {
    QWidget *panel = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(panel);
    
    // 创建标签页来分隔左右视图的控制
    QTabWidget *controlTabs = new QTabWidget;
    
    // 左侧视图控制面板 (使用BaseGLWidget的控制)
    QWidget *leftControlPanel = createBasicControlPanel(leftWidget, leftInfoLabel, mainWindow);
    controlTabs->addTab(leftControlPanel, "Left View Controls");
    
    // 右侧视图控制面板 (使用UVParamWidget的控制)
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
            
            mainWindow->setWindowTitle("OBJ Viewer - " + QFileInfo(filePath).fileName() + " (Dual View)");
        }
    });
    layout->addWidget(syncLoadButton);
    
    layout->addStretch();
    return panel;
}

#endif // DUALVIEW_TAB_H