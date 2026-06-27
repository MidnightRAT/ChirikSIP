#include "mainwindow.h"
#include "setupwizard.h"
#include <QApplication>
#include <QIcon>
#include <QSettings>

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

    if (SetupWizard::isFirstRun()) {
        SetupWizard wizard;
        if (wizard.exec() != QDialog::Accepted)
            return 0;

        QSettings settings;
        settings.setValue("server", wizard.server());
        settings.setValue("username", wizard.username());
        settings.setValue("password", wizard.password());
    }

    MainWindow window;
    window.show();

    return app.exec();
}
