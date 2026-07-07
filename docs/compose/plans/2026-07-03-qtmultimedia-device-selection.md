# Qt Multimedia Device Selection Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use compose:subagent (recommended) or compose:execute to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace PortAudio with Qt Multimedia for audio I/O, add device selection UI in SettingsDialog, MainWindow toolbar, and system tray.

**Architecture:** `AudioDeviceManager` singleton manages device enumeration via `QMediaDevices` and persists selection in `QSettings`. `AudioBridge` uses `QAudioSink`/`QAudioSource` instead of PortAudio, bridging to PJSIP via custom `pjmedia_port` adapter. Three UI locations (settings, toolbar, tray) provide device selection.

**Tech Stack:** Qt6, Qt6::Multimedia, PJSIP (pjmedia)

## Global Constraints

- Qt 6.5+ required (QAudioDevice API)
- PJSIP libpjproject for SIP/audio bridge
- Echo cancellation via pjmedia_echo_state (unchanged)
- Sample rate: 8000 Hz, mono, 16-bit PCM
- Frame size: 160 samples (20ms)

---

### Task 1: CMakeLists.txt — Replace PortAudio with Qt Multimedia

**Covers:** [S8]

**Files:**
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Update CMakeLists.txt**

Replace the PortAudio section with Qt Multimedia. Remove `pkg_check_modules(PORTAUDIO ...)`, add `Qt6::Multimedia`.

Current lines 23-34:
```cmake
find_package(Qt6 REQUIRED COMPONENTS Core Widgets DBus)

find_package(PkgConfig REQUIRED)
pkg_check_modules(PJSIP REQUIRED IMPORTED_TARGET libpjproject)
pkg_check_modules(PORTAUDIO REQUIRED portaudio-2.0)
target_link_libraries(chiriksip PRIVATE
    Qt6::Core Qt6::Widgets Qt6::DBus
    PkgConfig::PJSIP
    ${PORTAUDIO_LIBRARIES}
)
target_include_directories(chiriksip PRIVATE ${PORTAUDIO_INCLUDE_DIRS})
target_compile_options(chiriksip PRIVATE ${PJSIP_CFLAGS_OTHER} ${PORTAUDIO_CFLAGS_OTHER})
```

Replace with:
```cmake
find_package(Qt6 REQUIRED COMPONENTS Core Widgets DBus Multimedia)

find_package(PkgConfig REQUIRED)
pkg_check_modules(PJSIP REQUIRED IMPORTED_TARGET libpjproject)
target_link_libraries(chiriksip PRIVATE
    Qt6::Core Qt6::Widgets Qt6::DBus Qt6::Multimedia
    PkgConfig::PJSIP
)
target_compile_options(chiriksip PRIVATE ${PJSIP_CFLAGS_OTHER})
```

Also update the test target (lines 76-83) to remove PortAudio references:
```cmake
add_executable(test_parsing ${TEST_SOURCES})
target_link_libraries(test_parsing PRIVATE
    Qt6::Core Qt6::Widgets Qt6::Test Qt6::Multimedia
    PkgConfig::PJSIP
)
target_include_directories(test_parsing PRIVATE src)
target_compile_definitions(test_parsing PRIVATE PROJECT_VERSION="${PROJECT_VERSION}")
add_test(NAME test_parsing COMMAND test_parsing)
```

- [ ] **Step 2: Verify build compiles**

Run: `cmake -B build -S . 2>&1 | head -20`
Expected: No errors about missing PortAudio, Qt6::Multimedia found.

- [ ] **Step 3: Commit**

```bash
git add CMakeLists.txt
git commit -m "build: replace PortAudio with Qt6::Multimedia"
```

---

### Task 2: AudioDeviceManager — Device Enumeration Singleton

**Covers:** [S3, S6]

**Files:**
- Create: `src/audiodevicemanager.h`
- Create: `src/audiodevicemanager.cpp`

**Interfaces:**
- Produces: `AudioDeviceManager::instance()`, `audioInputs()`, `audioOutputs()`, `currentInputDevice()`, `currentOutputDevice()`, `setInputDevice(QAudioDevice)`, `setOutputDevice(QAudioDevice)`, signals `inputDeviceChanged`, `outputDeviceChanged`, `deviceListChanged`

- [ ] **Step 1: Create audiodevicemanager.h**

```cpp
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

private:
    explicit AudioDeviceManager(QObject *parent = nullptr);
    QAudioDevice loadDevice(const QString &key, bool isInput) const;
    void saveDevice(const QString &key, const QAudioDevice &device);
    QAudioDevice fallbackDevice(bool isInput) const;

    QSettings m_settings;
};

#endif // AUDIODEVICEMANAGER_H
```

- [ ] **Step 2: Create audiodevicemanager.cpp**

```cpp
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
```

- [ ] **Step 3: Verify build compiles**

Run: `cmake --build build 2>&1 | tail -5`
Expected: No errors.

- [ ] **Step 4: Commit**

```bash
git add src/audiodevicemanager.h src/audiodevicemanager.cpp
git commit -m "feat: add AudioDeviceManager singleton for device enumeration"
```

---

### Task 3: AudioBridge — Replace PortAudio with Qt Multimedia

**Covers:** [S3, S4, S7]

**Files:**
- Modify: `src/audiobridge.h`
- Modify: `src/audiobridge.cpp`

**Interfaces:**
- Consumes: `AudioDeviceManager::currentInputDevice()`, `AudioDeviceManager::currentOutputDevice()`
- Produces: `AudioBridge::open()`, `AudioBridge::close()`, `AudioBridge::port()`, `AudioBridge::confSlot()`

- [ ] **Step 1: Rewrite audiobridge.h**

```cpp
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
```

- [ ] **Step 2: Rewrite audiobridge.cpp**

```cpp
#include "audiobridge.h"
#include "audiodevicemanager.h"
#include <QDebug>
#include <QAudioFormat>
#include <cstring>

static const int SAMPLE_RATE = 8000;
static const int CHANNELS = 1;

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
    connect(&AudioDeviceManager::instance(), &AudioDeviceManager::inputDeviceChanged,
            this, &AudioBridge::onInputDeviceChanged);
    connect(&AudioDeviceManager::instance(), &AudioDeviceManager::outputDeviceChanged,
            this, &AudioBridge::onOutputDeviceChanged);
}

AudioBridge::~AudioBridge()
{
    if (m_sink || m_source || m_pool)
        close();
}

bool AudioBridge::open(bool echoCancel, int aggressiveness)
{
    m_echoCancel = echoCancel;
    m_echoAggressiveness = aggressiveness;

    startAudio();
    if (!m_source || !m_sink)
        return false;

    m_pool = pjsua_pool_create("aubridge", 512, 512);
    if (!m_pool) {
        qCritical() << "Failed to create pool for audio bridge";
        stopAudio();
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
        stopAudio();
        return false;
    }

    static const QByteArray portName("qtmultimedia");
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
        stopAudio();
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
    if (m_confSlot != PJSUA_INVALID_ID) {
        pjsua_conf_remove_port(m_confSlot);
        m_confSlot = PJSUA_INVALID_ID;
    }

    if (m_echoState) {
        pjmedia_echo_destroy(m_echoState);
        m_echoState = nullptr;
    }

    if (m_pool) {
        pj_pool_release(m_pool);
        m_pool = nullptr;
    }

    m_port = nullptr;
    stopAudio();

    qInfo() << "Audio bridge closed";
}

void AudioBridge::startAudio()
{
    AudioDeviceManager &dm = AudioDeviceManager::instance();

    QAudioFormat format;
    format.setSampleRate(SAMPLE_RATE);
    format.setChannelCount(CHANNELS);
    format.setSampleFormat(QAudioFormat::Int16);

    QAudioDevice inputDev = dm.currentInputDevice();
    m_source = new QAudioSource(inputDev, format, this);
    m_sourceDevice = m_source->start();

    QAudioDevice outputDev = dm.currentOutputDevice();
    m_sink = new QAudioSink(outputDev, format, this);
    m_sinkDevice = m_sink->start();

    if (!m_sourceDevice || !m_sinkDevice) {
        qCritical() << "Failed to start audio devices";
        stopAudio();
        return;
    }

    qInfo() << "Audio started with input:" << inputDev.description()
            << "output:" << outputDev.description();
}

void AudioBridge::stopAudio()
{
    if (m_source) {
        m_source->stop();
        delete m_source;
        m_source = nullptr;
        m_sourceDevice = nullptr;
    }
    if (m_sink) {
        m_sink->stop();
        delete m_sink;
        m_sink = nullptr;
        m_sinkDevice = nullptr;
    }
}

void AudioBridge::onInputDeviceChanged(const QAudioDevice &device)
{
    Q_UNUSED(device);
    if (m_source) {
        qInfo() << "Input device changed, restarting audio";
        stopAudio();
        startAudio();
    }
}

void AudioBridge::onOutputDeviceChanged(const QAudioDevice &device)
{
    Q_UNUSED(device);
    if (m_sink) {
        qInfo() << "Output device changed, restarting audio";
        stopAudio();
        startAudio();
    }
}

pj_status_t AudioBridge::putFrame(pjmedia_port *port, pjmedia_frame *frame)
{
    AudioBridge *self = static_cast<AudioBridge *>(port->port_data.pdata);
    if (!self || !self->m_sinkDevice)
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

    // Write to Qt audio sink
    float outBuf[FRAME_SIZE];
    if (self->m_playbackRing.pop(outBuf, count)) {
        QByteArray pcmData(reinterpret_cast<const char*>(outBuf), count * sizeof(float));
        self->m_sinkDevice->write(pcmData);
    }

    return PJ_SUCCESS;
}

pj_status_t AudioBridge::getFrame(pjmedia_port *port, pjmedia_frame *frame)
{
    AudioBridge *self = static_cast<AudioBridge *>(port->port_data.pdata);
    if (!self || !self->m_sourceDevice) {
        frame->size = 0;
        return PJ_EINVAL;
    }

    pj_int16_t *samples = (pj_int16_t *)frame->buf;
    unsigned count = frame->size / sizeof(pj_int16_t);
    if (count > FRAME_SIZE)
        count = FRAME_SIZE;

    // Read from Qt audio source
    QByteArray pcmData = self->m_sourceDevice->read(count * sizeof(float));
    unsigned readCount = pcmData.size() / sizeof(float);
    const float *floatData = reinterpret_cast<const float *>(pcmData.constData());

    float capBuf[FRAME_SIZE];
    if (readCount >= count) {
        for (unsigned i = 0; i < count; ++i)
            capBuf[i] = floatData[i];
    } else {
        std::memset(capBuf, 0, count * sizeof(float));
    }

    if (self->m_echoState) {
        pj_int16_t refI16[FRAME_SIZE];
        pj_int16_t micI16[FRAME_SIZE];
        unsigned refCount = count;

        float refFloat[FRAME_SIZE];
        if (!self->m_ecRefRing.pop(refFloat, refCount))
            std::memset(refFloat, 0, refCount * sizeof(float));

        for (unsigned i = 0; i < refCount; ++i) {
            float s = refFloat[i] * 32767.0f;
            if (s > 32767.0f) s = 32767.0f;
            if (s < -32768.0f) s = -32768.0f;
            refI16[i] = (pj_int16_t)s;
        }
        for (unsigned i = 0; i < count; ++i) {
            float s = capBuf[i] * 32767.0f;
            if (s > 32767.0f) s = 32767.0f;
            if (s < -32768.0f) s = -32768.0f;
            micI16[i] = (pj_int16_t)s;
        }

        pjmedia_echo_playback(self->m_echoState, refI16);
        pjmedia_echo_capture(self->m_echoState, micI16, 0);

        for (unsigned i = 0; i < count; ++i)
            capBuf[i] = micI16[i] / 32768.0f;
    }

    self->m_captureRing.push(capBuf, count);

    float capOut[FRAME_SIZE];
    if (self->m_captureRing.pop(capOut, count)) {
        for (unsigned i = 0; i < count; ++i) {
            float s = capOut[i] * 32767.0f;
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
```

- [ ] **Step 3: Verify build compiles**

Run: `cmake --build build 2>&1 | tail -10`
Expected: No errors.

- [ ] **Step 4: Commit**

```bash
git add src/audiobridge.h src/audiobridge.cpp
git commit -m "feat: replace PortAudio with Qt Multimedia in AudioBridge"
```

---

### Task 4: Remove PortAudio Manager

**Covers:** [S8]

**Files:**
- Delete: `src/portaudio_manager.h`

- [ ] **Step 1: Delete portaudio_manager.h**

```bash
git rm src/portaudio_manager.h
```

- [ ] **Step 2: Verify build compiles**

Run: `cmake --build build 2>&1 | tail -5`
Expected: No errors (portaudio_manager.h is no longer included).

- [ ] **Step 3: Commit**

```bash
git commit -m "chore: remove PortAudio manager (replaced by Qt Multimedia)"
```

---

### Task 5: SettingsDialog — Add Device Combo Boxes

**Covers:** [S3]

**Files:**
- Modify: `src/settingsdialog.h`
- Modify: `src/settingsdialog.cpp`

**Interfaces:**
- Consumes: `AudioDeviceManager::audioInputs()`, `AudioDeviceManager::audioOutputs()`, `AudioDeviceManager::currentInputDevice()`, `AudioDeviceManager::currentOutputDevice()`
- Produces: `SettingsDialog::selectedInputDevice()`, `SettingsDialog::selectedOutputDevice()`

- [ ] **Step 1: Update settingsdialog.h**

Add forward declarations and new members. After the existing `#include <QComboBox>` line, the class should have:

```cpp
class SettingsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget *parent = nullptr);

    QString server() const;
    QString username() const;
    QString password() const;
    int port() const;
    bool echoCancelEnabled() const;
    int echoAggressiveness() const;
    QAudioDevice selectedInputDevice() const;
    QAudioDevice selectedOutputDevice() const;

    void setServer(const QString &s);
    void setUsername(const QString &u);
    void setPassword(const QString &p);
    void setPort(int p);
    void setEchoCancelEnabled(bool enabled);
    void setEchoAggressiveness(int level);
    void setInputDevice(const QAudioDevice &device);
    void setOutputDevice(const QAudioDevice &device);

private:
    QLineEdit *m_serverEdit;
    QLineEdit *m_usernameEdit;
    QLineEdit *m_passwordEdit;
    QSpinBox *m_portEdit;
    QCheckBox *m_echoCancelCheck;
    QComboBox *m_echoAggressivenessCombo;
    QComboBox *m_inputDeviceCombo;
    QComboBox *m_outputDeviceCombo;
};
```

Add `#include <QAudioDevice>` at the top of the header.

- [ ] **Step 2: Update settingsdialog.cpp**

Add device combo boxes after the echo aggressiveness combo. Add these includes at the top:
```cpp
#include <QAudioDevice>
#include <QMediaDevices>
```

After the `m_echoAggressivenessCombo` section (around line 47), add:

```cpp
    m_inputDeviceCombo = new QComboBox(this);
    const auto inputs = QMediaDevices::audioInputs();
    for (const QAudioDevice &dev : inputs)
        m_inputDeviceCombo->addItem(dev.description(), QVariant::fromValue(dev));
    form->addRow("Microphone:", m_inputDeviceCombo);

    m_outputDeviceCombo = new QComboBox(this);
    const auto outputs = QMediaDevices::audioOutputs();
    for (const QAudioDevice &dev : outputs)
        m_outputDeviceCombo->addItem(dev.description(), QVariant::fromValue(dev));
    form->addRow("Speakers:", m_outputDeviceCombo);
```

Add accessor implementations at the bottom:

```cpp
QAudioDevice SettingsDialog::selectedInputDevice() const {
    return m_inputDeviceCombo->currentData().value<QAudioDevice>();
}
QAudioDevice SettingsDialog::selectedOutputDevice() const {
    return m_outputDeviceCombo->currentData().value<QAudioDevice>();
}
void SettingsDialog::setInputDevice(const QAudioDevice &device) {
    for (int i = 0; i < m_inputDeviceCombo->count(); ++i) {
        if (m_inputDeviceCombo->itemData(i).value<QAudioDevice>().id() == device.id()) {
            m_inputDeviceCombo->setCurrentIndex(i);
            break;
        }
    }
}
void SettingsDialog::setOutputDevice(const QAudioDevice &device) {
    for (int i = 0; i < m_outputDeviceCombo->count(); ++i) {
        if (m_outputDeviceCombo->itemData(i).value<QAudioDevice>().id() == device.id()) {
            m_outputDeviceCombo->setCurrentIndex(i);
            break;
        }
    }
}
```

- [ ] **Step 3: Verify build compiles**

Run: `cmake --build build 2>&1 | tail -5`
Expected: No errors.

- [ ] **Step 4: Commit**

```bash
git add src/settingsdialog.h src/settingsdialog.cpp
git commit -m "feat: add audio device combo boxes to SettingsDialog"
```

---

### Task 6: MainWindow — Add Device Toolbar and Tray Submenus

**Covers:** [S3, S5]

**Files:**
- Modify: `src/mainwindow.h`
- Modify: `src/mainwindow.cpp`

**Interfaces:**
- Consumes: `AudioDeviceManager` singleton, `SettingsDialog::selectedInputDevice()`, `SettingsDialog::selectedOutputDevice()`

- [ ] **Step 1: Update mainwindow.h**

Add new members. After the existing includes, add:
```cpp
#include <QComboBox>
#include <QAudioDevice>
```

Add forward declaration:
```cpp
class AudioDeviceManager;
```

Add new private members and slots:
```cpp
private slots:
    void onInputDeviceChanged(int index);
    void onOutputDeviceChanged(int index);
    void rebuildTrayDeviceMenus();
    void onInputDeviceActionTriggered();
    void onOutputDeviceActionTriggered();
```

Add new private members:
```cpp
    QComboBox *m_inputDeviceCombo = nullptr;
    QComboBox *m_outputDeviceCombo = nullptr;
    QMenu *m_inputDeviceMenu = nullptr;
    QMenu *m_outputDeviceMenu = nullptr;
```

- [ ] **Step 2: Update mainwindow.cpp — Add device toolbar**

Add includes at the top:
```cpp
#include "audiodevicemanager.h"
#include <QAudioDevice>
#include <QMediaDevices>
```

In the constructor, after `setupTray()` and before `loadSettings()`, add device combo creation:

```cpp
    // Device selection combos in status bar area
    QHBoxLayout *deviceLayout = new QHBoxLayout();
    m_inputDeviceCombo = new QComboBox(central);
    m_outputDeviceCombo = new QComboBox(central);
    m_inputDeviceCombo->setToolTip("Microphone");
    m_outputDeviceCombo->setToolTip("Speakers");

    const auto inputs = AudioDeviceManager::instance().audioInputs();
    for (const QAudioDevice &dev : inputs)
        m_inputDeviceCombo->addItem(dev.description(), QVariant::fromValue(dev));

    const auto outputs = AudioDeviceManager::instance().audioOutputs();
    for (const QAudioDevice &dev : outputs)
        m_outputDeviceCombo->addItem(dev.description(), QVariant::fromValue(dev));

    // Set current selection
    QAudioDevice curIn = AudioDeviceManager::instance().currentInputDevice();
    QAudioDevice curOut = AudioDeviceManager::instance().currentOutputDevice();
    setInputComboDevice(curIn);
    setOutputComboDevice(curOut);

    connect(m_inputDeviceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onInputDeviceChanged);
    connect(m_outputDeviceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onOutputDeviceChanged);

    connect(&AudioDeviceManager::instance(), &AudioDeviceManager::deviceListChanged,
            this, &MainWindow::rebuildTrayDeviceMenus);
```

Insert `deviceLayout` into the main layout (before the status layout, after the stretch):
```cpp
    mainLayout->addStretch();

    deviceLayout->addWidget(new QLabel("Mic:", central));
    deviceLayout->addWidget(m_inputDeviceCombo);
    deviceLayout->addWidget(new QLabel("Spk:", central));
    deviceLayout->addWidget(m_outputDeviceCombo);
    mainLayout->addLayout(deviceLayout);
```

- [ ] **Step 3: Update mainwindow.cpp — Add tray device submenus**

In `setupTray()`, after the existing `trayMenu` actions, add:

```cpp
    trayMenu->addSeparator();

    m_inputDeviceMenu = trayMenu->addMenu("Input Device");
    m_outputDeviceMenu = trayMenu->addMenu("Output Device");
    rebuildTrayDeviceMenus();
```

Add helper methods:

```cpp
void MainWindow::rebuildTrayDeviceMenus()
{
    if (!m_inputDeviceMenu || !m_outputDeviceMenu)
        return;

    m_inputDeviceMenu->clear();
    m_outputDeviceMenu->clear();

    QAudioDevice curIn = AudioDeviceManager::instance().currentInputDevice();
    const auto inputs = AudioDeviceManager::instance().audioInputs();
    for (const QAudioDevice &dev : inputs) {
        QAction *action = m_inputDeviceMenu->addAction(dev.description());
        action->setCheckable(true);
        action->setChecked(dev.id() == curIn.id());
        action->setData(QVariant::fromValue(dev));
        connect(action, &QAction::triggered, this, &MainWindow::onInputDeviceActionTriggered);
    }

    QAudioDevice curOut = AudioDeviceManager::instance().currentOutputDevice();
    const auto outputs = AudioDeviceManager::instance().audioOutputs();
    for (const QAudioDevice &dev : outputs) {
        QAction *action = m_outputDeviceMenu->addAction(dev.description());
        action->setCheckable(true);
        action->setChecked(dev.id() == curOut.id());
        action->setData(QVariant::fromValue(dev));
        connect(action, &QAction::triggered, this, &MainWindow::onOutputDeviceActionTriggered);
    }
}

void MainWindow::onInputDeviceActionTriggered()
{
    QAction *action = qobject_cast<QAction *>(sender());
    if (action)
        AudioDeviceManager::instance().setInputDevice(action->data().value<QAudioDevice>());
}

void MainWindow::onOutputDeviceActionTriggered()
{
    QAction *action = qobject_cast<QAction *>(sender());
    if (action)
        AudioDeviceManager::instance().setOutputDevice(action->data().value<QAudioDevice>());
}
```

- [ ] **Step 4: Update mainwindow.cpp — Add device change handlers**

```cpp
void MainWindow::onInputDeviceChanged(int index)
{
    if (index < 0) return;
    QAudioDevice dev = m_inputDeviceCombo->itemData(index).value<QAudioDevice>();
    AudioDeviceManager::instance().setInputDevice(dev);
}

void MainWindow::onOutputDeviceChanged(int index)
{
    if (index < 0) return;
    QAudioDevice dev = m_outputDeviceCombo->itemData(index).value<QAudioDevice>();
    AudioDeviceManager::instance().setOutputDevice(dev);
}
```

Add helper methods for setting combo selection:

```cpp
void MainWindow::setInputComboDevice(const QAudioDevice &device)
{
    for (int i = 0; i < m_inputDeviceCombo->count(); ++i) {
        if (m_inputDeviceCombo->itemData(i).value<QAudioDevice>().id() == device.id()) {
            m_inputDeviceCombo->setCurrentIndex(i);
            return;
        }
    }
}

void MainWindow::setOutputComboDevice(const QAudioDevice &device)
{
    for (int i = 0; i < m_outputDeviceCombo->count(); ++i) {
        if (m_outputDeviceCombo->itemData(i).value<QAudioDevice>().id() == device.id()) {
            m_outputDeviceCombo->setCurrentIndex(i);
            return;
        }
    }
}
```

Add declarations to header:
```cpp
    void setInputComboDevice(const QAudioDevice &device);
    void setOutputComboDevice(const QAudioDevice &device);
```

- [ ] **Step 5: Update onSettings() to handle device changes**

In `onSettings()`, after the existing dialog handling, add device selection:

```cpp
    dlg.setInputDevice(AudioDeviceManager::instance().currentInputDevice());
    dlg.setOutputDevice(AudioDeviceManager::instance().currentOutputDevice());
```

And after `if (dlg.exec() == QDialog::Accepted)`, before `saveSettings()`:

```cpp
        AudioDeviceManager::instance().setInputDevice(dlg.selectedInputDevice());
        AudioDeviceManager::instance().setOutputDevice(dlg.selectedOutputDevice());
```

- [ ] **Step 6: Verify build compiles**

Run: `cmake --build build 2>&1 | tail -10`
Expected: No errors.

- [ ] **Step 7: Commit**

```bash
git add src/mainwindow.h src/mainwindow.cpp
git commit -m "feat: add device selection toolbar and tray submenus"
```

---

### Task 7: Final Verification

**Covers:** [S1, S2, S3, S4, S5, S6, S7, S8]

**Files:**
- None (verification only)

- [ ] **Step 1: Full build**

Run: `cmake --build build 2>&1`
Expected: Clean build with no errors.

- [ ] **Step 2: Run tests**

Run: `cd build && ctest --output-on-failure`
Expected: Tests pass.

- [ ] **Step 3: Verify no PortAudio references remain**

Run: `grep -r "portaudio" src/ CMakeLists.txt`
Expected: No matches (except possibly comments).

- [ ] **Step 4: Verify Qt Multimedia is linked**

Run: `ldd build/chiriksip | grep -i qt`
Expected: Shows Qt6Multimedia library linked.

- [ ] **Step 5: Final commit if needed**

```bash
git add -A
git commit -m "chore: final cleanup for Qt Multimedia migration"
```
