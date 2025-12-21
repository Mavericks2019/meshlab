// tab_utils.cpp
#include "tab_utils.h"
#include <QMessageBox>
#include <QDebug>

namespace TabUtils {
    
    // 构造函数
    CloseableTabWidget::CloseableTabWidget(QWidget* parent) : QTabWidget(parent) {
        setTabsClosable(true);
        setMovable(true);
        
        // 设置TabBar样式
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
                color: #CCCCCC;
                font-size: 18px;
                font-weight: bold;
                position: absolute;
                top: 50%;
                left: 50%;
                transform: translate(-50%, -50%);
            }
            
            QTabBar::close-button:hover::after {
                color: white;
            }
            
            QTabBar::scroller {
                width: 20px;
            }
        )");
    }
    
    // 添加tab并记录标题
    void CloseableTabWidget::addTabWithTitle(QWidget* widget, const QString& title) {
        qDebug() << "Adding tab with title:" << title;
        tabTitles[widget] = title;
        addTab(widget, title);
    }
    
    // 获取tab的原始标题
    QString CloseableTabWidget::getTabTitle(QWidget* widget) const {
        return tabTitles.value(widget, QString());
    }
    
    // 通过标题获取widget
    QWidget* CloseableTabWidget::getWidgetByTitle(const QString& title) const {
        for (auto it = tabTitles.begin(); it != tabTitles.end(); ++it) {
            if (it.value() == title) {
                return it.key();
            }
        }
        return nullptr;
    }
    
} // namespace TabUtils