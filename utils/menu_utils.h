// menu_utils.h
#ifndef MENU_UTILS_H
#define MENU_UTILS_H

#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QActionGroup>
#include <QTabWidget>
#include <QWidget>
#include <QMap>
#include <QString>

namespace MenuUtils {
    
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
    QMenuBar* createMenuBar(QTabWidget* tabWidget, QWidget* mainWindow, 
                           QList<TabInfo>& tabInfos, QMap<QString, QWidget*>& controlPanelMap);
    
} // namespace MenuUtils

#endif // MENU_UTILS_H