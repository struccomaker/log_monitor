/**
 * @file log_generator.cpp
 * @brief testing log generator for testing :) lol
 * @author Nicholas Loo
 * @date 16/10/25
 * 
 * Generates realistic high-frequency log entries for testing
 * the log monitoring system. Creates order entries with timestamps, symbols,
 * prices, quantities, and various status keywords.
 */

#include <iostream>
#include <fstream>
#include <string>
#include <random>
#include <thread>
#include <chrono>
#include <vector>
#include <iomanip>
#include <sstream>

/**
 * @class HFTLogGenerator
 * @brief Generates trading logs
 * 
 * Creates log entries that simulate a high-frequency trading system,
 * including:
 * - Microsecond-precision timestamps
 * - Trading symbols (AAPL, GOOGL, etc.)
 * - Order types (LIMIT, MARKET, STOP, etc.)
 * - Order sides (BUY, SELL)
 * - Prices and quantities
 * - Status keywords (EXECUTION, FILL, REJECT, etc.)
 * - sometimes very long lines (>10,000 chars) for testing truncation
 * 
 */
class HFTLogGenerator {
private:
    std::ofstream logFile_;     ///< Output file stream
    std::mt19937_64 rng_;       ///< Random number generator (Mersenne Twister)
    
    /// Common stock symbols 
    std::vector<std::string> symbols_ = {
        "AAPL", "GOOGL", "MSFT", "AMZN", "TSLA", "META", "NVDA", "JPM", "BAC", "GS"
    };
    
    /// Order types 
    std::vector<std::string> orderTypes_ = {
        "LIMIT", "MARKET", "STOP", "IOC", "FOK", "GTD"
    };
    
    /// Order sides 
    std::vector<std::string> sides_ = {"BUY", "SELL"};
    
    /// Keywords for testing keyword matching 
    std::vector<std::string> keywords_ = {
        "key1", "key2", "EXECUTION", "REJECT", "FILL", "CANCEL", "ERROR", "WARNING"
    };

public:
    /**
     * @brief Constructs a log generator writing to the specified file
     * 
     * Opens the file in append mode to allow multiple generator runs
     * without overwriting previous data.
     * 
     * @param filename Path to output log file
     * @throw std::runtime_error if file cannot be opened
     */
    explicit HFTLogGenerator(const std::string& filename) 
        : rng_(std::random_device{}()) {
        logFile_.open(filename, std::ios::app);
        if (!logFile_.is_open()) {
            throw std::runtime_error("failed to open log file: " + filename);
        }
    }

    /**
     * @brief Destructor 
     */
    ~HFTLogGenerator() {
        if (logFile_.is_open()) {
            logFile_.close();
        }
    }

    /**
     * @brief Generates timestamp with microsecond precision
     * 
     * Format: YYYY-MM-DD HH:MM:SS.microseconds
     * 
     * @return Formatted timestamp string
     * 
     * Uses high_resolution_clock 
     */
    std::string generateTimestamp() {
        auto now = std::chrono::high_resolution_clock::now();
        auto duration = now.time_since_epoch();
        auto micros = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
        
        std::time_t tt = std::chrono::system_clock::to_time_t(
            std::chrono::system_clock::now());
        std::tm tm = *std::localtime(&tt);
        
        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") 
            << "." << std::setfill('0') << std::setw(6) << (micros % 1000000);
        return oss.str();
    }

    /**
     * @brief Generates a normal trading log entry
     * 
     */
    void generateNormalLog() {
        std::uniform_int_distribution<> symbolDist(0, symbols_.size() - 1);
        std::uniform_int_distribution<> orderDist(0, orderTypes_.size() - 1);
        std::uniform_int_distribution<> sideDist(0, sides_.size() - 1);
        std::uniform_int_distribution<> keywordDist(0, keywords_.size() - 1);
        std::uniform_int_distribution<> priceDist(10000, 50000);  
        std::uniform_int_distribution<> qtyDist(100, 10000);
        std::uniform_int_distribution<> orderIdDist(100000, 999999); //no std random 
        
        std::ostringstream log;
        log << "[" << generateTimestamp() << "] "
            << keywords_[keywordDist(rng_)] << " OrderID=" << orderIdDist(rng_)
            << " Symbol=" << symbols_[symbolDist(rng_)]
            << " Side=" << sides_[sideDist(rng_)]
            << " Type=" << orderTypes_[orderDist(rng_)]
            << " Price=" << (priceDist(rng_) / 100.0)
            << " Qty=" << qtyDist(rng_)
            << " Venue=NYSE" //raging bull market
            << " Latency=" << (std::uniform_int_distribution<>(10, 500)(rng_)) << "us";
        
        logFile_ << log.str() << std::endl;
    }

    /**
     * @brief Generates an extremely long log entry for testing truncation
     * 
     * Creates a log line exceeding 15,000 characters to test the monitor's
     * ability to handle and truncate very long lines. This simulates
     * malformed or debug logs that may contain excessive data.
     * 
     */
    void generateLongLog() {
        std::ostringstream log;
        log << "[" << generateTimestamp() << "] key1 MARKET_DATA_SNAPSHOT ";
        
        // generate extremely long line (simulates >10gb with 15000 chars)
        for (int i = 0; i < 15000; ++i) {
            log << "X";
        }
        
        logFile_ << log.str() << std::endl;
    }

    /**
     * @brief Main generation loop
     * 
     * Continuously generates log entries at the specified rate until
     * interrupted (Ctrl+C). 99.9% of logs are normal entries, 0.1% are
     * very long entries for testing.
     * 
     * @param intervalUs Interval between logs in microseconds
     *                   Default: 1000us = 1ms = 1000 logs/second
     * 
     * Examples:
     * - intervalUs = 1000: 1,000 logs/second
     * - intervalUs = 100:  10,000 logs/second
     * - intervalUs = 10:   100,000 logs/second (stress test)
     */
    void run(int intervalUs = 1000) {
        std::cout << "Log Generator started (1 log per " << intervalUs << "us)" << std::endl;
        std::cout << "press Ctrl+C to stop." << std::endl;
        
        std::uniform_int_distribution<> longLineDist(0, 999);
        uint64_t count = 0;
        
        while (true) {
            try {
                // 0.1% chance of long line (1 in 1000) CAN CHANGE HERE TO INCREASE FREQUENCY
                if (longLineDist(rng_) == 0) {
                    generateLongLog();
                } else {
                    generateNormalLog();
                }
                
                logFile_.flush();  // ensure data is written immediately
                count++;
                
                // print progress every 10,000 logs
                if (count % 10000 == 0) {
                    std::cout << "Generated " << count << " log entries" << std::endl;
                }
                
                // sleep for specified interval so dont overload the cpu
                std::this_thread::sleep_for(std::chrono::microseconds(intervalUs));
            } catch (const std::exception& e) {
                std::cerr << "Error: " << e.what() << std::endl;
                break;
            }
        }
    }
};

/**
 * @brief Main entry point for log_generator program
 * 
 * Usage: ./log_generator <output_file> [interval_microseconds]
 * 
 * @param argc Argument count
 * @param argv Argument values
 *             argv[1] = output log file path
 *             argv[2] = interval in microseconds (optional, default: 1000)
 * 
 * @return 0 on success, 1 on error
 */
int main(int argc, char* argv[]) {
    std::string filename = "a.log";
    int intervalUs = 1000; // 1000 logs per second w 1 ms frq
    
    // Parse command-line arguments
    if (argc > 1) filename = argv[1];
    if (argc > 2) intervalUs = std::stoi(argv[2]);
    
    // Print configuration
    std::cout << "=== Log Generator ===" << std::endl;
    std::cout << "Generating trading logs to: " << filename << std::endl;
    std::cout << "Interval: " << intervalUs << " microseconds" << std::endl;
    std::cout << "Rate: ~" << (1000000 / intervalUs) << " logs/second" << std::endl << std::endl;
    
    try {
        HFTLogGenerator generator(filename);
        generator.run(intervalUs);
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
