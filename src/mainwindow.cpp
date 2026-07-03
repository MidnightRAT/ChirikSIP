#include "mainwindow.h"
#include "sipclient.h"
#include "callmanager.h"
#include "settingsdialog.h"
#include "setupwizard.h"
#include "callnotification.h"
#include "scrollhelper.h"
#include "audiodevicemanager.h"
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
#include <QAudioDevice>
#include <QMediaDevices>

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
    , m_callManager(new CallManager(m_sipClient, this))
{
    setWindowTitle("ChirikSIP");
    setMinimumSize(300, 400);

    setupMenu();
    setupTray();
    loadSettings();
    m_callManager->setEchoCancel(m_echoCancel, m_echoAggressiveness);

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
    m_numberLabel->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
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

    // Device selection combos
    QHBoxLayout *deviceLayout = new QHBoxLayout();
    m_inputDeviceCombo = new QComboBox(central);
    m_outputDeviceCombo = new QComboBox(central);
    m_inputDeviceCombo->setToolTip("Microphone");
    m_outputDeviceCombo->setToolTip("Speakers");

    const auto inputs = AudioDeviceManager::instance().audioInputs();
    for (const QAudioDevice &dev : inputs)
        m_inputDeviceCombo->addItem(dev.description(), QVariant::fromValue(dev));

    const auto outputs = AudioDeviceManager::instance().audioOutputs();
    for (const QAudioDevice &dev : outputs)
        m_outputDeviceCombo->addItem(dev.description(), QVariant::fromValue(dev));

    QAudioDevice curIn = AudioDeviceManager::instance().currentInputDevice();
    QAudioDevice curOut = AudioDeviceManager::instance().currentOutputDevice();
    setInputComboDevice(curIn);
    setOutputComboDevice(curOut);

    connect(m_inputDeviceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onInputDeviceChanged);
    connect(m_outputDeviceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onOutputDeviceChanged);

    connect(&AudioDeviceManager::instance(), &AudioDeviceManager::deviceListChanged,
            this, &MainWindow::rebuildTrayDeviceMenus);

    deviceLayout->addWidget(new QLabel("Mic:", central));
    deviceLayout->addWidget(m_inputDeviceCombo);
    deviceLayout->addWidget(new QLabel("Spk:", central));
    deviceLayout->addWidget(m_outputDeviceCombo);
    mainLayout->addLayout(deviceLayout);

    QHBoxLayout *statusLayout = new QHBoxLayout();
    m_statusLabel = new QLabel("Not registered", central);
    m_statusLabel->setAlignment(Qt::AlignLeft);
    m_statusLabel->setStyleSheet(STYLE_STATUS_DEFAULT);
    m_callDurationLabel = new QLabel(central);
    m_callDurationLabel->setAlignment(Qt::AlignCenter);
    m_callDurationLabel->setStyleSheet(STYLE_STATUS_OK);
    m_callDurationLabel->hide();
    m_ownerLabel = new QLabel(central);
    m_ownerLabel->setAlignment(Qt::AlignCenter);
    m_ownerLabel->setStyleSheet(STYLE_STATUS_OK_BOLD);
    m_ownerLabel->hide();
    m_portLabel = new QLabel(central);
    m_portLabel->setAlignment(Qt::AlignRight);
    m_portLabel->setStyleSheet(STYLE_STATUS_OK);
    statusLayout->addWidget(m_statusLabel);
    statusLayout->addStretch();
    statusLayout->addWidget(m_callDurationLabel);
    statusLayout->addStretch();
    statusLayout->addWidget(m_ownerLabel);
    statusLayout->addStretch();
    statusLayout->addWidget(m_portLabel);
    mainLayout->addLayout(statusLayout);

    m_sipClient->init(m_port);

    if (!m_server.isEmpty() && !m_username.isEmpty()) {
        QTimer::singleShot(100, this, &MainWindow::onRegisterClicked);
    }

    connect(m_callBtn, &QPushButton::clicked, this, &MainWindow::onCallClicked);
    connect(m_callManager, &CallManager::registrationStatus, this, &MainWindow::onRegistrationStatus);
    connect(m_callManager, &CallManager::callStateChanged, this, &MainWindow::onCallStateChanged);
    connect(m_callManager, &CallManager::callMediaActive, this, &MainWindow::onCallMediaActive);
    connect(m_callManager, &CallManager::incomingCall, this, &MainWindow::onIncomingCall);
    connect(m_callManager, &CallManager::callDurationChanged, this, &MainWindow::onCallDurationChanged);
}

MainWindow::~MainWindow()
{
    m_sipClient->shutdown();
}

void MainWindow::loadSettings()
{
    QSettings settings;
    m_server = settings.value("server").toString();
    m_username = settings.value("username").toString();
    // NOTE: base64 is obfuscation only, not encryption. Anyone with file access
    // can decode it. For real security, use system keyring (KWallet, SecretService).
    QString storedPassword = settings.value("password", QString()).toString();
    if (storedPassword.startsWith("b64:")) {
        m_password = QString::fromUtf8(QByteArray::fromBase64(storedPassword.mid(4).toUtf8()));
    } else {
        // Migrate plaintext password to base64 on next save
        m_password = storedPassword;
        if (!m_password.isEmpty()) {
            settings.setValue("password", "b64:" + QString::fromUtf8(m_password.toUtf8().toBase64()));
            settings.sync();
        }
    }
    m_port = settings.value("port", 0).toInt();
    m_echoCancel = settings.value("echoCancel", true).toBool();
    m_echoAggressiveness = settings.value("echoAggressiveness", 1).toInt();

    if (!m_username.isEmpty() && m_ownerLabel) {
        m_ownerLabel->setText(m_username);
        m_ownerLabel->show();
    }
}

void MainWindow::saveSettings()
{
    QSettings settings;
    settings.setValue("server", m_server);
    settings.setValue("username", m_username);
    settings.setValue("password", "b64:" + QString::fromUtf8(m_password.toUtf8().toBase64()));
    settings.setValue("port", m_port);
    settings.setValue("echoCancel", m_echoCancel);
    settings.setValue("echoAggressiveness", m_echoAggressiveness);
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
        if (m_callManager->isInCall()) {
            m_callManager->hangup();
            m_callBtn->setEnabled(true);
            m_callBtn->setText("Call");
            m_hangupBtn->setEnabled(true);
            m_scrollHelper->stop();
            m_callDurationLabel->hide();
            setClockDisplay();
            m_statusLabel->setText("Call ended");
            m_statusLabel->setStyleSheet(STYLE_STATUS_DEFAULT);
        }
        event->accept();
        return;
    }

    if (key == Qt::Key_Backspace) {
        if (m_callManager->isInCall() || !m_dialingMode) {
            event->ignore();
            return;
        }
        QString current = m_numberLabel->text();
        if (!current.isEmpty()) {
            current.chop(1);
            m_numberLabel->setText(current);
        }
        if (current.isEmpty()) {
            setClockDisplay();
        }
        event->accept();
        return;
    }

    QString text = event->text();
    if (!text.isEmpty() && text[0].isPrint()) {
        if (m_callManager->isInCall()) {
            event->ignore();
            return;
        }
        QString current = m_numberLabel->text();
        if (current.contains(':'))
            current.clear();
        m_dialingMode = true;
        m_numberLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_numberLabel->setText(current + text);
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

QString MainWindow::formatDuration(int seconds)
{
    int h = seconds / 3600;
    int m = (seconds % 3600) / 60;
    int s = seconds % 60;
    return QString("%1:%2:%3")
        .arg(h, 2, 10, QChar('0'))
        .arg(m, 2, 10, QChar('0'))
        .arg(s, 2, 10, QChar('0'));
}

void MainWindow::onNumpadClicked()
{
    QPushButton *btn = qobject_cast<QPushButton *>(sender());
    if (!btn || m_callManager->isInCall())
        return;
    QString current = m_numberLabel->text();
    if (current.contains(':'))
        current.clear();
    m_dialingMode = true;
    m_numberLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_numberLabel->setText(current + btn->text());
}

void MainWindow::onZeroPressed()
{
    if (m_callManager->isInCall()) return;
    QString current = m_numberLabel->text();
    if (current.contains(':'))
        current.clear();
    m_dialingMode = true;
    m_numberLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_numberLabel->setText(current + "0");
    m_zeroTimer->start();
}

void MainWindow::onZeroReleased()
{
    if (m_zeroTimer->isActive())
        m_zeroTimer->stop();
}

void MainWindow::onZeroLongPress()
{
    if (m_callManager->isInCall()) return;
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
        if (m_callManager->isInCall() || m_callManager->isIncomingWaiting()) {
            m_callManager->hangup();
            m_scrollHelper->stop();
            if (m_callNotification)
                m_callNotification->hideNotification();
        } else if (m_dialingMode) {
            QString current = m_numberLabel->text();
            if (!current.isEmpty()) {
                current.chop(1);
                m_numberLabel->setText(current);
            }
            if (current.isEmpty()) {
                setClockDisplay();
            }
        }
    }
}

void MainWindow::onHangupLongPress()
{
    m_longPressFired = true;
    if (m_callManager->isInCall() || m_callManager->isIncomingWaiting()) {
        m_callManager->hangup();
        m_scrollHelper->stop();
        if (m_callNotification)
            m_callNotification->hideNotification();
    } else {
        setClockDisplay();
    }
}

void MainWindow::onCallClicked()
{
    if (m_callManager->isIncomingWaiting()) {
        if (m_callManager->answerCall()) {
            m_callBtn->setEnabled(false);
            m_hangupBtn->setEnabled(true);
            m_scrollHelper->stop();
            m_callDurationLabel->show();
            setCallDisplay();
            m_statusLabel->setText("Call active");
            m_statusLabel->setStyleSheet(STYLE_STATUS_OK_BOLD);
        }
        return;
    }

    QString number = m_numberLabel->text().trimmed();
    if (number.isEmpty())
        return;

    if (m_callManager->makeCall(number)) {
        m_dialedNumber = number;
        m_callBtn->setEnabled(false);
        m_hangupBtn->setEnabled(true);
        m_callDurationLabel->show();
        setCallDisplay();
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
        if (!m_username.isEmpty()) {
            m_ownerLabel->setText(m_username);
            m_ownerLabel->show();
        }
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
        m_callBtn->setEnabled(true);
        m_callBtn->setText("Call");
        m_hangupBtn->setEnabled(true);
        m_scrollHelper->stop();
        m_callDurationLabel->hide();
        setClockDisplay();
        m_statusLabel->setText("Call ended");
        m_statusLabel->setStyleSheet(STYLE_STATUS_DEFAULT);
        if (!m_username.isEmpty()) {
            m_ownerLabel->setText(m_username);
            m_ownerLabel->show();
        }
        if (m_callNotification)
            m_callNotification->hideNotification();
    } else if (state == "CALLING" || state == "EARLY" || state == "CONNECTING") {
        m_statusLabel->setText("Connecting...");
        m_statusLabel->setStyleSheet(STYLE_STATUS_CONNECTING);
    }
}

void MainWindow::onCallMediaActive(int callId)
{
    Q_UNUSED(callId);
    setCallDisplay();
}

void MainWindow::onIncomingCall(int callId, const QString &remoteUri)
{
    Q_UNUSED(callId);

    QString number = parseNumber(remoteUri);
    QString name = parseDisplayName(remoteUri);

    m_numberLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
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
        if (!m_callNotification) {
            m_callNotification = new CallNotification(m_sipClient, this);

            connect(m_callNotification, &CallNotification::accepted, this, [this]() {
                m_callBtn->setEnabled(false);
                m_hangupBtn->setEnabled(true);
                m_scrollHelper->stop();
                m_callDurationLabel->show();
                setCallDisplay();
                m_statusLabel->setText("Call active");
                m_statusLabel->setStyleSheet(STYLE_STATUS_OK_BOLD);
            });

            connect(m_callNotification, &CallNotification::rejected, this, [this]() {
                m_scrollHelper->stop();
                setClockDisplay();
                m_statusLabel->setText("Call rejected");
                m_statusLabel->setStyleSheet(STYLE_STATUS_DEFAULT);
            });
        }

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

    trayMenu->addSeparator();

    m_inputDeviceMenu = trayMenu->addMenu("Input Device");
    m_outputDeviceMenu = trayMenu->addMenu("Output Device");
    rebuildTrayDeviceMenus();

    m_trayIcon = new QSystemTrayIcon(this);
    m_trayIcon->setContextMenu(trayMenu);
    m_trayIcon->setToolTip("ChirikSIP");

    QIcon icon = QApplication::windowIcon();
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

    QAction *wizardAction = settingsMenu->addAction("&Setup Wizard");
    connect(wizardAction, &QAction::triggered, this, [this]() {
        SetupWizard wizard(this);
        if (wizard.exec() == QDialog::Accepted) {
            m_server = wizard.server();
            m_username = wizard.username();
            m_password = wizard.password();
            m_port = wizard.port();
            saveSettings();
            m_ownerLabel->setText(m_username);
            m_ownerLabel->show();
            if (!m_server.isEmpty() && !m_username.isEmpty()) {
                m_statusLabel->setText("Re-registering...");
                m_statusLabel->setStyleSheet(STYLE_STATUS_REGISTERING);
                m_sipClient->registerAccount(m_server, m_username, m_password, m_port);
            }
        }
    });

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
    dlg.setEchoCancelEnabled(m_echoCancel);
    dlg.setEchoAggressiveness(m_echoAggressiveness);
    dlg.setInputDevice(AudioDeviceManager::instance().currentInputDevice());
    dlg.setOutputDevice(AudioDeviceManager::instance().currentOutputDevice());

    if (dlg.exec() == QDialog::Accepted) {
        QString newServer = dlg.server();
        QString newUser = dlg.username();
        QString newPass = dlg.password();
        int newPort = dlg.port();
        bool newEchoCancel = dlg.echoCancelEnabled();
        int newEchoAggr = dlg.echoAggressiveness();

        bool changed = (newServer != m_server || newUser != m_username ||
                       newPass != m_password || newPort != m_port ||
                       newEchoCancel != m_echoCancel || newEchoAggr != m_echoAggressiveness);

        m_server = newServer;
        m_username = newUser;
        m_password = newPass;
        m_port = newPort;
        m_echoCancel = newEchoCancel;
        m_echoAggressiveness = newEchoAggr;
        AudioDeviceManager::instance().setInputDevice(dlg.selectedInputDevice());
        AudioDeviceManager::instance().setOutputDevice(dlg.selectedOutputDevice());
        saveSettings();
        m_callManager->setEchoCancel(m_echoCancel, m_echoAggressiveness);

        m_portLabel->setText(QString("UDP:%1").arg(effectivePort()));
        m_ownerLabel->setText(m_username);
        if (!m_username.isEmpty()) {
            m_ownerLabel->show();
        } else {
            m_ownerLabel->hide();
        }

        if (changed && !m_server.isEmpty() && !m_username.isEmpty()) {
            m_statusLabel->setText("Re-registering...");
            m_statusLabel->setStyleSheet(STYLE_STATUS_REGISTERING);
            m_sipClient->registerAccount(m_server, m_username, m_password, m_port);
        }
    }
}

void MainWindow::setClockDisplay()
{
    m_dialingMode = false;
    m_numberLabel->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    m_numberLabel->setText(QTime::currentTime().toString("HH:mm:ss"));
    m_nameLabel->setText("");
}

void MainWindow::setCallDisplay()
{
    m_numberLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_numberLabel->setText(m_dialedNumber);
    m_nameLabel->setText(formatDuration(m_callManager->callDuration()));
}

void MainWindow::updateClock()
{
    if (!m_callManager->isInCall() && !m_callManager->isIncomingWaiting() && !m_dialingMode) {
        m_numberLabel->setText(QTime::currentTime().toString("HH:mm:ss"));
    }
}

void MainWindow::onCallDurationChanged(int seconds)
{
    QString duration = formatDuration(seconds);
    m_callDurationLabel->setText(duration);
    if (m_callManager->isInCall())
        m_nameLabel->setText(duration);
}

void MainWindow::rebuildTrayDeviceMenus()
{
    if (!m_inputDeviceMenu || !m_outputDeviceMenu)
        return;

    m_inputDeviceMenu->clear();
    m_outputDeviceMenu->clear();

    QAudioDevice curIn = AudioDeviceManager::instance().currentInputDevice();
    const auto inputs = AudioDeviceManager::instance().audioInputs();
    for (const QAudioDevice &dev : inputs) {
        QAction *action = m_inputDeviceMenu->addAction(dev.description());
        action->setCheckable(true);
        action->setChecked(dev.id() == curIn.id());
        action->setData(QVariant::fromValue(dev));
        connect(action, &QAction::triggered, this, &MainWindow::onInputDeviceActionTriggered);
    }

    QAudioDevice curOut = AudioDeviceManager::instance().currentOutputDevice();
    const auto outputs = AudioDeviceManager::instance().audioOutputs();
    for (const QAudioDevice &dev : outputs) {
        QAction *action = m_outputDeviceMenu->addAction(dev.description());
        action->setCheckable(true);
        action->setChecked(dev.id() == curOut.id());
        action->setData(QVariant::fromValue(dev));
        connect(action, &QAction::triggered, this, &MainWindow::onOutputDeviceActionTriggered);
    }
}

void MainWindow::onInputDeviceActionTriggered()
{
    QAction *action = qobject_cast<QAction *>(sender());
    if (action)
        AudioDeviceManager::instance().setInputDevice(action->data().value<QAudioDevice>());
}

void MainWindow::onOutputDeviceActionTriggered()
{
    QAction *action = qobject_cast<QAction *>(sender());
    if (action)
        AudioDeviceManager::instance().setOutputDevice(action->data().value<QAudioDevice>());
}

void MainWindow::onInputDeviceChanged(int index)
{
    if (index < 0) return;
    QAudioDevice dev = m_inputDeviceCombo->itemData(index).value<QAudioDevice>();
    AudioDeviceManager::instance().setInputDevice(dev);
}

void MainWindow::onOutputDeviceChanged(int index)
{
    if (index < 0) return;
    QAudioDevice dev = m_outputDeviceCombo->itemData(index).value<QAudioDevice>();
    AudioDeviceManager::instance().setOutputDevice(dev);
}

void MainWindow::setInputComboDevice(const QAudioDevice &device)
{
    for (int i = 0; i < m_inputDeviceCombo->count(); ++i) {
        if (m_inputDeviceCombo->itemData(i).value<QAudioDevice>().id() == device.id()) {
            m_inputDeviceCombo->setCurrentIndex(i);
            return;
        }
    }
}

void MainWindow::setOutputComboDevice(const QAudioDevice &device)
{
    for (int i = 0; i < m_outputDeviceCombo->count(); ++i) {
        if (m_outputDeviceCombo->itemData(i).value<QAudioDevice>().id() == device.id()) {
            m_outputDeviceCombo->setCurrentIndex(i);
            return;
        }
    }
}

void MainWindow::onAbout()
{
    QMessageBox::about(this, "About ChirikSIP",
        "<h2>ChirikSIP</h2>"
        "<p>Version " + QApplication::applicationVersion() + "</p>"
        "<p>Build: " __DATE__ " " __TIME__ "</p>"
        "<p>A simple SIP client for KDE Plasma.</p>"
        "<p>Built with Qt6 and PJSIP.</p>"
        "<p>License: MIT</p>");
}
