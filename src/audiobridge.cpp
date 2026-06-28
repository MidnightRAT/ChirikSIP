#include "audiobridge.h"
#include "portaudio_manager.h"
#include <QDebug>
#include <cstring>
#include <portaudio.h>

static const int SAMPLE_RATE = 8000;
static const int CHANNELS = 1;
static const int FRAMES_PER_BUFFER = 160;

AudioBridge::AudioBridge(QObject *parent)
    : QObject(parent)
{
}

AudioBridge::~AudioBridge()
{
    close();
}

bool AudioBridge::open()
{
    PortAudioManager::initialize();

    PaStreamParameters inputParams;
    inputParams.device = Pa_GetDefaultInputDevice();
    if (inputParams.device == paNoDevice) {
        qCritical() << "No default input audio device";
        PortAudioManager::terminate();
        return false;
    }
    const PaDeviceInfo *inputDevInfo = Pa_GetDeviceInfo(inputParams.device);
    if (!inputDevInfo) {
        qCritical() << "Failed to get input device info";
        PortAudioManager::terminate();
        return false;
    }
    inputParams.channelCount = CHANNELS;
    inputParams.sampleFormat = paFloat32;
    inputParams.suggestedLatency = inputDevInfo->defaultLowInputLatency;
    inputParams.hostApiSpecificStreamInfo = nullptr;

    PaStreamParameters outputParams;
    outputParams.device = Pa_GetDefaultOutputDevice();
    if (outputParams.device == paNoDevice) {
        qCritical() << "No default output audio device";
        PortAudioManager::terminate();
        return false;
    }
    const PaDeviceInfo *outputDevInfo = Pa_GetDeviceInfo(outputParams.device);
    if (!outputDevInfo) {
        qCritical() << "Failed to get output device info";
        PortAudioManager::terminate();
        return false;
    }
    outputParams.channelCount = CHANNELS;
    outputParams.sampleFormat = paFloat32;
    outputParams.suggestedLatency = outputDevInfo->defaultLowOutputLatency;
    outputParams.hostApiSpecificStreamInfo = nullptr;

    PaError err = Pa_OpenStream(&m_stream,
                        &inputParams, &outputParams,
                        SAMPLE_RATE, FRAMES_PER_BUFFER,
                        paClipOff, paCallback, this);
    if (err != paNoError) {
        qCritical() << "Pa_OpenStream failed:" << Pa_GetErrorText(err);
        PortAudioManager::terminate();
        return false;
    }

    err = Pa_StartStream(m_stream);
    if (err != paNoError) {
        qCritical() << "Pa_StartStream failed:" << Pa_GetErrorText(err);
        Pa_CloseStream(m_stream);
        PortAudioManager::terminate();
        m_stream = nullptr;
        return false;
    }

    m_pool = pjsua_pool_create("aubridge", 512, 512);
    if (!m_pool) {
        qCritical() << "Failed to create pool for audio bridge";
        Pa_StopStream(m_stream);
        Pa_CloseStream(m_stream);
        PortAudioManager::terminate();
        m_stream = nullptr;
        return false;
    }

    m_port = (pjmedia_port *)pj_pool_zalloc(m_pool, sizeof(pjmedia_port));
    if (!m_port) {
        qCritical() << "Failed to allocate port";
        pj_pool_release(m_pool);
        m_pool = nullptr;
        Pa_StopStream(m_stream);
        Pa_CloseStream(m_stream);
        PortAudioManager::terminate();
        m_stream = nullptr;
        return false;
    }

    pj_str_t name = pj_str(const_cast<char*>("portaudio"));
    pjmedia_port_info_init(&m_port->info, &name, 0x41554442,
                           SAMPLE_RATE, CHANNELS, 16, FRAMES_PER_BUFFER);
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
        PortAudioManager::terminate();
        m_stream = nullptr;
        return false;
    }

    qInfo() << "Audio bridge opened, conf slot:" << m_confSlot;
    return true;
}

void AudioBridge::close()
{
    if (m_confSlot != PJSUA_INVALID_ID) {
        pjsua_conf_remove_port(m_confSlot);
        m_confSlot = PJSUA_INVALID_ID;
    }

    if (m_stream) {
        Pa_StopStream(m_stream);
        Pa_CloseStream(m_stream);
        m_stream = nullptr;
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
    const float *in = static_cast<const float *>(input);
    float *out = static_cast<float *>(output);

    if (frameCount > FRAMES_PER_BUFFER)
        frameCount = FRAMES_PER_BUFFER;

    if (input && self->m_captureReady.load(std::memory_order_relaxed)) {
        std::memcpy(self->m_captureBuffer, in,
                     frameCount * sizeof(float));
    }

    if (output) {
        if (self->m_playbackReady.load(std::memory_order_relaxed)) {
            std::memcpy(out, self->m_playbackBuffer,
                         frameCount * sizeof(float));
            self->m_playbackReady.store(false, std::memory_order_relaxed);
        } else {
            std::memset(out, 0, frameCount * sizeof(float));
        }
    }

    self->m_captureReady.store(input != nullptr, std::memory_order_relaxed);
    return paContinue;
}

pj_status_t AudioBridge::putFrame(pjmedia_port *port, pjmedia_frame *frame)
{
    AudioBridge *self = static_cast<AudioBridge *>(port->port_data.pdata);
    if (!self || !self->m_stream)
        return PJ_EINVAL;

    const pj_int16_t *samples = (const pj_int16_t *)frame->buf;
    unsigned count = frame->size / sizeof(pj_int16_t);
    if (count > FRAMES_PER_BUFFER)
        count = FRAMES_PER_BUFFER;

    for (unsigned i = 0; i < count; ++i)
        self->m_playbackBuffer[i] = samples[i] / 32768.0f;

    self->m_playbackReady.store(true, std::memory_order_release);
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
    if (count > FRAMES_PER_BUFFER)
        count = FRAMES_PER_BUFFER;

    if (self->m_captureReady.load(std::memory_order_acquire)) {
        for (unsigned i = 0; i < count; ++i) {
            float s = self->m_captureBuffer[i] * 32767.0f;
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
