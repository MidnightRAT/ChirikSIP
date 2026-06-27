#!/bin/bash
set -e

IMAGE_NAME="chiriksip-mingw"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

echo "=== Building Docker image ==="
docker build -t "$IMAGE_NAME" -f Dockerfile.mingw "$SCRIPT_DIR"

echo "=== Docker image built successfully ==="
echo "To build Windows binaries, run: ./build-windows.sh"
