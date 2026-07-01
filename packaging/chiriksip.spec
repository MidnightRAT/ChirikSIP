Name:           chiriksip
Version:        1.0.0
Release:        18%{?dist}
Summary:        A simple SIP client for KDE Plasma

License:        MIT
URL:            https://github.com/chirik/ChirikSIP

Source0:        %{name}-%{version}.tar.gz

BuildRequires:  cmake >= 3.20
BuildRequires:  gcc-c++
BuildRequires:  qt6-qtbase-devel
BuildRequires:  pkgconfig(libpjproject)
BuildRequires:  portaudio-devel
BuildRequires:  desktop-file-utils
BuildRequires:  hicolor-icon-theme

Requires:       qt6-qtbase%{?_isa}
Requires:       qt6-qtbase-gui%{?_isa}
Requires:       pjproject%{?_isa}
Requires:       portaudio%{?_isa}
Requires:       hicolor-icon-theme

%description
ChirikSIP is a minimal SIP client built with Qt6 and PJSIP.
Supports SIP registration, making and receiving calls, audio via
PortAudio, phone-style numpad UI, caller name display, setup wizard,
system tray with incoming call popup, and settings persistence.
Targets Fedora / KDE Plasma.

%prep
%autosetup -n %{name}-%{version}

%build
%cmake
%cmake_build

%install
%cmake_install
desktop-file-validate %{buildroot}%{_datadir}/applications/%{name}.desktop || :

%files
%license LICENSE
%doc README.md CHANGELOG.md
%{_bindir}/%{name}
%{_datadir}/applications/%{name}.desktop
%{_datadir}/icons/hicolor/16x16/apps/%{name}.png
%{_datadir}/icons/hicolor/32x32/apps/%{name}.png
%{_datadir}/icons/hicolor/48x48/apps/%{name}.png
%{_datadir}/icons/hicolor/64x64/apps/%{name}.png
%{_datadir}/icons/hicolor/128x128/apps/%{name}.png
%{_datadir}/icons/hicolor/256x256/apps/%{name}.png

%changelog
* Wed Jul 01 2026 Chirik <chirikrat@gmail.com> - 1.0.0-18
- Fixed audio disappearing after the first call (PortAudio lifecycle)
- Fixed clock overwriting dialed number while dialing
- Fixed Hangup/Backspace erasing clock digits
- Dialed number persists until end of call, right-aligned
- Call duration centered on second display line during active call
* Sun Jun 28 2026 Chirik <chirikrat@gmail.com> - 1.0.0-17
- Removed i686 build from Windows workflow (Qt6 not available for 32-bit)
- Disabled GitHub Actions workflows for Linux and Windows builds
- Clock centered on first display line
- Setup Wizard accessible from Settings menu
- About dialog shows build date and time
- Call duration timer centered in status bar during calls
* Sun Jun 28 2026 Chirik <chirikrat@gmail.com> - 1.0.0-16
- Clock centered on first display line
- Setup Wizard accessible from Settings menu
- About dialog shows build date and time
* Sun Jun 28 2026 Chirik <chirikrat@gmail.com> - 1.0.0-15
- Fixed audio disappearing when port was set to 0
- Fixed port label not updating after settings change
- Port label now shows effective port (5060 when configured as 0)
* Sun Jun 28 2026 Chirik <chirikrat@gmail.com> - 1.0.0-14
- Configurable SIP port in Setup Wizard and Settings dialog (default: 0 = auto)
- Dynamic port selection: port 0 assigns random available port on each launch
- Status bar shows current transport port (e.g. UDP:50600)
- Removed dead code: ensurePortBound() and orphaned makeCall block
* Sun Jun 28 2026 Chirik <chirikrat@gmail.com> - 1.0.0-13
- Deep code review: fixed data races in AudioBridge (memory_order),
  PortAudioManager refcount leak in AudioBridge/Ringtone close/stop,
  m_incomingCallId not reset on remote hangup, AudioBridge close() ordering,
  keyPressEvent input blocked during call, acc_id validation in callbacks,
  CMakeLists.txt uses PkgConfig::PJSIP and portaudio-2.0,
  SetupWizard "Finish" button text, User-Agent uses PROJECT_VERSION,
  system tray availability check, m_trayIcon nullptr initialization,
  SetupWizard Enter key support and active-field focus
* Sun Jun 28 2026 Chirik <chirikrat@gmail.com> - 1.0.0-12
- Deep code review fixes: fixed data races in AudioBridge (memory_order),
  PortAudioManager refcount leak in AudioBridge/Ringtone close/stop,
  m_incomingCallId not reset on remote hangup, AudioBridge close() ordering,
  keyPressEvent input blocked during call, acc_id validation in callbacks,
  CMakeLists.txt uses PkgConfig::PJSIP and portaudio-2.0,
  SetupWizard "Finish" button text, User-Agent uses PROJECT_VERSION,
  system tray availability check, m_trayIcon nullptr initialization
* Sun Jun 28 2026 Chirik <chirikrat@gmail.com> - 1.0.0-11
- Code review fixes: null pointer guard in onCallMediaState, dynamic version
  in About dialog, removed unused pjnath.h header, README RPM section update
* Sun Jun 28 2026 Chirik <chirikrat@gmail.com> - 1.0.0-10
- Simplified Linux CI workflow: ubuntu-latest without container
* Sun Jun 28 2026 Chirik <chirikrat@gmail.com> - 1.0.0-9
- Added GitHub Actions CI/CD workflows for Linux RPM and Windows builds
* Sun Jun 28 2026 Chirik <chirikrat@gmail.com> - 1.0.0-8
- Bug fixes: null pointer crashes, PortAudio lifecycle race conditions,
  atomic ringtone variables, ScrollHelper text restore, config file
  permissions restricted to 0600
* Sun Jun 28 2026 Chirik <chirikrat@gmail.com> - 1.0.0-7
- System tray, call notification popup, hangup fixes, ScrollHelper,
  PortAudioManager, Windows cross-compilation, Docker build scripts,
  GitHub Actions CI, screenshots, spec path fix
* Fri Jun 26 2026 Chirik <chirikrat@gmail.com> - 1.0.0-1
- Initial RPM package for Fedora
- SIP registration and calls via PJSIP
- Audio bridge via PortAudio
- Phone-style numpad UI
- Settings persistence
- System icons and .desktop file
