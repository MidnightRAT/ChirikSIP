#include "mainwindow.h"
#include "sipclient.h"
#include "settingsdialog.h"
#include "callnotification.h"
#include "scrollhelper.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QMessageBox>
#include <QSettings>
#include <QCloseEvent>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QSystemTrayIcon>
#include <QApplication>
#include <QFile>
#include <QTime>

static const QString STYLE_STATUS_DEFAULT = QStringLiteral("color: gray; padding: 4px; font-size: 11px;");
static const QString STYLE_STATUS_OK = QStringLiteral("color: #4CAF50; padding: 4px; font-size: 11px;");
static const QString STYLE_STATUS_OK_BOLD = QStringLiteral("color: #4CAF50; padding: 4px; font-size: 11px; font-weight: bold;");
static const QString STYLE_STATUS_REGISTERING = QStringLiteral("color: #FF9800; padding: 4px; font-size: 11px;");
static const QString STYLE_STATUS_REGISTERING_BOLD = QStringLiteral("color: #FF9800; padding: 4px; font-size: 11px; font-weight: bold;");
static const QString STYLE_STATUS_CONNECTING = QStringLiteral("color: #2196F3; padding: 4px; font-size: 11px;");
static const QString STYLE_STATUS_CONNECTING_BOLD = QStringLiteral("color: #2196F3; padding: 4px; font-size: 11px; font-weight: bold;");
static const QString STYLE_STATUS_ERROR = QStringLiteral("color: #f44336; padding: 4px; font-size: 11px;");
static const QString STYLE_CALL_BTN = QStringLiteral(
    "QPushButton { font-size: 16px; font-weight: bold; "
    "background: #2e7d32; color: white; border: none; border-radius: 8px; }"
    "QPushButton:pressed { background: #1b5e20; }"
    "QPushButton:disabled { background: #555; color: #999; }");
static const QString STYLE_CALL_BTN_ANSWER = QStringLiteral(
    "QPushButton { font-size: 16px; font-weight: bold; "
    "background: #2e7d32; color: white; border: none; border-radius: 8px; }"
    "QPushButton:pressed { background: #1b5e20; }");
static const QString STYLE_HANGUP_BTN = QStringLiteral(
    "QPushButton { font-size: 16px; font-weight: bold; "
    "background: #c62828; color: white; border: none; border-radius: 8px; }"
    "QPushButton:pressed { background: #8e0000; }"
    "QPushButton:disabled { background: #555; color: #999; }");

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_sipClient(new SipClient(this))
{
    setWindowTitle("ChirikSIP");
    setMinimumSize(300, 400);

    setupMenu();
    setupTray();
    loadSettings();

    QWidget *central = new QWidget(this);
    setCentralWidget(central);
    QVBoxLayout *mainLayout = new QVBoxLayout(central);
    mainLayout->setSpacing(4);
    mainLayout->setContentsMargins(8, 8, 8, 8);

    QWidget *displayWidget = new QWidget(central);
    displayWidget->setStyleSheet("background: #1a1a2e; border: 1px solid #444; border-radius: 6px;");
    QVBoxLayout *displayLayout = new QVBoxLayout(displayWidget);
    displayLayout->setContentsMargins(10, 8, 10, 8);
    displayLayout->setSpacing(0);

    m_numberLabel = new QLabel(displayWidget);
    m_numberLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_numberLabel->setStyleSheet("color: #e0e0e0; font-size: 32px; font-weight: bold; background: transparent; font-family: 'Segment16A';");
    m_numberLabel->setMinimumHeight(40);
    displayLayout->addWidget(m_numberLabel);

    m_nameLabel = new QLabel(displayWidget);
    m_nameLabel->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    m_nameLabel->setStyleSheet("color: #888; font-size: 13px; background: transparent; font-family: monospace;");
    m_nameLabel->setMinimumHeight(18);
    displayLayout->addWidget(m_nameLabel);

    mainLayout->addWidget(displayWidget);

    m_scrollHelper = new ScrollHelper(m_nameLabel, this);

    m_clockTimer = new QTimer(this);
    connect(m_clockTimer, &QTimer::timeout, this, &MainWindow::updateClock);
    m_clockTimer->start(1000);
    updateClock();

    QGridLayout *numpad = new QGridLayout();
    numpad->setSpacing(3);

    const char *labels[] = {
        "1", "2", "3",
        "4", "5", "6",
        "7", "8", "9",
        "*", "0+", "#"
    };

    for (int i = 0; i < 12; ++i) {
        m_numpad[i] = new QPushButton(QString(labels[i]), central);
        m_numpad[i]->setMinimumSize(80, 48);
        m_numpad[i]->setStyleSheet(
            "QPushButton { font-size: 20px; font-weight: bold; "
            "background: #2d2d44; color: #e0e0e0; border: 1px solid #555; border-radius: 6px; }"
            "QPushButton:pressed { background: #3d3d5c; }");
        numpad->addWidget(m_numpad[i], i / 3, i % 3);
        if (i == 10) {
            m_zeroTimer = new QTimer(this);
            m_zeroTimer->setSingleShot(true);
            m_zeroTimer->setInterval(500);
            connect(m_zeroTimer, &QTimer::timeout, this, &MainWindow::onZeroLongPress);
            connect(m_numpad[i], &QPushButton::pressed, this, &MainWindow::onZeroPressed);
            connect(m_numpad[i], &QPushButton::released, this, &MainWindow::onZeroReleased);
        } else {
            connect(m_numpad[i], &QPushButton::clicked, this, &MainWindow::onNumpadClicked);
        }
    }
    mainLayout->addLayout(numpad);

    QHBoxLayout *actionLayout = new QHBoxLayout();
    actionLayout->setSpacing(6);

    m_callBtn = new QPushButton("Call", central);
    m_callBtn->setMinimumHeight(50);
    m_callBtn->setStyleSheet(STYLE_CALL_BTN);

    m_hangupBtn = new QPushButton("Hangup", central);
    m_hangupBtn->setMinimumHeight(50);
    m_hangupBtn->setStyleSheet(STYLE_HANGUP_BTN);

    m_longPressTimer = new QTimer(this);
    m_longPressTimer->setSingleShot(true);
    m_longPressTimer->setInterval(500);
    connect(m_longPressTimer, &QTimer::timeout, this, &MainWindow::onHangupLongPress);

    connect(m_hangupBtn, &QPushButton::pressed, this, &MainWindow::onHangupPressed);
    connect(m_hangupBtn, &QPushButton::released, this, &MainWindow::onHangupReleased);

    actionLayout->addWidget(m_callBtn);
    actionLayout->addWidget(m_hangupBtn);
    mainLayout->addLayout(actionLayout);

    mainLayout->addStretch();

    QHBoxLayout *statusLayout = new QHBoxLayout();
    m_statusLabel = new QLabel("Not registered", central);
    m_statusLabel->setAlignment(Qt::AlignLeft);
    m_statusLabel->setStyleSheet(STYLE_STATUS_DEFAULT);
    m_portLabel = new QLabel(central);
    m_portLabel->setAlignment(Qt::AlignRight);
    m_portLabel->setStyleSheet(STYLE_STATUS_OK);
    statusLayout->addWidget(m_statusLabel);
    statusLayout->addStretch();
    statusLayout->addWidget(m_portLabel);
    mainLayout->addLayout(statusLayout);

    m_sipClient->init(m_port);

    if (!m_server.isEmpty() && !m_username.isEmpty()) {
        QTimer::singleShot(100, this, &MainWindow::onRegisterClicked);
    }

    connect(m_callBtn, &QPushButton::clicked, this, &MainWindow::onCallClicked);
    connect(m_sipClient, &SipClient::registrationStatus, this, &MainWindow::onRegistrationStatus);
    connect(m_sipClient, &SipClient::callStateChanged, this, &MainWindow::onCallStateChanged);
    connect(m_sipClient, &SipClient::incomingCall, this, &MainWindow::onIncomingCall);
}

MainWindow::~MainWindow()
{
}

void MainWindow::loadSettings()
{
    QSettings settings;
    m_server = settings.value("server").toString();
    m_username = settings.value("username").toString();
    m_password = settings.value("password").toString();
    m_port = settings.value("port", 0).toInt();
}

void MainWindow::saveSettings()
{
    QSettings settings;
    settings.setValue("server", m_server);
    settings.setValue("username", m_username);
    settings.setValue("password", m_password);
    settings.setValue("port", m_port);
    settings.sync();

    QFile configFile(settings.fileName());
    if (configFile.exists()) {
        configFile.setPermissions(QFile::ReadOwner | QFile::WriteOwner);
    }
}

int MainWindow::effectivePort() const
{
    return m_port > 0 ? m_port : m_sipClient->boundPort();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    saveSettings();
    if (!m_forceQuit && m_trayIcon) {
        event->ignore();
        m_trayIcon->show();
        hide();
        return;
    }
    QApplication::quit();
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->isAutoRepeat()) {
        event->accept();
        return;
    }

    int key = event->key();
    Qt::KeyboardModifiers mod = event->modifiers();

    if (key == Qt::Key_Q && (mod & Qt::ControlModifier)) {
        m_forceQuit = true;
        close();
        event->accept();
        return;
    }

    if (key == Qt::Key_Return || key == Qt::Key_Enter) {
        onCallClicked();
        event->accept();
        return;
    }

    if (key == Qt::Key_Escape) {
        if (m_inCall) {
            m_sipClient->hangup();
            m_inCall = false;
            m_incomingWaiting = false;
            m_callBtn->setEnabled(true);
            m_callBtn->setText("Call");
            m_hangupBtn->setEnabled(true);
            m_scrollHelper->stop();
            m_numberLabel->setText("");
            m_nameLabel->setText(QTime::currentTime().toString("HH:mm:ss"));
            m_statusLabel->setText("Call ended");
            m_statusLabel->setStyleSheet(STYLE_STATUS_DEFAULT);
        }
        event->accept();
        return;
    }

    if (key == Qt::Key_Backspace) {
        if (m_inCall) {
            event->ignore();
            return;
        }
        QString current = m_numberLabel->text();
        if (!current.isEmpty()) {
            current.chop(1);
            m_numberLabel->setText(current);
        }
        event->accept();
        return;
    }

    QString text = event->text();
    if (!text.isEmpty() && text[0].isPrint()) {
        if (m_inCall) {
            event->ignore();
            return;
        }
        m_numberLabel->setText(m_numberLabel->text() + text);
        event->accept();
        return;
    }

    QMainWindow::keyPressEvent(event);
}

QString MainWindow::parseNumber(const QString &uri)
{
    int sipIdx = uri.indexOf("sip:");
    if (sipIdx == -1)
        return uri;

    QString after = uri.mid(sipIdx + 4);
    int atIdx = after.indexOf('@');
    if (atIdx != -1)
        return after.left(atIdx);
    return after;
}

QString MainWindow::parseDisplayName(const QString &uri)
{
    if (uri.startsWith('"')) {
        int endQuote = uri.indexOf('"', 1);
        if (endQuote > 1)
            return uri.mid(1, endQuote - 1);
    }
    return QString();
}

void MainWindow::onNumpadClicked()
{
    QPushButton *btn = qobject_cast<QPushButton *>(sender());
    if (!btn || m_inCall)
        return;
    m_numberLabel->setText(m_numberLabel->text() + btn->text());
}

void MainWindow::onZeroPressed()
{
    if (m_inCall) return;
    m_numberLabel->setText(m_numberLabel->text() + "0");
    m_zeroTimer->start();
}

void MainWindow::onZeroReleased()
{
    if (m_zeroTimer->isActive())
        m_zeroTimer->stop();
}

void MainWindow::onZeroLongPress()
{
    if (m_inCall) return;
    QString current = m_numberLabel->text();
    if (!current.isEmpty() && current.right(1) == "0") {
        current.chop(1);
        m_numberLabel->setText(current + "+");
    }
}

void MainWindow::onHangupPressed()
{
    m_longPressFired = false;
    m_longPressTimer->start();
}

void MainWindow::onHangupReleased()
{
    if (m_longPressTimer->isActive()) {
        m_longPressTimer->stop();
        if (m_inCall || m_incomingWaiting) {
            m_sipClient->hangup();
            m_scrollHelper->stop();
            if (m_callNotification)
                m_callNotification->hideNotification();
        } else {
            QString current = m_numberLabel->text();
            if (!current.isEmpty()) {
                current.chop(1);
                m_numberLabel->setText(current);
            }
        }
    }
}

void MainWindow::onHangupLongPress()
{
    m_longPressFired = true;
    if (m_inCall || m_incomingWaiting) {
        m_sipClient->hangup();
        m_scrollHelper->stop();
        if (m_callNotification)
            m_callNotification->hideNotification();
    } else {
        m_numberLabel->setText("");
    }
}

void MainWindow::onCallClicked()
{
    if (m_incomingWaiting) {
        if (m_sipClient->answerCall()) {
            m_inCall = true;
            m_incomingWaiting = false;
            m_callBtn->setEnabled(false);
            m_hangupBtn->setEnabled(true);
            m_scrollHelper->stop();
            m_statusLabel->setText("Call active");
            m_statusLabel->setStyleSheet(STYLE_STATUS_OK_BOLD);
        }
        return;
    }

    QString number = m_numberLabel->text().trimmed();
    if (number.isEmpty())
        return;

    if (m_sipClient->makeCall(number)) {
        m_inCall = true;
        m_callBtn->setEnabled(false);
        m_hangupBtn->setEnabled(true);
        m_statusLabel->setText("Calling: " + number);
        m_statusLabel->setStyleSheet(STYLE_STATUS_CONNECTING_BOLD);
    }
}

void MainWindow::onRegisterClicked()
{
    if (m_server.isEmpty() || m_username.isEmpty()) {
        onSettings();
        return;
    }

    m_statusLabel->setText("Registering...");
    m_statusLabel->setStyleSheet(STYLE_STATUS_REGISTERING);
    m_sipClient->registerAccount(m_server, m_username, m_password, m_port);
}

void MainWindow::onRegistrationStatus(bool ok, const QString &message)
{
    if (ok) {
        m_statusLabel->setText("Registered");
        m_statusLabel->setStyleSheet(STYLE_STATUS_OK);
        m_portLabel->setText(QString("UDP:%1").arg(effectivePort()));
        m_callBtn->setEnabled(true);
        m_hangupBtn->setEnabled(true);
    } else {
        m_statusLabel->setText(message);
        m_statusLabel->setStyleSheet(STYLE_STATUS_ERROR);
    }
}

void MainWindow::onCallStateChanged(int callId, const QString &state)
{
    Q_UNUSED(callId);

    if (state == "CONFIRMED") {
        m_statusLabel->setText("Call active");
        m_statusLabel->setStyleSheet(STYLE_STATUS_OK_BOLD);
        if (m_callNotification)
            m_callNotification->hideNotification();
    } else if (state == "DISCONNECTED") {
        m_inCall = false;
        m_incomingWaiting = false;
        m_callBtn->setEnabled(true);
        m_callBtn->setText("Call");
        m_hangupBtn->setEnabled(true);
            m_scrollHelper->stop();
            m_numberLabel->setText("");
            m_nameLabel->setText(QTime::currentTime().toString("HH:mm:ss"));
        m_statusLabel->setText("Call ended");
        m_statusLabel->setStyleSheet(STYLE_STATUS_DEFAULT);
        if (m_callNotification)
            m_callNotification->hideNotification();
    } else if (state == "CALLING" || state == "EARLY" || state == "CONNECTING") {
        m_statusLabel->setText("Connecting...");
        m_statusLabel->setStyleSheet(STYLE_STATUS_CONNECTING);
    }
}

void MainWindow::onIncomingCall(int callId, const QString &remoteUri)
{
    Q_UNUSED(callId);

    m_incomingWaiting = true;
    QString number = parseNumber(remoteUri);
    QString name = parseDisplayName(remoteUri);

    m_numberLabel->setText(number);

    if (!name.isEmpty()) {
        m_scrollHelper->setText(name);
    } else {
        m_scrollHelper->stop();
        m_nameLabel->setText("-= UNKNOWN =-");
    }

    m_callBtn->setEnabled(true);
    m_callBtn->setText("Answer");
    m_callBtn->setStyleSheet(STYLE_CALL_BTN_ANSWER);
    m_hangupBtn->setEnabled(true);
    m_statusLabel->setText("Incoming call");
    m_statusLabel->setStyleSheet(STYLE_STATUS_REGISTERING_BOLD);

    if (!isVisible()) {
        if (!m_callNotification)
            m_callNotification = new CallNotification(m_sipClient, this);

        connect(m_callNotification, &CallNotification::accepted, this, [this]() {
            m_inCall = true;
            m_incomingWaiting = false;
            m_callBtn->setEnabled(false);
            m_hangupBtn->setEnabled(true);
            m_scrollHelper->stop();
            m_statusLabel->setText("Call active");
            m_statusLabel->setStyleSheet(STYLE_STATUS_OK_BOLD);
        }, Qt::UniqueConnection);

        connect(m_callNotification, &CallNotification::rejected, this, [this]() {
            m_incomingWaiting = false;
            m_scrollHelper->stop();
            m_numberLabel->setText("");
            m_nameLabel->setText(QTime::currentTime().toString("HH:mm:ss"));
            m_statusLabel->setText("Call rejected");
            m_statusLabel->setStyleSheet(STYLE_STATUS_DEFAULT);
        }, Qt::UniqueConnection);

        m_callNotification->showNotification(number, name);
    }
}

void MainWindow::setupTray()
{
    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        qWarning() << "System tray not available";
        return;
    }

    QMenu *trayMenu = new QMenu(this);

    QAction *restoreAction = trayMenu->addAction("Restore");
    connect(restoreAction, &QAction::triggered, this, &MainWindow::onTrayRestore);

    QAction *exitAction = trayMenu->addAction("Exit");
    connect(exitAction, &QAction::triggered, this, &MainWindow::onTrayExit);

    m_trayIcon = new QSystemTrayIcon(this);
    m_trayIcon->setContextMenu(trayMenu);
    m_trayIcon->setToolTip("ChirikSIP");

    QIcon icon;
#ifdef Q_OS_WIN
    QString appDir = QApplication::applicationDirPath();
    icon = QIcon(appDir + "/chiriksip.ico");
    if (icon.isNull())
        icon = QIcon(appDir + "/chiriksip.png");
#endif
    if (icon.isNull())
        icon = QApplication::windowIcon();
    if (icon.isNull())
        icon = QIcon("resources/icons/chiriksip.png");
    m_trayIcon->setIcon(icon);

    connect(m_trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::onTrayActivated);
}

void MainWindow::onTrayActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::DoubleClick || reason == QSystemTrayIcon::Trigger) {
        onTrayRestore();
    }
}

void MainWindow::onTrayRestore()
{
    show();
    raise();
    activateWindow();
    m_trayIcon->hide();
}

void MainWindow::onTrayExit()
{
    m_forceQuit = true;
    close();
}

void MainWindow::setupMenu()
{
    QMenuBar *menu = menuBar();

    QMenu *fileMenu = menu->addMenu("&File");
    QAction *exitAction = fileMenu->addAction("E&xit");
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, [this]() { m_forceQuit = true; close(); });

    QMenu *settingsMenu = menu->addMenu("&Settings");
    QAction *settingsAction = settingsMenu->addAction("&Settings");
    settingsAction->setShortcut(QKeySequence("Ctrl+,"));
    connect(settingsAction, &QAction::triggered, this, &MainWindow::onSettings);

    QMenu *helpMenu = menu->addMenu("&Help");
    QAction *aboutAction = helpMenu->addAction("&About ChirikSIP");
    connect(aboutAction, &QAction::triggered, this, &MainWindow::onAbout);
}

void MainWindow::onSettings()
{
    SettingsDialog dlg(this);
    dlg.setServer(m_server);
    dlg.setUsername(m_username);
    dlg.setPassword(m_password);
    dlg.setPort(m_port);

    if (dlg.exec() == QDialog::Accepted) {
        QString newServer = dlg.server();
        QString newUser = dlg.username();
        QString newPass = dlg.password();
        int newPort = dlg.port();

        bool changed = (newServer != m_server || newUser != m_username ||
                       newPass != m_password || newPort != m_port);

        m_server = newServer;
        m_username = newUser;
        m_password = newPass;
        m_port = newPort;
        saveSettings();

        m_portLabel->setText(QString("UDP:%1").arg(effectivePort()));

        if (changed && !m_server.isEmpty() && !m_username.isEmpty()) {
            m_statusLabel->setText("Re-registering...");
            m_statusLabel->setStyleSheet(STYLE_STATUS_REGISTERING);
            m_sipClient->registerAccount(m_server, m_username, m_password, m_port);
        }
    }
}

void MainWindow::updateClock()
{
    if (!m_inCall && !m_incomingWaiting) {
        m_nameLabel->setText(QTime::currentTime().toString("HH:mm:ss"));
    }
}

void MainWindow::onAbout()
{
    QMessageBox::about(this, "About ChirikSIP",
        "<h2>ChirikSIP</h2>"
        "<p>Version " + QApplication::applicationVersion() + "</p>"
        "<p>A simple SIP client for KDE Plasma.</p>"
        "<p>Built with Qt6 and PJSIP.</p>"
        "<p>License: MIT</p>");
}
