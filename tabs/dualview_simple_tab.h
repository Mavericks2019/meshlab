// dualview_simple_tab.h
#ifndef DUALVIEW_SIMPLE_TAB_H
#define DUALVIEW_SIMPLE_TAB_H

#include "../glwidget/baseglwidget.h"
#include "../glwidget/simplesquarewidget.h"  // 修改为新的SimpleSquareWidget
#include "basic_tab.h"
#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QPushButton>
#include <QLabel>
#include <QGroupBox>
#include <QCheckBox>
#include <QRadioButton>
#include <QButtonGroup>
#include <QTabWidget>
#include <QFileDialog>
#include <QFileInfo>

// 创建简单双视图标签页
QWidget* createDualViewSimpleTab(BaseGLWidget* leftWidget, SimpleSquareWidget* rightWidget) {
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
QWidget* createDualViewSimpleControlPanel(BaseGLWidget* leftWidget, SimpleSquareWidget* rightWidget, QLabel* leftInfoLabel, QLabel* rightInfoLabel, QWidget* mainWindow) {
    QWidget *panel = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(panel);
    
    // 创建标签页来分隔左右视图的控制
    QTabWidget *controlTabs = new QTabWidget;
    
    // 左侧视图控制面板 (使用BaseGLWidget的控制)
    QWidget *leftControlPanel = createBasicControlPanel(leftWidget, leftInfoLabel, mainWindow);
    controlTabs->addTab(leftControlPanel, "Left View Controls");
    
    // 右侧视图控制面板 - 简化版本
    QWidget *rightControlPanel = new QWidget;
    QVBoxLayout *rightLayout = new QVBoxLayout(rightControlPanel);
    rightLayout->setAlignment(Qt::AlignTop);
    
    // 右侧视图信息显示
    QGroupBox *rightInfoGroup = new QGroupBox("Right View Information");
    QVBoxLayout *rightInfoLayout = new QVBoxLayout(rightInfoGroup);
    rightInfoLayout->addWidget(rightInfoLabel);
    rightLayout->addWidget(rightInfoGroup);
    
    // 添加右侧视图颜色设置
    QGroupBox *rightColorGroup = new QGroupBox("Square Settings");
    QVBoxLayout *rightColorLayout = new QVBoxLayout(rightColorGroup);
    
    // 背景颜色按钮
    QPushButton *bgColorButton = new QPushButton("Change Background Color");
    bgColorButton->setStyleSheet(
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
    QObject::connect(bgColorButton, &QPushButton::clicked, [rightWidget]() {
        QColor color = QColorDialog::getColor(QColor(0, 85, 127), nullptr, "Select Background Color");
        if (color.isValid()) {
            rightWidget->setBackgroundColor(color);
        }
    });
    rightColorLayout->addWidget(bgColorButton);
    
    // 正方形颜色按钮
    QPushButton *squareColorButton = new QPushButton("Change Square Color");
    squareColorButton->setStyleSheet(
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
    QObject::connect(squareColorButton, &QPushButton::clicked, [rightWidget]() {
        QColor color = QColorDialog::getColor(Qt::white, nullptr, "Select Square Color");
        if (color.isValid()) {
            rightWidget->setSquareColor(color);
        }
    });
    rightColorLayout->addWidget(squareColorButton);
    
    rightLayout->addWidget(rightColorGroup);
    
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
        // 重置为默认颜色
        rightWidget->setBackgroundColor(QColor(0, 85, 127));
        rightWidget->setSquareColor(Qt::white);
    });
    rightLayout->addWidget(resetRightButton);
    
    rightLayout->addStretch();
    
    controlTabs->addTab(rightControlPanel, "Right View Controls");
    
    layout->addWidget(controlTabs);
    
    // 添加参数化选项组
    QGroupBox *parameterizationGroup = new QGroupBox("Parameterization Options");
    QVBoxLayout *paramLayout = new QVBoxLayout(parameterizationGroup);
    
    // 创建单选按钮组
    QButtonGroup *paramMethodGroup = new QButtonGroup(parameterizationGroup);
    
    // 极小曲面单选按钮
    QRadioButton *minimalSurfaceRadio = new QRadioButton("Minimal Surface");
    minimalSurfaceRadio->setChecked(true); // 默认选中
    minimalSurfaceRadio->setStyleSheet("color: white;");
    paramMethodGroup->addButton(minimalSurfaceRadio);
    paramLayout->addWidget(minimalSurfaceRadio);
    
    // 可以在这里添加更多单选按钮
    // QRadioButton *anotherMethodRadio = new QRadioButton("Another Method");
    // anotherMethodRadio->setStyleSheet("color: white;");
    // paramMethodGroup->addButton(anotherMethodRadio);
    // paramLayout->addWidget(anotherMethodRadio);
    
    layout->addWidget(parameterizationGroup);
    
    // 添加执行参数化按钮
    QPushButton *performParamButton = new QPushButton("Perform Parameterization");
    performParamButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #5050A0;"
        "   color: white;"
        "   border: none;"
        "   padding: 10px 20px;"
        "   font-size: 16px;"
        "   border-radius: 5px;"
        "}"
        "QPushButton:hover { background-color: #6060B0; }"
        "QPushButton:disabled {"
        "   background-color: #404040;"
        "   color: #808080;"
        "}"
    );
    
    // 初始时禁用参数化按钮（因为没有加载模型）
    performParamButton->setEnabled(false);
    
    // 连接按钮点击事件（暂时为空实现）
    QObject::connect(performParamButton, &QPushButton::clicked, [minimalSurfaceRadio, leftWidget, rightWidget, rightInfoLabel]() {
        // 获取当前选中的参数化方法
        if (minimalSurfaceRadio->isChecked()) {
            // 执行极小曲面参数化
            QMessageBox::information(nullptr, "Parameterization", 
                                   "Minimal Surface parameterization selected.\n"
                                   "Functionality to be implemented.");
            
            // 这里将来会实现实际的参数化逻辑
            // 暂时只是显示消息
            rightInfoLabel->setText("Minimal Surface Parameterization - Ready");
        }
        // 可以添加其他方法的处理
    });
    
    layout->addWidget(performParamButton);
    
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
    QObject::connect(syncLoadButton, &QPushButton::clicked, [leftWidget, rightWidget, leftInfoLabel, rightInfoLabel, mainWindow, performParamButton]() {
        QString filePath = QFileDialog::getOpenFileName(
            mainWindow, "Open OBJ File", "", "OBJ Files (*.obj)");
        
        if (!filePath.isEmpty()) {
            // 只加载到左侧视图
            leftWidget->loadOBJ(filePath);
            leftInfoLabel->setText("Model loaded (Left View): " + QFileInfo(filePath).fileName());
            
            // 右侧视图保持白色正方形
            rightInfoLabel->setText("White Square View - Ready for parameterization");
            
            // 启用参数化按钮，因为现在有模型了
            performParamButton->setEnabled(true);
            
            mainWindow->setWindowTitle("OBJ Viewer - " + QFileInfo(filePath).fileName() + " (Simple Dual View)");
        }
    });
    layout->addWidget(syncLoadButton);
    
    layout->addStretch();
    return panel;
}

#endif // DUALVIEW_SIMPLE_TAB_H