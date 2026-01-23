// blackhole_tab.h
#pragma once
#ifndef BLACKHOLE_TAB_H
#define BLACKHOLE_TAB_H

#include "../glwidget/blackholewidget.h"
#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QGroupBox>
#include <QSlider>
#include <QDoubleSpinBox>
#include <QColorDialog>

// 创建Black Hole标签页
inline QWidget* createBlackHoleTab(BlackHoleWidget* blackHoleWidget) {
    QWidget *tab = new QWidget;
    QHBoxLayout *layout = new QHBoxLayout(tab);
    layout->addWidget(blackHoleWidget);
    return tab;
}

// 创建重置视图按钮
inline QPushButton* createResetViewButton(BlackHoleWidget* blackHoleWidget) {
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
    QObject::connect(resetButton, &QPushButton::clicked, [blackHoleWidget]() {
        blackHoleWidget->resetView();
    });
    return resetButton;
}

// 创建背景颜色按钮
inline QPushButton* createBackgroundColorButton(BlackHoleWidget* blackHoleWidget) {
    QPushButton *bgColorButton = new QPushButton("Background Color");
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
    QObject::connect(bgColorButton, &QPushButton::clicked, [blackHoleWidget]() {
        QColor color = QColorDialog::getColor(Qt::black, nullptr, "Select Background Color");
        if (color.isValid()) {
            blackHoleWidget->setBackgroundColor(color);
        }
    });
    return bgColorButton;
}

// 创建光线颜色按钮
inline QPushButton* createRayColorButton(BlackHoleWidget* blackHoleWidget) {
    QPushButton *rayColorButton = new QPushButton("Ray Color");
    rayColorButton->setStyleSheet(
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
    QObject::connect(rayColorButton, &QPushButton::clicked, [blackHoleWidget]() {
        QColor color = QColorDialog::getColor(Qt::red, nullptr, "Select Ray Color");
        if (color.isValid()) {
            blackHoleWidget->setRayColor(color);
        }
    });
    return rayColorButton;
}

// 创建轨迹颜色按钮
inline QPushButton* createTrailColorButton(BlackHoleWidget* blackHoleWidget) {
    QPushButton *trailColorButton = new QPushButton("Trail Color");
    trailColorButton->setStyleSheet(
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
    QObject::connect(trailColorButton, &QPushButton::clicked, [blackHoleWidget]() {
        QColor color = QColorDialog::getColor(Qt::white, nullptr, "Select Trail Color");
        if (color.isValid()) {
            blackHoleWidget->setTrailColor(color);
        }
    });
    return trailColorButton;
}

// 创建模拟速度控制滑块
inline QGroupBox* createSimulationSpeedGroup(BlackHoleWidget* blackHoleWidget) {
    QGroupBox *group = new QGroupBox("Simulation Speed");
    QVBoxLayout *layout = new QVBoxLayout(group);
    
    QSlider *speedSlider = new QSlider(Qt::Horizontal);
    speedSlider->setRange(1, 100);
    speedSlider->setValue(10);
    speedSlider->setStyleSheet(
        "QSlider::groove:horizontal {"
        "   height: 8px;"
        "   background: #505050;"
        "   border-radius: 4px;"
        "}"
        "QSlider::handle:horizontal {"
        "   background: #42a2da;"
        "   width: 20px;"
        "   height: 20px;"
        "   margin: -6px 0;"
        "   border-radius: 10px;"
        "}"
    );
    
    QLabel *speedLabel = new QLabel("1.0x");
    speedLabel->setAlignment(Qt::AlignCenter);
    speedLabel->setStyleSheet("color: white; font-size: 14px;");
    
    QObject::connect(speedSlider, &QSlider::valueChanged, [blackHoleWidget, speedLabel](int value) {
        double speed = value / 10.0;
        blackHoleWidget->setSimulationSpeed(speed);
        speedLabel->setText(QString("%1x").arg(speed, 0, 'f', 1));
    });
    
    layout->addWidget(speedSlider);
    layout->addWidget(speedLabel);
    
    return group;
}

// 创建黑洞质量控制
inline QGroupBox* createBlackHoleMassGroup(BlackHoleWidget* blackHoleWidget) {
    QGroupBox *group = new QGroupBox("Black Hole Mass");
    QVBoxLayout *layout = new QVBoxLayout(group);
    
    QDoubleSpinBox *massSpinBox = new QDoubleSpinBox();
    massSpinBox->setRange(1e30, 1e40);
    massSpinBox->setValue(8.54e36);
    massSpinBox->setDecimals(2);
    massSpinBox->setSuffix(" kg");
    massSpinBox->setStyleSheet(
        "QDoubleSpinBox {"
        "   background-color: #3A3A3A;"
        "   color: white;"
        "   border: 1px solid #505050;"
        "   border-radius: 3px;"
        "   padding: 5px;"
        "   font-size: 14px;"
        "}"
    );
    
    QLabel *massLabel = new QLabel("Current: 8.54×10³⁶ kg (Sagittarius A*)");
    massLabel->setStyleSheet("color: white; font-size: 12px;");
    massLabel->setWordWrap(true);
    
    // 修复信号连接问题：使用函数指针指定double版本的valueChanged
    QObject::connect(massSpinBox, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
                     [blackHoleWidget, massLabel](double value) {
        blackHoleWidget->setBlackHoleMass(value);
        massLabel->setText(QString("Current: %1 kg").arg(value, 0, 'e', 2));
    });
    
    layout->addWidget(massSpinBox);
    layout->addWidget(massLabel);
    
    return group;
}

// 创建添加测试光线按钮
inline QPushButton* createAddTestRaysButton(BlackHoleWidget* blackHoleWidget) {
    QPushButton *addRaysButton = new QPushButton("Add Test Rays");
    addRaysButton->setStyleSheet(
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
    QObject::connect(addRaysButton, &QPushButton::clicked, [blackHoleWidget]() {
        blackHoleWidget->clearRays();
        // 添加几束测试光线
        blackHoleWidget->addRay(glm::vec2(-1e11, 3.27606302719999999e10), glm::vec2(299792458.0f, 0.0f));
        blackHoleWidget->addRay(glm::vec2(-1e11, 1e11), glm::vec2(299792458.0f, 0.0f));
        blackHoleWidget->addRay(glm::vec2(-1e11, 5e10), glm::vec2(299792458.0f, 0.0f));
        blackHoleWidget->addRay(glm::vec2(-1e11, 0.0f), glm::vec2(299792458.0f, 0.0f));
        blackHoleWidget->addRay(glm::vec2(-1e11, -5e10), glm::vec2(299792458.0f, 0.0f));
        blackHoleWidget->addRay(glm::vec2(-1e11, -1e11), glm::vec2(299792458.0f, 0.0f));
    });
    return addRaysButton;
}

// 创建清除光线按钮
inline QPushButton* createClearRaysButton(BlackHoleWidget* blackHoleWidget) {
    QPushButton *clearButton = new QPushButton("Clear All Rays");
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
    QObject::connect(clearButton, &QPushButton::clicked, [blackHoleWidget]() {
        blackHoleWidget->clearRays();
    });
    return clearButton;
}

// 创建Black Hole控制面板
inline QWidget* createBlackHoleControlPanel(BlackHoleWidget* blackHoleWidget, QLabel* infoLabel, QWidget* mainWindow) {
    QWidget *panel = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(panel);
    
    // 设置信息标签
    if (infoLabel) {
        infoLabel->setText("Black Hole Simulation - Sagittarius A*\n"
                          "Mass: 8.54×10³⁶ kg\n"
                          "Schwarzschild Radius: ~2.52×10¹⁰ m\n"
                          "Use Middle Mouse to pan, Wheel to zoom");
    }
    
    // 添加控件组
    layout->addWidget(createResetViewButton(blackHoleWidget));
    layout->addWidget(createBackgroundColorButton(blackHoleWidget));
    layout->addWidget(createRayColorButton(blackHoleWidget));
    layout->addWidget(createTrailColorButton(blackHoleWidget));
    layout->addWidget(createSimulationSpeedGroup(blackHoleWidget));
    layout->addWidget(createBlackHoleMassGroup(blackHoleWidget));
    
    // 添加光线控制按钮
    QPushButton *addRaysButton = createAddTestRaysButton(blackHoleWidget);
    QPushButton *clearButton = createClearRaysButton(blackHoleWidget);
    
    layout->addWidget(addRaysButton);
    layout->addWidget(clearButton);
    
    // 添加说明标签
    QLabel *instructions = new QLabel(
        "Instructions:\n"
        "• Middle Mouse: Pan view\n"
        "• Wheel: Zoom in/out\n"
        "• Black hole shows light bending\n"
        "• Rays follow geodesic equations\n"
        "• Red circle: Event horizon"
    );
    instructions->setStyleSheet("color: white; background-color: #3A3A3A; padding: 10px; border-radius: 5px; font-size: 12px;");
    instructions->setWordWrap(true);
    layout->addWidget(instructions);
    
    layout->addStretch();
    return panel;
}

#endif // BLACKHOLE_TAB_H