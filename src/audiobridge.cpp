#include "audiobridge.h"
#include "portaudio_manager.h"
#include <QDebug>
#include <cstring>
#include <portaudio.h>

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
}

AudioBridge::~AudioBridge()
{
    if (m_stream || m_pool)
        close();
}

bool AudioBridge::open(bool echoCancel, int aggressiveness)
{
    PortAudioManager::initialize();

    PaStreamParameters inputParams;
    inputParams.device = Pa_GetDefaultInputDevice();
    if (inputParams.device == paNoDevice) {
        qCritical() << "No default input audio device";
        return false;
    }
    const PaDeviceInfo *inputDevInfo = Pa_GetDeviceInfo(inputParams.device);
    if (!inputDevInfo) {
        qCritical() << "Failed to get input device info";
        return false;
    }
    inputParams.channelCount = CHANNELS;
    inputParams.sampleFormat = paFloat32;
    inputParams.suggestedLatency = inputDevInfo->defaultLowInputLatency * 4;
    inputParams.hostApiSpecificStreamInfo = nullptr;

    PaStreamParameters outputParams;
    outputParams.device = Pa_GetDefaultOutputDevice();
    if (outputParams.device == paNoDevice) {
        qCritical() << "No default output audio device";
        return false;
    }
    const PaDeviceInfo *outputDevInfo = Pa_GetDeviceInfo(outputParams.device);
    if (!outputDevInfo) {
        qCritical() << "Failed to get output device info";
        return false;
    }
    outputParams.channelCount = CHANNELS;
    outputParams.sampleFormat = paFloat32;
    outputParams.suggestedLatency = outputDevInfo->defaultLowOutputLatency * 4;
    outputParams.hostApiSpecificStreamInfo = nullptr;

    PaError err = Pa_OpenStream(&m_stream,
                        &inputParams, &outputParams,
                        SAMPLE_RATE, FRAME_SIZE,
                        paClipOff, paCallback, this);
    if (err != paNoError) {
        qCritical() << "Pa_OpenStream failed:" << Pa_GetErrorText(err);
        return false;
    }

    err = Pa_StartStream(m_stream);
    if (err != paNoError) {
        qCritical() << "Pa_StartStream failed:" << Pa_GetErrorText(err);
        Pa_CloseStream(m_stream);
        m_stream = nullptr;
        return false;
    }

    m_pool = pjsua_pool_create("aubridge", 512, 512);
    if (!m_pool) {
        qCritical() << "Failed to create pool for audio bridge";
        Pa_StopStream(m_stream);
        Pa_CloseStream(m_stream);
        m_stream = nullptr;
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
        Pa_StopStream(m_stream);
        Pa_CloseStream(m_stream);
        m_stream = nullptr;
        PortAudioManager::terminate();
        return false;
    }

    static const QByteArray portName("portaudio");
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
        Pa_StopStream(m_stream);
        Pa_CloseStream(m_stream);
        m_stream = nullptr;
        PortAudioManager::terminate();
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
    if (m_stream) {
        Pa_StopStream(m_stream);
        Pa_CloseStream(m_stream);
        m_stream = nullptr;
    }

    if (m_echoState) {
        pjmedia_echo_destroy(m_echoState);
        m_echoState = nullptr;
    }

    if (m_confSlot != PJSUA_INVALID_ID) {
        pjsua_conf_remove_port(m_confSlot);
        m_confSlot = PJSUA_INVALID_ID;
    }

    if (m_pool) {
        pj_pool_release(m_pool);
        m_pool = nullptr;
    }

    m_port = nullptr;

    qInfo() << "Audio bridge closed";
}

int AudioBridge::paCallback(const void *input, void *output,
                            unsigned long frameCount,
                            const PaStreamCallbackTimeInfo *timeInfo,
                            PaStreamCallbackFlags statusFlags,
                            void *userData)
{
    Q_UNUSED(timeInfo);
    Q_UNUSED(statusFlags);

    AudioBridge *self = static_cast<AudioBridge *>(userData);
    if (!self || !output)
        return paAbort;

    const float *in = static_cast<const float *>(input);
    float *out = static_cast<float *>(output);

    if (frameCount > FRAME_SIZE)
        frameCount = FRAME_SIZE;

    if (input) {
        float capBuf[FRAME_SIZE];
        std::memcpy(capBuf, in, frameCount * sizeof(float));

        if (self->m_echoState) {
            pj_int16_t refI16[FRAME_SIZE];
            pj_int16_t micI16[FRAME_SIZE];
            unsigned refCount = frameCount;

            float refFloat[FRAME_SIZE];
            if (!self->m_ecRefRing.pop(refFloat, refCount))
                std::memset(refFloat, 0, refCount * sizeof(float));

            for (unsigned i = 0; i < refCount; ++i) {
                float s = refFloat[i] * 32767.0f;
                if (s > 32767.0f) s = 32767.0f;
                if (s < -32768.0f) s = -32768.0f;
                refI16[i] = (pj_int16_t)s;
            }
            for (unsigned i = 0; i < frameCount; ++i) {
                float s = capBuf[i] * 32767.0f;
                if (s > 32767.0f) s = 32767.0f;
                if (s < -32768.0f) s = -32768.0f;
                micI16[i] = (pj_int16_t)s;
            }

            pjmedia_echo_playback(self->m_echoState, refI16);
            pjmedia_echo_capture(self->m_echoState, micI16, 0);

            for (unsigned i = 0; i < frameCount; ++i)
                capBuf[i] = micI16[i] / 32768.0f;
        }

        self->m_captureRing.push(capBuf, frameCount);
    }

    if (!self->m_playbackRing.pop(out, frameCount))
        std::memset(out, 0, frameCount * sizeof(float));

    return paContinue;
}

pj_status_t AudioBridge::putFrame(pjmedia_port *port, pjmedia_frame *frame)
{
    AudioBridge *self = static_cast<AudioBridge *>(port->port_data.pdata);
    if (!self || !self->m_stream)
        return PJ_EINVAL;

    const pj_int16_t *samples = (const pj_int16_t *)frame->buf;
    unsigned count = frame->size / sizeof(pj_int16_t);
    if (count > FRAME_SIZE)
        count = FRAME_SIZE;

    float playBuf[FRAME_SIZE];
    float ecRefFloat[FRAME_SIZE];
    for (unsigned i = 0; i < count; ++i) {
        playBuf[i] = samples[i] / 32768.0f;
        ecRefFloat[i] = samples[i] / 32768.0f;
    }

    if (self->m_echoState)
        self->m_ecRefRing.push(ecRefFloat, count);

    self->m_playbackRing.push(playBuf, count);
    return PJ_SUCCESS;
}

pj_status_t AudioBridge::getFrame(pjmedia_port *port, pjmedia_frame *frame)
{
    AudioBridge *self = static_cast<AudioBridge *>(port->port_data.pdata);
    if (!self || !self->m_stream) {
        frame->size = 0;
        return PJ_EINVAL;
    }

    pj_int16_t *samples = (pj_int16_t *)frame->buf;
    unsigned count = frame->size / sizeof(pj_int16_t);
    if (count > FRAME_SIZE)
        count = FRAME_SIZE;

    float capBuf[FRAME_SIZE];
    if (self->m_captureRing.pop(capBuf, count)) {
        for (unsigned i = 0; i < count; ++i) {
            float s = capBuf[i] * 32767.0f;
            if (s > 32767.0f) s = 32767.0f;
            if (s < -32768.0f) s = -32768.0f;
            samples[i] = (pj_int16_t)s;
        }
    } else {
        std::memset(samples, 0, count * sizeof(pj_int16_t));
    }

    return PJ_SUCCESS;
}

pj_status_t AudioBridge::onDestroy(pjmedia_port *port)
{
    Q_UNUSED(port);
    return PJ_SUCCESS;
}
