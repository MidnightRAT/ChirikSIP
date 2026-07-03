#ifndef RINGTONE_H
#define RINGTONE_H

#include <QObject>
#include <QAudioSink>
#include <QIODevice>
#include <atomic>

class Ringtone : public QObject
{
    Q_OBJECT
public:
    explicit Ringtone(QObject *parent = nullptr);
    ~Ringtone();

    void start();
    void stop();

private slots:
    void onAudioDataReady();

private:
    QAudioSink *m_sink = nullptr;
    QIODevice *m_device = nullptr;
    std::atomic<bool> m_playing{false};
    std::atomic<unsigned> m_phase{0};
};

#endif // RINGTONE_H
