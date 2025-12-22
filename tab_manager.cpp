// tab_manager.cpp
#include "tab_manager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QMessageBox>
#include <QDebug>

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
    // 清理所有已创建的tab
    QStringList tabNames = {
        "OpenMesh", "CGAL", "Model", "Shortest Path", 
        "UV Parameterization", "Dual View", 
        "Extended Dual View", "Simple Dual View"
    };
    
    for (const QString& title : tabNames) {
        cleanupTab(title);
    }
}

void TabManager::initializeTabs() {
    // 创建自定义的标签页widget
    tabWidget = new UIUtils::CloseableTabWidget(mainWindow);
    
    // 创建菜单栏，传递createTab函数
    auto createTabFunc = [this](const QString& title, bool switchToTab) { 
        this->createTab(title, switchToTab); 
    };
    menuBar = UIUtils::createMenuBar(tabWidget, mainWindow, tabInfos, controlPanelMap, createTabFunc);
    
    // 创建控制面板容器
    controlContainer = new QWidget(mainWindow);
    QVBoxLayout *controlContainerLayout = new QVBoxLayout(controlContainer);
    controlContainerLayout->setAlignment(Qt::AlignTop);
    controlContainerLayout->setContentsMargins(0, 0, 0, 0);
    
    // 设置控制面板固定宽度
    controlContainer->setFixedWidth(400);
    
    // 先连接信号
    connectSignals();
    
    // 创建第一个tab（OpenMesh），并切换到它
    createTab("OpenMesh", true);
    
    // 重要：手动触发第一次控制面板显示
    // 因为连接信号是在创建第一个tab之前，所以需要手动显示控制面板
    if (tabWidget->count() > 0) {
        QString currentTitle = tabWidget->tabText(0);
        
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

void TabManager::createTab(const QString& title, bool switchToTab) {
    // 首先检查tab是否已经存在且显示在tab栏中
    QWidget* existingWidget = tabWidget->getWidgetByTitle(title);
    if (existingWidget && tabWidget->indexOf(existingWidget) >= 0) {
        // Tab已存在且显示在tab栏中
        if (switchToTab) {
            // 切换到该tab
            tabWidget->setCurrentWidget(existingWidget);
            
            // 更新窗口标题
            mainWindow->setWindowTitle("OBJ Viewer - " + title);
        }
        return;
    }
    
    // 如果Tab之前被创建过但被删除了，清理残留资源
    if (tabCreated.value(title, false)) {
        // 检查widget是否仍然存在但不在tab栏中
        if (existingWidget) {
            // widget存在但不在tab栏中，重新添加到tab栏
            tabWidget->addTab(existingWidget, title);
            
            if (switchToTab) {
                // 切换到新添加的tab
                int index = tabWidget->indexOf(existingWidget);
                if (index >= 0) {
                    tabWidget->setCurrentIndex(index);
                }
            }
            
            // 更新tabInfos
            for (int i = 0; i < tabInfos.size(); ++i) {
                if (tabInfos[i].title == title) {
                    tabInfos[i].widget = existingWidget;
                    tabInfos[i].isVisible = true;
                    break;
                }
            }
            
            return;
        } else {
            // 清理残留资源
            cleanupTab(title);
        }
    }
    
    // 根据标题创建相应的tab
    if (title == "OpenMesh") {
        createBasicTab();
    } else if (title == "CGAL") {
        createCGALTab();
    } else if (title == "Model") {
        createModelTab();
    } else if (title == "Shortest Path") {
        createShortestPathTab();
    } else if (title == "UV Parameterization") {
        createUVParamTab();
    } else if (title == "Dual View") {
        createDualViewTab();
    } else if (title == "Extended Dual View") {
        createDualViewExtendedTab();
    } else if (title == "Simple Dual View") {
        createDualViewSimpleTab();
    }
    
    // 标记为已创建
    tabCreated[title] = true;
    
    // 如果switchToTab为true，切换到新创建的tab
    if (switchToTab) {
        QWidget* widget = tabWidget->getWidgetByTitle(title);
        if (widget) {
            tabWidget->setCurrentWidget(widget);
            
            // 更新窗口标题
            mainWindow->setWindowTitle("OBJ Viewer - " + title);
        }
    }
}

void TabManager::cleanupTab(const QString& title) {
    // 清理tab资源
    if (title == "OpenMesh") {
        if (basicGlWidget) {
            delete basicGlWidget;
            basicGlWidget = nullptr;
        }
        if (basicInfoLabel) {
            delete basicInfoLabel;
            basicInfoLabel = nullptr;
        }
    } else if (title == "CGAL") {
        if (cgalGlWidget) {
            delete cgalGlWidget;
            cgalGlWidget = nullptr;
        }
        if (cgalInfoLabel) {
            delete cgalInfoLabel;
            cgalInfoLabel = nullptr;
        }
    } else if (title == "Model") {
        if (modelGlWidget) {
            delete modelGlWidget;
            modelGlWidget = nullptr;
        }
        if (modelInfoLabel) {
            delete modelInfoLabel;
            modelInfoLabel = nullptr;
        }
    } else if (title == "Shortest Path") {
        if (shortestPathGlWidget) {
            delete shortestPathGlWidget;
            shortestPathGlWidget = nullptr;
        }
        if (shortestPathInfoLabel) {
            delete shortestPathInfoLabel;
            shortestPathInfoLabel = nullptr;
        }
    } else if (title == "UV Parameterization") {
        if (uvParamWidget) {
            delete uvParamWidget;
            uvParamWidget = nullptr;
        }
        if (uvParamInfoLabel) {
            delete uvParamInfoLabel;
            uvParamInfoLabel = nullptr;
        }
    } else if (title == "Dual View") {
        if (dualViewLeftWidget) {
            delete dualViewLeftWidget;
            dualViewLeftWidget = nullptr;
        }
        if (dualViewRightWidget) {
            delete dualViewRightWidget;
            dualViewRightWidget = nullptr;
        }
        if (dualViewLeftInfoLabel) {
            delete dualViewLeftInfoLabel;
            dualViewLeftInfoLabel = nullptr;
        }
        if (dualViewRightInfoLabel) {
            delete dualViewRightInfoLabel;
            dualViewRightInfoLabel = nullptr;
        }
    } else if (title == "Extended Dual View") {
        if (dualViewExtendedLeftWidget) {
            delete dualViewExtendedLeftWidget;
            dualViewExtendedLeftWidget = nullptr;
        }
        if (uvParamWidgetExtended) {
            delete uvParamWidgetExtended;
            uvParamWidgetExtended = nullptr;
        }
        if (dualViewExtendedLeftInfoLabel) {
            delete dualViewExtendedLeftInfoLabel;
            dualViewExtendedLeftInfoLabel = nullptr;
        }
        if (dualViewExtendedRightInfoLabel) {
            delete dualViewExtendedRightInfoLabel;
            dualViewExtendedRightInfoLabel = nullptr;
        }
    } else if (title == "Simple Dual View") {
        if (dualViewSimpleLeftWidget) {
            delete dualViewSimpleLeftWidget;
            dualViewSimpleLeftWidget = nullptr;
        }
        if (dualViewSimpleRightWidget) {
            delete dualViewSimpleRightWidget;
            dualViewSimpleRightWidget = nullptr;
        }
        if (dualViewSimpleLeftInfoLabel) {
            delete dualViewSimpleLeftInfoLabel;
            dualViewSimpleLeftInfoLabel = nullptr;
        }
        if (dualViewSimpleRightInfoLabel) {
            delete dualViewSimpleRightInfoLabel;
            dualViewSimpleRightInfoLabel = nullptr;
        }
    }
    
    // 清理控制面板
    if (controlPanelMap.contains(title)) {
        QWidget* controlPanel = controlPanelMap[title];
        controlContainer->layout()->removeWidget(controlPanel);
        delete controlPanel;
        controlPanelMap.remove(title);
    }
    
    // 从tabInfos中移除对应的记录
    for (int i = tabInfos.size() - 1; i >= 0; --i) {
        if (tabInfos[i].title == title) {
            // 清理widget关联
            if (tabInfos[i].widget) {
                tabWidget->removeWidgetAssociations(tabInfos[i].widget);
            }
            tabInfos.removeAt(i);
        }
    }
    
    // 重置创建标记
    tabCreated[title] = false;
}

void TabManager::deleteTab(const QString& title) {
    // 查找对应的widget
    QWidget* widget = tabWidget->getWidgetByTitle(title);
    if (!widget) return;
    
    // 从tabWidget中移除
    int index = tabWidget->indexOf(widget);
    if (index >= 0) {
        tabWidget->removeTab(index);
    }
    
    // 清理widget关联
    tabWidget->removeWidgetAssociations(widget);
    
    // 从tabInfos中更新状态
    for (int i = 0; i < tabInfos.size(); ++i) {
        if (tabInfos[i].title == title) {
            tabInfos[i].widget = nullptr;
            tabInfos[i].isVisible = false;
            if (tabInfos[i].action) {
                tabInfos[i].action->setChecked(false);
            }
            break;
        }
    }
}

void TabManager::createBasicTab() {
    if (!basicGlWidget) {
        basicGlWidget = new BaseGLWidget;
    }
    
    QWidget* basicTab = ::createBasicTab(basicGlWidget);
    tabWidget->addTabWithTitle(basicTab, "OpenMesh");
    
    // 创建控制面板（如果不存在）
    if (!controlPanelMap.contains("OpenMesh")) {
        createControlPanel("OpenMesh");
    }
    
    // 更新tabInfos
    UIUtils::TabInfo info;
    info.name = "OpenMesh";
    info.title = "OpenMesh";
    info.widget = basicTab;
    info.controlPanel = controlPanelMap["OpenMesh"];
    info.isVisible = true;
    info.originalIndex = 0;
    info.action = tabWidget->getActionForTitle("OpenMesh");
    
    // 替换或添加tab信息
    bool found = false;
    for (int i = 0; i < tabInfos.size(); ++i) {
        if (tabInfos[i].title == "OpenMesh") {
            tabInfos[i] = info;
            found = true;
            break;
        }
    }
    if (!found) {
        tabInfos.append(info);
    }
}

void TabManager::createCGALTab() {
    if (!cgalGlWidget) {
        cgalGlWidget = new CGALGLWidget;
    }
    
    QWidget* cgalTab = ::createCGALTab(cgalGlWidget);
    tabWidget->addTabWithTitle(cgalTab, "CGAL");
    
    // 创建控制面板（如果不存在）
    if (!controlPanelMap.contains("CGAL")) {
        createControlPanel("CGAL");
    }
    
    // 更新tabInfos
    UIUtils::TabInfo info;
    info.name = "CGAL";
    info.title = "CGAL";
    info.widget = cgalTab;
    info.controlPanel = controlPanelMap["CGAL"];
    info.isVisible = true;
    info.originalIndex = 1;
    info.action = tabWidget->getActionForTitle("CGAL");
    
    // 替换或添加tab信息
    bool found = false;
    for (int i = 0; i < tabInfos.size(); ++i) {
        if (tabInfos[i].title == "CGAL") {
            tabInfos[i] = info;
            found = true;
            break;
        }
    }
    if (!found) {
        tabInfos.append(info);
    }
}

void TabManager::createModelTab() {
    if (!modelGlWidget) {
        modelGlWidget = new ModelGLWidget;
    }
    
    QWidget* modelTab = ::createModelTab(modelGlWidget);
    tabWidget->addTabWithTitle(modelTab, "Model");
    
    // 创建控制面板（如果不存在）
    if (!controlPanelMap.contains("Model")) {
        createControlPanel("Model");
    }
    
    // 更新tabInfos
    UIUtils::TabInfo info;
    info.name = "Model";
    info.title = "Model";
    info.widget = modelTab;
    info.controlPanel = controlPanelMap["Model"];
    info.isVisible = true;
    info.originalIndex = 2;
    info.action = tabWidget->getActionForTitle("Model");
    
    // 替换或添加tab信息
    bool found = false;
    for (int i = 0; i < tabInfos.size(); ++i) {
        if (tabInfos[i].title == "Model") {
            tabInfos[i] = info;
            found = true;
            break;
        }
    }
    if (!found) {
        tabInfos.append(info);
    }
}

void TabManager::createShortestPathTab() {
    if (!shortestPathGlWidget) {
        shortestPathGlWidget = new ShortestPathGLWidget;
    }
    
    QWidget* shortestPathTab = ::createShortestPathTab(shortestPathGlWidget);
    tabWidget->addTabWithTitle(shortestPathTab, "Shortest Path");
    
    // 创建控制面板（如果不存在）
    if (!controlPanelMap.contains("Shortest Path")) {
        createControlPanel("Shortest Path");
    }
    
    // 更新tabInfos
    UIUtils::TabInfo info;
    info.name = "Shortest Path";
    info.title = "Shortest Path";
    info.widget = shortestPathTab;
    info.controlPanel = controlPanelMap["Shortest Path"];
    info.isVisible = true;
    info.originalIndex = 3;
    info.action = tabWidget->getActionForTitle("Shortest Path");
    
    // 替换或添加tab信息
    bool found = false;
    for (int i = 0; i < tabInfos.size(); ++i) {
        if (tabInfos[i].title == "Shortest Path") {
            tabInfos[i] = info;
            found = true;
            break;
        }
    }
    if (!found) {
        tabInfos.append(info);
    }
}

void TabManager::createUVParamTab() {
    if (!uvParamWidget) {
        uvParamWidget = new UVParamWidget;
    }
    
    QWidget* uvParamTab = ::createUVParamTab(uvParamWidget);
    tabWidget->addTabWithTitle(uvParamTab, "UV Parameterization");
    
    // 创建控制面板（如果不存在）
    if (!controlPanelMap.contains("UV Parameterization")) {
        createControlPanel("UV Parameterization");
    }
    
    // 更新tabInfos
    UIUtils::TabInfo info;
    info.name = "UV Parameterization";
    info.title = "UV Parameterization";
    info.widget = uvParamTab;
    info.controlPanel = controlPanelMap["UV Parameterization"];
    info.isVisible = true;
    info.originalIndex = 4;
    info.action = tabWidget->getActionForTitle("UV Parameterization");
    
    // 替换或添加tab信息
    bool found = false;
    for (int i = 0; i < tabInfos.size(); ++i) {
        if (tabInfos[i].title == "UV Parameterization") {
            tabInfos[i] = info;
            found = true;
            break;
        }
    }
    if (!found) {
        tabInfos.append(info);
    }
}

void TabManager::createDualViewTab() {
    if (!dualViewLeftWidget) {
        dualViewLeftWidget = new BaseGLWidget;
    }
    if (!dualViewRightWidget) {
        dualViewRightWidget = new UVParamWidget;
    }
    
    QWidget* dualViewTab = ::createDualViewTab(dualViewLeftWidget, dualViewRightWidget);
    tabWidget->addTabWithTitle(dualViewTab, "Dual View");
    
    // 创建控制面板（如果不存在）
    if (!controlPanelMap.contains("Dual View")) {
        createControlPanel("Dual View");
    }
    
    // 更新tabInfos
    UIUtils::TabInfo info;
    info.name = "Dual View";
    info.title = "Dual View";
    info.widget = dualViewTab;
    info.controlPanel = controlPanelMap["Dual View"];
    info.isVisible = true;
    info.originalIndex = 5;
    info.action = tabWidget->getActionForTitle("Dual View");
    
    // 替换或添加tab信息
    bool found = false;
    for (int i = 0; i < tabInfos.size(); ++i) {
        if (tabInfos[i].title == "Dual View") {
            tabInfos[i] = info;
            found = true;
            break;
        }
    }
    if (!found) {
        tabInfos.append(info);
    }
}

void TabManager::createDualViewExtendedTab() {
    if (!dualViewExtendedLeftWidget) {
        dualViewExtendedLeftWidget = new BaseGLWidget;
    }
    if (!uvParamWidgetExtended) {
        uvParamWidgetExtended = new UVParamWidgetExtended;
    }
    
    QWidget* dualViewExtendedTab = ::createDualViewExtendedTab(dualViewExtendedLeftWidget, uvParamWidgetExtended);
    tabWidget->addTabWithTitle(dualViewExtendedTab, "Extended Dual View");
    
    // 创建控制面板（如果不存在）
    if (!controlPanelMap.contains("Extended Dual View")) {
        createControlPanel("Extended Dual View");
    }
    
    // 更新tabInfos
    UIUtils::TabInfo info;
    info.name = "Extended Dual View";
    info.title = "Extended Dual View";
    info.widget = dualViewExtendedTab;
    info.controlPanel = controlPanelMap["Extended Dual View"];
    info.isVisible = true;
    info.originalIndex = 6;
    info.action = tabWidget->getActionForTitle("Extended Dual View");
    
    // 替换或添加tab信息
    bool found = false;
    for (int i = 0; i < tabInfos.size(); ++i) {
        if (tabInfos[i].title == "Extended Dual View") {
            tabInfos[i] = info;
            found = true;
            break;
        }
    }
    if (!found) {
        tabInfos.append(info);
    }
}

void TabManager::createDualViewSimpleTab() {
    if (!dualViewSimpleLeftWidget) {
        dualViewSimpleLeftWidget = new BaseGLWidget;
    }
    if (!dualViewSimpleRightWidget) {
        dualViewSimpleRightWidget = new SimpleSquareWidget;
    }
    
    QWidget* dualViewSimpleTab = ::createDualViewSimpleTab(dualViewSimpleLeftWidget, dualViewSimpleRightWidget);
    tabWidget->addTabWithTitle(dualViewSimpleTab, "Simple Dual View");
    
    // 创建控制面板（如果不存在）
    if (!controlPanelMap.contains("Simple Dual View")) {
        createControlPanel("Simple Dual View");
    }
    
    // 更新tabInfos
    UIUtils::TabInfo info;
    info.name = "Simple Dual View";
    info.title = "Simple Dual View";
    info.widget = dualViewSimpleTab;
    info.controlPanel = controlPanelMap["Simple Dual View"];
    info.isVisible = true;
    info.originalIndex = 7;
    info.action = tabWidget->getActionForTitle("Simple Dual View");
    
    // 替换或添加tab信息
    bool found = false;
    for (int i = 0; i < tabInfos.size(); ++i) {
        if (tabInfos[i].title == "Simple Dual View") {
            tabInfos[i] = info;
            found = true;
            break;
        }
    }
    if (!found) {
        tabInfos.append(info);
    }
}

void TabManager::createControlPanel(const QString& title) {
    // 如果控制面板已经存在，直接返回
    if (controlPanelMap.contains(title)) {
        return;
    }
    
    QWidget* controlPanel = nullptr;
    
    if (title == "OpenMesh") {
        if (!basicGlWidget) basicGlWidget = new BaseGLWidget;
        
        QWidget *basicControlPanel = new QWidget;
        QVBoxLayout *basicControlLayout = new QVBoxLayout(basicControlPanel);
        basicControlLayout->setAlignment(Qt::AlignTop);
        basicControlLayout->addWidget(UIUtils::createColorSettingsGroup(basicGlWidget));
        basicControlLayout->addWidget(UIUtils::createModelInfoGroup(&basicInfoLabel));
        basicControlLayout->addWidget(createBasicControlPanel(basicGlWidget, basicInfoLabel, mainWindow));
        
        controlPanel = basicControlPanel;
        
    } else if (title == "CGAL") {
        if (!cgalGlWidget) cgalGlWidget = new CGALGLWidget;
        
        QWidget *cgalControlPanel = new QWidget;
        QVBoxLayout *cgalControlLayout = new QVBoxLayout(cgalControlPanel);
        cgalControlLayout->setAlignment(Qt::AlignTop);
        cgalControlLayout->addWidget(UIUtils::createColorSettingsGroup(cgalGlWidget));
        cgalControlLayout->addWidget(UIUtils::createModelInfoGroup(&cgalInfoLabel));
        cgalControlLayout->addWidget(createCGALControlPanel(cgalGlWidget, cgalInfoLabel, mainWindow));
        
        controlPanel = cgalControlPanel;
        
    } else if (title == "Model") {
        if (!modelGlWidget) modelGlWidget = new ModelGLWidget;
        
        QWidget *modelControlPanel = new QWidget;
        QVBoxLayout *modelControlLayout = new QVBoxLayout(modelControlPanel);
        modelControlLayout->setAlignment(Qt::AlignTop);
        modelControlLayout->addWidget(UIUtils::createColorSettingsGroup(modelGlWidget));
        modelControlLayout->addWidget(UIUtils::createModelInfoGroup(&modelInfoLabel));
        modelControlLayout->addWidget(createModelControlPanel(modelGlWidget, modelInfoLabel, mainWindow));
        
        controlPanel = modelControlPanel;
        
    } else if (title == "Shortest Path") {
        if (!shortestPathGlWidget) shortestPathGlWidget = new ShortestPathGLWidget;
        
        QWidget *shortestPathControlPanel = new QWidget;
        QVBoxLayout *shortestPathControlLayout = new QVBoxLayout(shortestPathControlPanel);
        shortestPathControlLayout->setAlignment(Qt::AlignTop);
        shortestPathControlLayout->addWidget(UIUtils::createColorSettingsGroup(shortestPathGlWidget));
        shortestPathControlLayout->addWidget(UIUtils::createModelInfoGroup(&shortestPathInfoLabel));
        shortestPathControlLayout->addWidget(createShortestPathControlPanel(shortestPathGlWidget, shortestPathInfoLabel, mainWindow));
        
        controlPanel = shortestPathControlPanel;
        
    } else if (title == "UV Parameterization") {
        if (!uvParamWidget) uvParamWidget = new UVParamWidget;
        
        QWidget *uvParamControlPanel = new QWidget;
        QVBoxLayout *uvParamControlLayout = new QVBoxLayout(uvParamControlPanel);
        uvParamControlLayout->setAlignment(Qt::AlignTop);
        uvParamControlLayout->addWidget(UIUtils::createModelInfoGroup(&uvParamInfoLabel));
        uvParamControlLayout->addWidget(createUVParamControlPanel(uvParamWidget, uvParamInfoLabel, mainWindow));
        
        controlPanel = uvParamControlPanel;
        
    } else if (title == "Dual View") {
        if (!dualViewLeftWidget) dualViewLeftWidget = new BaseGLWidget;
        if (!dualViewRightWidget) dualViewRightWidget = new UVParamWidget;
        
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
        
        controlPanel = dualViewControlPanel;
        
    } else if (title == "Extended Dual View") {
        if (!dualViewExtendedLeftWidget) dualViewExtendedLeftWidget = new BaseGLWidget;
        if (!uvParamWidgetExtended) uvParamWidgetExtended = new UVParamWidgetExtended;
        
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
        
        controlPanel = dualViewExtendedControlPanel;
        
    } else if (title == "Simple Dual View") {
        if (!dualViewSimpleLeftWidget) dualViewSimpleLeftWidget = new BaseGLWidget;
        if (!dualViewSimpleRightWidget) dualViewSimpleRightWidget = new SimpleSquareWidget;
        
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
        
        controlPanel = dualViewSimpleControlPanel;
    }
    
    if (controlPanel) {
        // 添加到控制面板容器
        qobject_cast<QVBoxLayout*>(controlContainer->layout())->addWidget(controlPanel);
        
        // 添加到映射
        controlPanelMap[title] = controlPanel;
        
        // 初始隐藏
        controlPanel->setVisible(false);
    }
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
        
        // 删除tab
        deleteTab(tabTitle);
        
        // 切换到下一个tab
        if (tabWidget->count() > 0) {
            int newIndex = index > 0 ? index - 1 : 0;
            tabWidget->setCurrentIndex(newIndex);
            
            QWidget* currentWidget = tabWidget->widget(newIndex);
            QString currentTitle = tabWidget->tabText(newIndex);
            
            // 更新窗口标题
            mainWindow->setWindowTitle("OBJ Viewer - " + currentTitle);
            
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
            
            // 更新菜单栏中对应action的选中状态
            for (int i = 0; i < tabInfos.size(); ++i) {
                if (tabInfos[i].widget == currentWidget && tabInfos[i].action) {
                    tabInfos[i].action->setChecked(true);
                    break;
                }
            }
        }
    });
    
    // 连接tab切换信号
    QObject::connect(tabWidget, &QTabWidget::currentChanged, [this](int index) {
        if (index >= 0) {
            QWidget* currentWidget = tabWidget->widget(index);
            QString currentTitle = tabWidget->tabText(index);
            
            // 更新窗口标题
            mainWindow->setWindowTitle("OBJ Viewer - " + currentTitle);
            
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