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
#include <QMessageBox>

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
        rightWidget->clearMeshData();
    });
    rightLayout->addWidget(resetRightButton);
    
    rightLayout->addStretch();
    
    controlTabs->addTab(rightControlPanel, "Right View Controls");
    
    layout->addWidget(controlTabs);
    
    // 添加参数化选项组
    QGroupBox *parameterizationGroup = new QGroupBox("Parameterization Options");
    QVBoxLayout *paramLayout = new QVBoxLayout(parameterizationGroup);
    
    // 创建参数化方法选择组
    QGroupBox *methodGroup = new QGroupBox("Parameterization Method");
    QVBoxLayout *methodLayout = new QVBoxLayout(methodGroup);
    
    QButtonGroup *methodBtnGroup = new QButtonGroup(methodGroup);
    
    // 原来的参数化方法（默认选中）
    QRadioButton *originalRadio = new QRadioButton("Original Method (Cotangent Weights)");
    originalRadio->setChecked(true);
    originalRadio->setStyleSheet("color: white;");
    originalRadio->setToolTip("Original method using cotangent weights");
    methodBtnGroup->addButton(originalRadio, 0);
    methodLayout->addWidget(originalRadio);
    
    // 均匀参数化方法 (Tutte)
    QRadioButton *uniformRadio = new QRadioButton("Uniform Tutte Parametrization");
    uniformRadio->setStyleSheet("color: white;");
    uniformRadio->setToolTip("Each internal vertex is the barycenter of its neighbors");
    methodBtnGroup->addButton(uniformRadio, 1);
    methodLayout->addWidget(uniformRadio);
    
    // 加权Tutte参数化
    QRadioButton *weightedRadio = new QRadioButton("Weighted Tutte Parametrization");
    weightedRadio->setStyleSheet("color: white;");
    weightedRadio->setToolTip("Weights based on inverse squared distance");
    methodBtnGroup->addButton(weightedRadio, 2);
    methodLayout->addWidget(weightedRadio);
    
    // Floater形状保持参数化
    QRadioButton *floaterRadio = new QRadioButton("Floater Shape-Preserving");
    floaterRadio->setStyleSheet("color: white;");
    floaterRadio->setToolTip("Floater's shape-preserving parametrization");
    methodBtnGroup->addButton(floaterRadio, 3);
    methodLayout->addWidget(floaterRadio);
    
    paramLayout->addWidget(methodGroup);
    
    // 边界类型选择组
    QGroupBox *boundaryGroup = new QGroupBox("Boundary Type");
    QVBoxLayout *boundaryLayout = new QVBoxLayout(boundaryGroup);
    
    QButtonGroup *boundaryBtnGroup = new QButtonGroup(boundaryGroup);
    
    // 矩形边界
    QRadioButton *rectRadio = new QRadioButton("Rectangle Boundary");
    rectRadio->setChecked(true);
    rectRadio->setStyleSheet("color: white;");
    boundaryBtnGroup->addButton(rectRadio);
    boundaryLayout->addWidget(rectRadio);
    
    // 圆形边界
    QRadioButton *circleRadio = new QRadioButton("Circular Boundary");
    circleRadio->setStyleSheet("color: white;");
    boundaryBtnGroup->addButton(circleRadio);
    boundaryLayout->addWidget(circleRadio);
    
    paramLayout->addWidget(boundaryGroup);
    
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
    
    // 连接按钮点击事件 - 修复捕获列表，包含所有需要的变量
    QObject::connect(performParamButton, &QPushButton::clicked, 
        [=]() {  // 使用 [=] 捕获所有局部变量
            if (!leftWidget->modelLoaded) {
                QMessageBox::warning(mainWindow, "Parameterization Error", "No model loaded in left view.");
                return;
            }
            
            // 获取选中的参数化方法
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
            
            // 获取选中的边界类型
            BaseGLWidget::BoundaryType boundaryType = BaseGLWidget::Rectangle;
            if (circleRadio->isChecked()) {
                boundaryType = BaseGLWidget::Circle;
            }
            
            // 设置参数化方法
            rightWidget->setParameterizationMethod(paramMethod);
            
            // 将左侧网格数据传递给右侧进行参数化
            rightWidget->openMesh = leftWidget->openMesh;
            rightWidget->modelLoaded = true;
            rightWidget->faces = leftWidget->faces;
            rightWidget->edges = leftWidget->edges;
            rightWidget->hasOriginalMesh = true;
            rightWidget->originalMesh = leftWidget->openMesh;
            
            // 执行参数化
            rightWidget->performParameterization(boundaryType, paramMethod);
            
            if (rightWidget->isParameterized()) {
                // 获取参数化结果并传递给右视图
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
                
                QMessageBox::information(mainWindow, "Parameterization", 
                                       QString("Parameterization completed successfully!\n"
                                               "Method: %1\n"
                                               "Boundary: %2\n"
                                               "Vertices: %3, Faces: %4")
                                       .arg(methodStr)
                                       .arg(boundaryStr)
                                       .arg(vertices.size() / 3)
                                       .arg(faces.size() / 3));
            } else {
                QMessageBox::critical(mainWindow, "Parameterization Error", 
                                    "Parameterization failed. Please check the model and try again.");
            }
        });
    
    layout->addWidget(performParamButton);
    
    // 添加方法说明标签
    QLabel *methodInfoLabel = new QLabel(
        "<b>Method Descriptions:</b><br>"
        "• <b>Original Method:</b> Uses cotangent weights for shape preservation<br>"
        "• <b>Uniform Tutte:</b> Each vertex is placed at the barycenter of its neighbors<br>"
        "• <b>Weighted Tutte:</b> Weights based on inverse squared distance in 3D<br>"
        "• <b>Floater Shape-Preserving:</b> Advanced shape-preserving parametrization"
    );
    methodInfoLabel->setStyleSheet("color: #CCCCCC; background-color: #303030; padding: 10px; border-radius: 5px;");
    methodInfoLabel->setWordWrap(true);
    layout->addWidget(methodInfoLabel);
    
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
    QObject::connect(syncLoadButton, &QPushButton::clicked, [=]() {  // 使用 [=] 捕获所有局部变量
        QString filePath = QFileDialog::getOpenFileName(
            mainWindow, "Open OBJ File", "", "OBJ Files (*.obj)");
        
        if (!filePath.isEmpty()) {
            // 只加载到左侧视图
            leftWidget->loadOBJ(filePath);
            leftInfoLabel->setText("Model loaded (Left View): " + QFileInfo(filePath).fileName());
            
            // 清空右侧视图的网格数据，显示默认正方形
            rightWidget->clearMeshData();
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