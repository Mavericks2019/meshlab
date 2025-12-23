// dualview_simple_tab.h
#pragma once
#ifndef DUALVIEW_SIMPLE_TAB_H
#define DUALVIEW_SIMPLE_TAB_H

#include "../glwidget/baseglwidget.h"
#include "../glwidget/simplesquarewidget.h"
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
#include <QMessageBox>
#include <QColorDialog>

// 创建简单双视图标签页
inline QWidget* createDualViewSimpleTab(BaseGLWidget* leftWidget, SimpleSquareWidget* rightWidget) {
    QWidget *tab = new QWidget;
    QHBoxLayout *layout = new QHBoxLayout(tab);
    
    QSplitter *splitter = new QSplitter(Qt::Horizontal);
    
    splitter->addWidget(leftWidget);
    splitter->addWidget(rightWidget);
    
    splitter->setSizes(QList<int>() << 500 << 500);
    
    layout->addWidget(splitter);
    
    return tab;
}

// 创建简单双视图控制面板
inline QWidget* createDualViewSimpleControlPanel(BaseGLWidget* leftWidget, SimpleSquareWidget* rightWidget, QLabel* leftInfoLabel, QLabel* rightInfoLabel, QWidget* mainWindow) {
    QWidget *panel = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(panel);
    
    QTabWidget *controlTabs = new QTabWidget;
    
    QWidget *leftControlPanel = createBasicControlPanel(leftWidget, leftInfoLabel, mainWindow);
    controlTabs->addTab(leftControlPanel, "Left View Controls");
    
    QWidget *rightControlPanel = new QWidget;
    QVBoxLayout *rightLayout = new QVBoxLayout(rightControlPanel);
    rightLayout->setAlignment(Qt::AlignTop);
    
    QGroupBox *rightInfoGroup = new QGroupBox("Right View Information");
    QVBoxLayout *rightInfoLayout = new QVBoxLayout(rightInfoGroup);
    rightInfoLayout->addWidget(rightInfoLabel);
    rightLayout->addWidget(rightInfoGroup);
    
    QGroupBox *rightColorGroup = new QGroupBox("Square Settings");
    QVBoxLayout *rightColorLayout = new QVBoxLayout(rightColorGroup);
    
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
    
    // 添加检测翻转按钮
    QGroupBox *flipCheckGroup = new QGroupBox("Flip Detection");
    QVBoxLayout *flipCheckLayout = new QVBoxLayout(flipCheckGroup);
    
    QPushButton *checkFlipButton = new QPushButton("Check for Flipped Triangles");
    checkFlipButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #A05050;"
        "   color: white;"
        "   border: none;"
        "   padding: 10px 20px;"
        "   font-size: 16px;"
        "   border-radius: 5px;"
        "}"
        "QPushButton:hover { background-color: #B06060; }"
        "QPushButton:disabled {"
        "   background-color: #404040;"
        "   color: #808080;"
        "}"
    );
    
    // 初始时禁用检测按钮
    checkFlipButton->setEnabled(false);
    
    QLabel *flipResultLabel = new QLabel("No flip detection performed yet");
    flipResultLabel->setStyleSheet("color: #CCCCCC; background-color: #303030; padding: 5px; border-radius: 3px;");
    flipResultLabel->setWordWrap(true);
    
    QObject::connect(checkFlipButton, &QPushButton::clicked, [=]() {
        if (!rightWidget->isParameterized()) {
            QMessageBox::warning(mainWindow, "Flip Detection", 
                "No parameterized mesh available. Please perform parameterization first.");
            return;
        }
        
        int flipCount = rightWidget->checkForFlips();
        
        if (flipCount == 0) {
            flipResultLabel->setText("<b>✓ All triangles are oriented correctly!</b><br>"
                                     "No flipped triangles found.");
            flipResultLabel->setStyleSheet("color: #90EE90; background-color: #303030; padding: 5px; border-radius: 3px;");
        } else {
            auto flippedIndices = rightWidget->getFlippedTriangles();
            QString indicesStr;
            for (size_t i = 0; i < std::min(flippedIndices.size(), size_t(10)); i++) {
                indicesStr += QString::number(flippedIndices[i]) + " ";
            }
            if (flippedIndices.size() > 10) {
                indicesStr += "... (and " + QString::number(flippedIndices.size() - 10) + " more)";
            }
            
            flipResultLabel->setText(QString("<b>⚠ Warning: %1 triangles flipped!</b><br>"
                                           "Flipped triangle indices: %2<br>"
                                           "A flipped triangle has negative area in parameter space, "
                                           "indicating orientation reversal.")
                                    .arg(flipCount)
                                    .arg(indicesStr));
            flipResultLabel->setStyleSheet("color: #FFA07A; background-color: #303030; padding: 5px; border-radius: 3px;");
            
            QMessageBox::warning(mainWindow, "Flip Detection", 
                QString("Found %1 flipped triangles in the parameterization.\n"
                       "This means these triangles have negative area in parameter space, "
                       "which can cause issues in texture mapping and other applications.")
                .arg(flipCount));
        }
        
        rightInfoLabel->setText(QString("Parameterized Mesh\n"
                                      "Vertices: %1, Faces: %2\n"
                                      "Flipped triangles: %3")
                              .arg(rightWidget->getParameterizedVertices().size() / 3)
                              .arg(rightWidget->getParameterizedFaces().size() / 3)
                              .arg(flipCount));
    });
    
    flipCheckLayout->addWidget(checkFlipButton);
    flipCheckLayout->addWidget(flipResultLabel);
    rightLayout->addWidget(flipCheckGroup);
    
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
    QObject::connect(resetRightButton, &QPushButton::clicked, [=]() {
        rightWidget->setBackgroundColor(QColor(0, 85, 127));
        rightWidget->setSquareColor(Qt::white);
        rightWidget->clearMeshData();
        flipResultLabel->setText("No flip detection performed yet");
        flipResultLabel->setStyleSheet("color: #CCCCCC; background-color: #303030; padding: 5px; border-radius: 3px;");
        checkFlipButton->setEnabled(false);
    });
    rightLayout->addWidget(resetRightButton);
    
    rightLayout->addStretch();
    
    controlTabs->addTab(rightControlPanel, "Right View Controls");
    
    layout->addWidget(controlTabs);
    
    QGroupBox *parameterizationGroup = new QGroupBox("Parameterization Options");
    QVBoxLayout *paramLayout = new QVBoxLayout(parameterizationGroup);
    
    QGroupBox *methodGroup = new QGroupBox("Parameterization Method");
    QVBoxLayout *methodLayout = new QVBoxLayout(methodGroup);
    
    QButtonGroup *methodBtnGroup = new QButtonGroup(methodGroup);
    
    QRadioButton *originalRadio = new QRadioButton("Original Method (Cotangent Weights)");
    originalRadio->setChecked(true);
    originalRadio->setStyleSheet("color: white;");
    originalRadio->setToolTip("Original method using cotangent weights");
    methodBtnGroup->addButton(originalRadio, 0);
    methodLayout->addWidget(originalRadio);
    
    QRadioButton *uniformRadio = new QRadioButton("Uniform Tutte Parametrization");
    uniformRadio->setStyleSheet("color: white;");
    uniformRadio->setToolTip("Each internal vertex is the barycenter of its neighbors");
    methodBtnGroup->addButton(uniformRadio, 1);
    methodLayout->addWidget(uniformRadio);
    
    QRadioButton *weightedRadio = new QRadioButton("Weighted Tutte Parametrization");
    weightedRadio->setStyleSheet("color: white;");
    weightedRadio->setToolTip("Weights based on inverse squared distance");
    methodBtnGroup->addButton(weightedRadio, 2);
    methodLayout->addWidget(weightedRadio);
    
    QRadioButton *floaterRadio = new QRadioButton("Floater Shape-Preserving");
    floaterRadio->setStyleSheet("color: white;");
    floaterRadio->setToolTip("Floater's shape-preserving parametrization");
    methodBtnGroup->addButton(floaterRadio, 3);
    methodLayout->addWidget(floaterRadio);
    
    paramLayout->addWidget(methodGroup);
    
    QGroupBox *boundaryGroup = new QGroupBox("Boundary Type");
    QVBoxLayout *boundaryLayout = new QVBoxLayout(boundaryGroup);
    
    QButtonGroup *boundaryBtnGroup = new QButtonGroup(boundaryGroup);
    
    QRadioButton *rectRadio = new QRadioButton("Rectangle Boundary");
    rectRadio->setChecked(true);
    rectRadio->setStyleSheet("color: white;");
    boundaryBtnGroup->addButton(rectRadio);
    boundaryLayout->addWidget(rectRadio);
    
    QRadioButton *circleRadio = new QRadioButton("Circular Boundary");
    circleRadio->setStyleSheet("color: white;");
    boundaryBtnGroup->addButton(circleRadio);
    boundaryLayout->addWidget(circleRadio);
    
    paramLayout->addWidget(boundaryGroup);
    
    layout->addWidget(parameterizationGroup);
    
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
    
    performParamButton->setEnabled(false);
    
    QObject::connect(performParamButton, &QPushButton::clicked, 
        [=]() {
            if (!leftWidget->modelLoaded) {
                QMessageBox::warning(mainWindow, "Parameterization Error", "No model loaded in left view.");
                return;
            }
            
            BaseGLWidget::ParameterizationMethod paramMethod;
            int methodId = methodBtnGroup->checkedId();
            
            switch (methodId) {
            case 0:
                paramMethod = BaseGLWidget::OriginalMethod;
                break;
            case 1:
                paramMethod = BaseGLWidget::UniformTutte;
                break;
            case 2:
                paramMethod = BaseGLWidget::WeightedTutte;
                break;
            case 3:
                paramMethod = BaseGLWidget::FloaterShapePreserving;
                break;
            default:
                paramMethod = BaseGLWidget::OriginalMethod;
            }
            
            BaseGLWidget::BoundaryType boundaryType = BaseGLWidget::Rectangle;
            if (circleRadio->isChecked()) {
                boundaryType = BaseGLWidget::Circle;
            }
            
            rightWidget->setParameterizationMethod(paramMethod);
            
            rightWidget->openMesh = leftWidget->openMesh;
            rightWidget->modelLoaded = true;
            rightWidget->faces = leftWidget->faces;
            rightWidget->edges = leftWidget->edges;
            rightWidget->hasOriginalMesh = true;
            rightWidget->originalMesh = leftWidget->openMesh;
            
            rightWidget->performParameterization(boundaryType, paramMethod);
            
            if (rightWidget->isParameterized()) {
                auto vertices = rightWidget->getParameterizedVertices();
                auto faces = rightWidget->getParameterizedFaces();
                rightWidget->setMeshData(vertices, faces);
                
                QString methodStr;
                switch (paramMethod) {
                case BaseGLWidget::OriginalMethod:
                    methodStr = "Original Method (Cotangent Weights)";
                    break;
                case BaseGLWidget::UniformTutte:
                    methodStr = "Uniform Tutte";
                    break;
                case BaseGLWidget::WeightedTutte:
                    methodStr = "Weighted Tutte";
                    break;
                case BaseGLWidget::FloaterShapePreserving:
                    methodStr = "Floater Shape-Preserving";
                    break;
                default:
                    methodStr = "Original Method";
                }
                
                QString boundaryStr = boundaryType == BaseGLWidget::Circle ? "Circle" : "Rectangle";
                
                rightInfoLabel->setText(QString("Parameterized Mesh\nMethod: %1\nBoundary: %2\nVertices: %3, Faces: %4")
                                       .arg(methodStr)
                                       .arg(boundaryStr)
                                       .arg(vertices.size() / 3)
                                       .arg(faces.size() / 3));
                
                // 启用检测翻转按钮
                checkFlipButton->setEnabled(true);
                flipResultLabel->setText("Click 'Check for Flipped Triangles' to detect flips");
                flipResultLabel->setStyleSheet("color: #CCCCCC; background-color: #303030; padding: 5px; border-radius: 3px;");
                
            } else {
                rightInfoLabel->setText("Parameterization failed. Please check the model and try again.");
                checkFlipButton->setEnabled(false);
            }
        });
    
    layout->addWidget(performParamButton);
    
    QLabel *methodInfoLabel = new QLabel(
        "<b>Method Descriptions:</b><br>"
        "• <b>Original Method:</b> Uses cotangent weights for shape preservation<br>"
        "• <b>Uniform Tutte:</b> Each vertex is placed at the barycenter of its neighbors<br>"
        "• <b>Weighted Tutte:</b> Weights based on inverse squared distance in 3D<br>"
        "• <b>Floater Shape-Preserving:</b> Advanced shape-preserving parametrization<br><br>"
        "<b>Flip Detection:</b> Checks for triangles with negative area in parameter space, "
        "which indicates orientation reversal (flipping)."
    );
    methodInfoLabel->setStyleSheet("color: #CCCCCC; background-color: #303030; padding: 10px; border-radius: 5px;");
    methodInfoLabel->setWordWrap(true);
    layout->addWidget(methodInfoLabel);
    
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
    QObject::connect(syncLoadButton, &QPushButton::clicked, [=]() {
        QString filePath = QFileDialog::getOpenFileName(
            mainWindow, "Open OBJ File", "", "OBJ Files (*.obj)");
        
        if (!filePath.isEmpty()) {
            leftWidget->loadOBJ(filePath);
            leftInfoLabel->setText("Model loaded (Left View): " + QFileInfo(filePath).fileName());
            
            rightWidget->clearMeshData();
            rightInfoLabel->setText("White Square View - Ready for parameterization");
            
            performParamButton->setEnabled(true);
            checkFlipButton->setEnabled(false);
            flipResultLabel->setText("No flip detection performed yet");
            flipResultLabel->setStyleSheet("color: #CCCCCC; background-color: #303030; padding: 5px; border-radius: 3px;");
            
            mainWindow->setWindowTitle("OBJ Viewer - " + QFileInfo(filePath).fileName() + " (Simple Dual View)");
        }
    });
    layout->addWidget(syncLoadButton);
    
    layout->addStretch();
    return panel;
}

#endif // DUALVIEW_SIMPLE_TAB_H