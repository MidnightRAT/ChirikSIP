#include "sipclient.h"
#include "audiobridge.h"
#include "ringtone.h"
#include "portaudio_manager.h"
#include <QDebug>
#include <QMetaObject>
#include <QRegularExpression>
#include <QThread>

static const unsigned SIP_PORT = 5060;
static const unsigned CLOCK_RATE = 8000;
static const unsigned CHANNEL_COUNT = 1;
static const unsigned AUDIO_FRAME_PTIME = 20;
static const unsigned MAX_CODECS = 32;
static const unsigned CODEC_PRIORITY_ENABLED = 200;
static const unsigned CODEC_PRIORITY_DISABLED = 0;
static const unsigned MAX_CALLS = 64;
static const unsigned MAX_URI_LENGTH = 256;

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
    cfg.user_agent = pj_str(const_cast<char*>("ChirikSIP/" PROJECT_VERSION));

    pjsua_logging_config logCfg;
    pjsua_logging_config_default(&logCfg);
    logCfg.console_level = 4;

    status = pjsua_create();
    if (status != PJ_SUCCESS) {
        qCritical() << "pjsua_create failed:" << status;
        pj_shutdown();
        return false;
    }

    pjsua_media_config mediaCfg;
    pjsua_media_config_default(&mediaCfg);
    mediaCfg.has_ioqueue = PJ_TRUE;
    mediaCfg.clock_rate = CLOCK_RATE;
    mediaCfg.channel_count = CHANNEL_COUNT;
    mediaCfg.audio_frame_ptime = AUDIO_FRAME_PTIME;

    status = pjsua_init(&cfg, &logCfg, &mediaCfg);
    if (status != PJ_SUCCESS) {
        qCritical() << "pjsua_init failed:" << status;
        pjsua_destroy();
        return false;
    }

    pjsua_transport_config transportCfg;
    pjsua_transport_config_default(&transportCfg);
    transportCfg.port = SIP_PORT;

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

    unsigned codecCount = MAX_CODECS;
    pjsua_codec_info codecInfo[MAX_CODECS];
    pjsua_enum_codecs(codecInfo, &codecCount);
    for (unsigned i = 0; i < codecCount; ++i) {
        QString id = QString::fromUtf8(codecInfo[i].codec_id.ptr, codecInfo[i].codec_id.slen).toUpper();
        if (id.startsWith("PCMA") || id.startsWith("PCMU")) {
            pjsua_codec_set_priority(&codecInfo[i].codec_id, CODEC_PRIORITY_ENABLED);
            qInfo() << "Codec enabled:" << id;
        } else {
            pjsua_codec_set_priority(&codecInfo[i].codec_id, CODEC_PRIORITY_DISABLED);
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

    QThread::msleep(50);
    PortAudioManager::terminate();
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

    if (server.trimmed().isEmpty() || username.trimmed().isEmpty()) {
        qWarning() << "Server and username must not be empty";
        return false;
    }

    if (m_accId != PJSUA_INVALID_ID) {
        pjsua_acc_del(m_accId);
        m_accId = PJSUA_INVALID_ID;
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

    if (uri.trimmed().isEmpty() || uri.length() > MAX_URI_LENGTH) {
        qWarning() << "Invalid call URI: empty or too long";
        return false;
    }

    static const QRegularExpression validChars(QStringLiteral("^[+0-9*#a-zA-Z@.:;?!&=\\-_/~ ]+$"));
    if (!validChars.match(uri).hasMatch()) {
        qWarning() << "Invalid characters in call URI";
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

    pjsua_call_id calls[MAX_CALLS];
    unsigned count = MAX_CALLS;
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

    SipClient *self = static_cast<SipClient *>(pjsua_acc_get_user_data(accId));
    if (self) {
        QMetaObject::invokeMethod(self, [self, ok, msg]() {
            emit self->registrationStatus(ok, msg);
        }, Qt::QueuedConnection);
    }
}

void SipClient::onCallState(pjsua_call_id callId, pjsip_event *e)
{
    Q_UNUSED(e);

    pjsua_call_info ci;
    pj_status_t status = pjsua_call_get_info(callId, &ci);
    if (status != PJ_SUCCESS) return;

    static const char *stateNames[] = {
        "NULL", "CALLING", "INCOMING", "EARLY",
        "CONNECTING", "CONFIRMED", "DISCONNECTED"
    };

    QString state = (ci.state < sizeof(stateNames) / sizeof(stateNames[0]))
                        ? QString(stateNames[ci.state])
                        : QString("UNKNOWN(%1)").arg(ci.state);

    qInfo() << "Call" << callId << "state:" << state;

    if (ci.acc_id == PJSUA_INVALID_ID) return;

    SipClient *self = static_cast<SipClient *>(pjsua_acc_get_user_data(ci.acc_id));
    if (self) {
        QMetaObject::invokeMethod(self, [self, callId, state]() {
            emit self->callStateChanged(callId, state);
        }, Qt::QueuedConnection);

        if (state == "DISCONNECTED") {
            QMetaObject::invokeMethod(self, [self, callId]() {
                if (self->m_ringtone) {
                    self->m_ringtone->stop();
                }
                if (callId == self->m_incomingCallId) {
                    self->m_incomingCallId = PJSUA_INVALID_ID;
                }
                if (self->m_audioBridge) {
                    qInfo() << "Call ended, closing audio bridge";
                    AudioBridge *bridge = self->m_audioBridge;
                    self->m_audioBridge = nullptr;
                    bridge->close();
                    bridge->deleteLater();
                }
            }, Qt::QueuedConnection);
        }
    }
}

void SipClient::onCallMediaState(pjsua_call_id callId)
{
    pjsua_call_info ci;
    pj_status_t status = pjsua_call_get_info(callId, &ci);
    if (status != PJ_SUCCESS) return;

    if (ci.acc_id == PJSUA_INVALID_ID) return;

    SipClient *self = static_cast<SipClient *>(pjsua_acc_get_user_data(ci.acc_id));
    if (!self) return;

    if (ci.state == PJSIP_INV_STATE_DISCONNECTED)
        return;

    if (ci.media_status == PJSUA_CALL_MEDIA_ACTIVE) {
        for (unsigned i = 0; i < ci.media_cnt; ++i) {
            if (ci.media[i].type == PJMEDIA_TYPE_AUDIO) {
                pjsua_conf_port_id confSlot = ci.conf_slot;

                QMetaObject::invokeMethod(self, [self, callId, confSlot]() {
                    if (!self->m_audioBridge) {
                        self->m_audioBridge = new AudioBridge();
                        if (!self->m_audioBridge->open()) {
                            qWarning() << "Audio bridge failed to open";
                            delete self->m_audioBridge;
                            self->m_audioBridge = nullptr;
                        }
                    }

                    if (self->m_audioBridge && self->m_audioBridge->confSlot() != PJSUA_INVALID_ID) {
                        pjsua_conf_connect(confSlot, self->m_audioBridge->confSlot());
                        pjsua_conf_connect(self->m_audioBridge->confSlot(), confSlot);
                        qInfo() << "Call" << callId << "audio bridged to PortAudio";
                    } else {
                        pjsua_conf_connect(confSlot, 0);
                        pjsua_conf_connect(0, confSlot);
                    }
                }, Qt::QueuedConnection);
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

    SipClient *self = static_cast<SipClient *>(pjsua_acc_get_user_data(accId));
    if (self) {
        QMetaObject::invokeMethod(self, [self, callId, remoteUri]() {
            self->m_incomingCallId = callId;
            if (!self->m_ringtone) {
                self->m_ringtone = new Ringtone();
            }
            self->m_ringtone->start();
            emit self->incomingCall(callId, remoteUri);
        }, Qt::QueuedConnection);
    }
}
