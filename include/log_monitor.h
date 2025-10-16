/**
 * @file log_monitor.h
 * @brief Real-time log file monitoring and filtering system
 * @author Nicholas Loo
 * @date 16/10/25
 * 
 * High-performance log monitoring system designed for high-frequency trading
 * environments. Handles files exceeding 500GB with constant memory usage.
 * Supports real-time monitoring with configurable poll intervals.
 */

#pragma once

#include <string>
#include <fstream>
#include <vector>
#include <atomic>
#include <memory>
#include "keyword_matcher.h"

/**
 * @class LogMonitor
 * @brief Real-time log file monitor with keyword filtering
 * 
 * Monitors a log file for new content, filters lines containing specified keywords,
 * and writes matches to an output file. Designed for production trading systems
 * with emphasis on low latency and constant memory usage.
 * 
 * Key features:
 * - Constant memory usage regardless of file size (handles >500GB files)
 * - Configurable poll interval (default: 10ms for low latency)
 * - Automatic line truncation at 5000 characters
 * - Thread-safe shutdown via atomic flag
 * - Comprehensive statistics tracking
 * 
 * Memory usage: ~50MB constant (buffer + overhead), independent of file size
 * 
 */

class LogMonitor {
public:
    /**
     * @brief Default buffer size for reading log files 64kb
     * 
     * Chosen as balance between I/O efficiency and memory usage.
     */
    static constexpr size_t DEFAULT_BUFFER_SIZE = 64 * 1024;
    
    /**
     * @brief Maximum line length before truncation (5000 characters)
     * 
     * Lines exceeding this length are truncated to prevent memory exhaustion
     * from malformed or extremely long log entries. The remainder is discarded.
     */
    static constexpr size_t MAX_LINE_LENGTH = 5000;
    
    /**
     * @struct Config
     * @brief Configuration parameters for log monitoring
     */
    struct Config {
        std::string inputFile;                      ///< Path to log file to monitor
        std::string outputFile;                     ///< Path to output file for filtered logs
        std::vector<std::string> keywords;          ///< Keywords to filter on
        size_t bufferSize = DEFAULT_BUFFER_SIZE;    ///< Read buffer size in bytes
        int pollIntervalMs = 10;                    ///< Poll interval in milliseconds (10ms = low latency)
    };
    
    /**
     * @brief Constructs a log monitor with the given configuration
     * @param config Configuration parameters (see Config struct)
     * @throw std::runtime_error if output file cannot be opened
     * 
     * Opens the output file in append mode. If the file exists, new entries
     * are appended. The input file is opened when start() is called.
     */
    explicit LogMonitor(const Config& config);
    
    /**
     * @brief Destructor - stops monitoring and closes all files
     * 
     * Ensures clean shutdown even if stop() wasn't called explicitly.
     */
    ~LogMonitor();
    
    /**
     * @brief Starts monitoring the log file (blocking call)
     * 
     * This method blocks until stop() is called from another thread or
     * signal handler. Typically run in a separate thread.
     * 
     * The monitoring loop:
     * 1. Opens input file (if not open)
     * 2. Reads available data in chunks
     * 3. Processes complete lines, filters by keywords
     * 4. Writes matches to output file
     * 5. Sleeps for pollIntervalMs if no data available
     * 6. Repeats until stop() is called
     * 
     * @note This is a blocking call - run in a separate thread for async operation
     * @see stop()
     */
    void start();
    
    /**
     * @brief Stops the monitoring loop
     * 
     * Thread-safe method to signal the monitor to stop. Can be called from
     * signal handlers (e.g., SIGINT) or other threads.
     * 
     * The monitor will complete processing the current buffer before stopping.
     * 
     * @note Thread-safe via atomic flag
     */
    void stop();
    
    /**
     * @struct Statistics
     * @brief Runtime statistics for monitoring operations
     */
    struct Statistics {
        uint64_t linesProcessed = 0;      ///< Total lines read and processed
        uint64_t linesMatched = 0;        ///< Lines containing keywords (written to output)
        uint64_t bytesRead = 0;           ///< Total bytes read from input file
        uint64_t longLinesDiscarded = 0;  ///< Count of lines truncated due to length >5000
    };
    
    /**
     * @brief Retrieves current monitoring statistics
     * @return Copy of current statistics
     * 
     * Safe to call while monitoring is running. Returns a snapshot of
     * current statistics without blocking the monitor.
     * 
     * Useful for displaying progress or debugging performance issues.
     */
    Statistics getStatistics() const;
    
private:
    /**
     * @brief Processes a buffer of data read from the input file
     * @param buffer Pointer to buffer containing data
     * @param bytesRead Number of valid bytes in buffer
     * 
     * Scans for newline characters, extracts complete lines, and passes
     * them to processLine(). Handles partial lines across buffer boundaries
     * by accumulating them in partialLine_.
     * 
     * If a line exceeds MAX_LINE_LENGTH, it's truncated immediately to
     * prevent memory exhaustion.
     */
    void processBuffer(const char* buffer, size_t bytesRead);
    
    /**
     * @brief Processes a single complete line
     * @param line Line to process (string_view for zero-copy)
     * 
     * 1. Truncates if line exceeds MAX_LINE_LENGTH
     * 2. Checks if line contains any keyword
     * 3. If match found, writes to output file with flush
     * 4. Updates statistics
     * 
     * @note Uses string_view to avoid copying when line is within buffer
     */
    void processLine(std::string_view line);
    
    Config config_;                              ///< Configuration parameters
    std::unique_ptr<KeywordMatcher> matcher_;    ///< Keyword matcher instance
    std::ifstream inStream_;                     ///< Input file stream
    std::ofstream outStream_;                    ///< Output file stream (append mode)
    std::streampos lastPosition_;                ///< Last read position in input file
    std::string partialLine_;                    ///< Accumulator for lines split across buffers
    std::atomic<bool> running_;                  ///< Atomic flag for thread-safe shutdown
    Statistics stats_;                           ///< Runtime statistics
    std::unique_ptr<char[]> buffer_;             ///< Read buffer (size = config.bufferSize)
};
