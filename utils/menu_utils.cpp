// menu_utils.cpp
#include "menu_utils.h"
#include <QApplication>
#include <QMessageBox>
#include <QTabWidget>
#include <QDebug>

namespace MenuUtils {
    
    // 创建菜单栏
    QMenuBar* createMenuBar(QTabWidget* tabWidget, QWidget* mainWindow, 
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
            "Simple Dual View", "OpenMesh", "CGAL", "Model", "Shortest Path", 
            "UV Parameterization", "Dual View", "Extended Dual View"
        };
        
        // 创建动作组，确保只有一个被选中
        QActionGroup* tabActionGroup = new QActionGroup(parameterMenu);
        tabActionGroup->setExclusive(true);
        
        for (int i = 0; i < tabNames.size(); ++i) {
            QAction* action = new QAction(tabNames[i], parameterMenu);
            
            // 设置可选中
            action->setCheckable(true);
            
            // 设置Simple Dual View为默认选中
            if (tabNames[i] == "Simple Dual View") {
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
            info.isVisible = (tabNames[i] == "Simple Dual View"); // 只有Simple Dual View初始可见
            info.originalIndex = i;
            tabInfos.append(info);
            
            // 连接信号
            QObject::connect(action, &QAction::triggered, [tabWidget, title = tabNames[i], &tabInfos, mainWindow, action, &controlPanelMap]() {
                qDebug() << "Menu action triggered for tab:" << title;
                
                // 如果Tab当前不可见，先恢复它
                QWidget* widget = nullptr;
                for (auto& info : tabInfos) {
                    if (info.title == title && info.widget) {
                        widget = info.widget;
                        info.isVisible = true;
                        break;
                    }
                }
                
                if (widget && tabWidget->indexOf(widget) == -1) {
                    qDebug() << "Adding tab:" << title;
                    // 添加到tabWidget
                    tabWidget->addTab(widget, title);
                }
                
                // 切换到该tab
                if (widget) {
                    qDebug() << "Switching to tab:" << title;
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
                } else {
                    qWarning() << "Widget not found for tab:" << title;
                }
            });
        }
        
        // 添加分隔线
        parameterMenu->addSeparator();
        
        // 添加显示所有tab的选项
        QAction* showAllTabsAction = new QAction("Show All Tabs", parameterMenu);
        QObject::connect(showAllTabsAction, &QAction::triggered, [tabWidget, &tabInfos, &controlPanelMap]() {
            qDebug() << "Show all tabs action triggered";
            // 按原始顺序重新添加所有tab
            for (auto& info : tabInfos) {
                if (!info.isVisible && info.widget) {
                    qDebug() << "Restoring tab:" << info.title;
                    tabWidget->addTab(info.widget, info.title);
                    info.isVisible = true;
                    
                    // 更新对应的action状态
                    if (info.action) {
                        info.action->setChecked(tabWidget->currentWidget() == info.widget);
                    }
                }
            }
            
            // 确保至少有一个tab被选中
            if (tabWidget->count() > 0) {
                // 显示第一个tab的控制面板
                if (!controlPanelMap.isEmpty()) {
                    QString firstTitle = tabWidget->tabText(0);
                    if (controlPanelMap.contains(firstTitle)) {
                        QWidget* controlPanel = controlPanelMap[firstTitle];
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
        parameterMenu->addAction(showAllTabsAction);
        
        // 添加退出动作
        QAction* exitAction = new QAction("E&xit", parameterMenu);
        exitAction->setShortcut(QKeySequence::Quit);
        QObject::connect(exitAction, &QAction::triggered, []() {
            qDebug() << "Exit action triggered";
            QApplication::quit();
        });
        parameterMenu->addAction(exitAction);

        // Render 菜单
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
    
} // namespace MenuUtils