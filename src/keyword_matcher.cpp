/**
 * @file keyword_matcher.cpp
 * @brief Implementation of the KeywordMatcher class
 * @author Nicholas Loo
 * @date 16/10/25
 */

#include "keyword_matcher.h"
#include <algorithm>

/**
 * @brief Constructs a KeywordMatcher with the given keywords
 * 
 * Uses move semantics to avoid copying the keyword vector, improving
 * performance when constructing with large keyword lists.
 * 
 * @param keywords Vector of keywords to match against (moved, not copied)
 * 
 * Time complexity: O(1) due to move semantics
 * Space complexity: O(n) where n = total size of all keywords
 */
KeywordMatcher::KeywordMatcher(std::vector<std::string> keywords)
    : keywords_(std::move(keywords)) {
}

/**
 * @brief Checks if text contains any of the configured keywords
 * 
 * Performs case-sensitive substring matching. The algorithm iterates through
 * all keywords and uses std::string_view::find() for efficient searching
 * without copying the input text.
 * 
 * @param text Text to search 
 * @return true if at least one keyword is found, false otherwise
 * 
 * Performance:
 * - Best case: O(m) where m = length of first keyword (found immediately)
 * - Worst case: O(n*m) where n = number of keywords, m = text length
 * - Average case: O(k*m) where k = keywords checked before match
 * 
 * 
 * @note Early exit optimization: returns true on first match found
 * @note Thread-safe: const method with no mutable state
 */
bool KeywordMatcher::matches(std::string_view text) const {
    // using sting view
    for (const auto& keyword : keywords_) {
        if (text.find(keyword) != std::string_view::npos) {
            return true;  // early exit on first match
        }
    }
    return false;
}
