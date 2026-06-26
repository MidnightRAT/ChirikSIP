#include "mainwindow.h"
#include <QApplication>
#include <QIcon>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setOrganizationName("chiriksip");
    app.setApplicationName("chiriksip");
    app.setApplicationVersion("1.0.0");

    QIcon icon("/usr/share/icons/hicolor/256x256/apps/chiriksip.png");
    if (icon.isNull())
        icon = QIcon("resources/icons/chiriksip.png");
    app.setWindowIcon(icon);

    MainWindow window;
    window.show();

    return app.exec();
}
