#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
CROSS_LIBS="$HOME/cross-libs"

build_arch() {
    local arch=$1
    local host=$2
    local toolchain=$3
    local prefix="$CROSS_LIBS/$arch"
    local build_dir="$SCRIPT_DIR/build-win${arch#mingw}"

    echo "=== Building for $arch ==="
    mkdir -p "$build_dir"

    cmake -B "$build_dir" -S "$SCRIPT_DIR" \
        -DCMAKE_SYSTEM_NAME=Windows \
        -DCMAKE_C_COMPILER=${host}-gcc \
        -DCMAKE_CXX_COMPILER=${host}-g++ \
        -DCMAKE_RC_COMPILER=${host}-windres \
        -DCMAKE_INSTALL_PREFIX="$prefix" \
        -DCMAKE_PREFIX_PATH="$prefix" \
        -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
        -DCMAKE_BUILD_TYPE=Release

    cmake --build "$build_dir" -j$(nproc)

    echo "=== $arch build complete: $build_dir/chiriksip.exe ==="
}

build_arch mingw32 i686-w64-mingw32 toolchains/mingw32.cmake
build_arch mingw64 x86_64-w64-mingw32 toolchains/mingw64.cmake

echo "=== All builds complete ==="
ls -lh "$SCRIPT_DIR"/build-win{32,64}/chiriksip.exe
