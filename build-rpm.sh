#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
CONTAINER_FILE="$SCRIPT_DIR/Containerfile.rpmbuild"
IMAGE_NAME="chiriksip-rpmbuild"
ARTIFACTS_DIR="$SCRIPT_DIR/build/rpms"

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
VERSIONS="44"
FORCE_BUILD=false
INSTALL_RPMFUSION=false

usage() {
    cat <<EOF
Usage: $0 [OPTIONS] [VERSIONS]

Build ChirikSIP RPMs inside Fedora containers.

Arguments:
  VERSIONS    Space-separated Fedora versions (default: 44)
              Example: $0 "41 42 44"

Options:
  -f, --force       Force rebuild (no cache)
  --rpmfusion       Enable rpmfusion repositories
  -h, --help        Show this help

Examples:
  $0                         # Build for Fedora 44 only
  $0 "41 42 44"             # Build for F41-F44
  $0 --rpmfusion "36 37 38" # Build with rpmfusion for older Fedora
EOF
    exit 0
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        -f|--force) FORCE_BUILD=true; shift ;;
        --rpmfusion) INSTALL_RPMFUSION=true; shift ;;
        -h|--help) usage ;;
        *) VERSIONS="$1"; shift ;;
    esac
done

mkdir -p "$ARTIFACTS_DIR"

# Extract version from spec file
SPEC_VERSION=$(grep '^Version:' "$SCRIPT_DIR/packaging/chiriksip.spec" | awk '{print $2}')
SPEC_RELEASE=$(grep '^Release:' "$SCRIPT_DIR/packaging/chiriksip.spec" | awk '{print $2}' | sed 's/%{?dist}//')
FULL_VERSION="${SPEC_VERSION}-${SPEC_RELEASE}"
echo "=== ChirikSIP RPM Build ==="
echo "Package version: $FULL_VERSION"
echo "Fedora versions: $VERSIONS"
echo "Runtime: $CONTAINER_RUNTIME"
echo "RPM Fusion: $INSTALL_RPMFUSION"
echo ""

FAILED_VERSIONS=()
SUCCEEDED_VERSIONS=()

for FEDORA_VERSION in $VERSIONS; do
    echo "=== Building for Fedora $FEDORA_VERSION ==="

    # Build image
    BUILD_ARGS=""
    if [ "$FORCE_BUILD" = true ]; then
        BUILD_ARGS="--no-cache"
    fi

    if ! $CONTAINER_RUNTIME build \
        $BUILD_ARGS \
        -t "$IMAGE_NAME:$FEDORA_VERSION" \
        -f "$CONTAINER_FILE" \
        --build-arg "FEDORA_VERSION=$FEDORA_VERSION" \
        --build-arg "INSTALL_RPMFUSION=$INSTALL_RPMFUSION" \
        "$SCRIPT_DIR"; then
        echo "ERROR: Failed to build image for Fedora $FEDORA_VERSION"
        FAILED_VERSIONS+=("$FEDORA_VERSION")
        continue
    fi

    # Run rpmbuild inside container
    if ! $CONTAINER_RUNTIME run --rm \
        -v "$SCRIPT_DIR:/src:ro" \
        -v "$ARTIFACTS_DIR:/rpmbuild-output" \
        "$IMAGE_NAME:$FEDORA_VERSION" -c "
            set -e

            # Create rpmbuild tree
            mkdir -p ~/rpmbuild/{BUILD,RPMS,SOURCES,SPECS,SRPMS}

            # Create source tarball
            cd /src
            tar czf ~/rpmbuild/SOURCES/chiriksip-${SPEC_VERSION}.tar.gz \
                --transform 's,^,chiriksip-${SPEC_VERSION}/,' \
                -X .rpmignore .

            # Copy spec file
            cp packaging/chiriksip.spec ~/rpmbuild/SPECS/

            # Build RPMs
            rpmbuild -ba ~/rpmbuild/SPECS/chiriksip.spec

            # Copy artifacts to output volume (exclude debuginfo/debugsource)
            cp ~/rpmbuild/RPMS/*/*.rpm /rpmbuild-output/ 2>/dev/null || true
            cp ~/rpmbuild/SRPMS/*.rpm /rpmbuild-output/ 2>/dev/null || true
            rm -f /rpmbuild-output/*debuginfo*.rpm /rpmbuild-output/*debugsource*.rpm
        "; then
        echo "ERROR: rpmbuild failed for Fedora $FEDORA_VERSION"
        FAILED_VERSIONS+=("$FEDORA_VERSION")
        continue
    fi

    echo "=== Fedora $FEDORA_VERSION: SUCCESS ==="
    SUCCEEDED_VERSIONS+=("$FEDORA_VERSION")
done

echo ""
echo "=== Build Summary ==="
echo "Succeeded: ${SUCCEEDED_VERSIONS[*]:-none}"
echo "Failed:    ${FAILED_VERSIONS[*]:-none}"
echo "Artifacts: $ARTIFACTS_DIR/"

if [ ${#FAILED_VERSIONS[@]} -gt 0 ]; then
    exit 1
fi
