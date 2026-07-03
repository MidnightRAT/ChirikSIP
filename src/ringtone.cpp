#ifdef _MSC_VER
#define _USE_MATH_DEFINES
#endif
#include "ringtone.h"
#include "audiodevicemanager.h"
#include <QDebug>
#include <QAudioFormat>
#include <QTimer>
#include <cmath>
#include <cstring>

static const int RING_RATE = 8000;
static const int RING_CHANNELS = 1;
static const double RING_FREQ = 440.0;
static const int RING_ON_MS = 1000;
static const int RING_OFF_MS = 3000;
static const int RING_BUFFER_SIZE = 160;

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
    if (m_sink)
        return;

    QAudioFormat format;
    format.setSampleRate(RING_RATE);
    format.setChannelCount(RING_CHANNELS);
    format.setSampleFormat(QAudioFormat::Float);

    QAudioDevice outputDev = AudioDeviceManager::instance().currentOutputDevice();
    m_sink = new QAudioSink(outputDev, format, this);
    m_device = m_sink->start();

    if (!m_device) {
        qWarning() << "Failed to start ringtone audio device";
        delete m_sink;
        m_sink = nullptr;
        return;
    }

    m_playing = true;
    m_phase = 0;
    onAudioDataReady();
}

void Ringtone::stop()
{
    m_playing = false;

    if (m_sink) {
        m_sink->stop();
        delete m_sink;
        m_sink = nullptr;
        m_device = nullptr;
    }

    m_phase = 0;
}

void Ringtone::onAudioDataReady()
{
    if (!m_playing || !m_device)
        return;

    unsigned onSamples = (RING_ON_MS * RING_RATE) / 1000;
    unsigned offSamples = (RING_OFF_MS * RING_RATE) / 1000;
    unsigned cycleLen = onSamples + offSamples;

    float buffer[RING_BUFFER_SIZE];
    for (unsigned i = 0; i < RING_BUFFER_SIZE; ++i) {
        unsigned pos = (m_phase + i) % cycleLen;
        if (pos < onSamples) {
            double t = (double)pos / RING_RATE;
            buffer[i] = 0.3f * (float)sin(2.0 * M_PI * RING_FREQ * t);
        } else {
            buffer[i] = 0.0f;
        }
    }

    m_phase = (m_phase + RING_BUFFER_SIZE) % cycleLen;

    QByteArray data(reinterpret_cast<const char*>(buffer), sizeof(buffer));
    m_device->write(data);

    if (m_playing) {
        QTimer::singleShot(20, this, &Ringtone::onAudioDataReady);
    }
}
