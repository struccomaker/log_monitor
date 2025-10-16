#!/bin/bash
# ============================================================================
# build.sh - prod build script for ubuntu 22.04 LTS
# ============================================================================

set -e

echo "=== Nicholas Nayt Build Script ==="
echo ""

echo "Checking dependencies."
if ! command -v cmake &> /dev/null; then
    echo "Error: CMake is not installed"
    echo "Install with: sudo apt-get install cmake"
    exit 1
fi

if ! command -v g++ &> /dev/null; then
    echo "Error: g++ is not installed"
    echo "Install with: sudo apt-get install build-essential"
    exit 1
fi

echo "All dependencies found."
echo ""

BUILD_TYPE="${1:-Release}"
BUILD_DIR="build_${BUILD_TYPE}"

echo "Build type: $BUILD_TYPE"
echo "Build directory: $BUILD_DIR"
echo ""

rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

echo "Configuring..."
cmake .. \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DBUILD_TESTS=ON \
    -DBUILD_BENCHMARKS=ON \
    -DCMAKE_CXX_COMPILER=g++ \
    -DCMAKE_CXX_FLAGS_RELEASE="-O3 -march=native -DNDEBUG -flto" \
    -DCMAKE_INSTALL_PREFIX="../install"


echo ""
echo "Building."
cmake --build . -j$(nproc)

# Run tests
if [ "$BUILD_TYPE" = "Release" ] || [ "$BUILD_TYPE" = "Debug" ]; then
    echo ""
    echo "Running tests."
    ctest --output-on-failure -j$(nproc)
fi

echo ""
echo "=== Build Complete ==="
echo "Executables are in: $BUILD_DIR/"
echo "  - log_generator"
echo "  - log_monitor"
echo "  - log_monitor_tests (if built)"
echo "  - log_monitor_benchmark (if built)"
echo ""

