#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
MANIFEST="$SCRIPT_DIR/com.github.chirik.ChirikSIP.yml"

# Check if flatpak-builder is installed
if ! command -v flatpak-builder &>/dev/null; then
    echo "Error: flatpak-builder not found" >&2
    echo "Install with: sudo dnf install flatpak-builder" >&2
    echo "Or: sudo apt install flatpak-builder" >&2
    exit 1
fi

# Check if flatpak is installed
if ! command -v flatpak &>/dev/null; then
    echo "Error: flatpak not found" >&2
    echo "Install with: sudo dnf install flatpak" >&2
    echo "Or: sudo apt install flatpak" >&2
    exit 1
fi

# Add Flathub remote if not present
if ! flatpak remote-list | grep -q flathub; then
    echo "Adding Flathub remote..."
    flatpak remote-add --if-not-exists flathub https://dl.flathub.org/repo/flathub.flatpakrepo
fi

# Install KDE runtime and SDK if not present
echo "Ensuring KDE runtime and SDK are installed..."
flatpak install -y flathub org.kde.Platform//6.7 org.kde.Sdk//6.7 2>/dev/null || true

# Build
echo "Building ChirikSIP Flatpak..."
flatpak-builder --force-clean --install-deps-from=flathub --repo=repo builddir "$MANIFEST"

echo ""
echo "Build complete!"
echo "To install: flatpak install --user repo com.github.chirik.ChirikSIP"
echo "To run: flatpak run com.github.chirik.ChirikSIP"
