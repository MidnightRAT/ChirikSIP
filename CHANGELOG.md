# Changelog

[English](#english) | [Українська](#українська)

---

## English

### 1.0.3 (2026-07-08)

#### Documentation
- README: added Latest Release badge
- README: added Donate badge (WayForPay)
- README: restructured to match ElegooSlicer-rpm / OrcaSlicer-rpm style
- README: made CI badges clickable (link to workflow runs)
- README: added bilingual support (Ukrainian + English)
- CHANGELOG: added bilingual support (Ukrainian + English)

#### CI/CD
- All workflows: added `paths-ignore` for CHANGELOG.md and README.md to skip builds on documentation-only changes

### 1.0.2 (2026-07-03)

#### CI/CD
- RPM workflow: matrix build for Fedora 43 and Fedora 44 (was Fedora 44 only)
- DEB workflow: matrix build for Ubuntu 22.04 and 24.04 (was Ubuntu 24.04 only, also fixed missing .deb in release)
- Flatpak: publish only .flatpak bundle (removed source tarball)
- RPM: publish only binary RPMs (removed src.rpm from release)

### 1.0.1 (2026-07-02)

#### Bug Fixes
- Fixed echo cancellation reference buffer: normalized int16→float [-1,1] for correct speexdsp processing
- Fixed crash: added m_sipClient->shutdown() in MainWindow destructor
- Fixed m_registering not reset on all error paths in registerAccount()
- Fixed thread safety: atomic load of m_incomingCallId in answerCall()
- Reverted PJSIP console_level from 6 back to 4 (reduced log noise)
- Removed debug fprintf(stderr) calls left from development (15+ occurrences)
- Fixed m_accId deletion error handling: pjsua_acc_del failure no longer crashes

#### Features
- Password obfuscation: stored as base64 with "b64:" prefix (not encryption)
- Auto-migration: plaintext passwords automatically re-encoded to base64 on load
- Registration concurrency guard: prevents parallel registration attempts

### 1.0.0-19 (2026-07-01)

#### Bug Fixes
- Fixed putFrame empty loop body: restored int16→float conversion for remote audio
- Fixed SPSC ring buffer: replaced mask-and with modulo for non-power-of-2 capacity
- Fixed EC race condition: moved pjmedia_echo_playback/capture to same thread (PortAudio callback)
- Fixed AudioBridge double-close: destructor only calls close() if stream/pool exist
- Fixed PortAudioManager ref leak in AudioBridge error paths
- Fixed duplicate CallNotification signal connections on repeated incoming calls
- Fixed hangup() killing all calls: now tracks activeCallId and hangs up only that one
- Added null pointer checks in AudioBridge and Ringtone paCallback
- Added Q_ASSERT for buffer overrun detection in audio callbacks

#### Features
- Echo cancellation via pjmedia_echo with configurable aggressiveness (Conservative/Moderate/Aggressive)
- SPSC lock-free ring buffers for audio pipeline (16 frames × 160 samples)
- PortAudio latency increased 4x for smoother audio
- CallManager service layer between MainWindow and SipClient
- Call duration displayed centered on second display line during active call
- Unit tests for parseNumber, parseDisplayName, formatDuration
- Architecture diagram added to README

### 1.0.0-18 (2026-07-01)

#### Bug Fixes
- Fixed audio disappearing after the first call: PortAudio lifecycle moved to SipClient (init/shutdown only)
- Fixed PortAudioManager refCount going negative from multiple terminate() calls
- Fixed clock overwriting dialed number every second while dialing
- Fixed Hangup button and Backspace key erasing clock digits when not in dialing mode

#### Features
- Clock disappears on first keypress during dialing, showing dialed number right-aligned
- Dialed number persists on display until end of call
- Incoming caller number displayed right-aligned during incoming call
- Call duration shown centered on second display line during active call

### 1.0.0-17 (2026-07-01)

#### Bug Fixes
- Removed i686 build from Windows workflow (Qt6 not available for 32-bit in MSYS2)
- Disabled GitHub Actions workflows for Linux and Windows builds

#### Features
- Clock centered on first display line
- Setup Wizard accessible from Settings menu
- About dialog shows build date and time
- Call duration timer (HH:MM:SS) centered in status bar during calls

### 1.0.0-16 (2026-07-01)

#### Features
- Clock centered on first display line
- Setup Wizard accessible from Settings menu
- About dialog shows build date and time
- Fixed RPM changelog date warnings

### 1.0.0-15 (2026-07-01)

#### Bug Fixes
- Fixed audio disappearing when port was set to 0
- Fixed port label not updating after settings change
- Port label now shows effective port (5060 when configured as 0)

#### Features
- Display current time (HH:mm:ss) on first line of display when idle
- Show call duration timer (HH:MM:SS) centered in status bar during calls

### 1.0.0-14 (2026-07-01)

#### Features
- Configurable SIP port in Setup Wizard and Settings dialog (default: 0 = auto-select)
- Dynamic port selection: port 0 assigns random available port on each launch
- Status bar shows current transport port (e.g. `UDP:50600`) right-aligned
- Removed dead code: ensurePortBound() and orphaned makeCall block in sipclient.cpp

### 1.0.0-12 (2026-06-28)

#### Code Review Fixes
- Fixed data race in AudioBridge: memory_order_relaxed → release/acquire for capture/playback atomic stores
- Fixed PortAudioManager refcount leak: AudioBridge::close() and Ringtone::stop() now call terminate()
- Fixed m_incomingCallId not reset when remote party hangs up before answer
- Fixed AudioBridge::close() ordering: stop stream before removing conf port
- Fixed keyPressEvent allows input during active call
- Fixed onCallState/onCallMediaState: validate acc_id != PJSUA_INVALID_ID before getting user data
- Fixed CMakeLists.txt: use PkgConfig::PJSIP imported target and pkg-config for portaudio
- Fixed SetupWizard: "Next" button changes to "Finish" on last input page
- Fixed User-Agent version: uses PROJECT_VERSION instead of hardcoded "1.0.0"
- Added system tray availability check before creating QSystemTrayIcon
- Fixed m_trayIcon uninitialized pointer (now nullptr)
- Fixed _USE_MATH_DEFINES: only defined on MSVC
- Fixed SetupWizard: Enter triggers Next/Finish button and focus moves to active field

### 1.0.0-11 (2026-06-28)

#### Code Review Fixes
- Fixed potential null pointer dereference in SipClient::onCallMediaState(): check `self` before invoking lambda
- Fixed About dialog version string: now reads from CMake `PROJECT_VERSION` instead of hardcoded "1.0.0"
- Added `target_compile_definitions(PROJECT_VERSION)` so source always matches CMake version
- Removed unused `#include <pjnath.h>` from sipclient.h
- Updated README RPM section: use version from spec, `rpmbuild -bs` for src.rpm only, exclude `.mimocode`

### 1.0.0-10 (2026-06-28)

#### CI/CD
- Simplified Linux CI: ubuntu-latest without container for GitHub Actions compatibility

### 1.0.0-9 (2026-06-28)

#### CI/CD
- Added GitHub Actions workflow for Linux RPM builds (Fedora 42 container)
- Added GitHub Actions workflow for cmake build verification on Linux
- Updated Windows build workflow triggers for dev-ghaction and main branches
- Fixed container image: Fedora 44 → Fedora 42 for GitHub Actions compatibility

### 1.0.0-8 (2026-06-28)

#### Bug Fixes
- Fixed null pointer dereference in AudioBridge::open() when no audio device available
- Fixed PortAudio lifecycle: Ringtone and AudioBridge no longer call Pa_Terminate() prematurely; lifecycle managed by SipClient::shutdown()
- Fixed data race in Ringtone: m_playing and m_phase are now std::atomic
- Fixed race condition in onCallMediaState: skip AudioBridge creation if call is already disconnected
- Fixed ScrollHelper::stop() not restoring original text after scroll
- Fixed CallNotification::onAnswer() not handling answerCall() failure
- Added delay before PortAudioManager::terminate() to ensure callback threads finish
- Restricted config file permissions to 0600 (owner-only) for password security

#### Technical
- AudioBridge::open() validates Pa_GetDefaultInputDevice/OutputDevice and Pa_GetDeviceInfo() before use
- PortAudioManager::terminate() now only called from SipClient::shutdown()
- Added QThread::msleep(50) gap between stream close and Pa_Terminate()

### 1.0.0-7 (2026-06-28)

#### Features
- SIP registration with digest authentication (realm wildcard)
- Outgoing calls via numpad or SIP URI
- Incoming calls with ringtone (440Hz sine wave, 1s on / 3s off)
- Audio bridge: PortAudio <-> PJSIP conference bridge
- Phone-style numpad UI (123456789*0#)
- LCD-style two-line display with Segment16A digital font
- Scrolling caller name (static if fits)
- Setup wizard on first launch
- Settings in separate dialog (Settings > Settings, Ctrl+,)
- Auto re-registration when settings change (server/user/password)
- System tray: minimize to tray on close (X/Alt+F4), only Ctrl+Q exits
- Tray icon with right-click menu (Restore, Exit)
- Incoming call popup notification when minimized to tray
- Hangup button rejects incoming calls
- Settings persistence in `%APPDATA%/chiriksip/chiriksip.ini` (Windows) or `~/.config/chiriksip/chiriksip.conf` (Linux)
- Auto-registration on startup
- Menu bar: File (Exit), Settings, Help (About)
- Embedded icon via Qt resources (.qrc)
- .desktop file with hicolor icons (16–256px) on Linux
- Keyboard support: 0-9, *, #, +, Enter, Escape, Backspace
- Button "0+": short press = "0", long press = "+"
- Hangup button: short press = delete last digit / end call, long press = clear all / end call
- G.711 A-law (PCMA) and u-law (PCMU) codecs only
- Windows cross-compilation: MinGW32 and MinGW64 builds
- Windows executable: GUI app (no console window)
- Windows .ico icon embedded in .exe
- PortAudioManager: ref-counted init/term with mutex (#83fcb2d)
- ScrollHelper: smooth text scrolling for long caller names (#1eedaff)
- CallNotification: popup dialog with Answer/Reject buttons (#147422b)
- GitHub Actions CI for Windows builds (#a72b4b7)
- Docker + cross-compile scripts for MinGW (#aa054ef, #5d5e837)

#### Bug Fixes
- Fixed SIP URI construction for phone numbers (auto-append @server)
- Fixed call state mapping (pjsip_inv_state enum)
- Fixed remote hangup detection (DISCONNCTD state)
- Fixed PortAudio device enumeration (Fedora pjproject missing audio driver)
- Fixed audio feedback loop (on-demand PortAudio stream)
- Fixed thread safety for AudioBridge creation
- Fixed Hangup button state after call ends
- Fixed display text alignment and scrolling
- Fixed Windows icon loading and tray icon setup (#446e5d4)
- Fixed close-to-tray vs force-quit logic on Windows (#446e5d4)
- Fixed build artifacts tracked in git (#d1753e8)

#### Technical
- Built with Qt6, PJSIP 2.13.1, PortAudio 19.7
- Targets KDE Plasma on Fedora 44
- C++17, CMake 3.20+
- RPM packaging with hicolor icons and .desktop validation
- Cross-compilation: MinGW-w64 (x86_64 + i686) via Docker
- RPM spec in `packaging/chiriksip.spec`

---

## Українська

### 1.0.3 (2026-07-08)

#### Документація
- README: додано бейдж останнього релізу
- README: додано бейдж донату (WayForPay)
- README: переструктуровано за зразком ElegooSlicer-rpm / OrcaSlicer-rpm
- README: CI-бейджі зроблено клікабельними (посилаються на workflow runs)
- README: додано підтримку двох мов (українська + англійська)
- CHANGELOG: додано підтримку двох мов (українська + англійська)

#### CI/CD
- Усі workflow: додано `paths-ignore` для CHANGELOG.md та README.md, щоб пропускати збірки при зміні лише документації

### 1.0.2 (2026-07-03)

#### CI/CD
- RPM workflow: матрична збірка для Fedora 43 та Fedora 44 (була тільки Fedora 44)
- DEB workflow: матрична збірка для Ubuntu 22.04 та 24.04 (була тільки Ubuntu 24.04, також виправлено відсутність .deb у релізі)
- Flatpak: публікація тільки .flatpak bundle (видалено source tarball)
- RPM: публікація тільки бінарних RPM (видалено src.rpm з релізу)

### 1.0.1 (2026-07-02)

#### Виправлення помилок
- Виправлено буфер еталонного сигналу echo cancellation: нормалізовано int16→float [-1,1] для коректної обробки speexdsp
- Виправлено краш: додано m_sipClient->shutdown() у деструкторі MainWindow
- Виправлено m_registering, що не скидався на всіх шляхах помилок у registerAccount()
- Виправлено потокову безпеку: atomic load m_incomingCallId у answerCall()
- Повернено PJSIP console_level з 6 на 4 (зменшено шум логів)
- Видалено debug-виклики fprintf(stderr), що лишилися з розробки (15+ випадків)
- Виправлено обробку помилок видалення m_accId: помилка pjsua_acc_del більше не викликає краш

#### Можливості
- Обфускація паролів: зберігаються як base64 з префіксом "b64:" (не шифрування)
- Автоматична міграція: plaintext-паролі автоматично перекодовуються у base64 при завантаженні
- Захист від паралельної реєстрації: запобігає одночасним спробам реєстрації

### 1.0.0-19 (2026-07-01)

#### Виправлення помилок
- Виправлено порожнє тіло циклу putFrame: відновлено конвертацію int16→float для віддаленого аудіо
- Виправлено SPSC ring buffer: замінено mask-and на modulo для місткості, що не є степенем 2
- Виправлено умову гонки EC: перенесено pjmedia_echo_playback/capture в один потік (PortAudio callback)
- Виправлено подвійне закриття AudioBridge: деструктор викликає close() тільки якщо stream/pool існують
- Виправлено витік refcount PortAudioManager на шляхах помилок AudioBridge
- Виправлено дублювання з'єднань сигналів CallNotification при повторних вхідних викликах
- Виправлено hangup(), що завершував усі виклики: тепер відстежує activeCallId і завершує тільки його
- Додано перевірки на нульовий вказівник у AudioBridge та Ringtone paCallback
- Додано Q_ASSERT для виявлення перевищення буфера в аудіо-колбеках

#### Можливості
- Echo cancellation через pjmedia_echo з налаштовуваною агресивністю (Conservative/Moderate/Aggressive)
- SPSC lock-free ring buffers для аудіо-пайплайну (16 фреймів × 160 сэмплів)
- Затримка PortAudio збільшена в 4 рази для плавнішого аудіо
- Сервісний шар CallManager між MainWindow та SipClient
- Тривалість виклику відображається по центру на другому рядку дисплея під час активного виклику
- Юніт-тести для parseNumber, parseDisplayName, formatDuration
- Діаграму архітектури додано до README

### 1.0.0-18 (2026-07-01)

#### Виправлення помилок
- Виправлено зникнення аудіо після першого виклику: життєвий цикл PortAudio перенесено до SipClient (тільки init/shutdown)
- Виправлено від'ємний refCount PortAudioManager від множинних викликів terminate()
- Виправлено годинник, що перезаписував набраний номер щосекунди під час набору
- Виправлено кнопку Hangup та Backspace, що стирайали цифри годинника поза режимом набору

#### Можливості
- Годинник зникає при першому натисканні під час набору, показуючи набраний номер праворуч
- Набраний номер зберігається на дисплеї до кінця виклику
- Номер вхідного абонента відображається праворуч під час вхідного виклику
- Тривалість виклику показується по центру на другому рядку під час активного виклику

### 1.0.0-17 (2026-07-01)

#### Виправлення помилок
- Видалено збірку i686 з Windows workflow (Qt6 недоступний для 32-біт в MSYS2)
- Вимкнено GitHub Actions workflow для Linux та Windows збірок

#### Можливості
- Годинник центровано на першому рядку дисплея
- Майстер першого запуску доступний з меню Settings
- Діалог About показує дату та час збірки
- Таймер тривалості виклику (HH:MM:SS) центровано в статус-барі під час викликів

### 1.0.0-16 (2026-07-01)

#### Можливості
- Годинник центровано на першому рядку дисплея
- Майстер першого запуску доступний з меню Settings
- Діалог About показує дату та час збірки
- Виправлено попередження про дату в RPM changelog

### 1.0.0-15 (2026-07-01)

#### Виправлення помилок
- Виправлено зникнення аудіо, коли порт був встановлений на 0
- Виправлено мітку порту, що не оновлювалася після зміни налаштувань
- Мітка порту тепер показує ефективний порт (5060 при налаштуванні 0)

#### Можливості
- Відображення поточного часу (HH:mm:ss) на першому рядку дисплея в режимі простою
- Таймер тривалості виклику (HH:MM:SS) по центру в статус-барі під час викликів

### 1.0.0-14 (2026-07-01)

#### Можливості
- Налаштовуваний SIP-порт у майстрі першого запуску та діалозі налаштувань (за замовчуванням: 0 = авто-вибір)
- Динамічний вибір порту: порт 0 призначає випадковий доступний порт при кожному запуску
- Статус-бар показує поточний порт транспорту (наприклад `UDP:50600`) праворуч
- Видалено мертвий код: ensurePortBound() та осиротілий блок makeCall у sipclient.cpp

### 1.0.0-12 (2026-06-28)

#### Виправлення після code review
- Виправлено гонку даних в AudioBridge: memory_order_relaxed → release/acquire для capture/playback atomic stores
- Виправлено витік refcount PortAudioManager: AudioBridge::close() та Ringtone::stop() тепер викликають terminate()
- Виправлено m_incomingCallId, що не скидався, коли віддалена сторона завершила виклик до відповіді
- Виправлено порядок AudioBridge::close(): зупинка потоку перед видаленням conf port
- Виправлено keyPressEvent, що дозволяв ввід під час активного виклику
- Виправлено onCallState/onCallMediaState: перевірка acc_id != PJSUA_INVALID_ID перед отриманням user data
- Виправлено CMakeLists.txt: використання PkgConfig::PJSIP imported target та pkg-config для portaudio
- Виправлено SetupWizard: кнопка "Next" змінюється на "Finish" на останній сторінці введення
- Виправлено версію User-Agent: використовує PROJECT_VERSION замість захардкодженої "1.0.0"
- Додано перевірку наявності системного трею перед створенням QSystemTrayIcon
- Виправлено неініціалізований вказівник m_trayIcon (тепер nullptr)
- Виправлено _USE_MATH_DEFINES: визначається тільки на MSVC
- Виправлено SetupWizard: Enter запускає кнопку Next/Finish і фокус переміщується на активне поле

### 1.0.0-11 (2026-06-28)

#### Виправлення після code review
- Виправлено можливий розіменування нульового вказівника у SipClient::onCallMediaState(): перевірка `self` перед викликом lambda
- Виправлено рядок версії в діалозі About: тепер зчитується з CMake `PROJECT_VERSION` замість захардкодженої "1.0.0"
- Додано `target_compile_definitions(PROJECT_VERSION)`, щоб джерело завжди відповідало версії CMake
- Видалено невикористовуваний `#include <pjnath.h>` з sipclient.h
- Оновлено секцію RPM в README: версія з spec, `rpmbuild -bs` тільки для src.rpm, виключення `.mimocode`

### 1.0.0-10 (2026-06-28)

#### CI/CD
- Спрощено Linux CI: ubuntu-latest без контейнера для сумісності з GitHub Actions

### 1.0.0-9 (2026-06-28)

#### CI/CD
- Додано GitHub Actions workflow для Linux RPM збірок (контейнер Fedora 42)
- Додано GitHub Actions workflow для перевірки збірки cmake на Linux
- Оновлено тригери Windows build workflow для гілок dev-ghaction та main
- Виправлено образ контейнера: Fedora 44 → Fedora 42 для сумісності з GitHub Actions

### 1.0.0-8 (2026-06-28)

#### Виправлення помилок
- Виправлено розіменування нульового вказівника у AudioBridge::open() за відсутності аудіо-пристрою
- Виправлено життєвий цикл PortAudio: Ringtone та AudioBridge більше не викликають Pa_Terminate() передчасно; життєвий цикл керується SipClient::shutdown()
- Виправлено гонку даних у Ringtone: m_playing та m_phase тепер std::atomic
- Виправлено умову гонки в onCallMediaState: пропуск створення AudioBridge, якщо виклик вже розірвано
- Виправлено ScrollHelper::stop(), що не відновлював оригінальний текст після скролу
- Виправлено CallNotification::onAnswer(), що не обробляв помилку answerCall()
- Додано затримку перед PortAudioManager::terminate() для завершення потоків колбеків
- Обмежено права config-файлу до 0600 (тільки власник) для безпеки паролів

#### Технічне
- AudioBridge::open() перевіряє Pa_GetDefaultInputDevice/OutputDevice та Pa_GetDeviceInfo() перед використанням
- PortAudioManager::terminate() тепер викликається тільки з SipClient::shutdown()
- Додано QThread::msleep(50) між закриттям потоку та Pa_Terminate()

### 1.0.0-7 (2026-06-28)

#### Можливості
- Реєстрація SIP з digest-аутентифікацією (wildcard realm)
- Вихідні виклики через нумпад або SIP URI
- Вхідні виклики з рингтоном (синусоїда 440Hz, 1с увімкнено / 3с вимкнено)
- Аудіо-міст: PortAudio <-> PJSIP conference bridge
- Телефонний нумпад (123456789*0#)
- LCD-дисплей на два рядки з шрифтом Segment16A
- Скрол імені викликача (стаціонарний, якщо вміщується)
- Майстер першого запуску
- Налаштування в окремому діалозі (Settings > Settings, Ctrl+,)
- Автоматична ре-реєстрація при зміні налаштувань (сервер/користувач/пароль)
- Системний трей: мінімізація при закритті (X/Alt+F4), вихід тільки через Ctrl+Q
- Іконка трею з контекстним меню (Restore, Exit)
- Popup-сповіщення при вхідному виклику при згорнутому вікні
- Кнопка Hangup відхиляє вхідні виклики
- Збереження налаштувань у `%APPDATA%/chiriksip/chiriksip.ini` (Windows) або `~/.config/chiriksip/chiriksip.conf` (Linux)
- Автоматична реєстрація при запуску
- Меню: File (Exit), Settings, Help (About)
- Вбудована іконка через Qt resources (.qrc)
- .desktop файл з іконками hicolor (16–256px) на Linux
- Підтримка клавіатури: 0-9, *, #, +, Enter, Escape, Backspace
- Кнопка "0+": коротке натискання = "0", довге = "+"
- Кнопка Hangup: коротке натискання = видалити цифру / завершити виклик, довге = очистити / завершити виклик
- Тільки кодеки G.711 A-law (PCMA) та u-law (PCMU)
- Крос-компіляція Windows: збірки MinGW32 та MinGW64
- Windows executable: GUI-додаток (без консольного вікна)
- Windows .ico іконка вбудована в .exe
- PortAudioManager: ref-counted init/term з mutex (#83fcb2d)
- ScrollHelper: плавний скрол тексту для довгих імен викликачів (#1eedaff)
- CallNotification: popup-діалог з кнопками Answer/Reject (#147422b)
- GitHub Actions CI для Windows збірок (#a72b4b7)
- Docker + скрипти крос-компіляції для MinGW (#aa054ef, #5d5e837)

#### Виправлення помилок
- Виправлено конструювання SIP URI для номерів телефонів (автоматичне додавання @server)
- Виправлено маппинг стану виклику (enum pjsip_inv_state)
- Виправлено виявлення завершення виклику віддаленою стороною (стан DISCONNCTD)
- Виправлено перелік пристроїв PortAudio (Fedora pjproject без аудіо-драйвера)
- Виправлено петлю аудіо-зворотного зв'язку (PortAudio потік на вимогу)
- Виправлено потокову безпеку створення AudioBridge
- Виправлено стан кнопки Hangup після завершення виклику
- Виправлено вирівнювання та скрол тексту на дисплеї
- Виправлено завантаження іконки Windows та налаштування іконки трею (#446e5d4)
- Виправлено логіку згортання в трей vs примусового виходу на Windows (#446e5d4)
- Виправлено build-артефакти, що відстежувалися в git (#d1753e8)

#### Технічне
- Зібрано на Qt6, PJSIP 2.13.1, PortAudio 19.7
- Цільова платформа: KDE Plasma на Fedora 44
- C++17, CMake 3.20+
- RPM packaging з іконками hicolor та валідацією .desktop
- Крос-компіляція: MinGW-w64 (x86_64 + i686) через Docker
- RPM spec у `packaging/chiriksip.spec`
