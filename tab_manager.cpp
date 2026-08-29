// tab_manager.cpp
#include "tab_manager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QMessageBox>
#include <QDebug>
#include <QSizePolicy>

TabManager::TabManager(QWidget* mainWindow) 
    : QObject(mainWindow)
    , mainWindow(mainWindow)
    , tabWidget(nullptr)
    , menuBar(nullptr)
    , controlContainer(nullptr)
    // 初始化所有widget指针为nullptr
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
    , newCGALUVLeftWidget(nullptr)
    , newCGALUVRightWidget(nullptr)
//    , openMeshViewerWidget(nullptr)
    , relasticGlWidget(nullptr)
    , relativisticGlWidget(nullptr)
    , blackHoleWidget(nullptr)  // 初始化BlackHoleWidget
    , interactiveVolumeWidget(nullptr)
    , progressiveParameterizationWidget(nullptr)
    , atlasParameterizationWidget(nullptr)
    // 初始化所有info label指针为nullptr
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
    , dualViewSimpleRightInfoLabel(nullptr)
    , newCGALUVLeftInfoLabel(nullptr)
    , newCGALUVRightInfoLabel(nullptr)
//    , openMeshViewerInfoLabel(nullptr)
    , relasticInfoLabel(nullptr)
    , relativisticInfoLabel(nullptr)
    , blackHoleInfoLabel(nullptr)  // 初始化BlackHole信息标签
    , interactiveVolumeInfoLabel(nullptr)
{
    // 注册标签页配置
    registerTabConfigs();
}

TabManager::~TabManager() {
    // 清理所有标签页
    for (const auto& config : tabConfigs) {
        cleanupTab(config.title);
    }
}

void TabManager::registerTabConfigs() {
    tabConfigs = {
        {"OpenMesh", 
            [this]() -> QWidget* { 
                getOrCreateWidget(basicGlWidget);
                return ::createBasicTab(basicGlWidget);
            },
            [this]() -> QWidget* {
                getOrCreateWidget(basicGlWidget);
                QWidget* controlPanel = new QWidget;
                QVBoxLayout* layout = new QVBoxLayout(controlPanel);
                layout->setAlignment(Qt::AlignTop);
                layout->addWidget(UIUtils::createColorSettingsGroup(basicGlWidget));
                layout->addWidget(UIUtils::createModelInfoGroup(&basicInfoLabel));
                layout->addWidget(createBasicControlPanel(basicGlWidget, basicInfoLabel, mainWindow));
                return controlPanel;
            },
            [this]() {
                delete basicGlWidget; basicGlWidget = nullptr;
                delete basicInfoLabel; basicInfoLabel = nullptr;
            },
            0,
            "OpenMesh"
        },
        {"CGAL",
            [this]() -> QWidget* {
                getOrCreateWidget(cgalGlWidget);
                return ::createCGALTab(cgalGlWidget);
            },
            [this]() -> QWidget* {
                getOrCreateWidget(cgalGlWidget);
                QWidget* controlPanel = new QWidget;
                QVBoxLayout* layout = new QVBoxLayout(controlPanel);
                layout->setAlignment(Qt::AlignTop);
                layout->addWidget(UIUtils::createColorSettingsGroup(cgalGlWidget));
                layout->addWidget(UIUtils::createModelInfoGroup(&cgalInfoLabel));
                layout->addWidget(createCGALControlPanel(cgalGlWidget, cgalInfoLabel, mainWindow));
                return controlPanel;
            },
            [this]() {
                delete cgalGlWidget; cgalGlWidget = nullptr;
                delete cgalInfoLabel; cgalInfoLabel = nullptr;
            },
            1,
            "CGAL"
        },
        {"Model",
            [this]() -> QWidget* {
                getOrCreateWidget(modelGlWidget);
                return ::createModelTab(modelGlWidget);
            },
            [this]() -> QWidget* {
                getOrCreateWidget(modelGlWidget);
                QWidget* controlPanel = new QWidget;
                QVBoxLayout* layout = new QVBoxLayout(controlPanel);
                layout->setAlignment(Qt::AlignTop);
                layout->addWidget(UIUtils::createColorSettingsGroup(modelGlWidget));
                layout->addWidget(UIUtils::createModelInfoGroup(&modelInfoLabel));
                layout->addWidget(createModelControlPanel(modelGlWidget, modelInfoLabel, mainWindow));
                return controlPanel;
            },
            [this]() {
                delete modelGlWidget; modelGlWidget = nullptr;
                delete modelInfoLabel; modelInfoLabel = nullptr;
            },
            2,
            "Model"
        },
        {"Shortest Path",
            [this]() -> QWidget* {
                getOrCreateWidget(shortestPathGlWidget);
                return ::createShortestPathTab(shortestPathGlWidget);
            },
            [this]() -> QWidget* {
                getOrCreateWidget(shortestPathGlWidget);
                QWidget* controlPanel = new QWidget;
                QVBoxLayout* layout = new QVBoxLayout(controlPanel);
                layout->setAlignment(Qt::AlignTop);
                layout->addWidget(UIUtils::createColorSettingsGroup(shortestPathGlWidget));
                layout->addWidget(UIUtils::createModelInfoGroup(&shortestPathInfoLabel));
                layout->addWidget(createShortestPathControlPanel(shortestPathGlWidget, shortestPathInfoLabel, mainWindow));
                return controlPanel;
            },
            [this]() {
                delete shortestPathGlWidget; shortestPathGlWidget = nullptr;
                delete shortestPathInfoLabel; shortestPathInfoLabel = nullptr;
            },
            3,
            "Shortest Path"
        },
        {"UV Parameterization",
            [this]() -> QWidget* {
                getOrCreateWidget(uvParamWidget);
                return ::createUVParamTab(uvParamWidget);
            },
            [this]() -> QWidget* {
                getOrCreateWidget(uvParamWidget);
                QWidget* controlPanel = new QWidget;
                QVBoxLayout* layout = new QVBoxLayout(controlPanel);
                layout->setAlignment(Qt::AlignTop);
                layout->addWidget(UIUtils::createModelInfoGroup(&uvParamInfoLabel));
                layout->addWidget(createUVParamControlPanel(uvParamWidget, uvParamInfoLabel, mainWindow));
                return controlPanel;
            },
            [this]() {
                delete uvParamWidget; uvParamWidget = nullptr;
                delete uvParamInfoLabel; uvParamInfoLabel = nullptr;
            },
            4,
            "UV Parameterization"
        },
        {"Dual View",
            [this]() -> QWidget* {
                getOrCreateWidget(dualViewLeftWidget);
                getOrCreateWidget(dualViewRightWidget);
                return ::createDualViewTab(dualViewLeftWidget, dualViewRightWidget);
            },
            [this]() -> QWidget* {
                getOrCreateWidget(dualViewLeftWidget);
                getOrCreateWidget(dualViewRightWidget);
                
                QWidget* controlPanel = new QWidget;
                QVBoxLayout* layout = new QVBoxLayout(controlPanel);
                layout->setAlignment(Qt::AlignTop);
                
                dualViewLeftInfoLabel = new QLabel("No model loaded (Left View)");
                dualViewLeftInfoLabel->setAlignment(Qt::AlignCenter);
                dualViewLeftInfoLabel->setMinimumHeight(50);
                dualViewLeftInfoLabel->setStyleSheet("background-color: #3A3A3A; color: white; border-radius: 5px; padding: 5px; font-size: 14px;");
                dualViewLeftInfoLabel->setWordWrap(true);
                
                dualViewRightInfoLabel = new QLabel("No model loaded (Right View)");
                dualViewRightInfoLabel->setAlignment(Qt::AlignCenter);
                dualViewRightInfoLabel->setMinimumHeight(50);
                dualViewRightInfoLabel->setStyleSheet("background-color: #3A3A3A; color: white; border-radius: 5px; padding: 5px; font-size: 14px;");
                dualViewRightInfoLabel->setWordWrap(true);
                
                QGroupBox* infoGroup = new QGroupBox("Model Information");
                QVBoxLayout* infoLayout = new QVBoxLayout(infoGroup);
                infoLayout->addWidget(dualViewLeftInfoLabel);
                infoLayout->addWidget(dualViewRightInfoLabel);
                
                layout->addWidget(infoGroup);
                layout->addWidget(createDualViewControlPanel(dualViewLeftWidget, dualViewRightWidget, 
                    dualViewLeftInfoLabel, dualViewRightInfoLabel, mainWindow));
                return controlPanel;
            },
            [this]() {
                delete dualViewLeftWidget; dualViewLeftWidget = nullptr;
                delete dualViewRightWidget; dualViewRightWidget = nullptr;
                delete dualViewLeftInfoLabel; dualViewLeftInfoLabel = nullptr;
                delete dualViewRightInfoLabel; dualViewRightInfoLabel = nullptr;
            },
            5,
            "Dual View"
        },
        {"Extended Dual View",
            [this]() -> QWidget* {
                getOrCreateWidget(dualViewExtendedLeftWidget);
                getOrCreateWidget(uvParamWidgetExtended);
                return ::createDualViewExtendedTab(dualViewExtendedLeftWidget, uvParamWidgetExtended);
            },
            [this]() -> QWidget* {
                getOrCreateWidget(dualViewExtendedLeftWidget);
                getOrCreateWidget(uvParamWidgetExtended);
                
                QWidget* controlPanel = new QWidget;
                QVBoxLayout* layout = new QVBoxLayout(controlPanel);
                layout->setAlignment(Qt::AlignTop);
                
                dualViewExtendedLeftInfoLabel = new QLabel("No model loaded (Left View)");
                dualViewExtendedLeftInfoLabel->setAlignment(Qt::AlignCenter);
                dualViewExtendedLeftInfoLabel->setMinimumHeight(50);
                dualViewExtendedLeftInfoLabel->setStyleSheet("background-color: #3A3A3A; color: white; border-radius: 5px; padding: 5px; font-size: 14px;");
                dualViewExtendedLeftInfoLabel->setWordWrap(true);
                
                dualViewExtendedRightInfoLabel = new QLabel("No model loaded (Right View)");
                dualViewExtendedRightInfoLabel->setAlignment(Qt::AlignCenter);
                dualViewExtendedRightInfoLabel->setMinimumHeight(50);
                dualViewExtendedRightInfoLabel->setStyleSheet("background-color: #3A3A3A; color: white; border-radius: 5px; padding: 5px; font-size: 14px;");
                dualViewExtendedRightInfoLabel->setWordWrap(true);
                
                QGroupBox* extendedInfoGroup = new QGroupBox("Model Information");
                QVBoxLayout* extendedInfoLayout = new QVBoxLayout(extendedInfoGroup);
                extendedInfoLayout->addWidget(dualViewExtendedLeftInfoLabel);
                extendedInfoLayout->addWidget(dualViewExtendedRightInfoLabel);
                
                layout->addWidget(extendedInfoGroup);
                layout->addWidget(createDualViewExtendedControlPanel(dualViewExtendedLeftWidget, uvParamWidgetExtended, 
                    dualViewExtendedLeftInfoLabel, dualViewExtendedRightInfoLabel, mainWindow));
                return controlPanel;
            },
            [this]() {
                delete dualViewExtendedLeftWidget; dualViewExtendedLeftWidget = nullptr;
                delete uvParamWidgetExtended; uvParamWidgetExtended = nullptr;
                delete dualViewExtendedLeftInfoLabel; dualViewExtendedLeftInfoLabel = nullptr;
                delete dualViewExtendedRightInfoLabel; dualViewExtendedRightInfoLabel = nullptr;
            },
            6,
            "Extended Dual View"
        },
        {"Simple Dual View",
            [this]() -> QWidget* {
                getOrCreateWidget(dualViewSimpleLeftWidget);
                getOrCreateWidget(dualViewSimpleRightWidget);
                return ::createDualViewSimpleTab(dualViewSimpleLeftWidget, dualViewSimpleRightWidget);
            },
            [this]() -> QWidget* {
                getOrCreateWidget(dualViewSimpleLeftWidget);
                getOrCreateWidget(dualViewSimpleRightWidget);
                
                QWidget* controlPanel = new QWidget;
                QVBoxLayout* layout = new QVBoxLayout(controlPanel);
                layout->setAlignment(Qt::AlignTop);
                
                dualViewSimpleLeftInfoLabel = new QLabel("No model loaded (Left View)");
                dualViewSimpleLeftInfoLabel->setAlignment(Qt::AlignCenter);
                dualViewSimpleLeftInfoLabel->setMinimumHeight(50);
                dualViewSimpleLeftInfoLabel->setStyleSheet("background-color: #3A3A3A; color: white; border-radius: 5px; padding: 5px; font-size: 14px;");
                dualViewSimpleLeftInfoLabel->setWordWrap(true);
                
                dualViewSimpleRightInfoLabel = new QLabel("White Square View - Ready for extension");
                dualViewSimpleRightInfoLabel->setAlignment(Qt::AlignCenter);
                dualViewSimpleRightInfoLabel->setMinimumHeight(50);
                dualViewSimpleRightInfoLabel->setStyleSheet("background-color: #3A3A3A; color: white; border-radius: 5px; padding: 5px; font-size: 14px;");
                dualViewSimpleRightInfoLabel->setWordWrap(true);
                
                QGroupBox* simpleInfoGroup = new QGroupBox("Model Information");
                QVBoxLayout* simpleInfoLayout = new QVBoxLayout(simpleInfoGroup);
                simpleInfoLayout->addWidget(dualViewSimpleLeftInfoLabel);
                simpleInfoLayout->addWidget(dualViewSimpleRightInfoLabel);
                
                layout->addWidget(simpleInfoGroup);
                layout->addWidget(createDualViewSimpleControlPanel(dualViewSimpleLeftWidget, dualViewSimpleRightWidget, 
                    dualViewSimpleLeftInfoLabel, dualViewSimpleRightInfoLabel, mainWindow));
                return controlPanel;
            },
            [this]() {
                delete dualViewSimpleLeftWidget; dualViewSimpleLeftWidget = nullptr;
                delete dualViewSimpleRightWidget; dualViewSimpleRightWidget = nullptr;
                delete dualViewSimpleLeftInfoLabel; dualViewSimpleLeftInfoLabel = nullptr;
                delete dualViewSimpleRightInfoLabel; dualViewSimpleRightInfoLabel = nullptr;
            },
            7,
            "Simple Dual View"
        },
        {"New CGAL-UV View",
            [this]() -> QWidget* {
                getOrCreateWidget(newCGALUVLeftWidget);
                getOrCreateWidget(newCGALUVRightWidget);
                return ::createNewCGALUVTab(newCGALUVLeftWidget, newCGALUVRightWidget);
            },
            [this]() -> QWidget* {
                getOrCreateWidget(newCGALUVLeftWidget);
                getOrCreateWidget(newCGALUVRightWidget);
                
                QWidget* controlPanel = new QWidget;
                QVBoxLayout* layout = new QVBoxLayout(controlPanel);
                layout->setAlignment(Qt::AlignTop);
                
                newCGALUVLeftInfoLabel = new QLabel("No model loaded (ARAP View)");
                newCGALUVLeftInfoLabel->setAlignment(Qt::AlignCenter);
                newCGALUVLeftInfoLabel->setMinimumHeight(50);
                newCGALUVLeftInfoLabel->setStyleSheet("background-color: #3A3A3A; color: white; border-radius: 5px; padding: 5px; font-size: 14px;");
                newCGALUVLeftInfoLabel->setWordWrap(true);
                
                newCGALUVRightInfoLabel = new QLabel("No model loaded (UV View)");
                newCGALUVRightInfoLabel->setAlignment(Qt::AlignCenter);
                newCGALUVRightInfoLabel->setMinimumHeight(50);
                newCGALUVRightInfoLabel->setStyleSheet("background-color: #3A3A3A; color: white; border-radius: 5px; padding: 5px; font-size: 14px;");
                newCGALUVRightInfoLabel->setWordWrap(true);
                
                QGroupBox* newCGALUVInfoGroup = new QGroupBox("Model Information");
                QVBoxLayout* newCGALUVInfoLayout = new QVBoxLayout(newCGALUVInfoGroup);
                newCGALUVInfoLayout->addWidget(newCGALUVLeftInfoLabel);
                newCGALUVInfoLayout->addWidget(newCGALUVRightInfoLabel);
                
                layout->addWidget(newCGALUVInfoGroup);
                layout->addWidget(createNewCGALUVControlPanel(newCGALUVLeftWidget, newCGALUVRightWidget, 
                                                             newCGALUVLeftInfoLabel, newCGALUVRightInfoLabel, mainWindow));
                return controlPanel;
            },
            [this]() {
                delete newCGALUVLeftWidget; newCGALUVLeftWidget = nullptr;
                delete newCGALUVRightWidget; newCGALUVRightWidget = nullptr;
                delete newCGALUVLeftInfoLabel; newCGALUVLeftInfoLabel = nullptr;
                delete newCGALUVRightInfoLabel; newCGALUVRightInfoLabel = nullptr;
            },
            8,
            "New CGAL-UV View"
        },
        // {"OpenMesh Viewer",
        //     [this]() -> QWidget* {
        //         getOrCreateWidget(openMeshViewerWidget);
        //         return ::createOpenMeshViewerTab(openMeshViewerWidget);
        //     },
        //     [this]() -> QWidget* {
        //         getOrCreateWidget(openMeshViewerWidget);
        //         QWidget* controlPanel = new QWidget;
        //         QVBoxLayout* layout = new QVBoxLayout(controlPanel);
        //         layout->setAlignment(Qt::AlignTop);
        //         layout->addWidget(UIUtils::createModelInfoGroup(&openMeshViewerInfoLabel));
        //         layout->addWidget(createOpenMeshViewerControlPanel(openMeshViewerWidget, openMeshViewerInfoLabel, mainWindow));
        //         return controlPanel;
        //     },
        //     [this]() {
        //         delete openMeshViewerWidget; openMeshViewerWidget = nullptr;
        //         delete openMeshViewerInfoLabel; openMeshViewerInfoLabel = nullptr;
        //     },
        //     9,
        //     "OpenMesh Viewer"
        // },
        {"Relastic",
            [this]() -> QWidget* {
                getOrCreateWidget(relasticGlWidget);
                return ::createRelasticTab(relasticGlWidget);
            },
            [this]() -> QWidget* {
                getOrCreateWidget(relasticGlWidget);
                QWidget* controlPanel = new QWidget;
                QVBoxLayout* layout = new QVBoxLayout(controlPanel);
                layout->setAlignment(Qt::AlignTop);
                layout->addWidget(UIUtils::createModelInfoGroup(&relasticInfoLabel));
                layout->addWidget(createRelasticControlPanel(relasticGlWidget, relasticInfoLabel, mainWindow));
                return controlPanel;
            },
            [this]() {
                delete relasticGlWidget; relasticGlWidget = nullptr;
                delete relasticInfoLabel; relasticInfoLabel = nullptr;
            },
            10,
            "Relastic"
        },
        {"Relativistic",
            [this]() -> QWidget* {
                getOrCreateWidget(relativisticGlWidget);
                return ::createRelativisticTab(relativisticGlWidget);
            },
            [this]() -> QWidget* {
                getOrCreateWidget(relativisticGlWidget);
                QWidget* controlPanel = new QWidget;
                QVBoxLayout* layout = new QVBoxLayout(controlPanel);
                layout->setAlignment(Qt::AlignTop);
                layout->addWidget(UIUtils::createModelInfoGroup(&relativisticInfoLabel));
                layout->addWidget(createRelativisticControlPanel(relativisticGlWidget, relativisticInfoLabel, mainWindow));
                return controlPanel;
            },
            [this]() {
                delete relativisticGlWidget; relativisticGlWidget = nullptr;
                delete relativisticInfoLabel; relativisticInfoLabel = nullptr;
            },
            11,
            "Relativistic"
        },
        {"Black Hole",  // 添加Black Hole标签页配置
            [this]() -> QWidget* {
                getOrCreateWidget(blackHoleWidget);
                return ::createBlackHoleTab(blackHoleWidget);
            },
            [this]() -> QWidget* {
                getOrCreateWidget(blackHoleWidget);
                QWidget* controlPanel = new QWidget;
                QVBoxLayout* layout = new QVBoxLayout(controlPanel);
                layout->setAlignment(Qt::AlignTop);
                layout->addWidget(UIUtils::createModelInfoGroup(&blackHoleInfoLabel));
                layout->addWidget(createBlackHoleControlPanel(blackHoleWidget, blackHoleInfoLabel, mainWindow));
                return controlPanel;
            },
            [this]() {
                delete blackHoleWidget; blackHoleWidget = nullptr;
                delete blackHoleInfoLabel; blackHoleInfoLabel = nullptr;
            },
            12,
            "Black Hole"
        },
        {"Volume Base",
            [this]() -> QWidget* { 
                getOrCreateWidget(interactiveVolumeWidget);
                return ::createInteractiveVolumeTab(interactiveVolumeWidget);
            },
            [this]() -> QWidget* {
                getOrCreateWidget(interactiveVolumeWidget);
                QWidget* controlPanel = new QWidget;
                QVBoxLayout* layout = new QVBoxLayout(controlPanel);
                layout->setAlignment(Qt::AlignTop);
                layout->addWidget(UIUtils::createModelInfoGroup(&interactiveVolumeInfoLabel));
                layout->addWidget(::createInteractiveVolumeControlPanel(interactiveVolumeWidget, interactiveVolumeInfoLabel, mainWindow));
                return controlPanel;
            },
            [this]() {
                delete interactiveVolumeWidget; interactiveVolumeWidget = nullptr;
                delete interactiveVolumeInfoLabel; interactiveVolumeInfoLabel = nullptr;
            },
            13,  // 根据实际标签页数量调整索引
            "Volume Base"
        },
        {"Progressive Parameterization",
            [this]() -> QWidget* {
                getOrCreateWidget(progressiveParameterizationWidget);
                return ::createProgressiveParameterizationTab(progressiveParameterizationWidget);
            },
            [this]() -> QWidget* {
                getOrCreateWidget(progressiveParameterizationWidget);
                return ::createProgressiveParameterizationControlPanel(progressiveParameterizationWidget, mainWindow);
            },
            [this]() {
                delete progressiveParameterizationWidget;
                progressiveParameterizationWidget = nullptr;
            },
            14,
            "Progressive Parameterization"
        },
        {"AAAtlas Packing",
            [this]() -> QWidget* {
                getOrCreateWidget(atlasParameterizationWidget);
                return ::createAtlasParameterizationTab(atlasParameterizationWidget);
            },
            [this]() -> QWidget* {
                getOrCreateWidget(atlasParameterizationWidget);
                return ::createAtlasParameterizationControlPanel(
                    atlasParameterizationWidget, mainWindow);
            },
            [this]() {
                delete atlasParameterizationWidget;
                atlasParameterizationWidget = nullptr;
            },
            15,
            "AAAtlas Packing"
        }
    };
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
    
    controlContainer->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    
    // 先连接信号
    connectSignals();
    
    // 创建第一个tab（OpenMesh），并切换到它
    createTab("OpenMesh", true);
    
    // 重要：手动触发第一次控制面板显示
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
        // Tab已存在且在tab栏中显示
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
    
    // 查找对应的配置
    const TabConfig* config = nullptr;
    for (const auto& tabConfig : tabConfigs) {
        if (tabConfig.title == title) {
            config = &tabConfig;
            break;
        }
    }
    
    if (!config) {
        qDebug() << "Unknown tab title:" << title;
        return;
    }
    
    // 创建标签页
    QWidget* tabWidget = config->createWidgetFunc();
    if (tabWidget) {
        this->tabWidget->addTabWithTitle(tabWidget, title);
        
        // 创建控制面板（如果不存在）
        if (!controlPanelMap.contains(title)) {
            createControlPanelFromConfig(*config);
        }
        
        // 更新tabInfos
        updateTabInfo(*config, tabWidget);
        
        // 标记为已创建
        tabCreated[title] = true;
        
        // 如果switchToTab为true，切换到新创建的tab
        if (switchToTab) {
            QWidget* widget = this->tabWidget->getWidgetByTitle(title);
            if (widget) {
                this->tabWidget->setCurrentWidget(widget);
                
                // 更新窗口标题
                mainWindow->setWindowTitle("OBJ Viewer - " + title);
            }
        }
    }
}

QWidget* TabManager::createTabFromConfig(const TabConfig& config) {
    return config.createWidgetFunc();
}

void TabManager::cleanupTab(const QString& title) {
    // 清理tab资源
    cleanupTabResources(title);
    
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

void TabManager::cleanupTabResources(const QString& title) {
    // 查找对应的配置并执行清理函数
    for (const auto& config : tabConfigs) {
        if (config.title == title) {
            if (config.cleanupFunc) {
                config.cleanupFunc();
            }
            return;
        }
    }
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

void TabManager::createControlPanelFromConfig(const TabConfig& config) {
    // 如果控制面板已经存在，直接返回
    if (controlPanelMap.contains(config.title)) {
        return;
    }
    
    QWidget* controlPanel = config.createControlPanelFunc();
    if (controlPanel) {
        // 添加到控制面板容器
        qobject_cast<QVBoxLayout*>(controlContainer->layout())->addWidget(controlPanel);
        
        // 添加到映射
        controlPanelMap[config.title] = controlPanel;
        
        // 初始隐藏
        controlPanel->setVisible(false);
    }
}

void TabManager::updateTabInfo(const TabConfig& config, QWidget* widget) {
    UIUtils::TabInfo info;
    info.name = config.name;
    info.title = config.title;
    info.widget = widget;
    info.controlPanel = controlPanelMap[config.title];
    info.isVisible = true;
    info.originalIndex = config.originalIndex;
    info.action = tabWidget->getActionForTitle(config.title);
    
    // 替换或添加tab信息
    bool found = false;
    for (int i = 0; i < tabInfos.size(); ++i) {
        if (tabInfos[i].title == config.title) {
            tabInfos[i] = info;
            found = true;
            break;
        }
    }
    if (!found) {
        tabInfos.append(info);
    }
}

// 删除原有的各个createXXXTab函数，因为它们已经被TabConfig中的lambda替代

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
