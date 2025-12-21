// app_ui.cpp
#include "app_ui.h"
#include "../glwidget/modelglwidget.h"
#include "../glwidget/baseglwidget.h"
#include "../glwidget/cgalglwidget.h"
#include "../glwidget/shortestpathglwidget.h"
#include "../glwidget/uvparamwidget.h"
#include "../glwidget/uvparamwidget_extended.h"
#include "../glwidget/simplesquarewidget.h"

namespace AppUI {
    
    // 创建模型信息显示组
    QGroupBox* createModelInfoGroup(QLabel** infoLabel) {
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
                } else if (auto shortestPathGlWidget = qobject_cast<ShortestPathGLWidget*>(glWidget)) {
                    shortestPathGlWidget->setBackgroundColor(color);
                } else if (auto uvParamWidget = qobject_cast<UVParamWidget*>(glWidget)) {
                    // UVParamWidget 没有 setBackgroundColor 方法，需要添加或忽略
                } else if (auto uvParamWidgetExtended = qobject_cast<UVParamWidgetExtended*>(glWidget)) {
                    // UVParamWidgetExtended 没有 setBackgroundColor 方法，需要添加或忽略
                } else if (auto simpleSquareWidget = qobject_cast<SimpleSquareWidget*>(glWidget)) {
                    simpleSquareWidget->setBackgroundColor(color);
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
                } else if (auto shortestPathGlWidget = qobject_cast<ShortestPathGLWidget*>(glWidget)) {
                    shortestPathGlWidget->setWireframeColor(wireframeColor);
                } else if (auto uvParamWidget = qobject_cast<UVParamWidget*>(glWidget)) {
                    // UVParamWidget 没有 setWireframeColor 方法，需要添加或忽略
                } else if (auto uvParamWidgetExtended = qobject_cast<UVParamWidgetExtended*>(glWidget)) {
                    // UVParamWidgetExtended 没有 setWireframeColor 方法，需要添加或忽略
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
                } else if (auto shortestPathGlWidget = qobject_cast<ShortestPathGLWidget*>(glWidget)) {
                    shortestPathGlWidget->setSurfaceColor(surfaceColor);
                } else if (auto uvParamWidget = qobject_cast<UVParamWidget*>(glWidget)) {
                    // UVParamWidget 没有 setSurfaceColor 方法，需要添加或忽略
                } else if (auto uvParamWidgetExtended = qobject_cast<UVParamWidgetExtended*>(glWidget)) {
                    // UVParamWidgetExtended 没有 setSurfaceColor 方法，需要添加或忽略
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
            } else if (auto shortestPathGlWidget = qobject_cast<ShortestPathGLWidget*>(glWidget)) {
                shortestPathGlWidget->setSpecularEnabled(enabled);
            } else if (auto uvParamWidget = qobject_cast<UVParamWidget*>(glWidget)) {
                // UVParamWidget 没有 setSpecularEnabled 方法，需要添加或忽略
            } else if (auto uvParamWidgetExtended = qobject_cast<UVParamWidgetExtended*>(glWidget)) {
                // UVParamWidgetExtended 没有 setSpecularEnabled 方法，需要添加或忽略
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
            } else if (auto shortestPathGlWidget = qobject_cast<ShortestPathGLWidget*>(glWidget)) {
                shortestPathGlWidget->setShowAxis(show);
            } else if (auto uvParamWidget = qobject_cast<UVParamWidget*>(glWidget)) {
                // UVParamWidget 没有 setShowAxis 方法，需要添加或忽略
            } else if (auto uvParamWidgetExtended = qobject_cast<UVParamWidgetExtended*>(glWidget)) {
                // UVParamWidgetExtended 没有 setShowAxis 方法，需要添加或忽略
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
    
} // namespace AppUI