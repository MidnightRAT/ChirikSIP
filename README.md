# ChirikSIP

A minimal SIP client for KDE Plasma, built with Qt6 and PJSIP.

## Features

- SIP registration with digest authentication
- Outgoing calls (dial by number or SIP URI)
- Incoming calls with ringtone and Answer button
- Audio bridge via PortAudio (works with PipeWire/PulseAudio)
- Phone-style numpad UI (123456789*0#)
- LCD display with Segment16A digital font
- Caller name display with scrolling text
- Settings in separate dialog (Ctrl+,)
- Settings persistence (server, username, password)
- Auto-registration on startup
- System tray-compatible window icon
- Keyboard support: 0-9, *, #, +, Enter, Escape, Backspace
- G.711 A-law (PCMA) and G.711 u-law (PCMU) codecs only

## Build Dependencies

| Package | Purpose |
|---------|---------|
| `cmake >= 3.20` | Build system |
| `gcc-c++` | C++17 compiler |
| `qt6-qtbase-devel` | Qt6 Core, Widgets |
| `pkgconfig(libpjproject)` | PJSIP SIP stack |
| `portaudio-devel` | Audio I/O |
| `desktop-file-utils` | .desktop file validation |
| `hicolor-icon-theme` | Icon installation |

## Runtime Dependencies

| Package | Purpose |
|---------|---------|
| `qt6-qtbase` | Qt6 runtime |
| `qt6-qtbase-gui` | Qt6 GUI |
| `pjproject` | SIP/audio stack |
| `portaudio` | Audio device access |
| `hicolor-icon-theme` | System icons |

## Build

```bash
mkdir build && cd build
cmake ..
make
```

## Install

```bash
cmake --install build
```

## RPM Build

```bash
rpmbuild -ba packaging/chiriksip.spec
```

## Usage

1. Launch `chiriksip`
2. Open **Settings > Settings** (Ctrl+,) and enter SIP server, username, password
3. Click **Register** (or it registers automatically if settings are saved)
4. Dial a number using the numpad and press **Call**
5. For incoming calls, press **Answer**

## Keyboard Shortcuts

| Key | Action |
|-----|--------|
| 0-9 | Input digit |
| * | Input asterisk |
| # | Input hash |
| + | Input plus |
| Enter | Make call / Answer |
| Escape | End call |
| Backspace | Delete last digit |
| Ctrl+, | Open Settings |

## Button Behavior

| Button | Short Press | Long Press |
|--------|-------------|------------|
| Hangup | Delete last digit / End call | Clear all / End call |
| 0+ | Insert "0" | Insert "+" |

## License

MIT
