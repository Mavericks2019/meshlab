// openmesh_viewer_tab.h
#pragma once
#ifndef OPENMESH_VIEWER_TAB_H
#define OPENMESH_VIEWER_TAB_H

#include "../Cutting/QGLViewerWidget.h"
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
#include <QComboBox>
#include <QSpinBox>
#include <QButtonGroup>

// 创建OpenMesh Viewer标签页
inline QWidget* createOpenMeshViewerTab(QGLViewerWidget* glWidget) {
    QWidget *tab = new QWidget;
    QHBoxLayout *layout = new QHBoxLayout(tab);
    layout->addWidget(glWidget);
    return tab;
}

// 创建模型加载按钮
inline QWidget* createOpenMeshViewerLoadButton(QGLViewerWidget* glWidget, QLabel* infoLabel, QWidget* mainWindow) {
    QPushButton *button = new QPushButton("Load OBJ File (QGLViewer)");
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
            // 这里需要实现QGLViewerWidget的loadOBJ方法
            // 由于QGLViewerWidget没有loadOBJ方法，我们暂时只更新信息标签
            infoLabel->setText("Model loaded: " + QFileInfo(filePath).fileName());
            mainWindow->setWindowTitle("OBJ Viewer - " + QFileInfo(filePath).fileName() + " (QGLViewer)");
            
            // 临时消息，提示用户需要实现loadOBJ方法
            QMessageBox::information(mainWindow, "Info", 
                "Model loading for QGLViewerWidget needs to be implemented.\n" +
                filePath + " selected.");
        }
    });
    return button;
}

// 创建绘图模式选择组
inline QGroupBox* createOpenMeshViewerDrawModeGroup(QGLViewerWidget* glWidget) {
    QGroupBox *group = new QGroupBox("Draw Mode");
    QVBoxLayout *layout = new QVBoxLayout(group);
    
    QComboBox *drawModeCombo = new QComboBox();
    drawModeCombo->addItem("Wire Frame", QGLViewerWidget::WIRE_FRAME);
    drawModeCombo->addItem("Hidden Lines", QGLViewerWidget::HIDDEN_LINES);
    drawModeCombo->addItem("Solid Flat", QGLViewerWidget::SOLID_FLAT);
    drawModeCombo->addItem("Flat Points", QGLViewerWidget::FLAT_POINTS);
    drawModeCombo->addItem("Solid Smooth", QGLViewerWidget::SOLID_SMOOTH);
    
    // 设置样式
    drawModeCombo->setStyleSheet(
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
        "   border-left: 1px solid #606060;"
        "}"
    );
    
    // 连接信号
    QObject::connect(drawModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
        [glWidget, drawModeCombo](int index) {
            int drawMode = drawModeCombo->itemData(index).toInt();
            glWidget->setDrawMode(drawMode);
        });
    
    layout->addWidget(drawModeCombo);
    return group;
}

// 创建投影模式选择组
inline QGroupBox* createOpenMeshViewerProjectionModeGroup(QGLViewerWidget* glWidget) {
    QGroupBox *group = new QGroupBox("Projection Mode");
    QVBoxLayout *layout = new QVBoxLayout(group);
    
    QRadioButton *perspectiveRadio = new QRadioButton("Perspective");
    perspectiveRadio->setChecked(true);
    
    QRadioButton *orthoRadio = new QRadioButton("Orthotropic 2D");
    
    layout->addWidget(perspectiveRadio);
    layout->addWidget(orthoRadio);
    
    // 创建按钮组来管理单选按钮的互斥性
    QButtonGroup* projectionButtonGroup = new QButtonGroup(group);
    projectionButtonGroup->addButton(perspectiveRadio);
    projectionButtonGroup->addButton(orthoRadio);
    projectionButtonGroup->setExclusive(true);
    
    // 连接信号
    QObject::connect(perspectiveRadio, &QRadioButton::clicked, [glWidget]() {
        glWidget->set_pro_mode(QGLViewerWidget::PERSPECTIVE);
    });
    
    QObject::connect(orthoRadio, &QRadioButton::clicked, [glWidget]() {
        glWidget->set_pro_mode(QGLViewerWidget::ORTHOTROPIC2D);
    });
    
    return group;
}

// 创建控制选项组
inline QGroupBox* createOpenMeshViewerControlGroup(QGLViewerWidget* glWidget) {
    QGroupBox *group = new QGroupBox("Controls");
    QVBoxLayout *layout = new QVBoxLayout(group);
    
    // 旋转按钮
    QPushButton *rotateButton = new QPushButton("Rotate 90° Y-axis");
    rotateButton->setStyleSheet(
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
    QObject::connect(rotateButton, &QPushButton::clicked, [glWidget]() {
        glWidget->rotate_mesh_by_angle_slot(90.0);
    });
    
    // 视图重置按钮
    QPushButton *resetButton = new QPushButton("Reset View");
    resetButton->setStyleSheet(
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
    QObject::connect(resetButton, &QPushButton::clicked, [glWidget]() {
        glWidget->reset_modelview_matrix();
    });
    
    // 查看全部按钮
    QPushButton *viewAllButton = new QPushButton("View All");
    viewAllButton->setStyleSheet(
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
    QObject::connect(viewAllButton, &QPushButton::clicked, [glWidget]() {
        glWidget->view_all();
    });
    
    layout->addWidget(rotateButton);
    layout->addWidget(resetButton);
    layout->addWidget(viewAllButton);
    
    return group;
}

// 创建场景设置组
inline QGroupBox* createOpenMeshViewerSceneGroup(QGLViewerWidget* glWidget) {
    QGroupBox *group = new QGroupBox("Scene Settings");
    QFormLayout *layout = new QFormLayout(group);
    
    // 中心点设置
    QLabel *centerLabel = new QLabel("Center:");
    QLabel *centerValue = new QLabel("(0.0, 0.0, 0.0)");
    centerValue->setStyleSheet("color: #AAAAAA;");
    
    // 半径设置
    QLabel *radiusLabel = new QLabel("Radius:");
    QLabel *radiusValue = new QLabel("1.0");
    radiusValue->setStyleSheet("color: #AAAAAA;");
    
    // 更新场景按钮
    QPushButton *updateSceneButton = new QPushButton("Update Scene");
    updateSceneButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #505050;"
        "   color: white;"
        "   border: none;"
        "   padding: 5px 10px;"
        "   font-size: 12px;"
        "   border-radius: 3px;"
        "}"
        "QPushButton:hover { background-color: #606060; }"
    );
    
    // 连接更新场景按钮
    QObject::connect(updateSceneButton, &QPushButton::clicked, [glWidget, centerValue, radiusValue]() {
        // 这里可以添加更新场景中心的逻辑
        // 暂时只更新显示
        OpenMesh::Vec3d center = glWidget->center();
        float radius = glWidget->radius();
        
        centerValue->setText(QString("(%1, %2, %3)")
            .arg(center[0], 0, 'f', 2)
            .arg(center[1], 0, 'f', 2)
            .arg(center[2], 0, 'f', 2));
        radiusValue->setText(QString::number(radius, 'f', 2));
    });
    
    layout->addRow(centerLabel, centerValue);
    layout->addRow(radiusLabel, radiusValue);
    layout->addRow(updateSceneButton);
    
    return group;
}

// 创建OpenMesh Viewer控制面板
inline QWidget* createOpenMeshViewerControlPanel(QGLViewerWidget* glWidget, QLabel* infoLabel, QWidget* mainWindow) {
    QWidget *panel = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(panel);
    
    // 添加控件组
    layout->addWidget(createOpenMeshViewerLoadButton(glWidget, infoLabel, mainWindow));
    layout->addWidget(createOpenMeshViewerDrawModeGroup(glWidget));
    layout->addWidget(createOpenMeshViewerProjectionModeGroup(glWidget));
    layout->addWidget(createOpenMeshViewerControlGroup(glWidget));
    layout->addWidget(createOpenMeshViewerSceneGroup(glWidget));
    
    layout->addStretch();
    return panel;
}

#endif // OPENMESH_VIEWER_TAB_H