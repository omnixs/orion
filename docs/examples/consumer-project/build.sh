#!/bin/bash
# Build script for the consumer example project

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ORION_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

echo "=== Building Orion Consumer Example ==="
echo "Orion root: $ORION_ROOT"
echo "Example dir: $SCRIPT_DIR"

# Check if vcpkg is available
if [ -z "$VCPKG_ROOT" ]; then
    echo "Error: VCPKG_ROOT environment variable not set"
    echo "Please set it to your vcpkg installation directory:"
    echo "  export VCPKG_ROOT=/path/to/vcpkg"
    exit 1
fi

if [ ! -f "$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" ]; then
    echo "Error: vcpkg toolchain file not found at $VCPKG_ROOT"
    exit 1
fi

echo "Using vcpkg at: $VCPKG_ROOT"

# Configure with vcpkg overlay pointing to orion ports
echo ""
echo "=== Configuring project ==="
cmake -B "$SCRIPT_DIR/build" -S "$SCRIPT_DIR" \
    -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" \
    -DVCPKG_OVERLAY_PORTS="$ORION_ROOT/ports" \
    -DVCPKG_FEATURE_FLAGS=-binarycaching \
    -DCMAKE_BUILD_TYPE=Release

# Build
echo ""
echo "=== Building project ==="
cmake --build "$SCRIPT_DIR/build" --config Release

echo ""
echo "=== Build complete ==="
echo "Executable: $SCRIPT_DIR/build/consumer-app"
echo ""
echo "Run with:"
echo "  cd $SCRIPT_DIR"
echo "  ./build/consumer-app"
