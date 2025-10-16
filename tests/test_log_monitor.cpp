#include <gtest/gtest.h>
#include "log_monitor.h"
#include <fstream>
#include <thread>
#include <chrono>
#include <filesystem>

namespace fs = std::filesystem;


class LogMonitorTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto now = std::chrono::system_clock::now().time_since_epoch().count();
        testInputFile_ = "test_in_" + std::to_string(now) + "_" + std::to_string(rand()) + ".log";
        testOutputFile_ = "test_out_" + std::to_string(now) + "_" + std::to_string(rand()) + ".log";

        fs::remove(testInputFile_);
        fs::remove(testOutputFile_);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    void TearDown() override {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        fs::remove(testInputFile_);
        fs::remove(testOutputFile_);
    }

    void writeToInputFile(const std::string& content) {
        std::ofstream ofs(testInputFile_, std::ios::app);
        ofs << content;
        ofs.flush();
        ofs.close();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    std::string readOutputFile() {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::ifstream ifs(testOutputFile_);
        if (!ifs.is_open()) return "";
        return std::string((std::istreambuf_iterator<char>(ifs)),
                          std::istreambuf_iterator<char>());
    }

    std::string testInputFile_;
    std::string testOutputFile_;
};

TEST_F(LogMonitorTest, BasicKeywordFiltering) {
    {
        std::ofstream ofs(testInputFile_);
        ofs.close();
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    fs::remove(testOutputFile_);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    LogMonitor::Config config;
    config.inputFile = testInputFile_;
    config.outputFile = testOutputFile_;
    config.keywords = {"key1", "key2"};
    config.pollIntervalMs = 10;
    
    LogMonitor monitor(config);
    
    std::thread monitorThread([&monitor]() {
        monitor.start();
    });
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    writeToInputFile("line with key1\n");
    writeToInputFile("line without keyword\n");
    writeToInputFile("another with key2\n");
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    monitor.stop();
    monitorThread.join();
    
    std::string output = readOutputFile();
    EXPECT_TRUE(output.find("key1") != std::string::npos);
    EXPECT_TRUE(output.find("key2") != std::string::npos);
    EXPECT_TRUE(output.find("without keyword") == std::string::npos);
}

TEST_F(LogMonitorTest, TruncatesLongLines) {
    {
        std::ofstream ofs(testInputFile_);
        ofs.close();
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    fs::remove(testOutputFile_);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    LogMonitor::Config config;
    config.inputFile = testInputFile_;
    config.outputFile = testOutputFile_;
    config.keywords = {"key1"};
    config.pollIntervalMs = 10;
    
    LogMonitor monitor(config);
    
    std::thread monitorThread([&monitor]() {
        monitor.start();
    });
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    std::string longLine = "key1 " + std::string(10000, 'X') + "\n";
    writeToInputFile(longLine);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    monitor.stop();
    monitorThread.join();
    
    std::string output = readOutputFile();
    
    ASSERT_FALSE(output.empty()) << "Output file is empty!";
    ASSERT_TRUE(output.find("key1") != std::string::npos);
    
    size_t lineCount = std::count(output.begin(), output.end(), '\n');
    EXPECT_EQ(lineCount, 1) << "Output has " << lineCount << " lines, expected 1. Output: [" << output << "]";
    
    size_t firstNewline = output.find('\n');
    ASSERT_NE(firstNewline, std::string::npos);
    std::string firstLine = output.substr(0, firstNewline);
    
    EXPECT_LE(firstLine.length(), LogMonitor::MAX_LINE_LENGTH)
        << "Line has " << firstLine.length() << " chars, expected <= " << LogMonitor::MAX_LINE_LENGTH;
    
    auto stats = monitor.getStatistics();
    EXPECT_EQ(stats.longLinesDiscarded, 1);
    EXPECT_EQ(stats.linesProcessed, 1);
    EXPECT_EQ(stats.linesMatched, 1);
}

TEST_F(LogMonitorTest, HandlesRealTimeUpdates) {
    {
        std::ofstream ofs(testInputFile_);
        ofs.close();
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    // ensure output file DO NOT EXIST OR ELSE IM COOKED
    fs::remove(testOutputFile_);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    LogMonitor::Config config;
    config.inputFile = testInputFile_;
    config.outputFile = testOutputFile_;
    config.keywords = {"key1"};
    config.pollIntervalMs = 10;
    
    LogMonitor monitor(config);
    
    std::thread monitorThread([&monitor]() {
        monitor.start();
    });
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    writeToInputFile("first line with key1\n");
    
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    writeToInputFile("second line with key1\n");
    
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    monitor.stop();
    monitorThread.join();
    
    std::string output = readOutputFile();
    EXPECT_TRUE(output.find("first line") != std::string::npos);
    EXPECT_TRUE(output.find("second line") != std::string::npos);
    
    auto stats = monitor.getStatistics();
    EXPECT_GE(stats.linesProcessed, 2);
    EXPECT_GE(stats.linesMatched, 2);
}
