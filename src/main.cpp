#include "mainwindow.h"
#include "setupwizard.h"
#include <QApplication>
#include <QDBusConnection>
#include <QDir>
#include <QFile>
#include <QIcon>
#include <QMessageBox>
#include <QSettings>
#include <QStandardPaths>

static const QString dbusService = "com.github.chirik.ChirikSIP";

static bool isFlatpak()
{
    return QFile::exists("/.flatpak-info");
}

static QIcon findAppIcon()
{
    QIcon icon(":/icons/chiriksip.png");
    if (!icon.isNull()) return icon;

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

    if (isFlatpak()) {
        qputenv("XDG_CONFIG_HOME", (QDir::homePath() + "/.config").toUtf8());
    }

    app.setWindowIcon(findAppIcon());

    QDBusConnection bus = QDBusConnection::sessionBus();
    if (bus.registerService(dbusService)) {
    } else {
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setWindowTitle("ChirikSIP");
        msgBox.setText("ChirikSIP is already running.");
        msgBox.setInformativeText("Only one instance of ChirikSIP can be running at a time.");
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setDefaultButton(QMessageBox::Ok);
        msgBox.exec();
        return 0;
    }

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
