// 在tab_manager.h中添加头文件包含和成员变量
#pragma once
#ifndef TAB_MANAGER_H
#define TAB_MANAGER_H

#include <QObject>
#include <QWidget>
#include <QTabWidget>
#include <QMenuBar>
#include <QMap>
#include <QList>
#include <functional>
#include <QVector>

#include "menu_utils.h"

// 前向声明
#include "glwidget/modelglwidget.h"
#include "glwidget/baseglwidget.h"
#include "glwidget/cgalglwidget.h"
#include "glwidget/shortestpathglwidget.h"
#include "glwidget/uvparamwidget.h"
#include "glwidget/simplesquarewidget.h"
#include "glwidget/arapglwidget.h"
#include "glwidget/relasticwidget.h"
#include "glwidget/relativisticwidget.h"
#include "glwidget/blackholewidget.h"
#include "glwidget/progressiveparameterizationwidget.h"
#include "glwidget/atlasparameterizationwidget.h"
#include "glwidget/dchartswidget.h"
#include "glwidget/steady3ddissectionwidget.h"
#include "InteractiveWidget.h"
#include "tabs/model_tab.h"
#include "tabs/basic_tab.h"
#include "tabs/cgal_tab.h"
#include "tabs/shortestpath_tab.h"
#include "tabs/uvparam_tab.h"
#include "tabs/dualview_tab.h"
#include "tabs/dualview_extended_tab.h"
#include "tabs/dualview_simple_tab.h"
#include "tabs/relastic_tab.h"
#include "tabs/relativistic_tab.h"
#include "tabs/blackhole_tab.h"
#include "tabs/interactive_volume_tab.h"
#include "tabs/new_cgal_uv_tab.h"
#include "tabs/progressive_parameterization_tab.h"
#include "tabs/atlas_parameterization_tab.h"
#include "tabs/dcharts_tab.h"
#include "tabs/steady3ddissection_tab.h"
// #include "Cutting/QGLViewerWidget.h"

// 标签页配置结构体
struct TabConfig {
    QString title;
    std::function<QWidget*()> createWidgetFunc;
    std::function<QWidget*()> createControlPanelFunc;
    std::function<void()> cleanupFunc;
    int originalIndex;
    QString name;
};

// Tab管理器类
class TabManager : public QObject {
    Q_OBJECT

public:
    explicit TabManager(QWidget* mainWindow);
    ~TabManager();

    // 初始化所有tab
    void initializeTabs();
    
    // 获取tab widget
    UIUtils::CloseableTabWidget* getTabWidget() const { return tabWidget; }
    
    // 获取菜单栏
    QMenuBar* getMenuBar() const { return menuBar; }
    
    // 获取控制面板容器
    QWidget* getControlContainer() const { return controlContainer; }

private:
    // 动态创建各个tab页面
    void createTab(const QString& title, bool switchToTab);
    
    // 删除tab
    void deleteTab(const QString& title);
    
    // 清理tab的所有资源
    void cleanupTab(const QString& title);

    // 标签页配置表
    QVector<TabConfig> tabConfigs;
    
    // 注册标签页配置
    void registerTabConfigs();
    
    // 通用方法
    QWidget* createTabFromConfig(const TabConfig& config);
    void cleanupTabResources(const QString& title);
    void createControlPanelFromConfig(const TabConfig& config);
    void updateTabInfo(const TabConfig& config, QWidget* widget);
    
    // Widget管理辅助方法
    template<typename T>
    T* getOrCreateWidget(T*& widgetPtr) {
        if (!widgetPtr) {
            widgetPtr = new T;
        }
        return widgetPtr;
    }

    // 创建控制面板
    void createControlPanel(const QString& title);
    
    // 连接信号
    void connectSignals();

    // 主窗口引用
    QWidget* mainWindow;

    // Tab相关控件
    UIUtils::CloseableTabWidget* tabWidget;
    QMenuBar* menuBar;
    
    // 存储Tab信息的列表
    QList<UIUtils::TabInfo> tabInfos;
    
    // 存储控制面板映射
    QMap<QString, QWidget*> controlPanelMap;
    
    // 控制面板容器
    QWidget* controlContainer;
    
    // 存储是否已创建的标记
    QMap<QString, bool> tabCreated;

    // GLWidgets
    ModelGLWidget* modelGlWidget;
    BaseGLWidget* basicGlWidget;
    CGALGLWidget* cgalGlWidget;
    ShortestPathGLWidget* shortestPathGlWidget;
    UVParamWidget* uvParamWidget;
    
    // 双视图窗口
    BaseGLWidget* dualViewLeftWidget;
    UVParamWidget* dualViewRightWidget;
    
    // 扩展双视图窗口
    BaseGLWidget* dualViewExtendedLeftWidget;
    UVParamWidgetExtended* uvParamWidgetExtended;

    // 简单双视图窗口
    BaseGLWidget* dualViewSimpleLeftWidget;
    SimpleSquareWidget* dualViewSimpleRightWidget;

    // 新CGAL-UV视图窗口
    ARAPGLWidget* newCGALUVLeftWidget;
    UVParamWidget* newCGALUVRightWidget;

    // // OpenMesh Viewer窗口
    // QGLViewerWidget* openMeshViewerWidget;

    // Relastic窗口
    RelasticGLWidget* relasticGlWidget;

    // Relativistic窗口
    RelativisticGLWidget* relativisticGlWidget;

    // Black Hole窗口
    BlackHoleWidget* blackHoleWidget;

    // Interactive Volume窗口 (新增)
    InteractiveWidget* interactiveVolumeWidget;

    ProgressiveParameterizationWidget* progressiveParameterizationWidget;
    AtlasParameterizationWidget* atlasParameterizationWidget;
    DChartsWidget* dChartsWidget;
    Steady3DDissectionWidget* steady3DDissectionWidget;

    // 信息标签
    QLabel* basicInfoLabel;
    QLabel* cgalInfoLabel;
    QLabel* modelInfoLabel;
    QLabel* shortestPathInfoLabel;
    QLabel* uvParamInfoLabel;
    QLabel* dualViewLeftInfoLabel;
    QLabel* dualViewRightInfoLabel;
    QLabel* dualViewExtendedLeftInfoLabel;
    QLabel* dualViewExtendedRightInfoLabel;
    QLabel* dualViewSimpleLeftInfoLabel;
    QLabel* dualViewSimpleRightInfoLabel;
    
    // 新CGAL-UV视图信息标签
    QLabel* newCGALUVLeftInfoLabel;
    QLabel* newCGALUVRightInfoLabel;
    
    // OpenMesh Viewer信息标签
    QLabel* openMeshViewerInfoLabel;
    
    // Relastic信息标签
    QLabel* relasticInfoLabel;
    
    // Relativistic信息标签
    QLabel* relativisticInfoLabel;
    
    // Black Hole信息标签
    QLabel* blackHoleInfoLabel;
    
    // Interactive Volume信息标签 (新增)
    QLabel* interactiveVolumeInfoLabel;
};

#endif // TAB_MANAGER_H
