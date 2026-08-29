#include "glwidget/steady3ddissectionwidget.h"
#include "tabs/steady3ddissection_tab.h"

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
    if (argc < 4)
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

    const QString firstPath = QFileInfo(QString::fromLocal8Bit(argv[1])).absoluteFilePath();
    const QString secondPath = QFileInfo(QString::fromLocal8Bit(argv[2])).absoluteFilePath();
    const QString screenshotPath = QFileInfo(QString::fromLocal8Bit(argv[3])).absoluteFilePath();

    QWidget window;
    window.setWindowTitle("Steady 3D Dissection UI Smoke Test");
    window.resize(1600, 900);
    QHBoxLayout* layout = new QHBoxLayout(&window);
    layout->setContentsMargins(0, 0, 0, 0);

    Steady3DDissectionWidget* widget = new Steady3DDissectionWidget(&window);
    QScrollArea* controls = new QScrollArea(&window);
    controls->setWidgetResizable(true);
    controls->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    controls->setMinimumWidth(330);
    controls->setWidget(createSteady3DDissectionControlPanel(widget, &window));
    layout->addWidget(widget, 1);
    layout->addWidget(controls);

    int exitCode = 1;
    QObject::connect(widget, &Steady3DDissectionWidget::snapshotChanged, &window,
                     [&](const SteadyDissectionSnapshot& snapshot) {
        if (!snapshot.complete)
            return;
        QTimer::singleShot(900, &window, [&]() {
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
        SteadyDissectionParameters parameters;
        parameters.resolution = 10;
        parameters.pieceCount = 4;
        parameters.candidateLimit = 20;
        parameters.maxBoundaryDeletionRatio = 0.8;
        parameters.enforceGeneralizedInterlocking = true;
        widget->setExplosion(0.12f);
        widget->loadAndStart(firstPath, secondPath, parameters, true);
    });
    app.exec();
    return exitCode;
}
