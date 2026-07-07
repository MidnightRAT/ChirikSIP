# ChirikSIP

[![Build RPM](https://github.com/MidnightRAT/ChirikSIP/actions/workflows/build-rpm.yml/badge.svg)](https://github.com/MidnightRAT/ChirikSIP/actions/workflows/build-rpm.yml)
[![Build DEB](https://github.com/MidnightRAT/ChirikSIP/actions/workflows/build-deb.yml/badge.svg)](https://github.com/MidnightRAT/ChirikSIP/actions/workflows/build-deb.yml)
[![Build Flatpak](https://github.com/MidnightRAT/ChirikSIP/actions/workflows/build-flatpak.yml/badge.svg)](https://github.com/MidnightRAT/ChirikSIP/actions/workflows/build-flatpak.yml)
[![Latest Release](https://img.shields.io/github/v/release/MidnightRAT/ChirikSIP)](https://github.com/MidnightRAT/ChirikSIP/releases/latest)

[English](#english) | [Українська](#українська)

---

## English

Minimalist SIP client for KDE Plasma, built on Qt6 and PJSIP.

![Linux Main Window](screenshots/Linux_dark_main.png)

More screenshots in [screenshots/README.md](screenshots/README.md)

### Donate

[![Donate via WayForPay](https://img.shields.io/badge/Donate-WayForPay-blue?style=for-the-badge)](https://secure.wayforpay.com/donate/d29145e2b8e3c)

### Features

- SIP client: registration with digest authentication, incoming/outgoing calls, G.711 A-law/u-law codecs
- Audio: PortAudio ↔ PJSIP conference bridge, echo cancellation (Conservative/Moderate/Aggressive), PipeWire/PulseAudio/ALSA
- UI: LCD display (Segment16A), numpad, clock, call duration, status bar
- System tray: minimize on close, incoming call popup, exit via Ctrl+Q
- Settings: first-run wizard, settings dialog (Ctrl+,), config stored in `~/.config/chiriksip/`
- Single instance via D-Bus

Detailed architecture, audio pipeline, and keyboard shortcuts documentation below.

### Installation

#### From GitHub Release (Recommended)

Download the latest `chiriksip-*.rpm` or `chiriksip_*.deb` from [Releases](https://github.com/MidnightRAT/ChirikSIP/releases) and install:

```bash
# Fedora
sudo dnf install chiriksip-*.x86_64.rpm

# Ubuntu
sudo dpkg -i chiriksip_*.deb
```

#### From Flathub

```bash
flatpak install flathub com.github.chirik.ChirikSIP
flatpak run com.github.chirik.ChirikSIP
```

#### Build from Source

Build dependencies (Fedora):

| Package | Purpose |
|---------|---------|
| `cmake >= 3.20` | Build system |
| `gcc-c++` | C++17 compiler |
| `qt6-qtbase-devel` | Qt6 Core, Widgets, DBus |
| `pkgconfig(libpjproject)` | PJSIP SIP stack |
| `portaudio-devel` | Audio I/O |
| `desktop-file-utils` | .desktop file validation |
| `hicolor-icon-theme` | Icon installation |

Runtime dependencies:

| Package | Purpose |
|---------|---------|
| `qt6-qtbase` | Qt6 runtime |
| `qt6-qtbase-gui` | Qt6 GUI |
| `pjproject` | SIP/audio stack |
| `portaudio` | Audio devices |
| `hicolor-icon-theme` | System icons |

```bash
mkdir build && cd build
cmake ..
cmake --build .
cmake --install build
```

### Build

#### RPM (Fedora)

```bash
# Local
VERSION=$(grep 'Version:' packaging/chiriksip.spec | awk '{print $2}')
tar czf ~/rpmbuild/SOURCES/chiriksip-${VERSION}.tar.gz \
    --transform "s,^,chiriksip-${VERSION}/," \
    -X .rpmignore .
rpmbuild -ba packaging/chiriksip.spec

# In container (podman/docker)
./build-rpm.sh "43 44"
./build-rpm.sh --force "44"
```

Artifacts: `build/rpms/` — `chiriksip-*.rpm`, `chiriksip-*.src.rpm`

#### DEB (Ubuntu)

Supports Ubuntu 22.04 LTS (Jammy) and 24.04 LTS (Noble). PJSIP is built from source.

```bash
./build-deb.sh "22.04 24.04"
./build-deb.sh --force "24.04"
```

Artifacts: `build/debs/` — `chiriksip_*.deb`

#### Flatpak

```bash
./build-flatpak.sh

# Or manually
flatpak-builder --force-clean --install-deps-from=flathub \
    --repo=repo builddir com.github.chirik.ChirikSIP.yml
flatpak install --user repo com.github.chirik.ChirikSIP
flatpak run com.github.chirik.ChirikSIP
```

### Tests

```bash
cd build && ctest
```

Unit tests (`tests/test_parsing.cpp`):
- `testParseNumber` — 6 cases (number, SIP URI, display name, no @, empty, international)
- `testParseDisplayName` — 5 cases
- `testFormatDuration` — 8 cases (0, 1s, 59s, 1min, 1h, 1h1min1s, 23:59:59, 2min5s)

### Usage

1. Launch `chiriksip`
2. On first launch, the **Setup Wizard** opens — enter SIP server, login, password
3. Or open **Settings > Settings** (Ctrl+,) and configure your account
4. Click **Register** (or registration happens automatically)
5. Dial a number using the numpad and click **Call**
6. For incoming calls, click **Answer**

### Keyboard Shortcuts

| Key | Action |
|-----|--------|
| 0-9 | Enter digit |
| * | Enter asterisk |
| # | Enter hash |
| + | Enter plus |
| Enter | Dial / Answer |
| Escape | End call |
| Backspace | Delete last digit |
| Ctrl+, | Open settings |
| Ctrl+Q | Force quit |

### Button Behavior

| Button | Short Press | Long Press |
|--------|------------|------------|
| Hangup | Delete digit / End call | Clear all / End call |
| 0+ | Insert "0" | Insert "+" |

### Architecture

![Architecture](docs/architecture.svg)

```
┌──────────────┐     ┌──────────────┐     ┌──────────────┐
│  MainWindow  │────►│ CallManager  │────►│  SipClient   │
│  (Qt UI)     │     │ (lifecycle)  │     │  (PJSIP)     │
└──────────────┘     └──────┬───────┘     └──────┬───────┘
                            │                    │
                     ┌──────▼───────┐     ┌──────▼───────┐
                     │ AudioBridge  │     │   PJSIP      │
                     │              │     │  conference  │
                     │ ┌──────────┐ │     │   bridge     │
                     │ │ Echo CXL │ │     └──────────────┘
                     │ │(pjmedia) │ │
                     │ └──────────┘ │     ┌──────────────┐
                     │ ┌──────────┐ │     │  Ringtone    │
                     │ │Ring: play│ │     │ (440Hz sine) │
                     │ │Ring: capt│ │     └──────────────┘
                     │ │Ring: ec  │ │
                     │ └──────────┘ │     ┌──────────────┐
                     │  PortAudio   │     │PortAudioMgr  │
                     │   stream     │     │(ref-counted) │
                     └──────────────┘     └──────────────┘
```

#### Threading Model

| Thread | Components | Communication |
|--------|-----------|---------------|
| Qt main | MainWindow, CallManager, SipClient (API), AudioBridge, SettingsDialog | Signals/slots |
| PJSIP ioqueue | SipClient static callbacks | `QMetaObject::invokeMethod` → QueuedConnection |
| PortAudio | AudioBridge paCallback, Ringtone callback | SPSC ring buffers, `std::atomic` |

#### Audio Pipeline

```
Microphone → paCallback → EC (pjmedia_echo) → captureRing → getFrame → conf bridge
conf bridge → putFrame → playbackRing → paCallback → speaker
conf bridge → putFrame → ecRefRing → paCallback → EC reference signal
```

#### Class Dependencies

```
MainWindow ──owns──> SipClient, CallManager, CallNotification, ScrollHelper
CallManager ──owns──> AudioBridge
CallManager ──uses──> SipClient (delegates SIP operations)
SipClient ──owns──> Ringtone
AudioBridge ──uses──> PortAudioManager (ref-counted init/terminate)
Ringtone ──uses──> PortAudioManager
```

### CI/CD

GitHub Actions — automatic build and publish on push to `main`:

| Workflow | Platform | Artifacts |
|----------|----------|-----------|
| `build-rpm.yml` | Ubuntu + podman | RPM for Fedora 43, 44 (x86_64) |
| `build-deb.yml` | Ubuntu + podman | DEB for Ubuntu 22.04, 24.04 |
| `build-flatpak.yml` | flatpak-github-actions | Flatpak bundle |

Triggers: changes in `src/`, `packaging/`, `debian/`, `CMakeLists.txt`, `resources/`, respective workflow files.

**Note:** Workflow does not trigger on changes to CHANGELOG.md or README.md.

All artifacts are published to GitHub Releases (tag `v<version>`).

### Project Structure

```
ChirikSIP/
├── .github/workflows/    # CI: RPM, DEB, Flatpak
├── src/                  # C++ sources
├── tests/                # Unit tests
├── resources/            # .desktop, icons
├── packaging/            # RPM spec (Fedora)
├── debian/               # DEB packaging (Ubuntu)
├── docs/                 # Architecture, screenshots
└── screenshots/          # Interface screenshots
```

### License

MIT

---

## Українська

Мінімалістичний SIP-клієнт для KDE Plasma, побудований на Qt6 та PJSIP.

![Linux Main Window](screenshots/Linux_dark_main.png)

Детальніші скріншоти див. у [screenshots/README.md](screenshots/README.md)

### Donate

[![Donate via WayForPay](https://img.shields.io/badge/Donate-WayForPay-blue?style=for-the-badge)](https://secure.wayforpay.com/donate/d29145e2b8e3c)

### Можливості

- SIP-клієнт: реєстрація з digest-аутентифікацією, вихідні/вхідні виклики, кодеки G.711 A-law/u-law
- Аудіо: PortAudio ↔ PJSIP conference bridge, echo cancellation (Conservative/Moderate/Aggressive), PipeWire/PulseAudio/ALSA
- Інтерфейс: LCD-дисплей (Segment16A), нумпад, годинник, тривалість виклику, статус-бар
- Системний трей: мінімізація при закритті, popup вхідного виклику, вихід через Ctrl+Q
- Налаштування: майстер першого запуску, діалог налаштувань (Ctrl+,), зберігає у `~/.config/chiriksip/`
- Single instance через D-Bus

Детальніша документація по архітектурі, аудіо-пайплайну та клавіатурних скороченнях — див. нижче.

### Встановлення

#### З GitHub Release (рекомендовано)

Завантажте останній `chiriksip-*.rpm` або `chiriksip_*.deb` з [Releases](https://github.com/MidnightRAT/ChirikSIP/releases) та встановіть:

```bash
# Fedora
sudo dnf install chiriksip-*.x86_64.rpm

# Ubuntu
sudo dpkg -i chiriksip_*.deb
```

#### З Flathub

```bash
flatpak install flathub com.github.chirik.ChirikSIP
flatpak run com.github.chirik.ChirikSIP
```

#### Збірка з джерел

Залежності (Fedora):

| Пакет | Призначення |
|-------|-------------|
| `cmake >= 3.20` | Build system |
| `gcc-c++` | C++17 компілятор |
| `qt6-qtbase-devel` | Qt6 Core, Widgets, DBus |
| `pkgconfig(libpjproject)` | PJSIP SIP-стек |
| `portaudio-devel` | Аудіо I/O |
| `desktop-file-utils` | Валідація .desktop файлу |
| `hicolor-icon-theme` | Встановлення іконок |

Залежності (runtime):

| Пакет | Призначення |
|-------|-------------|
| `qt6-qtbase` | Qt6 runtime |
| `qt6-qtbase-gui` | Qt6 GUI |
| `pjproject` | SIP/audio стек |
| `portaudio` | Аудіо-пристрої |
| `hicolor-icon-theme` | Системні іконки |

```bash
mkdir build && cd build
cmake ..
cmake --build .
cmake --install build
```

### Збірка

#### RPM (Fedora)

```bash
# Локально
VERSION=$(grep 'Version:' packaging/chiriksip.spec | awk '{print $2}')
tar czf ~/rpmbuild/SOURCES/chiriksip-${VERSION}.tar.gz \
    --transform "s,^,chiriksip-${VERSION}/," \
    -X .rpmignore .
rpmbuild -ba packaging/chiriksip.spec

# В контейнері (podman/docker)
./build-rpm.sh "43 44"
./build-rpm.sh --force "44"
```

Артефакти: `build/rpms/` — `chiriksip-*.rpm`, `chiriksip-*.src.rpm`

#### DEB (Ubuntu)

Підтримуються Ubuntu 22.04 LTS (Jammy) та 24.04 LTS (Noble). PJSIP збирається з source.

```bash
./build-deb.sh "22.04 24.04"
./build-deb.sh --force "24.04"
```

Артефакти: `build/debs/` — `chiriksip_*.deb`

#### Flatpak

```bash
./build-flatpak.sh

# Або вручну
flatpak-builder --force-clean --install-deps-from=flathub \
    --repo=repo builddir com.github.chirik.ChirikSIP.yml
flatpak install --user repo com.github.chirik.ChirikSIP
flatpak run com.github.chirik.ChirikSIP
```

### Тести

```bash
cd build && ctest
```

Юніт-тести (`tests/test_parsing.cpp`):
- `testParseNumber` — 6 випадків (номер, SIP URI, display name, без @, пустий, міжнародний)
- `testParseDisplayName` — 5 випадків
- `testFormatDuration` — 8 випадків (0, 1с, 59с, 1хв, 1год, 1год1хв1с, 23:59:59, 2мін5с)

### Використання

1. Запустіть `chiriksip`
2. При першому запуску відкриється **Setup Wizard** — введіть SIP-сервер, логін, пароль
3. Або відкрийте **Settings > Settings** (Ctrl+,) та налаштуйте акаунт
4. Натисніть **Register** (або реєстрація відбудеться автоматично)
5. Набирайте номер нумпадом та натискайте **Call**
6. Для вхідних викликів натискайте **Answer**

### Клавіатурні скорочення

| Клавіша | Дія |
|---------|-----|
| 0-9 | Ввести цифру |
| * | Ввести зірочку |
| # | Ввести решітку |
| + | Ввести плюс |
| Enter | Подзвонити / Прийняти |
| Escape | Завершити виклик |
| Backspace | Видалити останню цифру |
| Ctrl+, | Відкрити налаштування |
| Ctrl+Q | Примусовий вихід |

### Поведінка кнопок

| Кнопка | Коротке натискання | Довге натискання |
|--------|-------------------|------------------|
| Hangup | Видалити цифру / Завершити виклик | Очистити / Завершити виклик |
| 0+ | Вставити "0" | Вставити "+" |

### Архітектура

![Architecture](docs/architecture.svg)

```
┌──────────────┐     ┌──────────────┐     ┌──────────────┐
│  MainWindow  │────►│ CallManager  │────►│  SipClient   │
│  (Qt UI)     │     │ (lifecycle)  │     │  (PJSIP)     │
└──────────────┘     └──────┬───────┘     └──────┬───────┘
                            │                    │
                     ┌──────▼───────┐     ┌──────▼───────┐
                     │ AudioBridge  │     │   PJSIP      │
                     │              │     │  conference  │
                     │ ┌──────────┐ │     │   bridge     │
                     │ │ Echo CXL │ │     └──────────────┘
                     │ │(pjmedia) │ │
                     │ └──────────┘ │     ┌──────────────┐
                     │ ┌──────────┐ │     │  Ringtone    │
                     │ │Ring: play│ │     │ (440Hz sine) │
                     │ │Ring: capt│ │     └──────────────┘
                     │ │Ring: ec  │ │
                     │ └──────────┘ │     ┌──────────────┐
                     │  PortAudio   │     │PortAudioMgr  │
                     │   stream     │     │(ref-counted) │
                     └──────────────┘     └──────────────┘
```

#### Потокова модель

| Потік | Компоненти | Зв'язок |
|-------|-----------|---------|
| Qt main | MainWindow, CallManager, SipClient (API), AudioBridge, SettingsDialog | Сигнали/слоти |
| PJSIP ioqueue | SipClient static callbacks | `QMetaObject::invokeMethod` → QueuedConnection |
| PortAudio | AudioBridge paCallback, Ringtone callback | SPSC ring buffers, `std::atomic` |

#### Аудіо-пайплайн

```
Мікрофон → paCallback → EC (pjmedia_echo) → captureRing → getFrame → conf bridge
conf bridge → putFrame → playbackRing → paCallback → динамік
conf bridge → putFrame → ecRefRing → paCallback → EC reference signal
```

#### Залежності між класами

```
MainWindow ──owns──> SipClient, CallManager, CallNotification, ScrollHelper
CallManager ──owns──> AudioBridge
CallManager ──uses──> SipClient (делегує SIP-операції)
SipClient ──owns──> Ringtone
AudioBridge ──uses──> PortAudioManager (ref-counted init/terminate)
Ringtone ──uses──> PortAudioManager
```

### CI/CD

GitHub Actions — автоматична збірка та публікація при push до `main`:

| Workflow | Платформа | Артефакти |
|----------|-----------|-----------|
| `build-rpm.yml` | Ubuntu + podman | RPM для Fedora 43, 44 (x86_64) |
| `build-deb.yml` | Ubuntu + podman | DEB для Ubuntu 22.04, 24.04 |
| `build-flatpak.yml` | flatpak-github-actions | Flatpak bundle |

Тригери: зміни в `src/`, `packaging/`, `debian/`, `CMakeLists.txt`, `resources/`, відповідних workflow-файлах.

**Примітка:** Workflow не запускається при змінах CHANGELOG.md та README.md.

Всі артефакти публікуються в GitHub Releases (tag `v<version>`).

### Структура проєкту

```
ChirikSIP/
├── .github/workflows/    # CI: RPM, DEB, Flatpak
├── src/                  # C++ джерела
├── tests/                # Юніт-тести
├── resources/            # .desktop, іконки
├── packaging/            # RPM spec (Fedora)
├── debian/               # DEB packaging (Ubuntu)
├── docs/                 # Архітектура, скріншоти
└── screenshots/          # Скріншоти інтерфейсу
```

### Ліцензія

MIT
