#include "mainwindow.h"
#include "setupwizard.h"
#include <QApplication>
#include <QIcon>
#include <QSettings>
#include <QStandardPaths>

static QIcon findAppIcon()
{
#ifdef Q_OS_WIN
    QString appDir = QCoreApplication::applicationDirPath();
    QIcon icon(appDir + "/chiriksip.ico");
    if (!icon.isNull()) return icon;
    icon = QIcon(appDir + "/resources/icons/chiriksip.png");
    if (!icon.isNull()) return icon;
#endif

    QIcon icon("/usr/share/icons/hicolor/256x256/apps/chiriksip.png");
    if (!icon.isNull()) return icon;

    return QIcon("resources/icons/chiriksip.png");
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setOrganizationName("chiriksip");
    app.setApplicationName("chiriksip");
    app.setApplicationVersion("1.0.0");

    app.setWindowIcon(findAppIcon());

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
