// main.cpp
#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTabWidget>
#include <QLabel>
#include <QStyleFactory>
#include <QColorDialog>
#include <QPalette>
#include <QStackedWidget>
#include <QSplitter>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QActionGroup>
#include <QToolButton>
#include <QStyle>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QMap>
#include "glwidget/modelglwidget.h"
#include "glwidget/baseglwidget.h"
#include "glwidget/cgalglwidget.h"
#include "glwidget/shortestpathglwidget.h"
#include "glwidget/uvparamwidget.h"
#include "glwidget/simplesquarewidget.h"
#include "tabs/model_tab.h"
#include "tabs/basic_tab.h"
#include "tabs/cgal_tab.h"
#include "tabs/shortestpath_tab.h"
#include "tabs/uvparam_tab.h"
#include "tabs/dualview_tab.h"
#include "tabs/dualview_extended_tab.h"
#include "tabs/dualview_simple_tab.h"

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

    // 自定义TabWidget，支持关闭按钮
    class CloseableTabWidget : public QTabWidget {
    private:
        QMap<QWidget*, QString> tabTitles;  // 存储每个widget的原始标题
        QMap<QString, QWidget*> titleToWidget;  // 通过标题查找widget
        QMap<QString, QAction*> titleToAction;  // 通过标题查找action
        
    public:
        CloseableTabWidget(QWidget* parent = nullptr) : QTabWidget(parent) {
            setTabsClosable(true);
            setMovable(true);
            
            // 设置TabBar样式
            setStyleSheet(R"(
                QTabWidget::pane {
                    border: 1px solid #505050;
                    background-color: #353535;
                }
                
                QTabBar::tab {
                    background-color: #404040;
                    color: white;
                    padding: 8px 30px 8px 12px;
                    margin-right: 2px;
                    border-top-left-radius: 4px;
                    border-top-right-radius: 4px;
                    border: 1px solid #505050;
                    border-bottom: none;
                    min-width: 100px;
                }
                
                QTabBar::tab:selected {
                    background-color: #505050;
                    border-bottom: 2px solid #42a2da;
                }
                
                QTabBar::tab:hover:!selected {
                    background-color: #484848;
                }
                
                QTabBar::close-button {
                    subcontrol-position: right;
                    subcontrol-origin: padding;
                    width: 16px;
                    height: 16px;
                    margin: 2px;
                    border-radius: 2px;
                }
                
                QTabBar::close-button:hover {
                    background-color: #606060;
                }
                
                /* 使用CSS绘制X关闭图标 */
                QTabBar::close-button::icon {
                    /* 清除默认图标 */
                    image: none;
                }
                
                QTabBar::close-button::after {
                    content: "×";
                    color: #CCCCCC;
                    font-size: 18px;
                    font-weight: bold;
                    position: absolute;
                    top: 50%;
                    left: 50%;
                    transform: translate(-50%, -50%);
                }
                
                QTabBar::close-button:hover::after {
                    color: white;
                }
                
                QTabBar::scroller {
                    width: 20px;
                }
            )");
        }
        
        // 添加tab并记录标题
        void addTabWithTitle(QWidget* widget, const QString& title) {
            tabTitles[widget] = title;
            titleToWidget[title] = widget;
            addTab(widget, title);
        }
        
        // 获取tab的原始标题
        QString getTabTitle(QWidget* widget) const {
            return tabTitles.value(widget, QString());
        }
        
        // 获取所有tab标题
        QStringList getAllTabTitles() const {
            return tabTitles.values();
        }
        
        // 通过标题获取widget
        QWidget* getWidgetByTitle(const QString& title) const {
            return titleToWidget.value(title, nullptr);
        }
        
        // 通过标题恢复tab
        bool restoreTabByTitle(const QString& title) {
            QWidget* widget = getWidgetByTitle(title);
            if (widget && indexOf(widget) == -1) {
                // Tab不存在，重新添加
                addTab(widget, title);
                return true;
            }
            return false;
        }
        
        // 设置标题对应的action
        void setActionForTitle(const QString& title, QAction* action) {
            titleToAction[title] = action;
        }
        
        // 获取标题对应的action
        QAction* getActionForTitle(const QString& title) const {
            return titleToAction.value(title, nullptr);
        }
        
        // 获取所有标题
        QStringList getTitles() const {
            QStringList titles;
            for (int i = 0; i < count(); ++i) {
                titles.append(tabText(i));
            }
            return titles;
        }
    };

    // Tab信息结构体
    struct TabInfo {
        QString name;
        QString title;
        QWidget* widget;
        QWidget* controlPanel;
        QAction* action;
        bool isVisible;
        int originalIndex;
    };

    // 创建菜单栏
    QMenuBar* createMenuBar(CloseableTabWidget* tabWidget, QWidget* mainWindow, 
                           QList<TabInfo>& tabInfos, QMap<QString, QWidget*>& controlPanelMap) {
        QMenuBar* menuBar = new QMenuBar(mainWindow);
        menuBar->setStyleSheet(R"(
            QMenuBar {
                background-color: #404040;
                color: white;
                font-size: 14px;
                padding: 5px;
            }
            QMenuBar::item {
                background-color: transparent;
                padding: 5px 15px;
                border-radius: 3px;
            }
            QMenuBar::item:selected {
                background-color: #505050;
            }
            QMenu {
                background-color: #404040;
                color: white;
                border: 1px solid #505050;
                font-size: 14px;
            }
            QMenu::item {
                padding: 8px 30px 8px 20px;
            }
            QMenu::item:selected {
                background-color: #505050;
            }
            QMenu::item:checked {
                background-color: #505050;
            }
            QMenu::separator {
                height: 1px;
                background-color: #505050;
                margin: 5px 0;
            }
        )");

        // Parameter 菜单
        QMenu* parameterMenu = menuBar->addMenu("&Parameter");
        
        // 为每个标签页创建菜单项
        QStringList tabNames = {
            "OpenMesh", "CGAL", "Model", "Shortest Path", 
            "UV Parameterization", "Dual View", 
            "Extended Dual View", "Simple Dual View"
        };
        
        // 创建动作组，确保只有一个被选中
        QActionGroup* tabActionGroup = new QActionGroup(parameterMenu);
        tabActionGroup->setExclusive(true);
        
        for (int i = 0; i < tabNames.size(); ++i) {
            QAction* action = new QAction(tabNames[i], parameterMenu);
            
            // 设置可选中
            action->setCheckable(true);
            
            // 设置第一个标签页为默认选中
            if (i == 0) {
                action->setChecked(true);
            }
            
            // 添加快捷键（Ctrl+数字键）
            if (i < 9) { // 只设置前9个快捷键
                action->setShortcut(QKeySequence(QString("Ctrl+%1").arg(i + 1)));
            }
            
            // 添加到动作组
            tabActionGroup->addAction(action);
            
            // 添加到菜单
            parameterMenu->addAction(action);
            
            // 保存Tab信息
            TabInfo info;
            info.name = tabNames[i];
            info.title = tabNames[i];
            info.action = action;
            info.isVisible = true;
            info.originalIndex = i;
            tabInfos.append(info);
            
            // 将action与tabWidget关联
            tabWidget->setActionForTitle(tabNames[i], action);
            
            // 连接信号 - 修复：正确传递所有需要的变量
            QObject::connect(action, &QAction::triggered, [tabWidget, title = tabNames[i], &tabInfos, mainWindow, action, &controlPanelMap, i]() {
                // 如果Tab当前不可见，先恢复它
                if (tabWidget->indexOf(tabWidget->getWidgetByTitle(title)) == -1) {
                    tabWidget->restoreTabByTitle(title);
                    tabInfos[i].isVisible = true;
                }
                
                // 切换到该tab
                QWidget* widget = tabWidget->getWidgetByTitle(title);
                if (widget) {
                    tabWidget->setCurrentWidget(widget);
                    mainWindow->setWindowTitle("OBJ Viewer - " + title);
                    
                    // 更新action选中状态
                    action->setChecked(true);
                    
                    // 显示对应的控制面板
                    if (controlPanelMap.contains(title)) {
                        QWidget* controlPanel = controlPanelMap[title];
                        // 隐藏所有控制面板
                        for (QWidget* panel : controlPanelMap.values()) {
                            panel->setVisible(false);
                        }
                        // 显示当前控制面板
                        controlPanel->setVisible(true);
                    }
                }
            });
        }
        
        // 添加分隔线
        parameterMenu->addSeparator();
        
        // 添加显示所有tab的选项
        QAction* showAllTabsAction = new QAction("Show All Tabs", parameterMenu);
        QObject::connect(showAllTabsAction, &QAction::triggered, [tabWidget, &tabInfos, tabNames, &controlPanelMap]() {
            // 按原始顺序重新添加所有tab
            for (int i = 0; i < tabNames.size(); ++i) {
                QString title = tabNames[i];
                QWidget* widget = tabWidget->getWidgetByTitle(title);
                if (widget && tabWidget->indexOf(widget) == -1) {
                    tabWidget->restoreTabByTitle(title);
                    tabInfos[i].isVisible = true;
                    
                    // 更新对应的action状态
                    if (tabInfos[i].action) {
                        tabInfos[i].action->setChecked(true);
                    }
                }
            }
            
            // 确保至少有一个tab被选中
            if (tabWidget->count() > 0) {
                tabWidget->setCurrentIndex(0);
                if (tabInfos[0].action) {
                    tabInfos[0].action->setChecked(true);
                }
                
                // 显示第一个tab的控制面板
                if (controlPanelMap.contains(tabNames[0])) {
                    QWidget* controlPanel = controlPanelMap[tabNames[0]];
                    // 隐藏所有控制面板
                    for (QWidget* panel : controlPanelMap.values()) {
                        panel->setVisible(false);
                    }
                    // 显示当前控制面板
                    controlPanel->setVisible(true);
                }
            }
        });
        parameterMenu->addAction(showAllTabsAction);
        
        // 添加退出动作
        QAction* exitAction = new QAction("E&xit", parameterMenu);
        exitAction->setShortcut(QKeySequence::Quit);
        QObject::connect(exitAction, &QAction::triggered, []() {
            QApplication::quit();
        });
        parameterMenu->addAction(exitAction);

        // Render 菜单 (暂时留空，后续可以添加功能)
        QMenu* renderMenu = menuBar->addMenu("&Render");
        
        // 添加渲染选项
        QAction* wireframeAction = new QAction("Toggle Wireframe", renderMenu);
        wireframeAction->setShortcut(QKeySequence("Ctrl+W"));
        wireframeAction->setCheckable(true);
        wireframeAction->setChecked(true);
        renderMenu->addAction(wireframeAction);
        
        renderMenu->addSeparator();
        
        QAction* flatShadingAction = new QAction("Flat Shading", renderMenu);
        flatShadingAction->setShortcut(QKeySequence("Ctrl+F"));
        flatShadingAction->setCheckable(true);
        renderMenu->addAction(flatShadingAction);
        
        QAction* smoothShadingAction = new QAction("Smooth Shading", renderMenu);
        smoothShadingAction->setShortcut(QKeySequence("Ctrl+S"));
        smoothShadingAction->setCheckable(true);
        smoothShadingAction->setChecked(true);
        renderMenu->addAction(smoothShadingAction);
        
        // 创建渲染模式动作组
        QActionGroup* shadingGroup = new QActionGroup(renderMenu);
        shadingGroup->addAction(flatShadingAction);
        shadingGroup->addAction(smoothShadingAction);
        shadingGroup->setExclusive(true);
        
        return menuBar;
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
    QVBoxLayout *outerLayout = new QVBoxLayout(&mainWindow);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->setSpacing(0);
    
    // 创建OpenGL窗口
    ModelGLWidget *modelGlWidget = new ModelGLWidget;
    BaseGLWidget *basicGlWidget = new BaseGLWidget;
    CGALGLWidget *cgalGlWidget = new CGALGLWidget;
    ShortestPathGLWidget *shortestPathGlWidget = new ShortestPathGLWidget;
    UVParamWidget *uvParamWidget = new UVParamWidget;
    
    // 创建双视图窗口 - 左侧为BaseGLWidget，右侧为UVParamWidget
    BaseGLWidget *dualViewLeftWidget = new BaseGLWidget;
    UVParamWidget *dualViewRightWidget = new UVParamWidget;
    
    // 创建扩展双视图窗口 - 左侧为另一个BaseGLWidget，右侧为UVParamWidgetExtended
    BaseGLWidget *dualViewExtendedLeftWidget = new BaseGLWidget;
    UVParamWidgetExtended *uvParamWidgetExtended = new UVParamWidgetExtended;

    // 创建简单双视图窗口 - 左侧为BaseGLWidget，右侧为SimpleSquareWidget
    BaseGLWidget *dualViewSimpleLeftWidget = new BaseGLWidget;
    SimpleSquareWidget *dualViewSimpleRightWidget = new SimpleSquareWidget;

    // 创建标签页widgets
    QWidget* basicTab = createBasicTab(basicGlWidget);
    QWidget* cgalTab = createCGALTab(cgalGlWidget);
    QWidget* modelTab = createModelTab(modelGlWidget);
    QWidget* shortestPathTab = createShortestPathTab(shortestPathGlWidget);
    QWidget* uvParamTab = createUVParamTab(uvParamWidget);
    QWidget* dualViewTab = createDualViewTab(dualViewLeftWidget, dualViewRightWidget);
    QWidget* dualViewExtendedTab = createDualViewExtendedTab(dualViewExtendedLeftWidget, uvParamWidgetExtended);
    QWidget* dualViewSimpleTab = createDualViewSimpleTab(dualViewSimpleLeftWidget, dualViewSimpleRightWidget);

    // 创建自定义的标签页widget
    UIUtils::CloseableTabWidget *tabWidget = new UIUtils::CloseableTabWidget;
    
    // 存储Tab信息的列表
    QList<UIUtils::TabInfo> tabInfos;
    
    // 存储控制面板映射
    QMap<QString, QWidget*> controlPanelMap;
    
    // 创建菜单栏
    QMenuBar* menuBar = UIUtils::createMenuBar(tabWidget, &mainWindow, tabInfos, controlPanelMap);
    outerLayout->addWidget(menuBar);
    
    // 模型信息组
    QLabel *modelInfoLabel = nullptr;
    QLabel *basicInfoLabel = nullptr;
    QLabel *cgalInfoLabel = nullptr;
    QLabel *shortestPathInfoLabel = nullptr;
    QLabel *uvParamInfoLabel = nullptr;
    QLabel *dualViewLeftInfoLabel = nullptr;
    QLabel *dualViewRightInfoLabel = nullptr;
    QLabel *dualViewExtendedLeftInfoLabel = nullptr;
    QLabel *dualViewExtendedRightInfoLabel = nullptr;
    QLabel *dualViewSimpleLeftInfoLabel = nullptr;
    QLabel *dualViewSimpleRightInfoLabel = nullptr;

    // 创建所有控制面板
    QWidget *basicControlPanel = new QWidget;
    QVBoxLayout *basicControlLayout = new QVBoxLayout(basicControlPanel);
    basicControlLayout->setAlignment(Qt::AlignTop);
    basicControlLayout->addWidget(UIUtils::createColorSettingsGroup(basicGlWidget));
    basicControlLayout->addWidget(UIUtils::createModelInfoGroup(&basicInfoLabel));
    basicControlLayout->addWidget(createBasicControlPanel(basicGlWidget, basicInfoLabel, &mainWindow));
    
    QWidget *cgalControlPanel = new QWidget;
    QVBoxLayout *cgalControlLayout = new QVBoxLayout(cgalControlPanel);
    cgalControlLayout->setAlignment(Qt::AlignTop);
    cgalControlLayout->addWidget(UIUtils::createColorSettingsGroup(cgalGlWidget));
    cgalControlLayout->addWidget(UIUtils::createModelInfoGroup(&cgalInfoLabel));
    cgalControlLayout->addWidget(createCGALControlPanel(cgalGlWidget, cgalInfoLabel, &mainWindow));
    
    QWidget *modelControlPanel = new QWidget;
    QVBoxLayout *modelControlLayout = new QVBoxLayout(modelControlPanel);
    modelControlLayout->setAlignment(Qt::AlignTop);
    modelControlLayout->addWidget(UIUtils::createColorSettingsGroup(modelGlWidget));
    modelControlLayout->addWidget(UIUtils::createModelInfoGroup(&modelInfoLabel));
    modelControlLayout->addWidget(createModelControlPanel(modelGlWidget, modelInfoLabel, &mainWindow));
    
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
    
    QWidget *dualViewControlPanel = new QWidget;
    QVBoxLayout *dualViewControlLayout = new QVBoxLayout(dualViewControlPanel);
    dualViewControlLayout->setAlignment(Qt::AlignTop);
    
    QLabel *leftInfoLabel = new QLabel("No model loaded (Left View)");
    leftInfoLabel->setAlignment(Qt::AlignCenter);
    leftInfoLabel->setFixedHeight(50);
    leftInfoLabel->setStyleSheet("background-color: #3A3A3A; color: white; border-radius: 5px; padding: 5px; font-size: 14px;");
    leftInfoLabel->setWordWrap(true);
    
    QLabel *rightInfoLabel = new QLabel("No model loaded (Right View)");
    rightInfoLabel->setAlignment(Qt::AlignCenter);
    rightInfoLabel->setFixedHeight(50);
    rightInfoLabel->setStyleSheet("background-color: #3A3A3A; color: white; border-radius: 5px; padding: 5px; font-size: 14px;");
    rightInfoLabel->setWordWrap(true);
    
    QGroupBox *infoGroup = new QGroupBox("Model Information");
    QVBoxLayout *infoLayout = new QVBoxLayout(infoGroup);
    infoLayout->addWidget(leftInfoLabel);
    infoLayout->addWidget(rightInfoLabel);
    
    dualViewControlLayout->addWidget(infoGroup);
    dualViewControlLayout->addWidget(createDualViewControlPanel(dualViewLeftWidget, dualViewRightWidget, leftInfoLabel, rightInfoLabel, &mainWindow));
    
    QWidget *dualViewExtendedControlPanel = new QWidget;
    QVBoxLayout *dualViewExtendedControlLayout = new QVBoxLayout(dualViewExtendedControlPanel);
    dualViewExtendedControlLayout->setAlignment(Qt::AlignTop);
    
    QLabel *extendedLeftInfoLabel = new QLabel("No model loaded (Left View)");
    extendedLeftInfoLabel->setAlignment(Qt::AlignCenter);
    extendedLeftInfoLabel->setFixedHeight(50);
    extendedLeftInfoLabel->setStyleSheet("background-color: #3A3A3A; color: white; border-radius: 5px; padding: 5px; font-size: 14px;");
    extendedLeftInfoLabel->setWordWrap(true);
    
    QLabel *extendedRightInfoLabel = new QLabel("No model loaded (Right View)");
    extendedRightInfoLabel->setAlignment(Qt::AlignCenter);
    extendedRightInfoLabel->setFixedHeight(50);
    extendedRightInfoLabel->setStyleSheet("background-color: #3A3A3A; color: white; border-radius: 5px; padding: 5px; font-size: 14px;");
    extendedRightInfoLabel->setWordWrap(true);
    
    QGroupBox *extendedInfoGroup = new QGroupBox("Model Information");
    QVBoxLayout *extendedInfoLayout = new QVBoxLayout(extendedInfoGroup);
    extendedInfoLayout->addWidget(extendedLeftInfoLabel);
    extendedInfoLayout->addWidget(extendedRightInfoLabel);
    
    dualViewExtendedControlLayout->addWidget(extendedInfoGroup);
    dualViewExtendedControlLayout->addWidget(createDualViewExtendedControlPanel(dualViewExtendedLeftWidget, uvParamWidgetExtended, extendedLeftInfoLabel, extendedRightInfoLabel, &mainWindow));
    
    QWidget *dualViewSimpleControlPanel = new QWidget;
    QVBoxLayout *dualViewSimpleControlLayout = new QVBoxLayout(dualViewSimpleControlPanel);
    dualViewSimpleControlLayout->setAlignment(Qt::AlignTop);
    
    QLabel *simpleLeftInfoLabel = new QLabel("No model loaded (Left View)");
    simpleLeftInfoLabel->setAlignment(Qt::AlignCenter);
    simpleLeftInfoLabel->setFixedHeight(50);
    simpleLeftInfoLabel->setStyleSheet("background-color: #3A3A3A; color: white; border-radius: 5px; padding: 5px; font-size: 14px;");
    simpleLeftInfoLabel->setWordWrap(true);
    
    QLabel *simpleRightInfoLabel = new QLabel("White Square View - Ready for extension");
    simpleRightInfoLabel->setAlignment(Qt::AlignCenter);
    simpleRightInfoLabel->setFixedHeight(50);
    simpleRightInfoLabel->setStyleSheet("background-color: #3A3A3A; color: white; border-radius: 5px; padding: 5px; font-size: 14px;");
    simpleRightInfoLabel->setWordWrap(true);
    
    QGroupBox *simpleInfoGroup = new QGroupBox("Model Information");
    QVBoxLayout *simpleInfoLayout = new QVBoxLayout(simpleInfoGroup);
    simpleInfoLayout->addWidget(simpleLeftInfoLabel);
    simpleInfoLayout->addWidget(simpleRightInfoLabel);
    
    dualViewSimpleControlLayout->addWidget(simpleInfoGroup);
    dualViewSimpleControlLayout->addWidget(createDualViewSimpleControlPanel(dualViewSimpleLeftWidget, dualViewSimpleRightWidget, simpleLeftInfoLabel, simpleRightInfoLabel, &mainWindow));
    
    // 将所有控制面板添加到映射中
    controlPanelMap["OpenMesh"] = basicControlPanel;
    controlPanelMap["CGAL"] = cgalControlPanel;
    controlPanelMap["Model"] = modelControlPanel;
    controlPanelMap["Shortest Path"] = shortestPathControlPanel;
    controlPanelMap["UV Parameterization"] = uvParamControlPanel;
    controlPanelMap["Dual View"] = dualViewControlPanel;
    controlPanelMap["Extended Dual View"] = dualViewExtendedControlPanel;
    controlPanelMap["Simple Dual View"] = dualViewSimpleControlPanel;
    
    // 隐藏所有控制面板，只显示第一个
    for (QWidget* panel : controlPanelMap.values()) {
        panel->setVisible(false);
    }
    if (!controlPanelMap.isEmpty()) {
        controlPanelMap["OpenMesh"]->setVisible(true);
    }
    
    // 添加所有tab到tabWidget，并更新tabInfos
    tabWidget->addTabWithTitle(basicTab, "OpenMesh");
    tabInfos[0].widget = basicTab;
    tabInfos[0].controlPanel = basicControlPanel;
    
    tabWidget->addTabWithTitle(cgalTab, "CGAL");
    tabInfos[1].widget = cgalTab;
    tabInfos[1].controlPanel = cgalControlPanel;
    
    tabWidget->addTabWithTitle(modelTab, "Model");
    tabInfos[2].widget = modelTab;
    tabInfos[2].controlPanel = modelControlPanel;
    
    tabWidget->addTabWithTitle(shortestPathTab, "Shortest Path");
    tabInfos[3].widget = shortestPathTab;
    tabInfos[3].controlPanel = shortestPathControlPanel;
    
    tabWidget->addTabWithTitle(uvParamTab, "UV Parameterization");
    tabInfos[4].widget = uvParamTab;
    tabInfos[4].controlPanel = uvParamControlPanel;
    
    tabWidget->addTabWithTitle(dualViewTab, "Dual View");
    tabInfos[5].widget = dualViewTab;
    tabInfos[5].controlPanel = dualViewControlPanel;
    
    tabWidget->addTabWithTitle(dualViewExtendedTab, "Extended Dual View");
    tabInfos[6].widget = dualViewExtendedTab;
    tabInfos[6].controlPanel = dualViewExtendedControlPanel;
    
    tabWidget->addTabWithTitle(dualViewSimpleTab, "Simple Dual View");
    tabInfos[7].widget = dualViewSimpleTab;
    tabInfos[7].controlPanel = dualViewSimpleControlPanel;
    
    // 连接tab关闭信号
    QObject::connect(tabWidget, &QTabWidget::tabCloseRequested, [tabWidget, &tabInfos, &controlPanelMap](int index) {
        if (tabWidget->count() <= 1) {
            QMessageBox::warning(nullptr, "Cannot Close Tab", 
                "You must have at least one tab open. Cannot close the last tab.");
            return;
        }
        
        // 获取要关闭的tab的widget和标题
        QWidget* widget = tabWidget->widget(index);
        QString tabTitle = tabWidget->tabText(index);
        
        // 在tabInfos中找到对应的tab
        for (int i = 0; i < tabInfos.size(); ++i) {
            if (tabInfos[i].widget == widget) {
                // 标记为不可见
                tabInfos[i].isVisible = false;
                // 取消选中对应的action
                if (tabInfos[i].action) {
                    tabInfos[i].action->setChecked(false);
                }
                // 隐藏对应的控制面板
                if (tabInfos[i].controlPanel) {
                    tabInfos[i].controlPanel->setVisible(false);
                }
                break;
            }
        }
        
        // 从tabWidget中移除tab（但不删除widget）
        tabWidget->removeTab(index);
        
        // 如果有其他tab，确保选中的action正确，并显示对应的控制面板
        if (tabWidget->count() > 0) {
            int currentIndex = tabWidget->currentIndex();
            if (currentIndex >= 0) {
                QWidget* currentWidget = tabWidget->widget(currentIndex);
                QString currentTitle = tabWidget->tabText(currentIndex);
                
                // 更新菜单栏中对应action的选中状态
                for (int i = 0; i < tabInfos.size(); ++i) {
                    if (tabInfos[i].widget == currentWidget && tabInfos[i].action) {
                        tabInfos[i].action->setChecked(true);
                        break;
                    }
                }
                
                // 显示当前tab对应的控制面板
                if (controlPanelMap.contains(currentTitle)) {
                    QWidget* controlPanel = controlPanelMap[currentTitle];
                    // 隐藏所有控制面板
                    for (QWidget* panel : controlPanelMap.values()) {
                        panel->setVisible(false);
                    }
                    // 显示当前控制面板
                    controlPanel->setVisible(true);
                }
            }
        }
    });
    
    // 连接tab切换信号
    QObject::connect(tabWidget, &QTabWidget::currentChanged, [tabWidget, &tabInfos, &controlPanelMap](int index) {
        if (index >= 0) {
            QWidget* currentWidget = tabWidget->widget(index);
            QString currentTitle = tabWidget->tabText(index);
            
            // 更新菜单栏中对应action的选中状态
            for (int i = 0; i < tabInfos.size(); ++i) {
                if (tabInfos[i].widget == currentWidget && tabInfos[i].action) {
                    tabInfos[i].action->setChecked(true);
                    break;
                }
            }
            
            // 显示当前tab对应的控制面板
            if (controlPanelMap.contains(currentTitle)) {
                QWidget* controlPanel = controlPanelMap[currentTitle];
                // 隐藏所有控制面板
                for (QWidget* panel : controlPanelMap.values()) {
                    panel->setVisible(false);
                }
                // 显示当前控制面板
                controlPanel->setVisible(true);
            }
        }
    });
    
    // 创建主内容区域
    QWidget *contentWidget = new QWidget;
    QHBoxLayout *mainLayout = new QHBoxLayout(contentWidget);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    
    // 创建右侧控制面板容器
    QWidget *controlContainer = new QWidget;
    QVBoxLayout *controlContainerLayout = new QVBoxLayout(controlContainer);
    controlContainerLayout->setAlignment(Qt::AlignTop);
    
    // 将所有控制面板添加到容器中
    for (QWidget* panel : controlPanelMap.values()) {
        controlContainerLayout->addWidget(panel);
    }
    
    // 设置控制面板固定宽度
    controlContainer->setFixedWidth(400);
    
    // 添加控件到主布局
    mainLayout->addWidget(tabWidget, 8);
    mainLayout->addWidget(controlContainer);
    
    // 将内容添加到外层布局
    outerLayout->addWidget(contentWidget, 1);
    
    // 设置主窗口
    mainWindow.setLayout(outerLayout);
    mainWindow.setWindowTitle("OBJ Viewer");
    mainWindow.show();

    return app.exec();
}