#ifndef SIPCLIENT_H
#define SIPCLIENT_H

#include <QObject>
#include <QString>
#include <atomic>
#include <pjlib.h>
#include <pjmedia.h>
#include <pjmedia-codec.h>
#include <pjsua.h>

class AudioBridge;
class Ringtone;

class SipClient : public QObject
{
    Q_OBJECT
public:
    explicit SipClient(QObject *parent = nullptr);
    ~SipClient();

    bool init(int port = 0);
    void shutdown();
    int boundPort() const;

    bool registerAccount(const QString &server,
                         const QString &username,
                         const QString &password,
                         int port = 0);
    bool makeCall(const QString &uri);
    bool answerCall();
    void hangup();
    void hangupCall(pjsua_call_id callId);

signals:
    void registrationStatus(bool ok, const QString &message);
    void callStateChanged(int callId, const QString &state);
    void callMediaActive(int callId, pjsua_conf_port_id confSlot);
    void incomingCall(int callId, const QString &remoteUri);

public slots:
    void stopRingtone();

private:
    static void onRegState2(pjsua_acc_id accId, pjsua_reg_info *info);
    static void onCallState(pjsua_call_id callId, pjsip_event *e);
    static void onCallMediaState(pjsua_call_id callId);
    static void onIncomingCall(pjsua_acc_id accId, pjsua_call_id callId, pjsip_rx_data *rdata);

    pjsua_acc_id m_accId = PJSUA_INVALID_ID;
    bool m_initialized = false;
    QString m_server;
    int m_port = 0;
    int m_boundPort = 0;
    Ringtone *m_ringtone = nullptr;
    std::atomic<pjsua_call_id> m_incomingCallId{PJSUA_INVALID_ID};
    std::atomic<pjsua_call_id> m_activeCallId{PJSUA_INVALID_ID};
};

#endif // SIPCLIENT_H
