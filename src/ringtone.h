#ifndef RINGTONE_H
#define RINGTONE_H

#include <QObject>
#include <atomic>
#include <portaudio.h>

class Ringtone : public QObject
{
    Q_OBJECT
public:
    explicit Ringtone(QObject *parent = nullptr);
    ~Ringtone();

    void start();
    void stop();

private:
    static int paCallback(const void *input, void *output,
                          unsigned long frameCount,
                          const PaStreamCallbackTimeInfo *timeInfo,
                          PaStreamCallbackFlags statusFlags,
                          void *userData);

    PaStream *m_stream = nullptr;
    std::atomic<bool> m_playing{false};
    std::atomic<unsigned> m_phase{0};
};

#endif // RINGTONE_H
