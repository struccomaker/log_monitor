#!/bin/bash
# ============================================================================
# clean.sh - clean for fresh install
# ============================================================================

set -e

echo "=== Cleaning ==="
echo ""

echo "Removing build directories"
rm -rf build_Release build_Debug build_* 2>/dev/null || true

echo "Removing test files"
rm -f test_input*.log test_output*.log 2>/dev/null || true
rm -f test_in_*.log test_out_*.log 2>/dev/null || true

echo "Removing generated log files"
rm -f a.log b.log demo.log filtered.log 2>/dev/null || true
rm -f *.log 2>/dev/null || true

echo "Removing benchmark files."
rm -f benchmark_test.log benchmark_output.log 2>/dev/null || true
rm -f buffer_bench.log buffer_out.log 2>/dev/null || true
rm -f benchmark_results.json 2>/dev/null || true

echo "Removing CMake cache"
rm -f CMakeCache.txt 2>/dev/null || true
rm -rf CMakeFiles 2>/dev/null || true

echo "Removing object files."
find . -name "*.o" -type f -delete 2>/dev/null || true
find . -name "*.a" -type f -delete 2>/dev/null || true

echo "Removing temporary files"
rm -f *~ .*.swp .*.swo 2>/dev/null || true
rm -rf .vscode .idea 2>/dev/null || true

rm -f core core.* 2>/dev/null || true

echo ""
echo "=== clean done ==="
echo ""
echo "To rebuild:"
echo "  ./build.sh Release"
echo ""
