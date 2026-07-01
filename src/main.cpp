#include "mainwindow.h"
#include "setupwizard.h"
#include <QApplication>
#include <QFile>
#include <QIcon>
#include <QSettings>
#include <QStandardPaths>

static QIcon findAppIcon()
{
    QIcon icon(":/icons/chiriksip.png");
    if (!icon.isNull()) return icon;

#ifdef Q_OS_WIN
    QString appDir = QCoreApplication::applicationDirPath();
    icon = QIcon(appDir + "/chiriksip.ico");
    if (!icon.isNull()) return icon;
    icon = QIcon(appDir + "/chiriksip.png");
    if (!icon.isNull()) return icon;
    icon = QIcon(appDir + "/resources/icons/chiriksip.png");
    if (!icon.isNull()) return icon;
#endif

    icon = QIcon("/usr/share/icons/hicolor/256x256/apps/chiriksip.png");
    if (!icon.isNull()) return icon;

    return QIcon("resources/icons/chiriksip.png");
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setOrganizationName("chiriksip");
    app.setApplicationName("chiriksip");
    app.setApplicationVersion(PROJECT_VERSION);
    QSettings::setDefaultFormat(QSettings::IniFormat);

    app.setWindowIcon(findAppIcon());

    if (SetupWizard::isFirstRun()) {
        SetupWizard wizard;
        if (wizard.exec() != QDialog::Accepted)
            return 0;

        QSettings settings;
        settings.setValue("server", wizard.server());
        settings.setValue("username", wizard.username());
        settings.setValue("password", wizard.password());
        settings.setValue("port", wizard.port());
        settings.sync();

        QFile configFile(settings.fileName());
        if (configFile.exists()) {
            configFile.setPermissions(QFile::ReadOwner | QFile::WriteOwner);
        }
    }

    MainWindow window;
    window.show();

    return app.exec();
}
