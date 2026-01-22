// relativistic_tab.h
#pragma once
#ifndef RELATIVISTIC_TAB_H
#define RELATIVISTIC_TAB_H

#include "../glwidget/relativisticwidget.h"
#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QPushButton>
#include <QLabel>
#include <QGroupBox>

// 创建Relativistic标签页
inline QWidget* createRelativisticTab(RelativisticGLWidget* glWidget) {
    QWidget *tab = new QWidget;
    QHBoxLayout *layout = new QHBoxLayout(tab);
    layout->addWidget(glWidget);
    return tab;
}

// 创建OBJ文件加载按钮（Relativistic版）
inline QWidget* createRelativisticModelLoadButton(RelativisticGLWidget* glWidget, QLabel* infoLabel, QWidget* mainWindow) {
    QPushButton *button = new QPushButton("Load OBJ File (Relativistic)");
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
            infoLabel->setText("Model loaded (Relativistic): " + QFileInfo(filePath).fileName());
            mainWindow->setWindowTitle("OBJ Viewer - " + QFileInfo(filePath).fileName() + " (Relativistic)");
        }
    });
    return button;
}

// 创建Relativistic控制面板
inline QWidget* createRelativisticControlPanel(RelativisticGLWidget* glWidget, QLabel* infoLabel, QWidget* mainWindow) {
    QWidget *panel = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(panel);
    
    // 添加加载按钮
    layout->addWidget(createRelativisticModelLoadButton(glWidget, infoLabel, mainWindow));
    
    // 视图重置按钮
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
    
    // 描述标签
    QLabel *descriptionLabel = new QLabel(
        "<b>Relativistic View</b><br><br>"
        "Camera at (0,0,0) looking forward<br>"
        "View space: 100×100×100<br>"
        "Model scaled to fit 10×10×10 box<br>"
        "Positioned at left center (-20,0,0)"
    );
    descriptionLabel->setStyleSheet("color: white; background-color: #3A3A3A; padding: 10px; border-radius: 5px;");
    descriptionLabel->setWordWrap(true);
    layout->addWidget(descriptionLabel);
    
    layout->addStretch();
    return panel;
}

#endif // RELATIVISTIC_TAB_H