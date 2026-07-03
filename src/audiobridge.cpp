#include "audiobridge.h"
#include "audiodevicemanager.h"
#include <QDebug>
#include <QAudioFormat>
#include <cstring>

static const int SAMPLE_RATE = 8000;
static const int CHANNELS = 1;
static const int EC_TAIL_MS = 200;

static unsigned ecFlagsFromAggressiveness(int level)
{
    unsigned flags = PJMEDIA_ECHO_USE_NOISE_SUPPRESSOR;
    switch (level) {
    case 0:  flags |= PJMEDIA_ECHO_AGGRESSIVENESS_CONSERVATIVE; break;
    case 1:  flags |= PJMEDIA_ECHO_AGGRESSIVENESS_MODERATE; break;
    case 2:  flags |= PJMEDIA_ECHO_AGGRESSIVENESS_AGGRESSIVE; break;
    default: flags |= PJMEDIA_ECHO_AGGRESSIVENESS_MODERATE; break;
    }
    return flags;
}

AudioBridge::AudioBridge(QObject *parent)
    : QObject(parent)
{
    connect(&AudioDeviceManager::instance(), &AudioDeviceManager::inputDeviceChanged,
            this, &AudioBridge::onInputDeviceChanged);
    connect(&AudioDeviceManager::instance(), &AudioDeviceManager::outputDeviceChanged,
            this, &AudioBridge::onOutputDeviceChanged);
}

AudioBridge::~AudioBridge()
{
    if (m_sink || m_source || m_pool)
        close();
}

bool AudioBridge::open(bool echoCancel, int aggressiveness)
{
    m_echoCancel = echoCancel;
    m_echoAggressiveness = aggressiveness;

    startAudio();
    if (!m_source || !m_sink)
        return false;

    m_pool = pjsua_pool_create("aubridge", 512, 512);
    if (!m_pool) {
        qCritical() << "Failed to create pool for audio bridge";
        stopAudio();
        return false;
    }

    if (echoCancel) {
        unsigned flags = ecFlagsFromAggressiveness(aggressiveness);
        pj_status_t ecStatus = pjmedia_echo_create(m_pool, SAMPLE_RATE,
                                                    FRAME_SIZE, EC_TAIL_MS,
                                                    0, flags, &m_echoState);
        if (ecStatus != PJ_SUCCESS) {
            qWarning() << "Echo canceller creation failed:" << ecStatus << "(continuing without AEC)";
            m_echoState = nullptr;
        } else {
            qInfo() << "Echo canceller created, tail:" << EC_TAIL_MS
                     << "ms, aggressiveness:" << aggressiveness;
        }
    }

    m_port = (pjmedia_port *)pj_pool_zalloc(m_pool, sizeof(pjmedia_port));
    if (!m_port) {
        qCritical() << "Failed to allocate port";
        pj_pool_release(m_pool);
        m_pool = nullptr;
        stopAudio();
        return false;
    }

    static const QByteArray portName("qtmultimedia");
    pj_str_t name = pj_str(const_cast<char*>(portName.constData()));
    pjmedia_port_info_init(&m_port->info, &name, 0x41554442,
                           SAMPLE_RATE, CHANNELS, 16, FRAME_SIZE);
    m_port->put_frame = putFrame;
    m_port->get_frame = getFrame;
    m_port->on_destroy = onDestroy;
    m_port->port_data.pdata = this;

    pj_status_t status = pjsua_conf_add_port(m_pool, m_port, &m_confSlot);
    if (status != PJ_SUCCESS) {
        qCritical() << "pjsua_conf_add_port failed:" << status;
        pj_pool_release(m_pool);
        m_pool = nullptr;
        m_port = nullptr;
        stopAudio();
        return false;
    }

    m_playbackRing.reset();
    m_captureRing.reset();
    m_ecRefRing.reset();

    qInfo() << "Audio bridge opened, conf slot:" << m_confSlot;
    return true;
}

void AudioBridge::close()
{
    if (m_confSlot != PJSUA_INVALID_ID) {
        pjsua_conf_remove_port(m_confSlot);
        m_confSlot = PJSUA_INVALID_ID;
    }

    if (m_echoState) {
        pjmedia_echo_destroy(m_echoState);
        m_echoState = nullptr;
    }

    if (m_pool) {
        pj_pool_release(m_pool);
        m_pool = nullptr;
    }

    m_port = nullptr;
    stopAudio();

    qInfo() << "Audio bridge closed";
}

void AudioBridge::startAudio()
{
    AudioDeviceManager &dm = AudioDeviceManager::instance();

    QAudioFormat format;
    format.setSampleRate(SAMPLE_RATE);
    format.setChannelCount(CHANNELS);
    format.setSampleFormat(QAudioFormat::Int16);

    // Try selected input device, fallback to default
    QAudioDevice inputDev = dm.currentInputDevice();
    m_source = new QAudioSource(inputDev, format, this);
    m_sourceDevice = m_source->start();
    if (!m_sourceDevice) {
        qWarning() << "Failed to open input device:" << inputDev.description() << ", trying default";
        delete m_source;
        m_source = new QAudioSource(QMediaDevices::defaultAudioInput(), format, this);
        m_sourceDevice = m_source->start();
    }

    // Try selected output device, fallback to default
    QAudioDevice outputDev = dm.currentOutputDevice();
    m_sink = new QAudioSink(outputDev, format, this);
    m_sinkDevice = m_sink->start();
    if (!m_sinkDevice) {
        qWarning() << "Failed to open output device:" << outputDev.description() << ", trying default";
        delete m_sink;
        m_sink = new QAudioSink(QMediaDevices::defaultAudioOutput(), format, this);
        m_sinkDevice = m_sink->start();
    }

    if (!m_sourceDevice || !m_sinkDevice) {
        qCritical() << "Failed to start audio devices even with defaults";
        stopAudio();
        return;
    }

    qInfo() << "Audio started with input:" << inputDev.description()
            << "output:" << outputDev.description();
}

void AudioBridge::stopAudio()
{
    if (m_source) {
        m_source->stop();
        delete m_source;
        m_source = nullptr;
        m_sourceDevice = nullptr;
    }
    if (m_sink) {
        m_sink->stop();
        delete m_sink;
        m_sink = nullptr;
        m_sinkDevice = nullptr;
    }
}

void AudioBridge::onInputDeviceChanged(const QAudioDevice &device)
{
    Q_UNUSED(device);
    if (m_source) {
        qInfo() << "Input device changed, restarting audio";
        stopAudio();
        startAudio();
    }
}

void AudioBridge::onOutputDeviceChanged(const QAudioDevice &device)
{
    Q_UNUSED(device);
    if (m_sink) {
        qInfo() << "Output device changed, restarting audio";
        stopAudio();
        startAudio();
    }
}

pj_status_t AudioBridge::putFrame(pjmedia_port *port, pjmedia_frame *frame)
{
    AudioBridge *self = static_cast<AudioBridge *>(port->port_data.pdata);
    if (!self || !self->m_sinkDevice)
        return PJ_EINVAL;

    const pj_int16_t *samples = (const pj_int16_t *)frame->buf;
    unsigned count = frame->size / sizeof(pj_int16_t);
    if (count > FRAME_SIZE)
        count = FRAME_SIZE;

    // Convert int16 to float for echo reference
    float ecRefFloat[FRAME_SIZE];
    for (unsigned i = 0; i < count; ++i)
        ecRefFloat[i] = samples[i] / 32768.0f;

    if (self->m_echoState)
        self->m_ecRefRing.push(ecRefFloat, count);

    // Convert int16 to float and write to Qt audio sink
    float playBuf[FRAME_SIZE];
    for (unsigned i = 0; i < count; ++i)
        playBuf[i] = samples[i] / 32768.0f;

    QByteArray pcmData(reinterpret_cast<const char*>(playBuf), count * sizeof(float));
    self->m_sinkDevice->write(pcmData);

    return PJ_SUCCESS;
}

pj_status_t AudioBridge::getFrame(pjmedia_port *port, pjmedia_frame *frame)
{
    AudioBridge *self = static_cast<AudioBridge *>(port->port_data.pdata);
    if (!self || !self->m_sourceDevice) {
        frame->size = 0;
        return PJ_EINVAL;
    }

    pj_int16_t *samples = (pj_int16_t *)frame->buf;
    unsigned count = frame->size / sizeof(pj_int16_t);
    if (count > FRAME_SIZE)
        count = FRAME_SIZE;

    // Read from Qt audio source directly into capture buffer
    QByteArray pcmData = self->m_sourceDevice->read(count * sizeof(float));
    unsigned readCount = pcmData.size() / sizeof(float);
    const float *floatData = reinterpret_cast<const float *>(pcmData.constData());

    float capBuf[FRAME_SIZE];
    if (readCount >= count) {
        for (unsigned i = 0; i < count; ++i)
            capBuf[i] = floatData[i];
    } else {
        std::memset(capBuf, 0, count * sizeof(float));
    }

    // Echo cancellation processing
    if (self->m_echoState) {
        pj_int16_t refI16[FRAME_SIZE];
        pj_int16_t micI16[FRAME_SIZE];

        float refFloat[FRAME_SIZE];
        if (!self->m_ecRefRing.pop(refFloat, count))
            std::memset(refFloat, 0, count * sizeof(float));

        for (unsigned i = 0; i < count; ++i) {
            float s = refFloat[i] * 32767.0f;
            if (s > 32767.0f) s = 32767.0f;
            if (s < -32768.0f) s = -32768.0f;
            refI16[i] = (pj_int16_t)s;
        }
        for (unsigned i = 0; i < count; ++i) {
            float s = capBuf[i] * 32767.0f;
            if (s > 32767.0f) s = 32767.0f;
            if (s < -32768.0f) s = -32768.0f;
            micI16[i] = (pj_int16_t)s;
        }

        pjmedia_echo_playback(self->m_echoState, refI16);
        pjmedia_echo_capture(self->m_echoState, micI16, 0);

        for (unsigned i = 0; i < count; ++i)
            capBuf[i] = micI16[i] / 32768.0f;
    }

    // Convert float to int16 for PJSIP
    for (unsigned i = 0; i < count; ++i) {
        float s = capBuf[i] * 32767.0f;
        if (s > 32767.0f) s = 32767.0f;
        if (s < -32768.0f) s = -32768.0f;
        samples[i] = (pj_int16_t)s;
    }

    return PJ_SUCCESS;
}

pj_status_t AudioBridge::onDestroy(pjmedia_port *port)
{
    Q_UNUSED(port);
    return PJ_SUCCESS;
}
