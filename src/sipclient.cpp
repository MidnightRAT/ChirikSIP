#include "sipclient.h"
#include "audiobridge.h"
#include "ringtone.h"
#include <QDebug>

SipClient::SipClient(QObject *parent)
    : QObject(parent)
{
}

SipClient::~SipClient()
{
    shutdown();
}

bool SipClient::init()
{
    if (m_initialized)
        return true;

    pj_status_t status;

    status = pj_init();
    if (status != PJ_SUCCESS) {
        qCritical() << "pj_init failed:" << status;
        return false;
    }

    pjsua_config cfg;
    pjsua_config_default(&cfg);
    cfg.cb.on_reg_state2 = &SipClient::onRegState2;
    cfg.cb.on_call_state = &SipClient::onCallState;
    cfg.cb.on_call_media_state = &SipClient::onCallMediaState;
    cfg.cb.on_incoming_call = &SipClient::onIncomingCall;
    cfg.user_agent = pj_str(const_cast<char*>("ChirikSIP/1.0"));

    pjsua_logging_config logCfg;
    pjsua_logging_config_default(&logCfg);
    logCfg.console_level = 4;

    status = pjsua_create();
    if (status != PJ_SUCCESS) {
        qCritical() << "pjsua_create failed:" << status;
        return false;
    }

    pjsua_media_config mediaCfg;
    pjsua_media_config_default(&mediaCfg);
    mediaCfg.has_ioqueue = PJ_TRUE;
    mediaCfg.clock_rate = 8000;
    mediaCfg.channel_count = 1;
    mediaCfg.audio_frame_ptime = 20;

    status = pjsua_init(&cfg, &logCfg, &mediaCfg);
    if (status != PJ_SUCCESS) {
        qCritical() << "pjsua_init failed:" << status;
        pjsua_destroy();
        return false;
    }

    pjsua_transport_config transportCfg;
    pjsua_transport_config_default(&transportCfg);
    transportCfg.port = 5060;

    status = pjsua_transport_create(PJSIP_TRANSPORT_UDP, &transportCfg, nullptr);
    if (status != PJ_SUCCESS) {
        qCritical() << "Transport create failed:" << status;
        pjsua_destroy();
        return false;
    }

    status = pjsua_start();
    if (status != PJ_SUCCESS) {
        qCritical() << "pjsua_start failed:" << status;
        pjsua_destroy();
        return false;
    }

    unsigned codecCount = 32;
    pjsua_codec_info codecInfo[32];
    pjsua_enum_codecs(codecInfo, &codecCount);
    for (unsigned i = 0; i < codecCount; ++i) {
        QString id = QString::fromUtf8(codecInfo[i].codec_id.ptr, codecInfo[i].codec_id.slen).toUpper();
        if (id.startsWith("PCMA") || id.startsWith("PCMU")) {
            pjsua_codec_set_priority(&codecInfo[i].codec_id, 200);
            qInfo() << "Codec enabled:" << id;
        } else {
            pjsua_codec_set_priority(&codecInfo[i].codec_id, 0);
        }
    }

    pjsua_set_null_snd_dev();

    m_initialized = true;
    qInfo() << "SIP client initialized";
    return true;
}

void SipClient::shutdown()
{
    if (!m_initialized)
        return;

    if (m_audioBridge) {
        m_audioBridge->close();
        delete m_audioBridge;
        m_audioBridge = nullptr;
    }

    if (m_ringtone) {
        m_ringtone->stop();
        delete m_ringtone;
        m_ringtone = nullptr;
    }

    pjsua_destroy();
    pj_shutdown();
    m_initialized = false;
    qInfo() << "SIP client shut down";
}

bool SipClient::registerAccount(const QString &server,
                                 const QString &username,
                                 const QString &password)
{
    if (!m_initialized) {
        qWarning() << "Not initialized";
        return false;
    }

    QByteArray serverBa = server.toUtf8();
    QByteArray userBa = username.toUtf8();
    QByteArray passBa = password.toUtf8();

    pjsua_acc_config cfg;
    pjsua_acc_config_default(&cfg);

    cfg.user_data = this;

    QByteArray idUri = QString("sip:%1@%2").arg(username, server).toUtf8();
    cfg.id = pj_str(idUri.data());

    QByteArray regUri = QString("sip:%1").arg(server).toUtf8();
    cfg.reg_uri = pj_str(regUri.data());

    cfg.cred_count = 1;
    cfg.cred_info[0].realm = pj_str(const_cast<char*>("*"));
    cfg.cred_info[0].scheme = pj_str(const_cast<char*>("digest"));
    cfg.cred_info[0].username = pj_str(userBa.data());
    cfg.cred_info[0].data_type = PJSIP_CRED_DATA_PLAIN_PASSWD;
    cfg.cred_info[0].data = pj_str(passBa.data());

    pj_status_t status = pjsua_acc_add(&cfg, PJ_TRUE, &m_accId);
    if (status != PJ_SUCCESS) {
        qCritical() << "Account add failed:" << status;
        emit registrationStatus(false, QString("Registration failed: %1").arg(status));
        return false;
    }

    m_server = server;
    qInfo() << "Registration requested for" << username << "@" << server;
    return true;
}

bool SipClient::makeCall(const QString &uri)
{
    if (!m_initialized || m_accId == PJSUA_INVALID_ID) {
        qWarning() << "Cannot make call: not registered";
        return false;
    }

    QString callUri = uri;
    if (!callUri.contains('@')) {
        if (m_server.isEmpty()) {
            qWarning() << "No server configured";
            return false;
        }
        if (callUri.startsWith("sip:", Qt::CaseInsensitive))
            callUri = callUri.mid(4);
        callUri = QString("sip:%1@%2").arg(callUri, m_server);
    }

    QByteArray uriBa = callUri.toUtf8();
    pjsua_call_id callId;
    pj_str_t dstUri = pj_str(uriBa.data());

    pjsua_call_setting callCfg;
    pjsua_call_setting_default(&callCfg);

    pj_status_t status = pjsua_call_make_call(m_accId, &dstUri, &callCfg, nullptr, nullptr, &callId);
    if (status != PJ_SUCCESS) {
        qCritical() << "Make call failed:" << status;
        return false;
    }

    qInfo() << "Calling" << uri << "callId:" << callId;
    return true;
}

bool SipClient::answerCall()
{
    if (m_incomingCallId == PJSUA_INVALID_ID) {
        qWarning() << "No incoming call to answer";
        return false;
    }

    pjsua_call_setting callCfg;
    pjsua_call_setting_default(&callCfg);

    pj_status_t status = pjsua_call_answer2(m_incomingCallId, &callCfg, 200, nullptr, nullptr);
    if (status != PJ_SUCCESS) {
        qCritical() << "Answer call failed:" << status;
        return false;
    }

    qInfo() << "Answering call" << m_incomingCallId;
    if (m_ringtone) {
        m_ringtone->stop();
    }
    m_incomingCallId = PJSUA_INVALID_ID;
    return true;
}

void SipClient::hangup()
{
    if (!m_initialized)
        return;

    if (m_ringtone) {
        m_ringtone->stop();
    }

    pjsua_call_id calls[64];
    unsigned count = 64;
    pjsua_enum_calls(calls, &count);

    for (unsigned i = 0; i < count; ++i) {
        pjsua_call_hangup(calls[i], 0, nullptr, nullptr);
    }

    qInfo() << "All calls hung up";
}

void SipClient::onRegState2(pjsua_acc_id accId, pjsua_reg_info *info)
{
    if (!info || !info->cbparam) return;

    bool ok = (info->cbparam->code / 100) == 2;
    QString msg = QString("Registration %1 (code %2)")
                      .arg(ok ? "OK" : "failed")
                      .arg(info->cbparam->code);

    qInfo() << msg;

    SipClient *self = reinterpret_cast<SipClient *>(pjsua_acc_get_user_data(accId));
    if (self)
        emit self->registrationStatus(ok, msg);
}

void SipClient::onCallState(pjsua_call_id callId, pjsip_event *e)
{
    Q_UNUSED(e);

    pjsua_call_info ci;
    pj_status_t status = pjsua_call_get_info(callId, &ci);
    if (status != PJ_SUCCESS) return;

    static const char *stateNames[] = {
        "NULL", "CALLING", "INCOMING", "EARLY",
        "CONNECTING", "CONFIRMED", "DISCONNCTD"
    };

    QString state = (ci.state < sizeof(stateNames) / sizeof(stateNames[0]))
                        ? QString(stateNames[ci.state])
                        : QString("UNKNOWN(%1)").arg(ci.state);

    qInfo() << "Call" << callId << "state:" << state;

    SipClient *self = reinterpret_cast<SipClient *>(pjsua_acc_get_user_data(ci.acc_id));
    if (self) {
        emit self->callStateChanged(callId, state);

        if (state == "DISCONNCTD") {
            if (self->m_ringtone) {
                self->m_ringtone->stop();
            }
            if (self->m_audioBridge) {
                qInfo() << "Call ended, closing audio bridge";
                AudioBridge *bridge = self->m_audioBridge;
                self->m_audioBridge = nullptr;
                bridge->close();
                bridge->deleteLater();
            }
        }
    }
}

void SipClient::onCallMediaState(pjsua_call_id callId)
{
    pjsua_call_info ci;
    pj_status_t status = pjsua_call_get_info(callId, &ci);
    if (status != PJ_SUCCESS) return;

    SipClient *self = reinterpret_cast<SipClient *>(pjsua_acc_get_user_data(ci.acc_id));

    if (ci.media_status == PJSUA_CALL_MEDIA_ACTIVE) {
        for (unsigned i = 0; i < ci.media_cnt; ++i) {
            if (ci.media[i].type == PJMEDIA_TYPE_AUDIO) {
                if (self && !self->m_audioBridge) {
                    self->m_audioBridge = new AudioBridge();
                    if (!self->m_audioBridge->open()) {
                        qWarning() << "Audio bridge failed to open";
                        delete self->m_audioBridge;
                        self->m_audioBridge = nullptr;
                    }
                }

                if (self && self->m_audioBridge && self->m_audioBridge->confSlot() != PJSUA_INVALID_ID) {
                    pjsua_conf_connect(ci.conf_slot, self->m_audioBridge->confSlot());
                    pjsua_conf_connect(self->m_audioBridge->confSlot(), ci.conf_slot);
                    qInfo() << "Call" << callId << "audio bridged to PortAudio";
                } else {
                    pjsua_conf_connect(ci.conf_slot, 0);
                    pjsua_conf_connect(0, ci.conf_slot);
                }
            }
        }
        qInfo() << "Call" << callId << "media active";
    }
}

void SipClient::onIncomingCall(pjsua_acc_id accId, pjsua_call_id callId, pjsip_rx_data *rdata)
{
    Q_UNUSED(rdata);

    pjsua_call_info ci;
    pj_status_t status = pjsua_call_get_info(callId, &ci);
    if (status != PJ_SUCCESS) return;

    QString remoteUri = QString::fromUtf8(ci.remote_info.ptr, ci.remote_info.slen);
    qInfo() << "Incoming call" << callId << "from" << remoteUri;

    SipClient *self = reinterpret_cast<SipClient *>(pjsua_acc_get_user_data(accId));
    if (self) {
        self->m_incomingCallId = callId;
        if (!self->m_ringtone) {
            self->m_ringtone = new Ringtone();
        }
        self->m_ringtone->start();
        emit self->incomingCall(callId, remoteUri);
    }
}
