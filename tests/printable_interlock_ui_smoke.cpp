#include "glwidget/printableinterlockwidget.h"
#include "tabs/printable_interlock_tab.h"

#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QPalette>
#include <QScrollArea>
#include <QStyleFactory>
#include <QTemporaryDir>
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
    const QString voxelScreenshotPath = argc > 3
        ? QFileInfo(QString::fromLocal8Bit(argv[3])).absoluteFilePath() : QString();

    QWidget window;
    window.setWindowTitle("Printable Interlocking Parts UI Smoke Test");
    window.resize(1600, 900);
    QHBoxLayout* layout = new QHBoxLayout(&window);
    layout->setContentsMargins(0, 0, 0, 0);

    PrintableInterlockWidget* widget = new PrintableInterlockWidget(&window);
    QScrollArea* controls = new QScrollArea(&window);
    controls->setWidgetResizable(true);
    controls->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    controls->setMinimumWidth(350);
    controls->setWidget(createPrintableInterlockControlPanel(widget, &window));
    QPushButton* voxelModeButton = controls->findChild<QPushButton*>("printableVoxelMode");
    QPushButton* surfaceModeButton = controls->findChild<QPushButton*>("printableSurfaceMode");
    QCheckBox* wireframeToggle = controls->findChild<QCheckBox*>("printableWireframeToggle");
    if (!voxelModeButton || !surfaceModeButton || !wireframeToggle)
        return 8;
    layout->addWidget(widget, 1);
    layout->addWidget(controls);

    int exitCode = 1;
    QTemporaryDir exportedParts;
    if (!exportedParts.isValid())
        return 5;
    QObject::connect(widget, &PrintableInterlockWidget::snapshotChanged, &window,
                     [&](const PrintableInterlockSnapshot& snapshot) {
        if (!snapshot.complete)
            return;
        QString error;
        widget->setSurfaceClippedMode(false);
        if (!widget->exportPrintableParts(exportedParts.path(), &error)) {
            exitCode = 6;
            app.quit();
            return;
        }
        widget->setSurfaceClippedMode(true);
        if (!widget->exportPrintableParts(exportedParts.path(), &error)
            || QDir(exportedParts.path()).entryList(
                   {"*.obj"}, QDir::Files).size() != snapshot.requestedPieces * 2) {
            exitCode = 7;
            app.quit();
            return;
        }
        wireframeToggle->setChecked(true);
        auto captureSurface = [&, screenshotPath]() {
            surfaceModeButton->click();
            QTimer::singleShot(700, &window, [&, screenshotPath]() {
                exitCode = window.grab().save(screenshotPath, "PNG") ? 0 : 3;
                app.quit();
            });
        };
        if (voxelScreenshotPath.isEmpty()) {
            captureSurface();
            return;
        }
        voxelModeButton->click();
        QTimer::singleShot(700, &window, [&, captureSurface, voxelScreenshotPath]() {
            if (!window.grab().save(voxelScreenshotPath, "PNG")) {
                exitCode = 9;
                app.quit();
                return;
            }
            captureSurface();
        });
    });
    QTimer::singleShot(90000, &window, [&]() {
        exitCode = 4;
        app.quit();
    });

    window.show();
    QTimer::singleShot(0, &window, [&]() {
        PrintableInterlockParameters parameters;
        parameters.resolution = 14;
        parameters.samplesPerVoxel = 4;
        parameters.pieceCount = 6;
        parameters.candidateLimit = 20;
        widget->setExplosion(0.13f);
        widget->loadAndStart(meshPath, parameters, true);
    });
    app.exec();
    return exitCode;
}
