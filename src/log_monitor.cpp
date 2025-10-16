/**
 * @file log_monitor.cpp
 * @brief Implementation of the LogMonitor class
 * @author Nicholas Loo
 * @date 16/10/25
 * 
 * Core implementation of real-time log monitoring with keyword filtering.
 * Optimized for high-frequency trading environments with focus on:
 * - Constant memory usage regardless of file size
 * - Low latency 
 */

#include "log_monitor.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <cstring>

/**
 * @brief Constructs a LogMonitor with the given configuration
 * 
 * Opens the output file in append mode with binary flag to avoid text
 * translation overhead. Allocates the read buffer and keyword matcher.
 * 
 * @param config Configuration including file paths, keywords, and tuning parameters
 * @throw std::runtime_error if output file cannot be opened
 * 
 * Memory allocations:
 * - Read buffer: config.bufferSize bytes (default 64KB)
 * - Partial line buffer: reserved to MAX_LINE_LENGTH (5000 bytes)
 * - KeywordMatcher: O(k) where k = total keyword string size
 * 
 * @note Input file is NOT opened here - it's opened when start() is called
 */
LogMonitor::LogMonitor(const Config& config)
    : config_(config),
      matcher_(std::make_unique<KeywordMatcher>(config.keywords)),
      lastPosition_(0),
      running_(false),
      buffer_(std::make_unique<char[]>(config.bufferSize)) {
    
    // open output file in append + binary mode
    // append: don't overwrite existing data (safe for restarts)
    // binary: avoid newline translation overhead
    outStream_.open(config_.outputFile, std::ios::app | std::ios::binary);
    if (!outStream_.is_open()) {
        throw std::runtime_error("Failed to open output file: " + config_.outputFile);
    }
    
    // reserve space for partial line to avoid repeated reallocations
    // during string concatenation across buffer boundaries
    partialLine_.reserve(MAX_LINE_LENGTH);
}

/**
 * @brief Destructor - ensures clean shutdown and file closure
 * 
 * Calls stop() to halt monitoring loop, then closes input and output streams.
 * Destructor guarantees cleanup even if stop() wasn't called explicitly.
 */
LogMonitor::~LogMonitor() {
    stop();
    if (inStream_.is_open()) {
        inStream_.close();
    }
    if (outStream_.is_open()) {
        outStream_.close();
    }
}

/**
 * @brief Processes a single complete line
 * 
 * Processing steps:
 * 1. Skip empty lines (no-op)
 * 2. Increment linesProcessed counter
 * 3. Truncate if line exceeds MAX_LINE_LENGTH
 * 4. Check for keyword match
 * 5. If match: write to output file with immediate flush
 * 
 * @param line Line to process (string_view for zero-copy when possible)
 * 
 * 
 * @note Flush overhead is acceptable because matched lines are relatively rare
 *       compared to total lines processed (typically <10% match rate)
 */
void LogMonitor::processLine(std::string_view line) {
    if (line.empty()) return;
    
    stats_.linesProcessed++;
    
    // truncate if exceeds max length
    std::string_view processedLine = line;
    if (processedLine.length() > MAX_LINE_LENGTH) {
        processedLine = processedLine.substr(0, MAX_LINE_LENGTH);
        stats_.longLinesDiscarded++;
    }
    
    // check for keyword match
    if (matcher_->matches(processedLine)) {
        stats_.linesMatched++;
        outStream_.write(processedLine.data(), processedLine.length());
        outStream_.put('\n');
        outStream_.flush(); // MUST FLUSH YES -stpud bug
    }
}

/**
 * @brief Processes a buffer of data read from input file
 * 
 * Implements a state machine to handle lines split across buffer boundaries:
 * 
 * 
 * State 1: Complete line within buffer
 * 
 * State 2: Partial line at end of buffer
 *
 * State 3: Continuation from previous buffer
 * 
 * @param buffer Pointer to buffer containing data
 * @param bytesRead Number of valid bytes in buffer
 * 
 * Memory safety:
 * - Prevents unbounded growth of partialLine_ via MAX_LINE_LENGTH check
 * - Even a 10GB single line consumes max 5000 bytes memory
 * 
 * Performance:
 * - Single pass through buffer: O(bytesRead)
 * - Zero-copy processing when line fits entirely in buffer
 * - String concatenation only when lines span buffers
 */
void LogMonitor::processBuffer(const char* buffer, size_t bytesRead) {
    stats_.bytesRead += bytesRead;
    size_t start = 0;
    
    // scan for newline characters
    for (size_t i = 0; i < bytesRead; ++i) {
        if (buffer[i] == '\n') {
            // complete line found
            size_t segmentLen = i - start;
            
            if (!partialLine_.empty()) {
                // append to partial line from previous buffer
                partialLine_.append(buffer + start, segmentLen);
                processLine(partialLine_);
                partialLine_.clear();
            } else {
                // process directly from buffer 
                processLine(std::string_view(buffer + start, segmentLen));
            }
            
            start = i + 1;
        }
    }
    
    // handle remaining partial line (no newline found before buffer end)
    if (start < bytesRead) {
        size_t remainingLen = bytesRead - start;
        
        // if partial line + remaining exceeds MAX_LINE_LENGTH, process and discard
        // this prevents memory exhaustion from malformed input 
        if (partialLine_.length() + remainingLen >= MAX_LINE_LENGTH) {
            partialLine_.append(buffer + start, 
                               MAX_LINE_LENGTH - partialLine_.length());
            processLine(partialLine_);
            partialLine_.clear();
            stats_.longLinesDiscarded++;
        } else {
            partialLine_.append(buffer + start, remainingLen);
        }
    }
}

/**
 * @brief Main monitoring loop - runs until stop() is called
 * 
 * Monitoring algorithm:
 * 1. Open input file (if not already open)
 * 2. Seek to last known position (for resume after file rotation)
 * 3. Read available data in chunks
 * 4. Process each buffer
 * 5. Track file position
 * 6. Handle EOF: clear flag and continue polling
 * 7. Handle errors: close file, reset position, retry
 * 8. Sleep if no data available (avoid busy-wait)
 * 
 * This is a blocking call - typically run in a separate thread.
 * 
 * File rotation handling:
 * - If read fails (file deleted/rotated), close and reopen on next iteration
 * - Position resets to 0, effectively starting from new file
 * 
 * Performance tuning:
 * - pollIntervalMs controls latency vs CPU usage tradeoff
 * - Smaller interval = lower latency, higher CPU
 * - Default 10ms is optimal for HFT (sub-100ms latency requirement)
 * 
 * @note Blocks until stop() is called from another thread or signal handler
 * @see stop()
 */
void LogMonitor::start() {
    running_ = true;
    

    std::cout << "Starting log monitor..." << std::endl;
    std::cout << "Input: " << config_.inputFile << std::endl;
    std::cout << "Output: " << config_.outputFile << std::endl;
    std::cout << "Keywords: ";
    for (size_t i = 0; i < config_.keywords.size(); ++i) {
        std::cout << config_.keywords[i];
        if (i < config_.keywords.size() - 1) std::cout << ", ";
    }
    std::cout << std::endl << std::endl;
    
    while (running_) {
        // open file if not open
        if (!inStream_.is_open()) {
            inStream_.open(config_.inputFile, std::ios::binary);
            if (inStream_.is_open()) {
                inStream_.seekg(lastPosition_);
            } else {
                // file doesn't exist yet try one more time
                std::this_thread::sleep_for(
                    std::chrono::milliseconds(config_.pollIntervalMs));
                continue;
            }
        }
        
        // read available data
        bool dataRead = false;
        while (inStream_.good()) {
            inStream_.read(buffer_.get(), config_.bufferSize);
            std::streamsize bytesRead = inStream_.gcount();
            
            if (bytesRead > 0) {
                dataRead = true;
                processBuffer(buffer_.get(), bytesRead);
                lastPosition_ = inStream_.tellg();
            }
            
            if (inStream_.eof()) {
                inStream_.clear();  // clear EOF flag to continue polling
                break;
            }
            
            if (inStream_.fail() && !inStream_.eof()) {
                // errpr reading file 
                inStream_.close();
                lastPosition_ = 0;
                partialLine_.clear();
                break;
            }
        }
        
        //avoid busy wait for sleeping due to no reading of data
        if (!dataRead) {
            std::this_thread::sleep_for(
                std::chrono::milliseconds(config_.pollIntervalMs));
        }
    }
}

/**
 * @brief Stops the monitoring loop
 * 
 * Sets the atomic running_ flag to false, causing the monitoring loop
 * to exit after completing the current buffer. This method is thread-safe
 * and can be called from signal handlers.
 * 
 * 
 * @note Thread-safe via std::atomic<bool>
 * @note Safe to call from signal handlers (SIGINT, SIGTERM)
 */
void LogMonitor::stop() {
    running_ = false;
}

/**
 * @brief Returns a copy of current statistics
 * 
 * Provides operational visibility into monitoring performance without
 * impacting the monitoring loop. Safe to call while monitoring is active.
 * 
 * @return Statistics struct containing current counters
 * 
 * 
 * @note Returns a copy, NOT a reference, to avoid race conditions
 */
LogMonitor::Statistics LogMonitor::getStatistics() const {
    return stats_;
}
