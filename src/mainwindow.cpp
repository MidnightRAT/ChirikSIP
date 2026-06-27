#include "mainwindow.h"
#include "sipclient.h"
#include "settingsdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QMessageBox>
#include <QSettings>
#include <QCloseEvent>
#include <QMenuBar>
#include <QMenu>
#include <QAction>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_sipClient(new SipClient(this))
{
    setWindowTitle("ChirikSIP");
    setMinimumSize(300, 400);

    setupMenu();
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

    m_scrollTimer = new QTimer(this);
    m_scrollTimer->setInterval(200);
    connect(m_scrollTimer, &QTimer::timeout, this, &MainWindow::onScrollTick);

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
    m_callBtn->setStyleSheet(
        "QPushButton { font-size: 16px; font-weight: bold; "
        "background: #2e7d32; color: white; border: none; border-radius: 8px; }"
        "QPushButton:pressed { background: #1b5e20; }"
        "QPushButton:disabled { background: #555; color: #999; }");

    m_hangupBtn = new QPushButton("Hangup", central);
    m_hangupBtn->setMinimumHeight(50);
    m_hangupBtn->setStyleSheet(
        "QPushButton { font-size: 16px; font-weight: bold; "
        "background: #c62828; color: white; border: none; border-radius: 8px; }"
        "QPushButton:pressed { background: #8e0000; }"
        "QPushButton:disabled { background: #555; color: #999; }");

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

    m_statusLabel = new QLabel("Not registered", central);
    m_statusLabel->setAlignment(Qt::AlignLeft);
    m_statusLabel->setStyleSheet("color: gray; padding: 4px; font-size: 11px;");
    mainLayout->addWidget(m_statusLabel);

    m_sipClient->init();

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
}

void MainWindow::saveSettings()
{
    QSettings settings;
    settings.setValue("server", m_server);
    settings.setValue("username", m_username);
    settings.setValue("password", m_password);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    saveSettings();
    QMainWindow::closeEvent(event);
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->isAutoRepeat()) {
        event->accept();
        return;
    }

    int key = event->key();

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
            m_scrollTimer->stop();
            m_numberLabel->setText("");
            m_nameLabel->setText("");
            m_statusLabel->setText("Call ended");
            m_statusLabel->setStyleSheet("color: gray; padding: 4px; font-size: 11px;");
        }
        event->accept();
        return;
    }

    if (key == Qt::Key_Backspace) {
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

void MainWindow::onScrollTick()
{
    if (m_scrollText.isEmpty()) {
        m_scrollTimer->stop();
        return;
    }

    QFontMetrics fm(m_nameLabel->font());
    int avail = m_nameLabel->width() - 20;
    if (fm.horizontalAdvance(m_scrollText) <= avail) {
        m_nameLabel->setText(m_scrollText);
        m_scrollTimer->stop();
        return;
    }

    QString sep = " *|* ";
    QString src = m_scrollText + sep;
    m_nameLabel->setText(src.mid(m_scrollOffset % src.length(), 30));
    m_scrollOffset++;
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
        if (m_inCall) {
            m_sipClient->hangup();
            m_scrollTimer->stop();
        } else if (!m_incomingWaiting) {
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
    if (m_inCall) {
        m_sipClient->hangup();
        m_scrollTimer->stop();
    } else if (!m_incomingWaiting) {
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
            m_scrollTimer->stop();
            m_statusLabel->setText("Call active");
            m_statusLabel->setStyleSheet("color: #4CAF50; padding: 4px; font-size: 11px; font-weight: bold;");
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
        m_statusLabel->setStyleSheet("color: #2196F3; padding: 4px; font-size: 11px; font-weight: bold;");
    }
}

void MainWindow::onRegisterClicked()
{
    if (m_server.isEmpty() || m_username.isEmpty()) {
        onSettings();
        return;
    }

    m_statusLabel->setText("Registering...");
    m_statusLabel->setStyleSheet("color: #FF9800; padding: 4px; font-size: 11px;");
    m_sipClient->registerAccount(m_server, m_username, m_password);
}

void MainWindow::onRegistrationStatus(bool ok, const QString &message)
{
    if (ok) {
        m_statusLabel->setText("Registered");
        m_statusLabel->setStyleSheet("color: #4CAF50; padding: 4px; font-size: 11px;");
        m_callBtn->setEnabled(true);
        m_hangupBtn->setEnabled(true);
    } else {
        m_statusLabel->setText(message);
        m_statusLabel->setStyleSheet("color: #f44336; padding: 4px; font-size: 11px;");
    }
}

void MainWindow::onCallStateChanged(int callId, const QString &state)
{
    Q_UNUSED(callId);

    if (state == "CONFIRMED") {
        m_statusLabel->setText("Call active");
        m_statusLabel->setStyleSheet("color: #4CAF50; padding: 4px; font-size: 11px; font-weight: bold;");
    } else if (state == "DISCONNCTD") {
        m_inCall = false;
        m_incomingWaiting = false;
        m_callBtn->setEnabled(true);
        m_callBtn->setText("Call");
        m_hangupBtn->setEnabled(true);
        m_scrollTimer->stop();
        m_numberLabel->setText("");
        m_nameLabel->setText("");
        m_statusLabel->setText("Call ended");
        m_statusLabel->setStyleSheet("color: gray; padding: 4px; font-size: 11px;");
    } else if (state == "CALLING" || state == "EARLY" || state == "CONNECTING") {
        m_statusLabel->setText("Connecting...");
        m_statusLabel->setStyleSheet("color: #2196F3; padding: 4px; font-size: 11px;");
    }
}

void MainWindow::onIncomingCall(int callId, const QString &remoteUri)
{
    Q_UNUSED(callId);

    m_incomingWaiting = true;
    m_numberLabel->setText(parseNumber(remoteUri));

    QString name = parseDisplayName(remoteUri);
    if (!name.isEmpty()) {
        m_scrollText = name;
        m_scrollOffset = 0;
        m_scrollTimer->start();
    } else {
        m_scrollText.clear();
        m_nameLabel->setText("-= UNKNOWN =-");
    }

    m_callBtn->setEnabled(true);
    m_callBtn->setText("Answer");
    m_callBtn->setStyleSheet(
        "QPushButton { font-size: 16px; font-weight: bold; "
        "background: #2e7d32; color: white; border: none; border-radius: 8px; }"
        "QPushButton:pressed { background: #1b5e20; }");
    m_hangupBtn->setEnabled(true);
    m_statusLabel->setText("Incoming call");
    m_statusLabel->setStyleSheet("color: #FF9800; padding: 4px; font-size: 11px; font-weight: bold;");
}

void MainWindow::setupMenu()
{
    QMenuBar *menu = menuBar();

    QMenu *fileMenu = menu->addMenu("&File");
    QAction *exitAction = fileMenu->addAction("E&xit");
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &QWidget::close);

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

    if (dlg.exec() == QDialog::Accepted) {
        QString newServer = dlg.server();
        QString newUser = dlg.username();
        QString newPass = dlg.password();

        bool changed = (newServer != m_server || newUser != m_username || newPass != m_password);

        m_server = newServer;
        m_username = newUser;
        m_password = newPass;
        saveSettings();

        if (changed && !m_server.isEmpty() && !m_username.isEmpty()) {
            m_statusLabel->setText("Re-registering...");
            m_statusLabel->setStyleSheet("color: #FF9800; padding: 4px; font-size: 11px;");
            m_sipClient->registerAccount(m_server, m_username, m_password);
        }
    }
}

void MainWindow::onAbout()
{
    QMessageBox::about(this, "About ChirikSIP",
        "<h2>ChirikSIP</h2>"
        "<p>Version 1.0.0</p>"
        "<p>A simple SIP client for KDE Plasma.</p>"
        "<p>Built with Qt6 and PJSIP.</p>"
        "<p>License: MIT</p>");
}
