// tab_utils.h
#ifndef TAB_UTILS_H
#define TAB_UTILS_H

#include <QTabWidget>
#include <QWidget>
#include <QString>
#include <QMap>  // 添加QMap头文件

namespace TabUtils {
    
    // 自定义TabWidget，支持关闭按钮
    class CloseableTabWidget : public QTabWidget {
    private:
        QMap<QWidget*, QString> tabTitles;  // 存储每个widget的原始标题
        
    public:
        CloseableTabWidget(QWidget* parent = nullptr);
        
        // 添加tab并记录标题
        void addTabWithTitle(QWidget* widget, const QString& title);
        
        // 获取tab的原始标题
        QString getTabTitle(QWidget* widget) const;
        
        // 通过标题获取widget
        QWidget* getWidgetByTitle(const QString& title) const;
    };
    
} // namespace TabUtils

#endif // TAB_UTILS_H