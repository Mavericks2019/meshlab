// tab_manager.cpp
#include "tab_manager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QMessageBox>

TabManager::TabManager(QWidget* mainWindow) 
    : QObject(mainWindow)
    , mainWindow(mainWindow)
    , tabWidget(nullptr)
    , menuBar(nullptr)
    , controlContainer(nullptr)
    , modelGlWidget(nullptr)
    , basicGlWidget(nullptr)
    , cgalGlWidget(nullptr)
    , shortestPathGlWidget(nullptr)
    , uvParamWidget(nullptr)
    , dualViewLeftWidget(nullptr)
    , dualViewRightWidget(nullptr)
    , dualViewExtendedLeftWidget(nullptr)
    , uvParamWidgetExtended(nullptr)
    , dualViewSimpleLeftWidget(nullptr)
    , dualViewSimpleRightWidget(nullptr)
    , basicInfoLabel(nullptr)
    , cgalInfoLabel(nullptr)
    , modelInfoLabel(nullptr)
    , shortestPathInfoLabel(nullptr)
    , uvParamInfoLabel(nullptr)
    , dualViewLeftInfoLabel(nullptr)
    , dualViewRightInfoLabel(nullptr)
    , dualViewExtendedLeftInfoLabel(nullptr)
    , dualViewExtendedRightInfoLabel(nullptr)
    , dualViewSimpleLeftInfoLabel(nullptr)
    , dualViewSimpleRightInfoLabel(nullptr) {
}

TabManager::~TabManager() {
    // 注意：Qt的父子对象关系会自动管理大部分内存
}

void TabManager::initializeTabs() {
    // 创建OpenGL窗口
    modelGlWidget = new ModelGLWidget;
    basicGlWidget = new BaseGLWidget;
    cgalGlWidget = new CGALGLWidget;
    shortestPathGlWidget = new ShortestPathGLWidget;
    uvParamWidget = new UVParamWidget;
    
    // 创建双视图窗口
    dualViewLeftWidget = new BaseGLWidget;
    dualViewRightWidget = new UVParamWidget;
    
    // 创建扩展双视图窗口
    dualViewExtendedLeftWidget = new BaseGLWidget;
    uvParamWidgetExtended = new UVParamWidgetExtended;

    // 创建简单双视图窗口
    dualViewSimpleLeftWidget = new BaseGLWidget;
    dualViewSimpleRightWidget = new SimpleSquareWidget;

    // 创建自定义的标签页widget
    tabWidget = new UIUtils::CloseableTabWidget(mainWindow);
    
    // 创建菜单栏
    menuBar = UIUtils::createMenuBar(tabWidget, mainWindow, tabInfos, controlPanelMap);
    
    // 创建控制面板
    createControlPanels();
    
    // 创建所有tab页面
    createBasicTab();
    createCGALTab();
    createModelTab();
    createShortestPathTab();
    createUVParamTab();
    createDualViewTab();
    createDualViewExtendedTab();
    createDualViewSimpleTab();
    
    // 隐藏所有控制面板，只显示第一个
    for (QWidget* panel : controlPanelMap.values()) {
        panel->setVisible(false);
    }
    if (!controlPanelMap.isEmpty()) {
        controlPanelMap["OpenMesh"]->setVisible(true);
    }
    
    // 创建控制面板容器
    controlContainer = new QWidget(mainWindow);
    QVBoxLayout *controlContainerLayout = new QVBoxLayout(controlContainer);
    controlContainerLayout->setAlignment(Qt::AlignTop);
    
    // 将所有控制面板添加到容器中
    for (QWidget* panel : controlPanelMap.values()) {
        controlContainerLayout->addWidget(panel);
    }
    
    // 设置控制面板固定宽度
    controlContainer->setFixedWidth(400);
    
    // 连接信号
    connectSignals();
}

void TabManager::createBasicTab() {
    QWidget* basicTab = ::createBasicTab(basicGlWidget);
    tabWidget->addTabWithTitle(basicTab, "OpenMesh");
    
    // 更新tabInfos
    UIUtils::TabInfo info;
    info.name = "OpenMesh";
    info.title = "OpenMesh";
    info.widget = basicTab;
    info.controlPanel = controlPanelMap["OpenMesh"];
    info.isVisible = true;
    info.originalIndex = 0;
    info.action = tabWidget->getActionForTitle("OpenMesh");
    tabInfos.append(info);
}

void TabManager::createCGALTab() {
    QWidget* cgalTab = ::createCGALTab(cgalGlWidget);
    tabWidget->addTabWithTitle(cgalTab, "CGAL");
    
    // 更新tabInfos
    UIUtils::TabInfo info;
    info.name = "CGAL";
    info.title = "CGAL";
    info.widget = cgalTab;
    info.controlPanel = controlPanelMap["CGAL"];
    info.isVisible = true;
    info.originalIndex = 1;
    info.action = tabWidget->getActionForTitle("CGAL");
    tabInfos.append(info);
}

void TabManager::createModelTab() {
    QWidget* modelTab = ::createModelTab(modelGlWidget);
    tabWidget->addTabWithTitle(modelTab, "Model");
    
    // 更新tabInfos
    UIUtils::TabInfo info;
    info.name = "Model";
    info.title = "Model";
    info.widget = modelTab;
    info.controlPanel = controlPanelMap["Model"];
    info.isVisible = true;
    info.originalIndex = 2;
    info.action = tabWidget->getActionForTitle("Model");
    tabInfos.append(info);
}

void TabManager::createShortestPathTab() {
    QWidget* shortestPathTab = ::createShortestPathTab(shortestPathGlWidget);
    tabWidget->addTabWithTitle(shortestPathTab, "Shortest Path");
    
    // 更新tabInfos
    UIUtils::TabInfo info;
    info.name = "Shortest Path";
    info.title = "Shortest Path";
    info.widget = shortestPathTab;
    info.controlPanel = controlPanelMap["Shortest Path"];
    info.isVisible = true;
    info.originalIndex = 3;
    info.action = tabWidget->getActionForTitle("Shortest Path");
    tabInfos.append(info);
}

void TabManager::createUVParamTab() {
    QWidget* uvParamTab = ::createUVParamTab(uvParamWidget);
    tabWidget->addTabWithTitle(uvParamTab, "UV Parameterization");
    
    // 更新tabInfos
    UIUtils::TabInfo info;
    info.name = "UV Parameterization";
    info.title = "UV Parameterization";
    info.widget = uvParamTab;
    info.controlPanel = controlPanelMap["UV Parameterization"];
    info.isVisible = true;
    info.originalIndex = 4;
    info.action = tabWidget->getActionForTitle("UV Parameterization");
    tabInfos.append(info);
}

void TabManager::createDualViewTab() {
    QWidget* dualViewTab = ::createDualViewTab(dualViewLeftWidget, dualViewRightWidget);
    tabWidget->addTabWithTitle(dualViewTab, "Dual View");
    
    // 更新tabInfos
    UIUtils::TabInfo info;
    info.name = "Dual View";
    info.title = "Dual View";
    info.widget = dualViewTab;
    info.controlPanel = controlPanelMap["Dual View"];
    info.isVisible = true;
    info.originalIndex = 5;
    info.action = tabWidget->getActionForTitle("Dual View");
    tabInfos.append(info);
}

void TabManager::createDualViewExtendedTab() {
    QWidget* dualViewExtendedTab = ::createDualViewExtendedTab(dualViewExtendedLeftWidget, uvParamWidgetExtended);
    tabWidget->addTabWithTitle(dualViewExtendedTab, "Extended Dual View");
    
    // 更新tabInfos
    UIUtils::TabInfo info;
    info.name = "Extended Dual View";
    info.title = "Extended Dual View";
    info.widget = dualViewExtendedTab;
    info.controlPanel = controlPanelMap["Extended Dual View"];
    info.isVisible = true;
    info.originalIndex = 6;
    info.action = tabWidget->getActionForTitle("Extended Dual View");
    tabInfos.append(info);
}

void TabManager::createDualViewSimpleTab() {
    QWidget* dualViewSimpleTab = ::createDualViewSimpleTab(dualViewSimpleLeftWidget, dualViewSimpleRightWidget);
    tabWidget->addTabWithTitle(dualViewSimpleTab, "Simple Dual View");
    
    // 更新tabInfos
    UIUtils::TabInfo info;
    info.name = "Simple Dual View";
    info.title = "Simple Dual View";
    info.widget = dualViewSimpleTab;
    info.controlPanel = controlPanelMap["Simple Dual View"];
    info.isVisible = true;
    info.originalIndex = 7;
    info.action = tabWidget->getActionForTitle("Simple Dual View");
    tabInfos.append(info);
}

void TabManager::createControlPanels() {
    // 创建基本控制面板
    QWidget *basicControlPanel = new QWidget;
    QVBoxLayout *basicControlLayout = new QVBoxLayout(basicControlPanel);
    basicControlLayout->setAlignment(Qt::AlignTop);
    basicControlLayout->addWidget(UIUtils::createColorSettingsGroup(basicGlWidget));
    basicControlLayout->addWidget(UIUtils::createModelInfoGroup(&basicInfoLabel));
    basicControlLayout->addWidget(createBasicControlPanel(basicGlWidget, basicInfoLabel, mainWindow));
    
    // 创建CGAL控制面板
    QWidget *cgalControlPanel = new QWidget;
    QVBoxLayout *cgalControlLayout = new QVBoxLayout(cgalControlPanel);
    cgalControlLayout->setAlignment(Qt::AlignTop);
    cgalControlLayout->addWidget(UIUtils::createColorSettingsGroup(cgalGlWidget));
    cgalControlLayout->addWidget(UIUtils::createModelInfoGroup(&cgalInfoLabel));
    cgalControlLayout->addWidget(createCGALControlPanel(cgalGlWidget, cgalInfoLabel, mainWindow));
    
    // 创建模型控制面板
    QWidget *modelControlPanel = new QWidget;
    QVBoxLayout *modelControlLayout = new QVBoxLayout(modelControlPanel);
    modelControlLayout->setAlignment(Qt::AlignTop);
    modelControlLayout->addWidget(UIUtils::createColorSettingsGroup(modelGlWidget));
    modelControlLayout->addWidget(UIUtils::createModelInfoGroup(&modelInfoLabel));
    modelControlLayout->addWidget(createModelControlPanel(modelGlWidget, modelInfoLabel, mainWindow));
    
    // 创建最短路径控制面板
    QWidget *shortestPathControlPanel = new QWidget;
    QVBoxLayout *shortestPathControlLayout = new QVBoxLayout(shortestPathControlPanel);
    shortestPathControlLayout->setAlignment(Qt::AlignTop);
    shortestPathControlLayout->addWidget(UIUtils::createColorSettingsGroup(shortestPathGlWidget));
    shortestPathControlLayout->addWidget(UIUtils::createModelInfoGroup(&shortestPathInfoLabel));
    shortestPathControlLayout->addWidget(createShortestPathControlPanel(shortestPathGlWidget, shortestPathInfoLabel, mainWindow));
    
    // 创建UV参数化控制面板
    QWidget *uvParamControlPanel = new QWidget;
    QVBoxLayout *uvParamControlLayout = new QVBoxLayout(uvParamControlPanel);
    uvParamControlLayout->setAlignment(Qt::AlignTop);
    uvParamControlLayout->addWidget(UIUtils::createModelInfoGroup(&uvParamInfoLabel));
    uvParamControlLayout->addWidget(createUVParamControlPanel(uvParamWidget, uvParamInfoLabel, mainWindow));
    
    // 创建双视图控制面板
    QWidget *dualViewControlPanel = new QWidget;
    QVBoxLayout *dualViewControlLayout = new QVBoxLayout(dualViewControlPanel);
    dualViewControlLayout->setAlignment(Qt::AlignTop);
    
    dualViewLeftInfoLabel = new QLabel("No model loaded (Left View)");
    dualViewLeftInfoLabel->setAlignment(Qt::AlignCenter);
    dualViewLeftInfoLabel->setFixedHeight(50);
    dualViewLeftInfoLabel->setStyleSheet("background-color: #3A3A3A; color: white; border-radius: 5px; padding: 5px; font-size: 14px;");
    dualViewLeftInfoLabel->setWordWrap(true);
    
    dualViewRightInfoLabel = new QLabel("No model loaded (Right View)");
    dualViewRightInfoLabel->setAlignment(Qt::AlignCenter);
    dualViewRightInfoLabel->setFixedHeight(50);
    dualViewRightInfoLabel->setStyleSheet("background-color: #3A3A3A; color: white; border-radius: 5px; padding: 5px; font-size: 14px;");
    dualViewRightInfoLabel->setWordWrap(true);
    
    QGroupBox *infoGroup = new QGroupBox("Model Information");
    QVBoxLayout *infoLayout = new QVBoxLayout(infoGroup);
    infoLayout->addWidget(dualViewLeftInfoLabel);
    infoLayout->addWidget(dualViewRightInfoLabel);
    
    dualViewControlLayout->addWidget(infoGroup);
    dualViewControlLayout->addWidget(createDualViewControlPanel(dualViewLeftWidget, dualViewRightWidget, dualViewLeftInfoLabel, dualViewRightInfoLabel, mainWindow));
    
    // 创建扩展双视图控制面板
    QWidget *dualViewExtendedControlPanel = new QWidget;
    QVBoxLayout *dualViewExtendedControlLayout = new QVBoxLayout(dualViewExtendedControlPanel);
    dualViewExtendedControlLayout->setAlignment(Qt::AlignTop);
    
    dualViewExtendedLeftInfoLabel = new QLabel("No model loaded (Left View)");
    dualViewExtendedLeftInfoLabel->setAlignment(Qt::AlignCenter);
    dualViewExtendedLeftInfoLabel->setFixedHeight(50);
    dualViewExtendedLeftInfoLabel->setStyleSheet("background-color: #3A3A3A; color: white; border-radius: 5px; padding: 5px; font-size: 14px;");
    dualViewExtendedLeftInfoLabel->setWordWrap(true);
    
    dualViewExtendedRightInfoLabel = new QLabel("No model loaded (Right View)");
    dualViewExtendedRightInfoLabel->setAlignment(Qt::AlignCenter);
    dualViewExtendedRightInfoLabel->setFixedHeight(50);
    dualViewExtendedRightInfoLabel->setStyleSheet("background-color: #3A3A3A; color: white; border-radius: 5px; padding: 5px; font-size: 14px;");
    dualViewExtendedRightInfoLabel->setWordWrap(true);
    
    QGroupBox *extendedInfoGroup = new QGroupBox("Model Information");
    QVBoxLayout *extendedInfoLayout = new QVBoxLayout(extendedInfoGroup);
    extendedInfoLayout->addWidget(dualViewExtendedLeftInfoLabel);
    extendedInfoLayout->addWidget(dualViewExtendedRightInfoLabel);
    
    dualViewExtendedControlLayout->addWidget(extendedInfoGroup);
    dualViewExtendedControlLayout->addWidget(createDualViewExtendedControlPanel(dualViewExtendedLeftWidget, uvParamWidgetExtended, dualViewExtendedLeftInfoLabel, dualViewExtendedRightInfoLabel, mainWindow));
    
    // 创建简单双视图控制面板
    QWidget *dualViewSimpleControlPanel = new QWidget;
    QVBoxLayout *dualViewSimpleControlLayout = new QVBoxLayout(dualViewSimpleControlPanel);
    dualViewSimpleControlLayout->setAlignment(Qt::AlignTop);
    
    dualViewSimpleLeftInfoLabel = new QLabel("No model loaded (Left View)");
    dualViewSimpleLeftInfoLabel->setAlignment(Qt::AlignCenter);
    dualViewSimpleLeftInfoLabel->setFixedHeight(50);
    dualViewSimpleLeftInfoLabel->setStyleSheet("background-color: #3A3A3A; color: white; border-radius: 5px; padding: 5px; font-size: 14px;");
    dualViewSimpleLeftInfoLabel->setWordWrap(true);
    
    dualViewSimpleRightInfoLabel = new QLabel("White Square View - Ready for extension");
    dualViewSimpleRightInfoLabel->setAlignment(Qt::AlignCenter);
    dualViewSimpleRightInfoLabel->setFixedHeight(50);
    dualViewSimpleRightInfoLabel->setStyleSheet("background-color: #3A3A3A; color: white; border-radius: 5px; padding: 5px; font-size: 14px;");
    dualViewSimpleRightInfoLabel->setWordWrap(true);
    
    QGroupBox *simpleInfoGroup = new QGroupBox("Model Information");
    QVBoxLayout *simpleInfoLayout = new QVBoxLayout(simpleInfoGroup);
    simpleInfoLayout->addWidget(dualViewSimpleLeftInfoLabel);
    simpleInfoLayout->addWidget(dualViewSimpleRightInfoLabel);
    
    dualViewSimpleControlLayout->addWidget(simpleInfoGroup);
    dualViewSimpleControlLayout->addWidget(createDualViewSimpleControlPanel(dualViewSimpleLeftWidget, dualViewSimpleRightWidget, dualViewSimpleLeftInfoLabel, dualViewSimpleRightInfoLabel, mainWindow));
    
    // 将所有控制面板添加到映射中
    controlPanelMap["OpenMesh"] = basicControlPanel;
    controlPanelMap["CGAL"] = cgalControlPanel;
    controlPanelMap["Model"] = modelControlPanel;
    controlPanelMap["Shortest Path"] = shortestPathControlPanel;
    controlPanelMap["UV Parameterization"] = uvParamControlPanel;
    controlPanelMap["Dual View"] = dualViewControlPanel;
    controlPanelMap["Extended Dual View"] = dualViewExtendedControlPanel;
    controlPanelMap["Simple Dual View"] = dualViewSimpleControlPanel;
}

void TabManager::connectSignals() {
    // 连接tab关闭信号
    QObject::connect(tabWidget, &QTabWidget::tabCloseRequested, [this](int index) {
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
    QObject::connect(tabWidget, &QTabWidget::currentChanged, [this](int index) {
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
}