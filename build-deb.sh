#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
CONTAINER_FILE="$SCRIPT_DIR/Containerfile.debbuild"
IMAGE_NAME="chiriksip-debbuild"
ARTIFACTS_DIR="$SCRIPT_DIR/build/debs"

# Detect container runtime
if command -v podman &>/dev/null; then
    CONTAINER_RUNTIME="podman"
elif command -v docker &>/dev/null; then
    CONTAINER_RUNTIME="docker"
else
    echo "Error: Neither podman nor docker found" >&2
    exit 1
fi

# Defaults
VERSIONS="22.04"
FORCE_BUILD=false

usage() {
    cat <<EOF
Usage: $0 [OPTIONS] [VERSIONS]

Build ChirikSIP .deb packages inside Ubuntu containers.

Arguments:
  VERSIONS    Space-separated Ubuntu versions (default: 22.04)
              Example: $0 "22.04 24.04"

Options:
  -f, --force       Force rebuild (no cache)
  -h, --help        Show this help

Examples:
  $0                     # Build for Ubuntu 22.04 only
  $0 "22.04 24.04"      # Build for both LTS versions
  $0 --force "24.04"    # Force rebuild for Ubuntu 24.04
EOF
    exit 0
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        -f|--force) FORCE_BUILD=true; shift ;;
        -h|--help) usage ;;
        *) VERSIONS="$1"; shift ;;
    esac
done

mkdir -p "$ARTIFACTS_DIR"

# Extract version from spec file
SPEC_VERSION=$(grep '^Version:' "$SCRIPT_DIR/packaging/chiriksip.spec" | awk '{print $2}')
SPEC_RELEASE=$(grep '^Release:' "$SCRIPT_DIR/packaging/chiriksip.spec" | awk '{print $2}' | sed 's/%{?dist}//')
FULL_VERSION="${SPEC_VERSION}-${SPEC_RELEASE}"
echo "=== ChirikSIP DEB Build ==="
echo "Package version: $FULL_VERSION"
echo "Ubuntu versions: $VERSIONS"
echo "Runtime: $CONTAINER_RUNTIME"
echo ""

FAILED_VERSIONS=()
SUCCEEDED_VERSIONS=()

for UBUNTU_VERSION in $VERSIONS; do
    echo "=== Building for Ubuntu $UBUNTU_VERSION ==="

    # Build image
    BUILD_ARGS=""
    if [ "$FORCE_BUILD" = true ]; then
        BUILD_ARGS="--no-cache"
    fi

    if ! $CONTAINER_RUNTIME build \
        $BUILD_ARGS \
        -t "$IMAGE_NAME:$UBUNTU_VERSION" \
        -f "$CONTAINER_FILE" \
        --build-arg "UBUNTU_VERSION=$UBUNTU_VERSION" \
        "$SCRIPT_DIR"; then
        echo "ERROR: Failed to build image for Ubuntu $UBUNTU_VERSION"
        FAILED_VERSIONS+=("$UBUNTU_VERSION")
        continue
    fi

    # Run dpkg-buildpackage inside container
    if ! $CONTAINER_RUNTIME run --rm \
        -v "$SCRIPT_DIR:/src:ro" \
        -v "$ARTIFACTS_DIR:/deb-output" \
        "$IMAGE_NAME:$UBUNTU_VERSION" -c "
            set -e

            # Create build directory
            mkdir -p /build/chiriksip-${FULL_VERSION}
            cp -a /src/. /build/chiriksip-${FULL_VERSION}/
            cd /build/chiriksip-${FULL_VERSION}

            # Build package
            dpkg-buildpackage -us -uc -b

            # Copy artifacts to output volume
            cp /build/chiriksip_*.deb /deb-output/ 2>/dev/null || true
            cp /build/chiriksip_*.changes /deb-output/ 2>/dev/null || true
            cp /build/chiriksip_*.buildinfo /deb-output/ 2>/dev/null || true
        "; then
        echo "ERROR: dpkg-buildpackage failed for Ubuntu $UBUNTU_VERSION"
        FAILED_VERSIONS+=("$UBUNTU_VERSION")
        continue
    fi

    echo "=== Ubuntu $UBUNTU_VERSION: SUCCESS ==="
    SUCCEEDED_VERSIONS+=("$UBUNTU_VERSION")
done

echo ""
echo "=== Build Summary ==="
echo "Succeeded: ${SUCCEEDED_VERSIONS[*]:-none}"
echo "Failed:    ${FAILED_VERSIONS[*]:-none}"
echo "Artifacts: $ARTIFACTS_DIR/"

if [ ${#FAILED_VERSIONS[@]} -gt 0 ]; then
    exit 1
fi
