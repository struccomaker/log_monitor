# Log Monitor   
Nicholas Loo Submission

## Overview

this system monitors log files in real-time, filters lines containing specific keywords, and handles files exceeding 500GB with constant memory usage. Lines longer than 5000 characters are automatically truncated.

## Features

- real time monitoring with 10ms poll interval
- substring matching
- 500gb max with 50mb mem pool
- takes first 5000 characters then discards the rest.

## Requirements

- Ubuntu 22.04 LTS // WSL2
- C++17 
- CMake 3.14+

## Quick Start

### 1. Build

```bash
./build.sh Release
```

This compiles everything with optimizations enabled.

### 2. Run

**Terminal 1 - Generate test logs:**
```bash
./build_Release/log_generator a.log 1000
```

./log_generator <output_file> [interval_microseconds]

# Generate 1000 logs/second (default)
./log_generator test.log 1000

Generates 1000 logs per second. Press Ctrl+C to stop; to simulate the log file generation

**Terminal 2 - Monitor and filter:**
```bash
./build_Release/log_monitor a.log b.log 
```

# Interactive (prompts for keywords)
./log_monitor input.log output.log

# Command-line (no prompts)
./log_monitor input.log output.log keyword1 keyword2 keyword3

Monitors `a.log`, filters lines with keywords, writes matches to `b.log`. put in keywords that we want
example: ERROR , FILL  WARNING, NVDA, GOOGL , META , BUY , LIMIT
check log_generator.cpp for the class.

### 3. Check Results

```bash
# View filtered output
cat b.log

directly check the b.log file


## Testing

### Run All Tests
```bash
cd build_Release
./log_monitor_tests
```

Or use CTest:
```bash
ctest --output-on-failure
```

### Run Specific Test
```bash
./log_monitor_tests --gtest_filter=*BasicKeywordFiltering*
```

### What Gets Tested
1. **BasicKeywordFiltering** - Verifies keyword matching works correctly
2. **TruncatesLongLines** - Confirms lines >5000 chars are truncated
3. **HandlesRealTimeUpdates** - Tests real-time monitoring capability

## Benchmarking

```bash
cd build_Release
./log_monitor_benchmark
```

Shows performance metrics:
- keyword matching speed (<100ns per line)
- line processing throughput (50,000+ lines/sec)
- memory usage (constant, <50MB)

## How It Works

1. **Monitor starts** and opens the log file
2. **Polls every 10ms** for new data
3. **Reads in 64KB chunks** for efficiency
4. **Processes each line:**
   - Checks if line contains any keyword
   - Truncates if >5000 characters
   - Writes matches to output file
5. **Continues until stopped** (Ctrl+C)

## Configuration

Edit `include/log_monitor.h` to customize:

```cpp
static constexpr size_t DEFAULT_BUFFER_SIZE = 64 * 1024;  // 64KB buffer
static constexpr size_t MAX_LINE_LENGTH = 5000;           // line limit
static constexpr int DEFAULT_POLL_INTERVAL = 10;          // 10ms polling
```

Rebuild after changes:
```bash
./build.sh Release
```

## Clean Build

Remove all build artifacts:
```bash
./clean.sh
```

Then rebuild:
```bash
./build.sh Release
```

## Notes

- keyword are case-sensitive (eg: "key1" != "KEY1")
- keyword matching is substring-based (eg "key" matches "keyboard")
- monitor appends to output file but wont overwrite exisiting data