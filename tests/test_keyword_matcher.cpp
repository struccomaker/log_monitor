#include <gtest/gtest.h>
#include "keyword_matcher.h"

class KeywordMatcherTest : public ::testing::Test {
protected:
    void SetUp() override {
        keywords_ = {"key1", "key2", "ERROR", "EXECUTION"};
        matcher_ = std::make_unique<KeywordMatcher>(keywords_);
    }

    std::vector<std::string> keywords_;
    std::unique_ptr<KeywordMatcher> matcher_;
};

TEST_F(KeywordMatcherTest, MatchesSingleKeyword) {
    EXPECT_TRUE(matcher_->matches("This line contains key1 somewhere"));
    EXPECT_TRUE(matcher_->matches("Another line with key2 in it"));
    EXPECT_TRUE(matcher_->matches("ERROR: something went wrong"));
}

TEST_F(KeywordMatcherTest, MatchesMultipleKeywords) {
    EXPECT_TRUE(matcher_->matches("This has both key1 and key2"));
    EXPECT_TRUE(matcher_->matches("ERROR in key1 processing"));
}

TEST_F(KeywordMatcherTest, NoMatchWhenKeywordAbsent) {
    EXPECT_FALSE(matcher_->matches("This line has nothing interesting"));
    EXPECT_FALSE(matcher_->matches("Just some random text"));
    EXPECT_FALSE(matcher_->matches(""));
}

TEST_F(KeywordMatcherTest, CaseSensitiveMatching) {
    EXPECT_TRUE(matcher_->matches("Found key1 here"));
    EXPECT_FALSE(matcher_->matches("Found KEY1 here")); 
    EXPECT_FALSE(matcher_->matches("Found Key1 here")); 
}

TEST_F(KeywordMatcherTest, KeywordAtDifferentPositions) {
    EXPECT_TRUE(matcher_->matches("key1 at the start"));
    EXPECT_TRUE(matcher_->matches("In the middle key1 somewhere"));
    EXPECT_TRUE(matcher_->matches("At the end key1"));
}

TEST_F(KeywordMatcherTest, PartialMatchDoesNotCount) {
    KeywordMatcher matcher({"key"});
    EXPECT_TRUE(matcher.matches("This has key"));
    EXPECT_TRUE(matcher.matches("This has keyboard")); 
}

TEST_F(KeywordMatcherTest, VeryLongLine) {
    std::string longLine(10000, 'x');
    longLine += "key1";
    longLine += std::string(10000, 'y');
    EXPECT_TRUE(matcher_->matches(longLine));
}

TEST_F(KeywordMatcherTest, EmptyKeywordList) {
    KeywordMatcher emptyMatcher({});
    EXPECT_FALSE(emptyMatcher.matches("Any text here"));
    EXPECT_FALSE(emptyMatcher.matches("key1"));
}
