// menu_utils.cpp
#include "menu_utils.h"
#include <QApplication>
#include <QTabWidget>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QActionGroup>
#include <QMessageBox>
#include <QKeySequence>

namespace UIUtils {

    CloseableTabWidget::CloseableTabWidget(QWidget* parent) : QTabWidget(parent) {
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
    
    void CloseableTabWidget::addTabWithTitle(QWidget* widget, const QString& title) {
        tabTitles[widget] = title;
        titleToWidget[title] = widget;
        addTab(widget, title);
    }
    
    QString CloseableTabWidget::getTabTitle(QWidget* widget) const {
        return tabTitles.value(widget, QString());
    }
    
    QStringList CloseableTabWidget::getAllTabTitles() const {
        return tabTitles.values();
    }
    
    QWidget* CloseableTabWidget::getWidgetByTitle(const QString& title) const {
        return titleToWidget.value(title, nullptr);
    }
    
    bool CloseableTabWidget::restoreTabByTitle(const QString& title) {
        QWidget* widget = getWidgetByTitle(title);
        if (widget && indexOf(widget) == -1) {
            // Tab不存在，重新添加
            addTab(widget, title);
            return true;
        }
        return false;
    }
    
    void CloseableTabWidget::setActionForTitle(const QString& title, QAction* action) {
        titleToAction[title] = action;
    }
    
    QAction* CloseableTabWidget::getActionForTitle(const QString& title) const {
        return titleToAction.value(title, nullptr);
    }
    
    QStringList CloseableTabWidget::getTitles() const {
        QStringList titles;
        for (int i = 0; i < count(); ++i) {
            titles.append(tabText(i));
        }
        return titles;
    }

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

} // namespace UIUtils