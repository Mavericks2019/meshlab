// main.cpp
#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTabWidget>
#include <QLabel>
#include <QStyleFactory>
#include <QColorDialog>
#include <QPalette>
#include "glwidget/modelglwidget.h"
#include "glwidget/baseglwidget.h"
#include "glwidget/cgalglwidget.h"
#include "glwidget/shortestpathglwidget.h"  // 新增
#include "glwidget/uvparamwidget.h"
#include "tabs/model_tab.h"
#include "tabs/basic_tab.h"
#include "tabs/cgal_tab.h"
#include "tabs/shortestpath_tab.h"  // 新增
#include "tabs/uvparam_tab.h"

namespace UIUtils {
    // 创建模型信息显示组
    QGroupBox* createModelInfoGroup(QLabel** infoLabel = nullptr) {
        QGroupBox *group = new QGroupBox("Model Information");
        QVBoxLayout *layout = new QVBoxLayout(group);
        
        QLabel *label = new QLabel("No model loaded");
        label->setAlignment(Qt::AlignCenter);
        label->setFixedHeight(50);
        label->setStyleSheet("background-color: #3A3A3A; color: white; border-radius: 5px; padding: 5px; font-size: 14px;");
        label->setWordWrap(true);
        
        layout->addWidget(label);
        
        if (infoLabel) *infoLabel = label;
        return group;
    }

    // 创建颜色设置组
    QGroupBox* createColorSettingsGroup(QWidget* glWidget) {
        QGroupBox *group = new QGroupBox("Color Settings");
        QVBoxLayout *layout = new QVBoxLayout(group);
        layout->setSpacing(10);
        
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
        QObject::connect(bgColorButton, &QPushButton::clicked, [glWidget]() {
            QColor color = QColorDialog::getColor(QColor(0, 85, 127), nullptr, "Select Background Color");
            if (color.isValid()) {
                if (auto modelGlWidget = qobject_cast<ModelGLWidget*>(glWidget)) {
                    modelGlWidget->setBackgroundColor(color);
                } else if (auto baseGlWidget = qobject_cast<BaseGLWidget*>(glWidget)) {
                    baseGlWidget->setBackgroundColor(color);
                } else if (auto cgalGlWidget = qobject_cast<CGALGLWidget*>(glWidget)) {
                    cgalGlWidget->setBackgroundColor(color);
                }
            }
        });
        layout->addWidget(bgColorButton);
        
        // 线框颜色按钮
        QPushButton *lineColorButton = new QPushButton("Change Wireframe Color");
        lineColorButton->setStyleSheet(
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
        QObject::connect(lineColorButton, &QPushButton::clicked, [glWidget]() {
            QColor color = QColorDialog::getColor(Qt::red, nullptr, "Select Wireframe Color");
            if (color.isValid()) {
                QVector4D wireframeColor(
                    color.redF(), 
                    color.greenF(), 
                    color.blueF(), 
                    1.0f
                );
                if (auto modelGlWidget = qobject_cast<ModelGLWidget*>(glWidget)) {
                    modelGlWidget->setWireframeColor(wireframeColor);
                } else if (auto baseGlWidget = qobject_cast<BaseGLWidget*>(glWidget)) {
                    baseGlWidget->setWireframeColor(wireframeColor);
                } else if (auto cgalGlWidget = qobject_cast<CGALGLWidget*>(glWidget)) {
                    cgalGlWidget->setWireframeColor(wireframeColor);
                }
            }
        });
        layout->addWidget(lineColorButton);
        
        // 表面颜色按钮
        QPushButton *surfaceColorButton = new QPushButton("Change Surface Color");
        surfaceColorButton->setStyleSheet(
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
        QObject::connect(surfaceColorButton, &QPushButton::clicked, [glWidget]() {
            QColor color = QColorDialog::getColor(QColor(179, 179, 204), nullptr, "Select Surface Color");
            if (color.isValid()) {
                QVector3D surfaceColor(
                    color.redF(), 
                    color.greenF(), 
                    color.blueF()
                );
                if (auto modelGlWidget = qobject_cast<ModelGLWidget*>(glWidget)) {
                    modelGlWidget->setSurfaceColor(surfaceColor);
                } else if (auto baseGlWidget = qobject_cast<BaseGLWidget*>(glWidget)) {
                    baseGlWidget->setSurfaceColor(surfaceColor);
                } else if (auto cgalGlWidget = qobject_cast<CGALGLWidget*>(glWidget)) {
                    cgalGlWidget->setSurfaceColor(surfaceColor);
                }
            }
        });
        layout->addWidget(surfaceColorButton);
        
        // 高光控制复选框
        QCheckBox *specularCheckbox = new QCheckBox("Disable Specular Highlight");
        specularCheckbox->setStyleSheet("color: white;");
        specularCheckbox->setChecked(true);  // 默认选中，表示禁用高光
        QObject::connect(specularCheckbox, &QCheckBox::stateChanged, [glWidget](int state) {
            bool enabled = state != Qt::Checked;
            if (auto modelGlWidget = qobject_cast<ModelGLWidget*>(glWidget)) {
                modelGlWidget->setSpecularEnabled(enabled);
            } else if (auto baseGlWidget = qobject_cast<BaseGLWidget*>(glWidget)) {
                baseGlWidget->setSpecularEnabled(enabled);
            } else if (auto cgalGlWidget = qobject_cast<CGALGLWidget*>(glWidget)) {
                cgalGlWidget->setSpecularEnabled(enabled);
            }
        });
        layout->addWidget(specularCheckbox);
        
        // 坐标轴显示控制复选框
        QCheckBox *axisCheckbox = new QCheckBox("Show XYZ Axis");
        axisCheckbox->setStyleSheet("color: white;");
        axisCheckbox->setChecked(false);  // 默认不选中，表示不显示坐标轴
        QObject::connect(axisCheckbox, &QCheckBox::stateChanged, [glWidget](int state) {
            bool show = state == Qt::Checked;
            if (auto modelGlWidget = qobject_cast<ModelGLWidget*>(glWidget)) {
                modelGlWidget->setShowAxis(show);
            } else if (auto baseGlWidget = qobject_cast<BaseGLWidget*>(glWidget)) {
                baseGlWidget->setShowAxis(show);
            } else if (auto cgalGlWidget = qobject_cast<CGALGLWidget*>(glWidget)) {
                cgalGlWidget->setShowAxis(show);
            }
        });
        layout->addWidget(axisCheckbox);
        
        return group;
    }

    // 应用深色主题
    void applyDarkTheme(QApplication& app) {
        QApplication::setStyle(QStyleFactory::create("Fusion"));
        
        QPalette palette;
        palette.setColor(QPalette::Window, QColor(53, 53, 53));
        palette.setColor(QPalette::WindowText, Qt::white);
        palette.setColor(QPalette::Base, QColor(25, 25, 25));
        palette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
        palette.setColor(QPalette::ToolTipBase, Qt::white);
        palette.setColor(QPalette::ToolTipText, Qt::white);
        palette.setColor(QPalette::Text, Qt::white);
        palette.setColor(QPalette::Button, QColor(53, 53, 53));
        palette.setColor(QPalette::ButtonText, Qt::white);
        palette.setColor(QPalette::BrightText, Qt::red);
        palette.setColor(QPalette::Link, QColor(42, 130, 218));
        palette.setColor(QPalette::Highlight, QColor(42, 130, 218));
        palette.setColor(QPalette::HighlightedText, Qt::black);
        app.setPalette(palette);
        
        QFont defaultFont("Arial", 12);
        app.setFont(defaultFont);
    }
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    UIUtils::applyDarkTheme(app);

    // 创建主窗口
    QWidget mainWindow;
    mainWindow.resize(2480, 1800);
    
    // 创建主布局
    QHBoxLayout *mainLayout = new QHBoxLayout(&mainWindow);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    
    // 创建OpenGL窗口
    ModelGLWidget *modelGlWidget = new ModelGLWidget;
    BaseGLWidget *basicGlWidget = new BaseGLWidget;
    CGALGLWidget *cgalGlWidget = new CGALGLWidget;
    ShortestPathGLWidget *shortestPathGlWidget = new ShortestPathGLWidget;  // 新增
    UVParamWidget *uvParamWidget = new UVParamWidget;  // 新增
    
    // 创建标签页 - 添加UV参数化标签页
    QTabWidget *tabWidget = new QTabWidget;
    tabWidget->addTab(createBasicTab(basicGlWidget), "OpenMesh");
    tabWidget->addTab(createCGALTab(cgalGlWidget), "CGAL");
    tabWidget->addTab(createModelTab(modelGlWidget), "Model");
    tabWidget->addTab(createShortestPathTab(shortestPathGlWidget), "Shortest Path");
    tabWidget->addTab(createUVParamTab(uvParamWidget), "UV Parameterization");

    // 创建右侧控制面板堆栈
    QStackedWidget *controlStack = new QStackedWidget;
    
    // 模型信息组
    QLabel *modelInfoLabel = nullptr;
    QLabel *basicInfoLabel = nullptr;
    QLabel *cgalInfoLabel = nullptr;
    QLabel *shortestPathInfoLabel = nullptr;  // 新增
    QLabel *uvParamInfoLabel = nullptr;  // 新增

    // 创建OpenMesh标签页的控制面板
    QWidget *basicControlPanel = new QWidget;
    QVBoxLayout *basicControlLayout = new QVBoxLayout(basicControlPanel);
    basicControlLayout->setAlignment(Qt::AlignTop);
    basicControlLayout->addWidget(UIUtils::createColorSettingsGroup(basicGlWidget));
    basicControlLayout->addWidget(UIUtils::createModelInfoGroup(&basicInfoLabel));
    basicControlLayout->addWidget(createBasicControlPanel(basicGlWidget, basicInfoLabel, &mainWindow));
    
    // 创建CGAL标签页的控制面板
    QWidget *cgalControlPanel = new QWidget;
    QVBoxLayout *cgalControlLayout = new QVBoxLayout(cgalControlPanel);
    cgalControlLayout->setAlignment(Qt::AlignTop);
    cgalControlLayout->addWidget(UIUtils::createColorSettingsGroup(cgalGlWidget));
    cgalControlLayout->addWidget(UIUtils::createModelInfoGroup(&cgalInfoLabel));
    cgalControlLayout->addWidget(createCGALControlPanel(cgalGlWidget, cgalInfoLabel, &mainWindow));
    
    // 创建模型标签页的控制面板
    QWidget *modelControlPanel = new QWidget;
    QVBoxLayout *modelControlLayout = new QVBoxLayout(modelControlPanel);
    modelControlLayout->setAlignment(Qt::AlignTop);
    modelControlLayout->addWidget(UIUtils::createColorSettingsGroup(modelGlWidget));
    modelControlLayout->addWidget(UIUtils::createModelInfoGroup(&modelInfoLabel));
    modelControlLayout->addWidget(createModelControlPanel(modelGlWidget, modelInfoLabel, &mainWindow));
    
    // 创建最短路径标签页的控制面板  // 新增
    QWidget *shortestPathControlPanel = new QWidget;
    QVBoxLayout *shortestPathControlLayout = new QVBoxLayout(shortestPathControlPanel);
    shortestPathControlLayout->setAlignment(Qt::AlignTop);
    shortestPathControlLayout->addWidget(UIUtils::createColorSettingsGroup(shortestPathGlWidget));
    shortestPathControlLayout->addWidget(UIUtils::createModelInfoGroup(&shortestPathInfoLabel));
    shortestPathControlLayout->addWidget(createShortestPathControlPanel(shortestPathGlWidget, shortestPathInfoLabel, &mainWindow));
    
    QWidget *uvParamControlPanel = new QWidget;
    QVBoxLayout *uvParamControlLayout = new QVBoxLayout(uvParamControlPanel);
    uvParamControlLayout->setAlignment(Qt::AlignTop);
    uvParamControlLayout->addWidget(UIUtils::createModelInfoGroup(&uvParamInfoLabel));
    uvParamControlLayout->addWidget(createUVParamControlPanel(uvParamWidget, uvParamInfoLabel, &mainWindow));
    
    // 添加到堆栈 - 调整顺序以匹配标签页顺序
    controlStack->addWidget(basicControlPanel);   // OpenMesh
    controlStack->addWidget(cgalControlPanel);    // CGAL
    controlStack->addWidget(modelControlPanel);   // Model
    controlStack->addWidget(shortestPathControlPanel);  // Shortest Path
    controlStack->addWidget(uvParamControlPanel); // UV Parameterization  // 新增
    
    // 连接标签切换信号
    QObject::connect(tabWidget, &QTabWidget::currentChanged, [controlStack](int index) {
        controlStack->setCurrentIndex(index);
    });
    
    // 设置控制面板固定宽度
    controlStack->setFixedWidth(400);
    
    // 添加控件到主布局
    mainLayout->addWidget(tabWidget, 8);
    mainLayout->addWidget(controlStack);
    
    // 设置主窗口
    mainWindow.setLayout(mainLayout);
    mainWindow.setWindowTitle("OBJ Viewer");
    mainWindow.show();

    return app.exec();
}