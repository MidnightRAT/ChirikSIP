#!/bin/bash
set -e

IMAGE_NAME="chiriksip-mingw"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

build_arch() {
    local arch=$1
    local toolchain=$2
    local prefix=$3
    local build_dir="build-win${arch}"

    echo "=== Building for win${arch} ==="
    mkdir -p "$SCRIPT_DIR/$build_dir"

    docker run --rm \
        -v "$SCRIPT_DIR:/src" \
        -v "$SCRIPT_DIR/$build_dir:/build" \
        "$IMAGE_NAME" \
        -c "cd /src && \
            mkdir -p /build && cd /build && \
            cmake /src \
                -DCMAKE_TOOLCHAIN_FILE=/opt/toolchains/${toolchain}.cmake \
                -DCMAKE_INSTALL_PREFIX=/opt/${prefix} \
                -DCMAKE_PREFIX_PATH='/opt/${prefix};/opt/${prefix}/qt6' \
                -DCMAKE_BUILD_TYPE=Release && \
            make -j\$(nproc)"

    echo "=== win${arch} build complete: $build_dir/chiriksip.exe ==="
}

build_arch 32 mingw32 mingw32
build_arch 64 mingw64 mingw64

echo "=== All builds complete ==="
ls -lh "$SCRIPT_DIR"/build-win*/chiriksip.exe
