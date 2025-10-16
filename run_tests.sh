cat > ../run_tests.sh << 'EOF'
#!/bin/bash

set -e

BUILD_DIR="build_Release"

if [ ! -f "$BUILD_DIR/log_monitor_tests" ]; then
    echo "Error: Tests not built. Run ./build.sh first."
    exit 1
fi

echo "=== Running HFT Log Monitor Tests ==="
echo ""

cd "$BUILD_DIR"
./log_monitor_tests --gtest_color=yes

echo ""
echo "All tests passed!"
EOF

chmod +x ../run_tests.sh
