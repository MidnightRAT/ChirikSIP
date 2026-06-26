#ifndef AUDIOBRIDGE_H
#define AUDIOBRIDGE_H

#include <QObject>
#include <pjmedia.h>
#include <pjsua.h>
#include <portaudio.h>

class AudioBridge : public QObject
{
    Q_OBJECT
public:
    explicit AudioBridge(QObject *parent = nullptr);
    ~AudioBridge();

    bool open();
    void close();

    pjmedia_port *port() const { return m_port; }
    pjsua_conf_port_id confSlot() const { return m_confSlot; }

private:
    static int paCallback(const void *input, void *output,
                          unsigned long frameCount,
                          const PaStreamCallbackTimeInfo *timeInfo,
                          PaStreamCallbackFlags statusFlags,
                          void *userData);

    static pj_status_t putFrame(pjmedia_port *port, pjmedia_frame *frame);
    static pj_status_t getFrame(pjmedia_port *port, pjmedia_frame *frame);
    static pj_status_t onDestroy(pjmedia_port *port);

    PaStream *m_stream = nullptr;
    pj_pool_t *m_pool = nullptr;
    pjmedia_port *m_port = nullptr;
    pjsua_conf_port_id m_confSlot = PJSUA_INVALID_ID;

    float m_captureBuffer[160];
    float m_playbackBuffer[160];
    bool m_captureReady = false;
    bool m_playbackReady = false;
};

#endif // AUDIOBRIDGE_H
