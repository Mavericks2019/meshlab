#ifndef UVPARAM_TAB_H
#define UVPARAM_TAB_H

#include "../glwidget/uvparamwidget.h"
#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QPushButton>
#include <QLabel>
#include <QGroupBox>
#include <QCheckBox>

// 创建UV参数化标签页
QWidget* createUVParamTab(UVParamWidget* uvWidget) {
    QWidget *tab = new QWidget;
    QHBoxLayout *layout = new QHBoxLayout(tab);
    layout->addWidget(uvWidget);
    return tab;
}

// 创建OBJ文件加载按钮（UV参数化版）
QWidget* createUVParamModelLoadButton(UVParamWidget* uvWidget, QLabel* infoLabel, QWidget* mainWindow) {
    QPushButton *button = new QPushButton("Load OBJ File (UV Parameterization)");
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
            mainWindow, "Open OBJ File", "", "OBJ Files (*.obj)");
        
        if (!filePath.isEmpty()) {
            uvWidget->loadOBJ(filePath);
            QString status = uvWidget->hasUVData() ? 
                "Model loaded with UV data: " : "Model loaded (no UV data): ";
            infoLabel->setText(status + QFileInfo(filePath).fileName());
            mainWindow->setWindowTitle("UV Parameterization - " + QFileInfo(filePath).fileName());
        }
    });
    return button;
}

// 创建显示控制组
QGroupBox* createUVDisplayControlGroup(UVParamWidget* uvWidget) {
    QGroupBox *group = new QGroupBox("Display Options");
    group->setStyleSheet("QGroupBox { color: white; font-weight: bold; }");
    QVBoxLayout *layout = new QVBoxLayout(group);
    
    // 显示点的复选框 - 默认不选中
    QCheckBox *pointsCheckbox = new QCheckBox("Show Points");
    pointsCheckbox->setStyleSheet("color: white;");
    pointsCheckbox->setChecked(false);  // 改为false
    QObject::connect(pointsCheckbox, &QCheckBox::stateChanged, [uvWidget](int state) {
        uvWidget->setShowPoints(state == Qt::Checked);
    });
    layout->addWidget(pointsCheckbox);
    
    // 显示线框的复选框 - 默认不选中
    QCheckBox *wireframeCheckbox = new QCheckBox("Show Wireframe");
    wireframeCheckbox->setStyleSheet("color: white;");
    wireframeCheckbox->setChecked(false);  // 改为false
    QObject::connect(wireframeCheckbox, &QCheckBox::stateChanged, [uvWidget](int state) {
        uvWidget->setShowWireframe(state == Qt::Checked);
    });
    layout->addWidget(wireframeCheckbox);
    
    // 显示面的复选框 - 默认选中
    QCheckBox *facesCheckbox = new QCheckBox("Show Faces");
    facesCheckbox->setStyleSheet("color: white;");
    facesCheckbox->setChecked(true);  // 保持true
    QObject::connect(facesCheckbox, &QCheckBox::stateChanged, [uvWidget](int state) {
        uvWidget->setShowFaces(state == Qt::Checked);
    });
    layout->addWidget(facesCheckbox);
    
    return group;
}

// 创建UV参数化控制面板
QWidget* createUVParamControlPanel(UVParamWidget* uvWidget, QLabel* infoLabel, QWidget* mainWindow) {
    QWidget *panel = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(panel);
    
    // 添加显示控制组
    layout->addWidget(createUVDisplayControlGroup(uvWidget));
    
    // 添加控件组
    layout->addWidget(createUVParamModelLoadButton(uvWidget, infoLabel, mainWindow));
    
    // 清除按钮
    QPushButton *clearButton = new QPushButton("Clear");
    clearButton->setStyleSheet(
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
    QObject::connect(clearButton, &QPushButton::clicked, [uvWidget, infoLabel]() {
        uvWidget->clearData();
        infoLabel->setText("No model loaded");
    });
    layout->addWidget(clearButton);
    
    layout->addStretch();
    return panel;
}

#endif // UVPARAM_TAB_H