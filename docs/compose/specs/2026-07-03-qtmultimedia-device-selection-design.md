# Qt Multimedia для вибору аудіопристроїв

## [S1] Проблема

ChirikSIP використовує PortAudio для аудіо I/O з PJSIP. Поточна реалізація працює тільки з default пристроями — користувач не може вибрати мікрофон або динаміки. Немає UI для перемикання пристроїв, збереження вибору між сесіями, та оновлення при hot-plug.

## [S2] Рішення

Замінити PortAudio на Qt Multimedia (`QAudioSink`, `QAudioSource`, `QMediaDevices`). Додати вибір пристроїв у три місця: SettingsDialog, MainWindow, System Tray. Зберігати вибір у `QSettings`. Підтримувати hot-plug.

## [S3] Компоненти

### AudioDeviceManager (новий)

Singleton-менеджер, який:
- Використовує `QMediaDevices` для отримання списків input/output пристроїв
- Зберігає вибір у `QSettings` (input/output device ID)
- Емітує сигнали `inputDeviceChanged(QAudioDevice)`, `outputDeviceChanged(QAudioDevice)`
- Оновлює списки при hot-plug через `QMediaDevices::audioInputsChanged` / `audioOutputsChanged`
- При зникненні активного пристрою — fallback на default

### AudioBridge (переписується)

- Замість PortAudio використовує `QAudioSink` (playback) + `QAudioSource` (capture)
- Зберігає `SpscRingBuffer` для буферизації між Qt audio та PJSIP
- Кастомний `pjmedia_port` адаптер для PJSIP інтеграції
- Echo cancellation через `pjmedia_echo_state` (без змін)

### SettingsDialog (оновлюється)

- Два `QComboBox`: Input device, Output device
- Заповнюються з `AudioDeviceManager::audioInputs()` / `audioOutputs()`
- При зміні — змінюють активний пристрій через `AudioDeviceManager`

### MainWindow (оновлюється)

- Toolbar з двома комбобоксами для швидкого перемикання пристроїв

### System Tray (оновлюється)

- Підменю «Input» зі списком input пристроїв (галочка на поточному)
- Підменю «Output» зі списком output пристроїв (галочка на поточному)

## [S4] Потік даних

### Capture (мікрофон → PJSIP)

```
QAudioSource → SpscRingBuffer → pjmedia_port::getFrame() → PJSIP conference bridge
```

### Playback (PJSIP → динаміки)

```
PJSIP conference bridge → pjmedia_port::putFrame() → SpscRingBuffer → QAudioSink
```

### Echo cancellation

Та сама логіка, що й раніше. Дані проходять через Qt буфери, AEC працює на рівні pjmedia.

## [S5] Зміна пристрою під час дзвінка

1. Користувач вибирає новий пристрій в UI
2. `AudioDeviceManager` зберігає у `QSettings`, емітує сигнал
3. `AudioBridge` отримує сигнал → `close()` → змінює пристрій → `open()`
4. PJSIP conference slot не змінюється (port залишається тим самим)

## [S6] Hot-plug

1. `QMediaDevices::audioInputsChanged` → `AudioDeviceManager` оновлює списки
2. Якщо активний пристрій зник — автоматичний fallback на default
3. UI оновлює комбобокси та підменю треї

## [S7] Обробка помилок

- Якщо `QAudioSink`/`QAudioSource` не може відкритися — fallback на default device з логом
- Якщо збережений пристрій не знайдено — fallback на default з попередженням
- Echo cancellation — optional, як і раніше

## [S8] Зміни файлів

### Нові
- `src/audiodevicemanager.h` / `.cpp`

### Оновлюються
- `src/audiobridge.h` / `.cpp`
- `src/settingsdialog.h` / `.cpp`
- `src/mainwindow.h` / `.cpp`
- `CMakeLists.txt`

### Видаляються
- `src/portaudio_manager.h`

### CMakeLists.txt
- Додати `Qt6::Multimedia`
- Видалити `pkg_check_modules(PORTAUDIO ...)` та пов'язані include/link
