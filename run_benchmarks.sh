cat > ../run_benchmarks.sh << 'EOF'
#!/bin/bash

set -e

BUILD_DIR="build_Release"

if [ ! -f "$BUILD_DIR/log_monitor_benchmark" ]; then
    echo "Error: Benchmarks not built. Run ./build.sh first."
    exit 1
fi

echo "=== Running HFT Log Monitor Benchmarks ==="
echo ""

cd "$BUILD_DIR"
./log_monitor_benchmark \
    --benchmark_out=benchmark_results.json \
    --benchmark_out_format=json \
    --benchmark_repetitions=3 \
    --benchmark_report_aggregates_only=true

echo ""
echo "Benchmark results saved to: $BUILD_DIR/benchmark_results.json"
EOF

chmod +x ../run_benchmarks.sh
