// main.cpp
#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include "menu_utils.h"
#include "tab_manager.h"

namespace UIUtils {
    // 应用深色主题
    inline void applyDarkTheme(QApplication& app) {  // 添加 inline
        QApplication::setStyle(QStyleFactory::create("Fusion"));
        
        QPalette palette;
        palette.setColor(QPalette::Window, QColor(53, 53, 53));
        palette.setColor(QPalette::WindowText, Qt::white);
        palette.setColor(QPalette::Base, QColor(25, 25, 25));
        palette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
        palette.setColor(QPalette::ToolTipBase, Qt::white);
        palette.setColor(QPalette::ToolTipText, Qt::white);
        palette.setColor(QPalette::Text, Qt::white);
        palette.setColor(QPalette::Button, QColor(53, 53, 53));
        palette.setColor(QPalette::ButtonText, Qt::white);
        palette.setColor(QPalette::BrightText, Qt::red);
        palette.setColor(QPalette::Link, QColor(42, 130, 218));
        palette.setColor(QPalette::Highlight, QColor(42, 130, 218));
        palette.setColor(QPalette::HighlightedText, Qt::black);
        app.setPalette(palette);
        
        QFont defaultFont("Arial", 12);
        app.setFont(defaultFont);
    }
} // namespace UIUtils

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    UIUtils::applyDarkTheme(app);

    // 创建主窗口
    QWidget mainWindow;
    mainWindow.resize(2480, 1800);
    
    // 创建主布局
    QVBoxLayout *outerLayout = new QVBoxLayout(&mainWindow);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->setSpacing(0);
    
    // 创建Tab管理器
    TabManager* tabManager = new TabManager(&mainWindow);
    tabManager->initializeTabs();
    
    // 将菜单栏添加到外层布局
    outerLayout->addWidget(tabManager->getMenuBar());
    
    // 创建主内容区域
    QWidget *contentWidget = new QWidget;
    QHBoxLayout *mainLayout = new QHBoxLayout(contentWidget);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    
    // 添加tab widget和控制面板到主布局
    mainLayout->addWidget(tabManager->getTabWidget(), 8);
    mainLayout->addWidget(tabManager->getControlContainer());
    
    // 将内容添加到外层布局
    outerLayout->addWidget(contentWidget, 1);
    
    // 设置主窗口
    mainWindow.setLayout(outerLayout);
    mainWindow.setWindowTitle("OBJ Viewer - OpenMesh");
    mainWindow.show();

    return app.exec();
}