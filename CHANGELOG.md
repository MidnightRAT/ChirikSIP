## 1.0.0-18 (2026-07-01)

### Bug Fixes
- Fixed audio disappearing after the first call: PortAudio lifecycle moved to SipClient (init/shutdown only)
- Fixed PortAudioManager refCount going negative from multiple terminate() calls
- Fixed clock overwriting dialed number every second while dialing
- Fixed Hangup button and Backspace key erasing clock digits when not in dialing mode

### Features
- Clock disappears on first keypress during dialing, showing dialed number right-aligned
- Dialed number persists on display until end of call
- Incoming caller number displayed right-aligned during incoming call
- Call duration shown centered on second display line during active call

## 1.0.0-17 (2026-07-01)

### Bug Fixes
- Removed i686 build from Windows workflow (Qt6 not available for 32-bit in MSYS2)
- Disabled GitHub Actions workflows for Linux and Windows builds

### Features
- Clock centered on first display line
- Setup Wizard accessible from Settings menu
- About dialog shows build date and time
- Call duration timer (HH:MM:SS) centered in status bar during calls

## 1.0.0-16 (2026-07-01)

### Features
- Clock centered on first display line
- Setup Wizard accessible from Settings menu
- About dialog shows build date and time
- Fixed RPM changelog date warnings

## 1.0.0-15 (2026-07-01)

### Bug Fixes
- Fixed audio disappearing when port was set to 0
- Fixed port label not updating after settings change
- Port label now shows effective port (5060 when configured as 0)

### Features
- Display current time (HH:mm:ss) on first line of display when idle
- Show call duration timer (HH:MM:SS) centered in status bar during calls

## 1.0.0-14 (2026-07-01)

### Features
- Configurable SIP port in Setup Wizard and Settings dialog (default: 0 = auto-select)
- Dynamic port selection: port 0 assigns random available port on each launch
- Status bar shows current transport port (e.g. `UDP:50600`) right-aligned
- Removed dead code: ensurePortBound() and orphaned makeCall block in sipclient.cpp

## 1.0.0-12 (2026-06-28)

### Code Review Fixes
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

## 1.0.0-11 (2026-06-28)

### Code Review Fixes
- Fixed potential null pointer dereference in SipClient::onCallMediaState(): check `self` before invoking lambda
- Fixed About dialog version string: now reads from CMake `PROJECT_VERSION` instead of hardcoded "1.0.0"
- Added `target_compile_definitions(PROJECT_VERSION)` so source always matches CMake version
- Removed unused `#include <pjnath.h>` from sipclient.h
- Updated README RPM section: use version from spec, `rpmbuild -bs` for src.rpm only, exclude `.mimocode`

## 1.0.0-10 (2026-06-28)

### CI/CD
- Simplified Linux CI: ubuntu-latest without container for GitHub Actions compatibility

## 1.0.0-9 (2026-06-28)

### CI/CD
- Added GitHub Actions workflow for Linux RPM builds (Fedora 42 container)
- Added GitHub Actions workflow for cmake build verification on Linux
- Updated Windows build workflow triggers for dev-ghaction and main branches
- Fixed container image: Fedora 44 → Fedora 42 for GitHub Actions compatibility

## 1.0.0-8 (2026-06-28)

### Bug Fixes
- Fixed null pointer dereference in AudioBridge::open() when no audio device available
- Fixed PortAudio lifecycle: Ringtone and AudioBridge no longer call Pa_Terminate() prematurely; lifecycle managed by SipClient::shutdown()
- Fixed data race in Ringtone: m_playing and m_phase are now std::atomic
- Fixed race condition in onCallMediaState: skip AudioBridge creation if call is already disconnected
- Fixed ScrollHelper::stop() not restoring original text after scroll
- Fixed CallNotification::onAnswer() not handling answerCall() failure
- Added delay before PortAudioManager::terminate() to ensure callback threads finish
- Restricted config file permissions to 0600 (owner-only) for password security

### Technical
- AudioBridge::open() validates Pa_GetDefaultInputDevice/OutputDevice and Pa_GetDeviceInfo() before use
- PortAudioManager::terminate() now only called from SipClient::shutdown()
- Added QThread::msleep(50) gap between stream close and Pa_Terminate()

## 1.0.0-7 (2026-06-28)

### Features
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

### Bug Fixes
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

### Technical
- Built with Qt6, PJSIP 2.13.1, PortAudio 19.7
- Targets KDE Plasma on Fedora 44
- C++17, CMake 3.20+
- RPM packaging with hicolor icons and .desktop validation
- Cross-compilation: MinGW-w64 (x86_64 + i686) via Docker
- RPM spec in `packaging/chiriksip.spec`
