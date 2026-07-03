#ifndef AUDIOBRIDGE_H
#define AUDIOBRIDGE_H

#include <QObject>
#include <QAudioSink>
#include <QAudioSource>
#include <QAudioDevice>
#include <QIODevice>
#include <atomic>
#include <pjmedia.h>
#include <pjmedia/echo.h>
#include <pjsua.h>

static const int FRAME_SIZE = 160;
static const int RING_CAPACITY = 16;

class SpscRingBuffer
{
public:
    void reset() { m_readPos.store(0, std::memory_order_relaxed); m_writePos.store(0, std::memory_order_relaxed); }

    bool push(const float *data, unsigned count)
    {
        unsigned wp = m_writePos.load(std::memory_order_relaxed);
        unsigned rp = m_readPos.load(std::memory_order_acquire);
        unsigned avail = capacity() - (wp - rp);
        if (count > avail) return false;
        for (unsigned i = 0; i < count; ++i)
            m_buffer[(wp + i) % capacity()] = data[i];
        m_writePos.store(wp + count, std::memory_order_release);
        return true;
    }

    bool pop(float *data, unsigned count)
    {
        unsigned rp = m_readPos.load(std::memory_order_relaxed);
        unsigned wp = m_writePos.load(std::memory_order_acquire);
        unsigned avail = wp - rp;
        if (count > avail) return false;
        for (unsigned i = 0; i < count; ++i)
            data[i] = m_buffer[(rp + i) % capacity()];
        m_readPos.store(rp + count, std::memory_order_release);
        return true;
    }

    unsigned available() const
    {
        return m_writePos.load(std::memory_order_acquire) - m_readPos.load(std::memory_order_relaxed);
    }

private:
    static constexpr unsigned capacity() { return RING_CAPACITY * FRAME_SIZE; }

    float m_buffer[RING_CAPACITY * FRAME_SIZE];
    std::atomic<unsigned> m_readPos{0};
    std::atomic<unsigned> m_writePos{0};
};

class AudioBridge : public QObject
{
    Q_OBJECT
public:
    explicit AudioBridge(QObject *parent = nullptr);
    ~AudioBridge();

    bool open(bool echoCancel = true, int aggressiveness = 1);
    void close();

    pjmedia_port *port() const { return m_port; }
    pjsua_conf_port_id confSlot() const { return m_confSlot; }

signals:
    void deviceChanged();

private slots:
    void onInputDeviceChanged(const QAudioDevice &device);
    void onOutputDeviceChanged(const QAudioDevice &device);

private:
    static pj_status_t putFrame(pjmedia_port *port, pjmedia_frame *frame);
    static pj_status_t getFrame(pjmedia_port *port, pjmedia_frame *frame);
    static pj_status_t onDestroy(pjmedia_port *port);

    void startAudio();
    void stopAudio();

    QAudioSink *m_sink = nullptr;
    QAudioSource *m_source = nullptr;
    QIODevice *m_sinkDevice = nullptr;
    QIODevice *m_sourceDevice = nullptr;

    pj_pool_t *m_pool = nullptr;
    pjmedia_port *m_port = nullptr;
    pjsua_conf_port_id m_confSlot = PJSUA_INVALID_ID;

    SpscRingBuffer m_playbackRing;
    SpscRingBuffer m_captureRing;
    SpscRingBuffer m_ecRefRing;

    pjmedia_echo_state *m_echoState = nullptr;
    bool m_echoCancel = true;
    int m_echoAggressiveness = 1;
};

#endif // AUDIOBRIDGE_H
