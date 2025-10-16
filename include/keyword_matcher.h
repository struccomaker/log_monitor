/**
 * @file keyword_matcher.h
 * @brief  keyword matching using substring search
 * @author Nicholas Loo
 * @date 16/10/25
 * 
 * This module provides fast, case-sensitive keyword matching for log filtering.
 * Uses std::string_view for zero-copy operations to minimize overhead.
 */

#pragma once

#include <string>
#include <vector>
#include <string_view>
#include <algorithm>

/**
 * @class KeywordMatcher
 * @brief High-performance keyword matcher for log filtering
 * 
 * Performs case-sensitive substring matching against a list of keywords.
 * Optimized for minimal memory allocation and CPU overhead.
 * 
 * @note Keywords are matched using substring search, not whole-word matching.
 * @note Matching is case-sensitive: "ERROR" will not match "error"
 * 
 * @endcode
 */
class KeywordMatcher {
public:
    /**
     * @brief Constructs a keyword matcher with the given keywords
     * @param keywords Vector of keywords to match against
     * 
     * Uses move semantics to avoid copying the keyword vector.
     * Keywords are stored internally for the lifetime of the matcher.
     */
    explicit KeywordMatcher(std::vector<std::string> keywords);
    
    /**
     * @brief Checks if the given text contains any of the configured keywords
     * @param text Text to search (uses string_view for zero-copy)
     * @return true if text contains at least one keyword, false otherwise
     * 
     * Performs substring search. For example, if keyword is "key", 
     * it will match "keyboard", "monkey", etc.
     * 
     * Time complexity: O(n*m) where n = number of keywords, m = text length
     * 
     * @note This is a const method and thread-safe for reading
     */
    bool matches(std::string_view text) const;
    
    /**
     * @brief Returns the list of keywords being matched
     * @return Const reference to the internal keyword vector
     * 
     * Useful for debugging and logging which keywords are active.
     */
    const std::vector<std::string>& getKeywords() const { return keywords_; }
    
private:
    std::vector<std::string> keywords_;  ///< List of keywords to match against
};
