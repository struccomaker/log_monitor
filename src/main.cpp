/**
 * @file main.cpp
 * @brief Entry point for the log_monitor program
 * @author Nicholas Loo
 * @date 16/10/25
 * 
 * Provides command-line interface for the log monitoring system.
 */

#include "log_monitor.h"
#include <iostream>
#include <sstream>
#include <csignal>
#include <memory>

/// Global monitor instance for signal handler access
std::unique_ptr<LogMonitor> g_monitor;

/**
 * @brief Signal handler for graceful shutdown
 * 
 * Catches SIGINT (Ctrl+C) and SIGTERM (kill command) to stop monitoring
 * cleanly and print statistics before exit.
 * 
 * @param signal Signal number received
 * 
 */
void signalHandler(int signal) {
    std::cout << "\nReceived signal " << signal << ", stopping monitor..." << std::endl;
    if (g_monitor) {
        g_monitor->stop();
    }
}

/**
 * @brief Prompts user for keywords interactively
 * 
 * Reads keywords from stdin, supporting both space and comma separation.
 * If no keywords provided, uses default: "key1", "key2"
 * 
 * @return Vector of keyword strings
 * 
 * @note Strips trailing commas automatically
 */
std::vector<std::string> getKeywordsFromUser() {
    std::vector<std::string> keywords;
    std::string input;
    
    std::cout << "Enter keywords to filter (space or comma separated): ";
    std::getline(std::cin, input);
    
    // parse keywords from input string
    std::stringstream ss(input);
    std::string keyword;
    
    while (ss >> keyword) {
        // remove trailing commas
        if (!keyword.empty() && keyword.back() == ',') {
            keyword.pop_back();
        }
        if (!keyword.empty()) {
            keywords.push_back(keyword);
        }
    }
    
    // use defaults if no keywords provided
    if (keywords.empty()) {
        std::cout << "No keywords provided. Using defaults: key1, key2" << std::endl;
        keywords = {"key1", "key2"};
    }
    
    return keywords;
}

/**
 * @brief Main entry point for log_monitor program
 * 
 * Usage modes:
 * 1. Interactive: ./log_monitor input.log output.log
 *    - Prompts for keywords
 * 
 * 2. Command-line: ./log_monitor input.log output.log key1 key2 key3
 *    - Keywords provided as arguments
 * 
 * @param argc Argument count
 * @param argv Argument values
 *             argv[1] = input log file path
 *             argv[2] = output log file path
 *             argv[3..n] = keywords (optional)
 * 
 * @return 0 on success, 1 on error
 * 
 */
int main(int argc, char* argv[]) {
    // setp signal handlers for graceful shutdown
    std::signal(SIGINT, signalHandler);   // Ctrl+C
    std::signal(SIGTERM, signalHandler);  // kill command
    
    // config monitor with defaults
    LogMonitor::Config config;
    config.inputFile = "a.log";
    config.outputFile = "b.log";
    
    if (argc > 1) config.inputFile = argv[1];
    if (argc > 2) config.outputFile = argv[2];

    std::cout << "=== Log Monitor ===" << std::endl;
    std::cout << "Input file: " << config.inputFile << std::endl;
    std::cout << "Output file: " << config.outputFile << std::endl << std::endl;
    
    if (argc > 3) {
        for (int i = 3; i < argc; ++i) {
            config.keywords.push_back(argv[i]);
        }
    } else {
        //  mode: prompt user
        config.keywords = getKeywordsFromUser();
    }
    
    try {
        // make monitor and store in global for signal handler access
        g_monitor = std::make_unique<LogMonitor>(config);
        
        // start monitoring (blocks until stopped)
        g_monitor->start();

        //print stats
        auto stats = g_monitor->getStatistics();
        std::cout << "\n=== Statistics ===" << std::endl;
        std::cout << "Lines processed: " << stats.linesProcessed << std::endl;
        std::cout << "Lines matched: " << stats.linesMatched << std::endl;
        std::cout << "Bytes read: " << stats.bytesRead << std::endl;
        std::cout << "Long lines discarded: " << stats.longLinesDiscarded << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
