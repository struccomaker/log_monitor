#include <benchmark/benchmark.h>
#include "keyword_matcher.h"
#include "log_monitor.h"
#include <fstream>
#include <random>
#include <filesystem>
#include <thread>
#include <chrono>

namespace fs = std::filesystem;

// finds single keyword in line
static void BM_KeywordMatcher_SingleKeyword(benchmark::State& state) {
    KeywordMatcher matcher({"EXECUTION"});
    std::string line = "2024-10-15 12:34:56.789123 EXECUTION OrderID=123456 Symbol=AAPL Side=BUY";
    
    for (auto _ : state) {
        benchmark::DoNotOptimize(matcher.matches(line));
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_KeywordMatcher_SingleKeyword);

//multiple keywords in line
static void BM_KeywordMatcher_MultipleKeywords(benchmark::State& state) {
    KeywordMatcher matcher({"key1", "key2", "EXECUTION", "REJECT", "FILL", 
                            "CANCEL", "ERROR", "WARNING", "INFO", "DEBUG"});
    std::string line = "2024-10-15 12:34:56.789123 EXECUTION OrderID=123456 Symbol=AAPL Side=BUY";
    
    for (auto _ : state) {
        benchmark::DoNotOptimize(matcher.matches(line));
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_KeywordMatcher_MultipleKeywords);

//check false positives
static void BM_KeywordMatcher_NoMatch(benchmark::State& state) {
    KeywordMatcher matcher({"key1", "key2", "EXECUTION"});
    std::string line = "2024-10-15 12:34:56.789123 INFO OrderID=123456 Symbol=AAPL Side=BUY";
    
    for (auto _ : state) {
        benchmark::DoNotOptimize(matcher.matches(line));
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_KeywordMatcher_NoMatch);

//check for long lines truncation
static void BM_KeywordMatcher_LongLine(benchmark::State& state) {
    KeywordMatcher matcher({"EXECUTION"});
    std::string line(5000, 'X');
    line += "EXECUTION";
    line += std::string(5000, 'Y');
    
    for (auto _ : state) {
        benchmark::DoNotOptimize(matcher.matches(line));
    }
    
    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * line.size());
}
BENCHMARK(BM_KeywordMatcher_LongLine);

// bench line processing throughput
class BenchmarkFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& state) override {
        testFile_ = "benchmark_test.log";
        outputFile_ = "benchmark_output.log";
        fs::remove(testFile_); //clean up old files from prev runs
        fs::remove(outputFile_);
    }
    
    void TearDown(const ::benchmark::State& state) override {
        fs::remove(testFile_); //clean up test files
        fs::remove(outputFile_);
    }
    
    std::string testFile_;
    std::string outputFile_;
};

BENCHMARK_F(BenchmarkFixture, BM_ProcessShortLines)(benchmark::State& state) {
    // create test file with 10000 short lines
    {
        std::ofstream ofs(testFile_);
        for (int i = 0; i < 10000; ++i) {
            ofs << "2024-10-15 12:34:56.789123 EXECUTION OrderID=" << i 
                << " Symbol=AAPL Side=BUY Price=150.25 Qty=100\n";
        }
    }
    
    for (auto _ : state) {
        state.PauseTiming();
        fs::remove(outputFile_);
        LogMonitor::Config config;
        config.inputFile = testFile_;
        config.outputFile = outputFile_;
        config.keywords = {"EXECUTION"};
        config.pollIntervalMs = 0;
        
        LogMonitor monitor(config);
        state.ResumeTiming();
        //THREAD STARTS HERE 
        std::thread t([&monitor]() {
            monitor.start();
        });
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); //need time to process here for cpu
        monitor.stop();
        t.join(); //join back thread
        
        auto stats = monitor.getStatistics();
        state.SetItemsProcessed(stats.linesProcessed);
        state.SetBytesProcessed(stats.bytesRead);
    }
}

//bench mixed lines
//mix of matching + nonmatching lines
BENCHMARK_F(BenchmarkFixture, BM_ProcessMixedLines)(benchmark::State& state) {
    // create test file with mixed matching + nonmatching lines
    {
        std::ofstream ofs(testFile_);
        for (int i = 0; i < 10000; ++i) {
            if (i % 3 == 0) {
                ofs << "2024-10-15 12:34:56.789123 EXECUTION OrderID=" << i << "\n";
            } else if (i % 3 == 1) {
                ofs << "2024-10-15 12:34:56.789123 INFO OrderID=" << i << "\n";
            } else {
                ofs << "2024-10-15 12:34:56.789123 DEBUG OrderID=" << i << "\n";
            }
        }
    }
    
    for (auto _ : state) {
        state.PauseTiming();
        fs::remove(outputFile_);
        LogMonitor::Config config;
        config.inputFile = testFile_;
        config.outputFile = outputFile_;
        config.keywords = {"EXECUTION"};
        config.pollIntervalMs = 0;
        
        LogMonitor monitor(config);
        state.ResumeTiming();
        
        std::thread t([&monitor]() {
            monitor.start(); //similar as above
        });
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        monitor.stop();
        t.join();
        
        auto stats = monitor.getStatistics();
        state.SetItemsProcessed(stats.linesProcessed);
        state.SetBytesProcessed(stats.bytesRead);
    }
}

//process long lines
BENCHMARK_F(BenchmarkFixture, BM_ProcessLongLines)(benchmark::State& state) {
    // Create test file with very long lines
    {
        std::ofstream ofs(testFile_);
        for (int i = 0; i < 1000; ++i) {
            ofs << "EXECUTION " << std::string(6000, 'X') << "\n";
        }
    }
    
    for (auto _ : state) {
        state.PauseTiming();
        fs::remove(outputFile_);
        LogMonitor::Config config;
        config.inputFile = testFile_;
        config.outputFile = outputFile_;
        config.keywords = {"EXECUTION"};
        config.pollIntervalMs = 0;
        
        LogMonitor monitor(config);
        state.ResumeTiming();
        
        std::thread t([&monitor]() {
            monitor.start(); //similar as above
        });
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        monitor.stop();
        t.join();
        
        auto stats = monitor.getStatistics();
        state.SetItemsProcessed(stats.linesProcessed);
        state.SetBytesProcessed(stats.bytesRead);
    }
}

// Benchmark different buffer sizeson throughput
static void BM_BufferSize(benchmark::State& state) {
    std::string testFile = "buffer_bench.log";
    std::string outputFile = "buffer_out.log";
    
    // make test file
    {
        std::ofstream ofs(testFile);
        for (int i = 0; i < 50000; ++i) {
            ofs << "2024-10-15 12:34:56.789123 EXECUTION OrderID=" << i << "\n";
        }
    }
    
    size_t bufferSize = state.range(0);
    
    for (auto _ : state) {
        state.PauseTiming();
        fs::remove(outputFile);
        
        LogMonitor::Config config;
        config.inputFile = testFile;
        config.outputFile = outputFile;
        config.keywords = {"EXECUTION"};
        config.bufferSize = bufferSize;
        config.pollIntervalMs = 0;
        
        LogMonitor monitor(config);
        state.ResumeTiming();
        
        std::thread t([&monitor]() {
            monitor.start();
        });
        
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        monitor.stop();
        t.join();
        
        auto stats = monitor.getStatistics();
        state.SetItemsProcessed(stats.linesProcessed);
        state.SetBytesProcessed(stats.bytesRead);
    }
    
    fs::remove(testFile);
    fs::remove(outputFile);
}
BENCHMARK(BM_BufferSize)->RangeMultiplier(2)->Range(4096, 256*1024);

BENCHMARK_MAIN();