#ifdef _MSC_VER
#define _USE_MATH_DEFINES
#endif
#include "ringtone.h"
#include "portaudio_manager.h"
#include <QDebug>
#include <cmath>
#include <cstring>

static const int RING_RATE = 8000;
static const int RING_CHANNELS = 1;
static const double RING_FREQ = 440.0;
static const int RING_ON_MS = 1000;
static const int RING_OFF_MS = 3000;

Ringtone::Ringtone(QObject *parent)
    : QObject(parent)
{
}

Ringtone::~Ringtone()
{
    stop();
}

void Ringtone::start()
{
    if (m_stream)
        return;

    PortAudioManager::initialize();

    PaDeviceIndex dev = Pa_GetDefaultOutputDevice();
    if (dev == paNoDevice) {
        qWarning() << "No audio output device found for ringtone";
        return;
    }

    PaStreamParameters outputParams;
    outputParams.device = dev;
    outputParams.channelCount = RING_CHANNELS;
    outputParams.sampleFormat = paFloat32;
    outputParams.suggestedLatency =
        Pa_GetDeviceInfo(dev)->defaultLowOutputLatency;
    outputParams.hostApiSpecificStreamInfo = nullptr;

    PaError err = Pa_OpenStream(&m_stream,
                        nullptr, &outputParams,
                        RING_RATE, 160,
                        paClipOff, paCallback, this);
    if (err != paNoError) {
        qWarning() << "Ringtone Pa_OpenStream failed:" << Pa_GetErrorText(err);
        return;
    }

    err = Pa_StartStream(m_stream);
    if (err != paNoError) {
        qWarning() << "Ringtone Pa_StartStream failed:" << Pa_GetErrorText(err);
        Pa_CloseStream(m_stream);
        m_stream = nullptr;
        return;
    }

    m_playing = true;
    m_phase = 0;
}

void Ringtone::stop()
{
    m_playing = false;

    if (m_stream) {
        Pa_StopStream(m_stream);
        Pa_CloseStream(m_stream);
        m_stream = nullptr;
    }

    m_phase = 0;
}

int Ringtone::paCallback(const void *input, void *output,
                         unsigned long frameCount,
                         const PaStreamCallbackTimeInfo *timeInfo,
                         PaStreamCallbackFlags statusFlags,
                         void *userData)
{
    Q_UNUSED(input);
    Q_UNUSED(timeInfo);
    Q_UNUSED(statusFlags);

    Ringtone *self = static_cast<Ringtone *>(userData);
    float *out = static_cast<float *>(output);

    if (!self->m_playing) {
        std::memset(out, 0, frameCount * sizeof(float));
        return paComplete;
    }

    unsigned onSamples = (RING_ON_MS * RING_RATE) / 1000;
    unsigned offSamples = (RING_OFF_MS * RING_RATE) / 1000;
    unsigned cycleLen = onSamples + offSamples;

    for (unsigned i = 0; i < frameCount; ++i) {
        unsigned pos = (self->m_phase + i) % cycleLen;
        if (pos < onSamples) {
            double t = (double)pos / RING_RATE;
            out[i] = 0.3f * (float)sin(2.0 * M_PI * RING_FREQ * t);
        } else {
            out[i] = 0.0f;
        }
    }

    self->m_phase = (self->m_phase + frameCount) % cycleLen;
    return paContinue;
}
