#ifndef AUDIODEVICEMANAGER_H
#define AUDIODEVICEMANAGER_H

#include <QObject>
#include <QAudioDevice>
#include <QMediaDevices>
#include <QSettings>
#include <QVector>

class AudioDeviceManager : public QObject
{
    Q_OBJECT
public:
    static AudioDeviceManager &instance();

    QVector<QAudioDevice> audioInputs() const;
    QVector<QAudioDevice> audioOutputs() const;

    QAudioDevice currentInputDevice() const;
    QAudioDevice currentOutputDevice() const;

    void setInputDevice(const QAudioDevice &device);
    void setOutputDevice(const QAudioDevice &device);

signals:
    void inputDeviceChanged(const QAudioDevice &device);
    void outputDeviceChanged(const QAudioDevice &device);
    void deviceListChanged();

private slots:
    void onDeviceListChanged();

private:
    explicit AudioDeviceManager(QObject *parent = nullptr);
    QAudioDevice loadDevice(const QString &key, bool isInput) const;
    void saveDevice(const QString &key, const QAudioDevice &device);
    QAudioDevice fallbackDevice(bool isInput) const;

    QSettings m_settings;
};

#endif // AUDIODEVICEMANAGER_H
