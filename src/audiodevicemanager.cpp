#include "audiodevicemanager.h"
#include <QDebug>

AudioDeviceManager::AudioDeviceManager(QObject *parent)
    : QObject(parent)
{
    connect(&QMediaDevices::instance(), &QMediaDevices::audioInputsChanged,
            this, &AudioDeviceManager::deviceListChanged);
    connect(&QMediaDevices::instance(), &QMediaDevices::audioOutputsChanged,
            this, &AudioDeviceManager::deviceListChanged);
}

AudioDeviceManager &AudioDeviceManager::instance()
{
    static AudioDeviceManager inst;
    return inst;
}

QVector<QAudioDevice> AudioDeviceManager::audioInputs() const
{
    QVector<QAudioDevice> result;
    for (const QAudioDevice &dev : QMediaDevices::audioInputs())
        result.append(dev);
    return result;
}

QVector<QAudioDevice> AudioDeviceManager::audioOutputs() const
{
    QVector<QAudioDevice> result;
    for (const QAudioDevice &dev : QMediaDevices::audioOutputs())
        result.append(dev);
    return result;
}

QAudioDevice AudioDeviceManager::currentInputDevice() const
{
    return loadDevice("audio/inputDevice", true);
}

QAudioDevice AudioDeviceManager::currentOutputDevice() const
{
    return loadDevice("audio/outputDevice", false);
}

void AudioDeviceManager::setInputDevice(const QAudioDevice &device)
{
    saveDevice("audio/inputDevice", device);
    emit inputDeviceChanged(device);
}

void AudioDeviceManager::setOutputDevice(const QAudioDevice &device)
{
    saveDevice("audio/outputDevice", device);
    emit outputDeviceChanged(device);
}

QAudioDevice AudioDeviceManager::loadDevice(const QString &key, bool isInput) const
{
    QString id = m_settings.value(key).toString();
    if (!id.isEmpty()) {
        const auto devices = isInput ? QMediaDevices::audioInputs() : QMediaDevices::audioOutputs();
        for (const QAudioDevice &dev : devices) {
            if (dev.id() == id.toUtf8())
                return dev;
        }
        qWarning() << "Saved device not found, using default:" << id;
    }
    return fallbackDevice(isInput);
}

void AudioDeviceManager::saveDevice(const QString &key, const QAudioDevice &device)
{
    m_settings.setValue(key, QString::fromUtf8(device.id()));
}

QAudioDevice AudioDeviceManager::fallbackDevice(bool isInput) const
{
    return isInput ? QMediaDevices::defaultAudioInput() : QMediaDevices::defaultAudioOutput();
}
