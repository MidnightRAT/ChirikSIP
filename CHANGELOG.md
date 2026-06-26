## 1.0.0 (2026-06-26)

### Features
- SIP registration with digest authentication (realm wildcard)
- Outgoing calls via numpad or SIP URI
- Incoming calls with ringtone (440Hz sine wave, 1s on / 3s off)
- Audio bridge: PortAudio <-> PJSIP conference bridge
- Phone-style numpad UI (123456789*0#)
- LCD-style two-line display with scrolling caller name
- Settings persistence in `~/.config/chiriksip/chiriksip.conf`
- Auto-registration on startup
- Menu bar: File (Exit), Settings (Account toggle), Help (About)
- Window icon from project logo
- .desktop file with hicolor icons (16-256px)
- Keyboard support: 0-9, *, #, +, Enter, Escape, Backspace
- Button "0+": short press = "0", long press = "+"
- Hangup button: short press = delete last digit / end call, long press = clear all / end call
- G.711 A-law (PCMA) and u-law (PCMU) codecs only
- Caller name display: static if fits, scrolling with " *|* " separator if not

### Bug Fixes
- Fixed SIP URI construction for phone numbers (auto-append @server)
- Fixed call state mapping (pjsip_inv_state enum)
- Fixed remote hangup detection (DISCONNCTD state)
- Fixed PortAudio device enumeration (Fedora pjproject missing audio driver)
- Fixed audio feedback loop (on-demand PortAudio stream)
- Fixed thread safety for AudioBridge creation
- Fixed window resize when toggling account settings
- Fixed Hangup button state after call ends

### Technical
- Built with Qt6, PJSIP 2.13.1, PortAudio 19.7
- Targets KDE Plasma on Fedora 44
- C++17, CMake 3.20+
- RPM packaging with hicolor icons and .desktop validation
