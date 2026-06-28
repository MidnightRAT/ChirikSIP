Name:           chiriksip
Version:        1.0.0
Release:        9%{?dist}
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
* Sun Jun 28 2026 Chirik <chirik@example.com> - 1.0.0-9
- Added GitHub Actions CI/CD workflows for Linux RPM and Windows builds
* Sun Jun 28 2026 Chirik <chirik@example.com> - 1.0.0-8
- Bug fixes: null pointer crashes, PortAudio lifecycle race conditions,
  atomic ringtone variables, ScrollHelper text restore, config file
  permissions restricted to 0600
* Sun Jun 28 2026 Chirik <chirik@example.com> - 1.0.0-7
- System tray, call notification popup, hangup fixes, ScrollHelper,
  PortAudioManager, Windows cross-compilation, Docker build scripts,
  GitHub Actions CI, screenshots, spec path fix
* Fri Jun 26 2026 Chirik <chirik@example.com> - 1.0.0-1
- Initial RPM package for Fedora
- SIP registration and calls via PJSIP
- Audio bridge via PortAudio
- Phone-style numpad UI
- Settings persistence
- System icons and .desktop file
