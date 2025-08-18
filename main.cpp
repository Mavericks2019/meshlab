#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTabWidget>
#include <QLabel>
#include <QStyleFactory>
#include <QColorDialog>
#include <QPalette>
#include "model_tab.h"

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
    QGroupBox* createColorSettingsGroup(GLWidget* glWidget) {
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
            QColor color = QColorDialog::getColor(Qt::black, nullptr, "Select Background Color");
            if (color.isValid()) {
                glWidget->setBackgroundColor(color);
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
                glWidget->setWireframeColor(QVector4D(
                    color.redF(), 
                    color.greenF(), 
                    color.blueF(), 
                    1.0f
                ));
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
                glWidget->setSurfaceColor(QVector3D(
                    color.redF(), 
                    color.greenF(), 
                    color.blueF()
                ));
            }
        });
        layout->addWidget(surfaceColorButton);
        
        // 高光控制复选框
        QCheckBox *specularCheckbox = new QCheckBox("Disable Specular Highlight");
        specularCheckbox->setStyleSheet("color: white;");
        QObject::connect(specularCheckbox, &QCheckBox::stateChanged, [glWidget](int state) {
            glWidget->setSpecularEnabled(state != Qt::Checked);
        });
        layout->addWidget(specularCheckbox);
        
        layout->addStretch();
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
    GLWidget *glWidget = new GLWidget;
    
    // 创建模型标签页
    QWidget *modelTab = createModelTab(glWidget);
    
    // 创建右侧控制面板
    QWidget *controlPanel = new QWidget;
    QVBoxLayout *controlLayout = new QVBoxLayout(controlPanel);
    controlLayout->setAlignment(Qt::AlignTop);
    controlPanel->setFixedWidth(400);
    
    // 颜色设置组
    controlLayout->addWidget(UIUtils::createColorSettingsGroup(glWidget));
    
    // 模型信息组
    QLabel *modelInfoLabel = nullptr;
    controlLayout->addWidget(UIUtils::createModelInfoGroup(&modelInfoLabel));
    
    // 模型控制面板
    controlLayout->addWidget(createModelControlPanel(glWidget, modelInfoLabel, &mainWindow));
    
    // 添加控件到主布局
    mainLayout->addWidget(modelTab, 8);
    mainLayout->addWidget(controlPanel);
    
    // 设置主窗口
    mainWindow.setLayout(mainLayout);
    mainWindow.setWindowTitle("OBJ Viewer");
    mainWindow.show();

    return app.exec();
}