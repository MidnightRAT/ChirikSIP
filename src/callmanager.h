#ifndef CALLMANAGER_H
#define CALLMANAGER_H

#include <QObject>
#include <QString>
#include <QTimer>
#include <pjsua.h>

class SipClient;
class AudioBridge;

class CallManager : public QObject
{
    Q_OBJECT
public:
    explicit CallManager(SipClient *sipClient, QObject *parent = nullptr);
    ~CallManager();

    bool makeCall(const QString &number);
    bool answerCall();
    void hangup();

    void setEchoCancel(bool enabled, int aggressiveness = 1);

    bool isInCall() const { return m_inCall; }
    bool isIncomingWaiting() const { return m_incomingWaiting; }
    int callDuration() const { return m_callDuration; }
    AudioBridge *audioBridge() const { return m_audioBridge; }

signals:
    void callStateChanged(int callId, const QString &state);
    void callMediaActive(int callId, pjsua_conf_port_id confSlot);
    void incomingCall(int callId, const QString &remoteUri);
    void registrationStatus(bool ok, const QString &message);
    void callDurationChanged(int seconds);

private slots:
    void onSipCallStateChanged(int callId, const QString &state);
    void onSipCallMediaActive(int callId, pjsua_conf_port_id confSlot);
    void onSipIncomingCall(int callId, const QString &remoteUri);
    void onSipRegistrationStatus(bool ok, const QString &message);
    void updateCallDuration();

private:
    void setupConnections();
    void createAudioBridge(pjsua_conf_port_id confSlot);
    void destroyAudioBridge();

    SipClient *m_sipClient;
    AudioBridge *m_audioBridge = nullptr;

    bool m_inCall = false;
    bool m_incomingWaiting = false;
    int m_callDuration = 0;
    bool m_echoCancel = true;
    int m_echoAggressiveness = 1;

    QTimer m_durationTimer;
};

#endif // CALLMANAGER_H
