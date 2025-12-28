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
        // 修改样式表中的关闭按钮部分
        // 将原来的 #CCCCCC 改为白色（white），并添加悬停效果

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
                color: black;  /* 改为白色 */
                font-size: 18px;
                font-weight: bold;
                position: absolute;
                top: 50%;
                left: 50%;
                transform: translate(-50%, -50%);
            }
            
            QTabBar::close-button:hover::after {
                color: #FF6666;  /* 悬停时改为红色，增强可见性 */
                font-size: 20px; /* 悬停时稍微放大 */
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
    
    void CloseableTabWidget::removeWidgetAssociations(QWidget* widget) {
        if (!widget) return;
        
        QString title = tabTitles.take(widget);
        if (!title.isEmpty()) {
            titleToWidget.remove(title);
            titleToAction.remove(title);
        }
    }

    QMenuBar* createMenuBar(CloseableTabWidget* tabWidget, QWidget* mainWindow, 
                           QList<TabInfo>& tabInfos, QMap<QString, QWidget*>& controlPanelMap,
                           std::function<void(const QString&, bool)> createTabFunc) {
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
        
        // 为每个标签页创建菜单项 - 添加新tab "New CGAL-UV View"
        QStringList tabNames = {
            "OpenMesh", "CGAL", "Model", "Shortest Path", 
            "UV Parameterization", "Dual View", 
            "Extended Dual View", "Simple Dual View", "New CGAL-UV View"  // 添加新tab
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
            info.isVisible = false; // 初始不可见，除了第一个
            info.originalIndex = i;
            info.widget = nullptr; // 初始没有widget
            info.controlPanel = nullptr; // 初始没有控制面板
            tabInfos.append(info);
            
            // 将action与tabWidget关联
            tabWidget->setActionForTitle(tabNames[i], action);
            
            // 连接信号 - 菜单点击tab逻辑
            QObject::connect(action, &QAction::triggered, [tabWidget, title = tabNames[i], &tabInfos, mainWindow, action, &controlPanelMap, i, createTabFunc]() {
                // 查找tab是否已经存在且显示在tab栏中
                QWidget* widget = tabWidget->getWidgetByTitle(title);
                bool tabExists = false;
                
                if (widget) {
                    // 检查widget是否在tabWidget中（是否显示）
                    int tabIndex = tabWidget->indexOf(widget);
                    if (tabIndex >= 0) {
                        // Tab已存在且在tab栏中显示，直接切换到该tab
                        tabWidget->setCurrentIndex(tabIndex);
                        tabExists = true;
                    }
                }
                
                if (!tabExists && createTabFunc) {
                    // Tab不存在或不显示，调用创建函数
                    createTabFunc(title, true); // true表示创建后切换到该tab
                }
                
                // 更新窗口标题
                mainWindow->setWindowTitle("OBJ Viewer - " + title);
                
                // 更新所有action选中状态
                for (int j = 0; j < tabInfos.size(); ++j) {
                    if (tabInfos[j].action) {
                        tabInfos[j].action->setChecked(tabInfos[j].title == title);
                    }
                }
                
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
            });
        }
        
        // 添加分隔线
        parameterMenu->addSeparator();
        
        // 添加显示所有tab的选项
        QAction* showAllTabsAction = new QAction("Show All Tabs", parameterMenu);
        QObject::connect(showAllTabsAction, &QAction::triggered, [tabWidget, &tabInfos, tabNames, &controlPanelMap, createTabFunc]() {
            // 创建所有tab
            for (int i = 0; i < tabNames.size(); ++i) {
                QString title = tabNames[i];
                QWidget* widget = tabWidget->getWidgetByTitle(title);
                
                if (!widget && createTabFunc) {
                    createTabFunc(title, false); // 只创建，不切换到该tab
                    widget = tabWidget->getWidgetByTitle(title);
                }
                
                if (widget && tabWidget->indexOf(widget) == -1) {
                    tabWidget->addTab(widget, title);
                    tabInfos[i].isVisible = true;
                }
            }
            
            // 确保至少有一个tab被选中
            if (tabWidget->count() > 0) {
                tabWidget->setCurrentIndex(0);
                
                // 显示第一个tab的控制面板
                QString firstTitle = tabNames[0];
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

namespace UIUtils {

    // 创建颜色设置组
    QGroupBox* createColorSettingsGroup(QWidget* glWidget) {
        QGroupBox *colorGroup = new QGroupBox("Color Settings");
        QVBoxLayout *colorLayout = new QVBoxLayout(colorGroup);
        
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
        
        // 网格颜色按钮
        QPushButton *gridColorButton = new QPushButton("Change Grid Color");
        gridColorButton->setStyleSheet(
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
        
        // 模型颜色按钮
        QPushButton *modelColorButton = new QPushButton("Change Model Color");
        modelColorButton->setStyleSheet(
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
        
        colorLayout->addWidget(bgColorButton);
        colorLayout->addWidget(gridColorButton);
        colorLayout->addWidget(modelColorButton);
        
        // 默认情况下，我们只启用背景颜色按钮，其他按钮可以根据需要连接相应的信号
        // 这里只是一个示例实现，具体功能需要根据BaseGLWidget的接口来调整
        
        // 背景颜色按钮的连接（假设BaseGLWidget有设置背景颜色的方法）
        QObject::connect(bgColorButton, &QPushButton::clicked, [glWidget]() {
            QColor color = QColorDialog::getColor(QColor(25, 25, 25), nullptr, "Select Background Color");
            if (color.isValid()) {
                // 这里需要调用BaseGLWidget的方法来设置背景颜色
                // 例如：glWidget->setBackgroundColor(color);
                // 由于BaseGLWidget的具体实现未知，这里只显示一个消息框
                QMessageBox::information(nullptr, "Color Change", 
                    QString("Background color changed to (%1, %2, %3). Note: This is a demo - implement in BaseGLWidget.")
                    .arg(color.red()).arg(color.green()).arg(color.blue()));
            }
        });
        
        return colorGroup;
    }
    
    // 创建模型信息组
    QGroupBox* createModelInfoGroup(QLabel** infoLabel) {
        QGroupBox *infoGroup = new QGroupBox("Model Information");
        QVBoxLayout *infoLayout = new QVBoxLayout(infoGroup);
        
        // 创建信息标签
        QLabel *label = new QLabel("No model loaded");
        label->setAlignment(Qt::AlignCenter);
        label->setFixedHeight(50);
        label->setStyleSheet("background-color: #3A3A3A; color: white; border-radius: 5px; padding: 5px; font-size: 14px;");
        label->setWordWrap(true);
        
        // 如果提供了infoLabel指针，则赋值
        if (infoLabel) {
            *infoLabel = label;
        }
        
        infoLayout->addWidget(label);
        return infoGroup;
    }

} // namespace UIUtils