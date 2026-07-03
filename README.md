# ChirikSIP

Мінімалістичний SIP-клієнт для KDE Plasma, побудований на Qt6 та PJSIP.

![Build RPM](https://github.com/MidnightRAT/ChirikSIP/actions/workflows/build-rpm.yml/badge.svg)
![Build DEB](https://github.com/MidnightRAT/ChirikSIP/actions/workflows/build-deb.yml/badge.svg)
![Build Flatpak](https://github.com/MidnightRAT/ChirikSIP/actions/workflows/build-flatpak.yml/badge.svg)

![Linux Main Window](screenshots/Linux_dark_main.png)

Детальніші скріншоти див. у [screenshots/README.md](screenshots/README.md)

## Можливості

### SIP
- Реєстрація з digest-аутентифікацією
- Вихідні виклики за номером або SIP URI
- Вхідні виклики з рингтоном та кнопкою Answer
- Автоматична реєстрація при запуску
- Повторна реєстрація при зміні налаштувань
- Кодеки: G.711 A-law (PCMA) та G.711 u-law (PCMU)
- Налаштування SIP-порту (0 = авто-вибір)

### Аудіо
- Міст PortAudio ↔ PJSIP conference bridge
- Echo cancellation через `pjmedia_echo` (Conservative / Moderate / Aggressive)
- SPSC lock-free ring buffers (16 frames × 160 samples)
- Підтримка PipeWire, PulseAudio, ALSA

### Інтерфейс
- LCD-дисплей з шрифтом Segment16A
- Нумпад у стилі телефону (123456789*0#)
- Годинник (HH:mm:ss) на дисплеї, коли не викликаєш
- Тривалість виклику (HH:MM:SS) у статус-барі
- Номер власника (центрований, жирний) у статус-барі
- Поточний порт транспорту (наприклад `UDP:50600`) праворуч у статус-барі
- Скрол довгих імен викликача

### Системний трей
- Мінімізація в трей при закритті (X / Alt+F4)
- Вхідний виклик — popup-сповіщення при згорнутому вікні
- Вихід тільки через Ctrl+Q або меню трей

### Налаштування
- Майстер першого запуску (4 сторінки)
- Окремий діалог налаштувань (Ctrl+,)
- Поля: сервер, користувач, пароль, порт, echo cancellation
- Налаштування зберігаються у `~/.config/chiriksip/`

### Безпека
- Паролі зберігаються як base64 (обфускація, не шифрування)
- Автоматична міграція plaintext → base64 при першому завантаженні
- Права config-файлу обмежені 0600 (тільки власник)
- Single instance через D-Bus

## Архітектура

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

### Потокова модель

| Потік | Компоненти | Зв'язок |
|-------|-----------|---------|
| Qt main | MainWindow, CallManager, SipClient (API), AudioBridge, SettingsDialog | Сигнали/слоти |
| PJSIP ioqueue | SipClient static callbacks | `QMetaObject::invokeMethod` → QueuedConnection |
| PortAudio | AudioBridge paCallback, Ringtone callback | SPSC ring buffers, `std::atomic` |

### Аудіо-пайплайн

```
Мікрофон → paCallback → EC (pjmedia_echo) → captureRing → getFrame → conf bridge
conf bridge → putFrame → playbackRing → paCallback → динамік
conf bridge → putFrame → ecRefRing → paCallback → EC reference signal
```

### Залежності між класами

```
MainWindow ──owns──> SipClient, CallManager, CallNotification, ScrollHelper
CallManager ──owns──> AudioBridge
CallManager ──uses──> SipClient (делегує SIP-операції)
SipClient ──owns──> Ringtone
AudioBridge ──uses──> PortAudioManager (ref-counted init/terminate)
Ringtone ──uses──> PortAudioManager
```

## Використання

1. Запустіть `chiriksip`
2. При першому запуску відкриється **Setup Wizard** — введіть SIP-сервер, логін, пароль
3. Або відкрийте **Settings > Settings** (Ctrl+,) та налаштуйте акаунт
4. Натисніть **Register** (або реєстрація відбудеться автоматично)
5. Набирайте номер нумпадом та натискайте **Call**
6. Для вхідних викликів натискайте **Answer**

## Клавіатурні скорочення

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

## Поведінка кнопок

| Кнопка | Коротке натискання | Довге натискання |
|--------|-------------------|------------------|
| Hangup | Видалити цифру / Завершити виклик | Очистити / Завершити виклик |
| 0+ | Вставити "0" | Вставити "+" |

## Збірка з джерел

### Залежності (Fedora)

| Пакет | Призначення |
|-------|-------------|
| `cmake >= 3.20` | Build system |
| `gcc-c++` | C++17 компілятор |
| `qt6-qtbase-devel` | Qt6 Core, Widgets, DBus |
| `pkgconfig(libpjproject)` | PJSIP SIP-стек |
| `portaudio-devel` | Аудіо I/O |
| `desktop-file-utils` | Валідація .desktop файлу |
| `hicolor-icon-theme` | Встановлення іконок |

### Залежності (runtime)

| Пакет | Призначення |
|-------|-------------|
| `qt6-qtbase` | Qt6 runtime |
| `qt6-qtbase-gui` | Qt6 GUI |
| `pjproject` | SIP/audio стек |
| `portaudio` | Аудіо-пристрої |
| `hicolor-icon-theme` | Системні іконки |

### Збірка

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

### Встановлення

```bash
cmake --install build
```

## RPM Build (Fedora)

### Локальна збірка

```bash
VERSION=$(grep 'Version:' packaging/chiriksip.spec | awk '{print $2}')
tar czf ~/rpmbuild/SOURCES/chiriksip-${VERSION}.tar.gz \
    --transform "s,^,chiriksip-${VERSION}/," \
    -X .rpmignore .

# Тільки source RPM:
rpmbuild -bs packaging/chiriksip.spec

# Source + binary RPM (потрібні всі залежності):
rpmbuild -ba packaging/chiriksip.spec
```

### Збірка в контейнері (podman/docker)

```bash
# Fedora 43
./build-rpm.sh "43"

# Fedora 43 + 44
./build-rpm.sh "43 44"

# З rpmfusion
./build-rpm.sh --rpmfusion "40 41 42"

# Примусова перезбірка
./build-rpm.sh --force "44"
```

Артефакти: `build/rpms/` — `chiriksip-*.rpm`, `chiriksip-*.src.rpm`

Опції: `--rpmfusion`, `--force`, `-h/--help`

## DEB Build (Ubuntu)

### Збірка в контейнері (podman/docker)

Підтримуються Ubuntu 22.04 LTS (Jammy) та 24.04 LTS (Noble). PJSIP збирається з source.

```bash
# Ubuntu 22.04
./build-deb.sh "22.04"

# Ubuntu 22.04 + 24.04
./build-deb.sh "22.04 24.04"

# Примусова перезбірка
./build-deb.sh --force "24.04"
```

Артефакти: `build/debs/` — `chiriksip_*.deb`

Опції: `--force`, `-h/--help`

## Flatpak Build

### Локальна збірка

```bash
./build-flatpak.sh

# Або вручну
flatpak-builder --force-clean --install-deps-from=flathub \
    --repo=repo builddir com.github.chirik.ChirikSIP.yml
flatpak install --user repo com.github.chirik.ChirikSIP
flatpak run com.github.chirik.ChirikSIP
```

### З Flathub

```bash
flatpak install flathub com.github.chirik.ChirikSIP
flatpak run com.github.chirik.ChirikSIP
```

## Тести

```bash
cd build && ctest
```

Юніт-тести (`tests/test_parsing.cpp`):
- `testParseNumber` — 6 випадків (номер, SIP URI, display name, без @, пустий, міжнародний)
- `testParseDisplayName` — 5 випадків
- `testFormatDuration` — 8 випадків (0, 1с, 59с, 1хв, 1год, 1год1хв1с, 23:59:59, 2мін5с)

## CI/CD

GitHub Actions — автоматична збірка та публікація при push до `main`:

| Workflow | Тригери | Платформа | Артефакти |
|----------|---------|-----------|-----------|
| `build-rpm.yml` | push/PR → `main` | Ubuntu + podman | RPM для Fedora 43, 44 (x86_64) |
| `build-deb.yml` | push/PR → `main` | Ubuntu + podman | DEB для Ubuntu 22.04, 24.04 |
| `build-flatpak.yml` | push/PR → `main` | flatpak-github-actions | Flatpak bundle |

Тригери: зміни в `src/`, `packaging/`, `debian/`, `CMakeLists.txt`, `resources/`, відповідних workflow-файлах.

Всі артефакти публікуються в GitHub Releases (tag `v<version>`).

## Структура проєкту

```
src/
  main.cpp                — точка входу, D-Bus single instance
  mainwindow.h/.cpp       — головне вікно (UI + логіка)
  sipclient.h/.cpp        — обгортка PJSIP
  callmanager.h/.cpp      — service layer для викликів
  audiobridge.h/.cpp      — міст PortAudio ↔ PJSIP conference bridge
  ringtone.h/.cpp         — генерація рингтону (440 Hz)
  callnotification.h/.cpp — popup вхідного виклику
  scrollhelper.h/.cpp     — горизонтальний скрол тексту
  settingsdialog.h/.cpp   — діалог налаштувань
  setupwizard.h/.cpp      — майстер першого запуску
  portaudio_manager.h     — ref-counted обгортка PortAudio
tests/
  test_parsing.cpp        — юніт-тести
resources/
  chiriksip.desktop       — XDG desktop entry
  icons/                  — іконки 16x16 ... 256x256
packaging/
  chiriksip.spec          — RPM spec (Fedora)
debian/
  changelog, control, rules — DEB packaging (Ubuntu)
.github/workflows/
  build-rpm.yml           — CI: RPM
  build-deb.yml           — CI: DEB
  build-flatpak.yml       — CI: Flatpak
```

## Ліцензія

MIT
