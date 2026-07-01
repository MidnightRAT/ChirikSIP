#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QLabel>
#include <QTimer>
#include <QSystemTrayIcon>

class SipClient;
class CallNotification;
class ScrollHelper;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void onNumpadClicked();
    void onZeroPressed();
    void onZeroReleased();
    void onZeroLongPress();
    void onCallClicked();
    void onHangupPressed();
    void onHangupReleased();
    void onHangupLongPress();
    void onRegisterClicked();
    void onRegistrationStatus(bool ok, const QString &message);
    void onCallStateChanged(int callId, const QString &state);
    void onIncomingCall(int callId, const QString &remoteUri);

    void onAbout();
    void onSettings();
    void onTrayActivated(QSystemTrayIcon::ActivationReason reason);
    void onTrayRestore();
    void onTrayExit();

private:
    static QString parseNumber(const QString &uri);
    static QString parseDisplayName(const QString &uri);

    void setupMenu();
    void setupTray();
    void loadSettings();
    void saveSettings();

    SipClient *m_sipClient;

    QSystemTrayIcon *m_trayIcon = nullptr;
    bool m_forceQuit = false;

    QLabel *m_numberLabel;
    QLabel *m_nameLabel;
    QPushButton *m_numpad[12];
    QPushButton *m_callBtn;
    QPushButton *m_hangupBtn;
    QLabel *m_statusLabel;
    QLabel *m_portLabel;

    ScrollHelper *m_scrollHelper;
    QTimer *m_longPressTimer;
    QTimer *m_zeroTimer;

    bool m_inCall = false;
    bool m_incomingWaiting = false;
    bool m_longPressFired = false;

    QString m_server;
    QString m_username;
    QString m_password;
    int m_port = 0;

    CallNotification *m_callNotification = nullptr;
};

#endif // MAINWINDOW_H
