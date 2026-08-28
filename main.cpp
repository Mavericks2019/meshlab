// main.cpp
#include <QApplication>
#include <QCursor>
#include <QGuiApplication>
#include <QScreen>
#include <QScrollArea>
#include <QSplitter>
#include <QVBoxLayout>
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
        
    }
} // namespace UIUtils

int main(int argc, char *argv[])
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif

    QApplication app(argc, argv);
    UIUtils::applyDarkTheme(app);

    // 创建主窗口
    QWidget mainWindow;
    
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
    QSplitter *contentSplitter = new QSplitter(Qt::Horizontal);
    contentSplitter->setChildrenCollapsible(false);
    contentSplitter->setHandleWidth(6);

    QScrollArea *controlScrollArea = new QScrollArea;
    controlScrollArea->setWidgetResizable(true);
    controlScrollArea->setFrameShape(QFrame::NoFrame);
    controlScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    controlScrollArea->setMinimumWidth(240);
    controlScrollArea->setWidget(tabManager->getControlContainer());

    contentSplitter->addWidget(tabManager->getTabWidget());
    contentSplitter->addWidget(controlScrollArea);
    contentSplitter->setStretchFactor(0, 4);
    contentSplitter->setStretchFactor(1, 1);
    
    // 将内容添加到外层布局
    outerLayout->addWidget(contentSplitter, 1);
    
    // 设置主窗口
    mainWindow.setLayout(outerLayout);
    mainWindow.setWindowTitle("OBJ Viewer - OpenMesh");

    QScreen *screen = QGuiApplication::screenAt(QCursor::pos());
    if (!screen) {
        screen = QGuiApplication::primaryScreen();
    }

    if (screen) {
        const QRect available = screen->availableGeometry();
        const qreal scale = available.height() < 900 ? 0.96 : 0.90;
        const QSize initialSize(
            qRound(available.width() * scale),
            qRound(available.height() * scale));
        QRect initialGeometry(QPoint(0, 0), initialSize);
        initialGeometry.moveCenter(available.center());
        mainWindow.setGeometry(initialGeometry);

        const int controlWidth = qBound(260, qRound(initialSize.width() * 0.22), 420);
        contentSplitter->setSizes({initialSize.width() - controlWidth, controlWidth});
    }

    mainWindow.show();

    return app.exec();
}
