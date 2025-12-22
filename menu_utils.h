// menu_utils.h
#pragma once
#ifndef MENU_UTILS_H
#define MENU_UTILS_H

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
#include <QGroupBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QMap>
#include <QList>
#include <QColorDialog>
#include <functional>

namespace UIUtils {

    // 自定义TabWidget，支持关闭按钮
    class CloseableTabWidget : public QTabWidget {
    private:
        QMap<QWidget*, QString> tabTitles;  // 存储每个widget的原始标题
        QMap<QString, QWidget*> titleToWidget;  // 通过标题查找widget
        QMap<QString, QAction*> titleToAction;  // 通过标题查找action
        
    public:
        CloseableTabWidget(QWidget* parent = nullptr);
        
        // 添加tab并记录标题
        void addTabWithTitle(QWidget* widget, const QString& title);
        
        // 获取tab的原始标题
        QString getTabTitle(QWidget* widget) const;
        
        // 获取所有tab标题
        QStringList getAllTabTitles() const;
        
        // 通过标题获取widget
        QWidget* getWidgetByTitle(const QString& title) const;
        
        // 通过标题恢复tab
        bool restoreTabByTitle(const QString& title);
        
        // 设置标题对应的action
        void setActionForTitle(const QString& title, QAction* action);
        
        // 获取标题对应的action
        QAction* getActionForTitle(const QString& title) const;
        
        // 获取所有标题
        QStringList getTitles() const;
        
        // 清理widget的关联
        void removeWidgetAssociations(QWidget* widget);
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
                           QList<TabInfo>& tabInfos, QMap<QString, QWidget*>& controlPanelMap,
                           std::function<void(const QString&, bool)> createTabFunc = nullptr);

    // 创建模型信息组
    QGroupBox* createModelInfoGroup(QLabel** infoLabel = nullptr);

    // 创建颜色设置组
    QGroupBox* createColorSettingsGroup(QWidget* glWidget);

} // namespace UIUtils

#endif // MENU_UTILS_H