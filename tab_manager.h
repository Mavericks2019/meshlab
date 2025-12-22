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

#include "menu_utils.h"

// 前向声明
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
#include "menu_utils.h"

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
    // 创建各个tab页面
    void createBasicTab();
    void createCGALTab();
    void createModelTab();
    void createShortestPathTab();
    void createUVParamTab();
    void createDualViewTab();
    void createDualViewExtendedTab();
    void createDualViewSimpleTab();

    // 创建控制面板
    void createControlPanels();
    
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
};

#endif // TAB_MANAGER_H