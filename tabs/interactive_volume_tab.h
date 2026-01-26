// interactive_volume_tab.h
#pragma once
#ifndef INTERACTIVE_VOLUME_TAB_H
#define INTERACTIVE_VOLUME_TAB_H

// InteractiveWidget的前向声明，假设已经在其他地方定义
class InteractiveWidget;
#include "InteractiveWidget.h"
#
#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QPushButton>
#include <QLabel>
#include <QGroupBox>
#include <QFileInfo>
#include <QString>
#include <QByteArray>
#include <QSlider>

// 创建Interactive Volume标签页
inline QWidget* createInteractiveVolumeTab(InteractiveWidget* interactiveWidget) {
    std::cout << "1" << std::endl;
    QWidget *tab = new QWidget;
    QHBoxLayout *layout = new QHBoxLayout(tab);
    layout->addWidget(interactiveWidget);
    return tab;
}

inline QWidget* createVolumeModelLoadButton(InteractiveWidget* interactiveWidget, QLabel* infoLabel, QWidget* mainWindow) {
    std::cout << "2" << std::endl;
    QPushButton *button = new QPushButton("Load OBJ File (OpenMesh)");
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
    QObject::connect(button, &QPushButton::clicked, [interactiveWidget, infoLabel, mainWindow]() {
        QString filePath = QFileDialog::getOpenFileName(
            mainWindow, "Open OVM File", "", "OVM Files (*.ovm)");
        
        if (!filePath.isEmpty()) {
            interactiveWidget->openMesh(filePath.toLocal8Bit());
            infoLabel->setText("Model loaded (OpenMesh): " + QFileInfo(filePath).fileName());
            mainWindow->setWindowTitle("OBJ Viewer - " + QFileInfo(filePath).fileName() + " (OpenMesh)");
        }
    });
    return button;
}

// 创建Interactive Volume控制面板
inline QWidget* createInteractiveVolumeControlPanel(InteractiveWidget* interactiveWidget, QLabel* infoLabel, QWidget* mainWindow) {
    QWidget *panel = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(panel);
    
    // 添加控件组
    layout->addWidget(createVolumeModelLoadButton(interactiveWidget, infoLabel, mainWindow));
    
    // 添加X轴滑块
    QLabel *xSliderLabel = new QLabel("X : ");
    QSlider *XSlider = new QSlider(Qt::Horizontal);
    XSlider->setMinimum(0);
    XSlider->setMaximum(100);
    XSlider->setValue(0);
    xSliderLabel->setBuddy(XSlider);
    layout->addWidget(xSliderLabel);
    layout->addWidget(XSlider);
    
    // 添加Y轴滑块
    QLabel *ySliderLabel = new QLabel("Y : ");
    QSlider *YSlider = new QSlider(Qt::Horizontal);
    YSlider->setMinimum(0);
    YSlider->setMaximum(100);
    YSlider->setValue(0);
    ySliderLabel->setBuddy(YSlider);
    layout->addWidget(ySliderLabel);
    layout->addWidget(YSlider);
    
    // 添加Z轴滑块
    QLabel *zSliderLabel = new QLabel("Z : ");
    QSlider *ZSlider = new QSlider(Qt::Horizontal);
    ZSlider->setMinimum(0);
    ZSlider->setMaximum(100);
    ZSlider->setValue(0);
    zSliderLabel->setBuddy(ZSlider);
    layout->addWidget(zSliderLabel);
    layout->addWidget(ZSlider);
    
    // 连接滑块信号
    QObject::connect(XSlider, SIGNAL(valueChanged(int)), interactiveWidget, SLOT(xSliderShow(int)));
    QObject::connect(YSlider, SIGNAL(valueChanged(int)), interactiveWidget, SLOT(ySliderShow(int)));
    QObject::connect(ZSlider, SIGNAL(valueChanged(int)), interactiveWidget, SLOT(zSliderShow(int)));
    
    layout->addStretch();
    return panel;
}
#endif // INTERACTIVE_VOLUME_TAB_H