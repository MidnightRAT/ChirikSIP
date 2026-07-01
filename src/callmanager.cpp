#include "callmanager.h"
#include "sipclient.h"
#include "audiobridge.h"
#include <QDebug>

CallManager::CallManager(SipClient *sipClient, QObject *parent)
    : QObject(parent)
    , m_sipClient(sipClient)
{
    m_durationTimer.setInterval(1000);
    connect(&m_durationTimer, &QTimer::timeout, this, &CallManager::updateCallDuration);
    setupConnections();
}

CallManager::~CallManager()
{
    destroyAudioBridge();
}

void CallManager::setupConnections()
{
    connect(m_sipClient, &SipClient::callStateChanged,
            this, &CallManager::onSipCallStateChanged);
    connect(m_sipClient, &SipClient::callMediaActive,
            this, &CallManager::onSipCallMediaActive);
    connect(m_sipClient, &SipClient::incomingCall,
            this, &CallManager::onSipIncomingCall);
    connect(m_sipClient, &SipClient::registrationStatus,
            this, &CallManager::onSipRegistrationStatus);
}

bool CallManager::makeCall(const QString &number)
{
    if (m_inCall || number.trimmed().isEmpty())
        return false;

    if (!m_sipClient->makeCall(number))
        return false;

    m_inCall = true;
    m_callDuration = 0;
    m_durationTimer.start();
    return true;
}

bool CallManager::answerCall()
{
    if (!m_incomingWaiting)
        return false;

    if (!m_sipClient->answerCall())
        return false;

    m_inCall = true;
    m_incomingWaiting = false;
    m_callDuration = 0;
    m_durationTimer.start();
    return true;
}

void CallManager::hangup()
{
    m_sipClient->hangup();
}

void CallManager::setEchoCancel(bool enabled, int aggressiveness)
{
    m_echoCancel = enabled;
    m_echoAggressiveness = aggressiveness;
}

void CallManager::onSipCallStateChanged(int callId, const QString &state)
{
    if (state == "DISCONNECTED") {
        m_inCall = false;
        m_incomingWaiting = false;
        m_durationTimer.stop();
        destroyAudioBridge();
        m_sipClient->stopRingtone();
    }

    emit callStateChanged(callId, state);
}

void CallManager::onSipCallMediaActive(int callId, pjsua_conf_port_id confSlot)
{
    Q_UNUSED(callId);
    createAudioBridge(confSlot);
}

void CallManager::onSipIncomingCall(int callId, const QString &remoteUri)
{
    m_incomingWaiting = true;
    emit incomingCall(callId, remoteUri);
}

void CallManager::onSipRegistrationStatus(bool ok, const QString &message)
{
    emit registrationStatus(ok, message);
}

void CallManager::updateCallDuration()
{
    if (m_inCall) {
        m_callDuration++;
        emit callDurationChanged(m_callDuration);
    }
}

void CallManager::createAudioBridge(pjsua_conf_port_id confSlot)
{
    if (m_audioBridge)
        return;

    m_audioBridge = new AudioBridge(this);
    if (!m_audioBridge->open(m_echoCancel, m_echoAggressiveness)) {
        qWarning() << "Audio bridge failed to open";
        delete m_audioBridge;
        m_audioBridge = nullptr;
        return;
    }

    if (m_audioBridge->confSlot() != PJSUA_INVALID_ID) {
        pjsua_conf_connect(confSlot, m_audioBridge->confSlot());
        pjsua_conf_connect(m_audioBridge->confSlot(), confSlot);
        qInfo() << "Audio bridged to PortAudio, conf slot:" << m_audioBridge->confSlot();
    } else {
        pjsua_conf_connect(confSlot, 0);
        pjsua_conf_connect(0, confSlot);
    }
}

void CallManager::destroyAudioBridge()
{
    if (m_audioBridge) {
        m_audioBridge->close();
        m_audioBridge->deleteLater();
        m_audioBridge = nullptr;
    }
}
