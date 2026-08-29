#include "glwidget/dchartswidget.h"
#include "tabs/dcharts_tab.h"

#include <QApplication>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QPalette>
#include <QScrollArea>
#include <QStyleFactory>
#include <QTimer>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    if (argc < 3)
        return 2;

    QApplication::setStyle(QStyleFactory::create("Fusion"));
    QPalette palette;
    palette.setColor(QPalette::Window, QColor(53, 53, 53));
    palette.setColor(QPalette::WindowText, Qt::white);
    palette.setColor(QPalette::Base, QColor(25, 25, 25));
    palette.setColor(QPalette::Text, Qt::white);
    palette.setColor(QPalette::Button, QColor(53, 53, 53));
    palette.setColor(QPalette::ButtonText, Qt::white);
    palette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    app.setPalette(palette);

    const QString meshPath = QFileInfo(QString::fromLocal8Bit(argv[1])).absoluteFilePath();
    const QString screenshotPath = QFileInfo(QString::fromLocal8Bit(argv[2])).absoluteFilePath();

    QWidget window;
    window.setWindowTitle("D-Charts UI Smoke Test");
    window.resize(1440, 900);
    QHBoxLayout* layout = new QHBoxLayout(&window);
    layout->setContentsMargins(0, 0, 0, 0);

    DChartsWidget* widget = new DChartsWidget(&window);
    QScrollArea* controls = new QScrollArea(&window);
    controls->setWidgetResizable(true);
    controls->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    controls->setMinimumWidth(300);
    controls->setWidget(createDChartsControlPanel(widget, &window));
    layout->addWidget(widget, 1);
    layout->addWidget(controls);

    int exitCode = 1;
    QObject::connect(widget, &DChartsWidget::snapshotChanged, &window,
                     [&](const DChartsSnapshot& snapshot) {
        if (!snapshot.complete)
            return;
        QTimer::singleShot(750, &window, [&]() {
            exitCode = window.grab().save(screenshotPath, "PNG") ? 0 : 3;
            app.quit();
        });
    });
    QTimer::singleShot(90000, &window, [&]() {
        exitCode = 4;
        app.quit();
    });

    window.show();
    QTimer::singleShot(0, &window, [&]() {
        DChartsParameters parameters;
        parameters.maxIterations = 100;
        widget->loadAndStart(meshPath, parameters, true);
    });
    app.exec();
    return exitCode;
}
