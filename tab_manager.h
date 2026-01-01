// tab_manager.h
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

#include "menu_utils.h"

// 前向声明
#include "glwidget/modelglwidget.h"
#include "glwidget/baseglwidget.h"
#include "glwidget/cgalglwidget.h"
#include "glwidget/shortestpathglwidget.h"
#include "glwidget/uvparamwidget.h"
#include "glwidget/simplesquarewidget.h"
#include "glwidget/arapglwidget.h"  // 新增：ARAPGLWidget前向声明
#include "tabs/model_tab.h"
#include "tabs/basic_tab.h"
#include "tabs/cgal_tab.h"
#include "tabs/shortestpath_tab.h"
#include "tabs/uvparam_tab.h"
#include "tabs/dualview_tab.h"
#include "tabs/dualview_extended_tab.h"
#include "tabs/dualview_simple_tab.h"
#include "menu_utils.h"

// 新增的头文件
#include "tabs/new_cgal_uv_tab.h"
#include "tabs/openmesh_viewer_tab.h"
#include "Cutting/QGLViewerWidget.h"

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

    // 创建特定的tab页面
    void createBasicTab();
    void createCGALTab();
    void createModelTab();
    void createShortestPathTab();
    void createUVParamTab();
    void createDualViewTab();
    void createDualViewExtendedTab();
    void createDualViewSimpleTab();
    void createNewCGALUVTab(); // 新增：创建新CGAL-UV视图
    void createOpenMeshViewerTab(); // 新增：创建OpenMesh Viewer

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

    // 新增：新CGAL-UV视图窗口
    ARAPGLWidget* newCGALUVLeftWidget;  // 修改为ARAPGLWidget
    UVParamWidget* newCGALUVRightWidget;

    // 新增：OpenMesh Viewer窗口
    QGLViewerWidget* openMeshViewerWidget;

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
    
    // 新增：新CGAL-UV视图信息标签
    QLabel* newCGALUVLeftInfoLabel;
    QLabel* newCGALUVRightInfoLabel;
    
    // 新增：OpenMesh Viewer信息标签
    QLabel* openMeshViewerInfoLabel;
};

#endif // TAB_MANAGER_H