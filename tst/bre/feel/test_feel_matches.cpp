/**
 * @file test_feel_matches.cpp
 * @brief Test suite for FEEL matches() function with PCRE2 regex support
 */

#include <boost/test/unit_test.hpp>
#include <orion/bre/feel/evaluator.hpp>
#include <orion/bre/feel/regex_cache.hpp>
#include <nlohmann/json.hpp>
#include <thread>
#include <vector>

using namespace orion::bre::feel;

BOOST_AUTO_TEST_SUITE(feel_matches_function_tests)

/**
 * Test cases where matches() should return true
 */
BOOST_AUTO_TEST_CASE(test_matches_true_cases)
{
    nlohmann::json context = nlohmann::json::object();
    
    // XPath/DMN spec: partial (substring) matching is default behavior
    // Per W3C XPath fn:matches: "returns true if input or SOME SUBSTRING matches"
    BOOST_CHECK_EQUAL(Evaluator::evaluate(R"(matches("abcdef", "abc"))", context), true);  // substring at start
    BOOST_CHECK_EQUAL(Evaluator::evaluate(R"(matches("abcdef", "cde"))", context), true);  // substring in middle
    BOOST_CHECK_EQUAL(Evaluator::evaluate(R"(matches("abcdef", "def"))", context), true);  // substring at end
    BOOST_CHECK_EQUAL(Evaluator::evaluate(R"(matches("testing", "test"))", context), true); // substring match
    
    // Simple patterns
    BOOST_CHECK_EQUAL(Evaluator::evaluate(R"(matches("abc", "a.*"))", context), true);
    BOOST_CHECK_EQUAL(Evaluator::evaluate(R"(matches("abc", ".*c"))", context), true);
    BOOST_CHECK_EQUAL(Evaluator::evaluate(R"(matches("abc", "a.c"))", context), true);
    
    // Exact match
    BOOST_CHECK_EQUAL(Evaluator::evaluate(R"(matches("test", "test"))", context), true);
    
    // Character classes
    BOOST_CHECK_EQUAL(Evaluator::evaluate(R"(matches("test123", "[a-z]+[0-9]+"))", context), true);
    BOOST_CHECK_EQUAL(Evaluator::evaluate(R"(matches("ABC", "[A-Z]+"))", context), true);
    
    // Anchors (PCRE2 in ANCHORED mode matches full string)
    BOOST_CHECK_EQUAL(Evaluator::evaluate(R"(matches("hello", "^hello$"))", context), true);
    
    // Quantifiers
    BOOST_CHECK_EQUAL(Evaluator::evaluate(R"(matches("aaa", "a+"))", context), true);
    BOOST_CHECK_EQUAL(Evaluator::evaluate(R"(matches("", "a*"))", context), true);
    BOOST_CHECK_EQUAL(Evaluator::evaluate(R"(matches("ab", "a?b"))", context), true);
}

/**
 * Test cases where matches() should return false
 */
BOOST_AUTO_TEST_CASE(test_matches_false_cases)
{
    nlohmann::json context = nlohmann::json::object();
    
    // Pattern doesn't match
    BOOST_CHECK_EQUAL(Evaluator::evaluate(R"(matches("abc", "x.*"))", context), false);
    BOOST_CHECK_EQUAL(Evaluator::evaluate(R"(matches("test", "^abc"))", context), false);
    
    // XPath/DMN spec: partial (substring) matching is default behavior
    // Per W3C XPath: "returns true if input or SOME SUBSTRING matches"
    // To require full-string match, pattern must use anchors: ^pattern$
    BOOST_CHECK_EQUAL(Evaluator::evaluate(R"(matches("abcdef", "^abc$"))", context), false);
    BOOST_CHECK_EQUAL(Evaluator::evaluate(R"(matches("testing", "^test$"))", context), false);
    // Character class mismatch
    BOOST_CHECK_EQUAL(Evaluator::evaluate(R"(matches("123", "[a-z]+"))", context), false);
}

/**
 * Test that invalid regex patterns return null
 */
BOOST_AUTO_TEST_CASE(test_invalid_pattern_returns_null)
{
    nlohmann::json context = nlohmann::json::object();
    
    // Unclosed bracket
    auto result1 = Evaluator::evaluate(R"(matches("text", "[invalid"))", context);
    BOOST_CHECK(result1.is_null());
    
    // Unclosed parenthesis
    auto result2 = Evaluator::evaluate(R"(matches("text", "(unclosed"))", context);
    BOOST_CHECK(result2.is_null());
    
    // Invalid escape sequence
    auto result3 = Evaluator::evaluate(R"(matches("text", "\\k"))", context);
    BOOST_CHECK(result3.is_null());
    
    // Invalid quantifier
    auto result4 = Evaluator::evaluate(R"(matches("text", "*invalid"))", context);
    BOOST_CHECK(result4.is_null());
}

/**
 * Test that the same pattern is compiled only once (cache behavior)
 */
BOOST_AUTO_TEST_CASE(test_cache_behavior)
{
    nlohmann::json context = nlohmann::json::object();
    
    // Clear cache to start fresh
#if defined(__GNUC__) || defined(__clang__)
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
    get_regex_cache().clear();
    BOOST_CHECK_EQUAL(get_regex_cache().size(), 0);
#if defined(__GNUC__) || defined(__clang__)
    #pragma GCC diagnostic pop
#endif
    
    // First use - should compile and cache
    BOOST_CHECK_EQUAL(Evaluator::evaluate(R"(matches("test", "t.*t"))", context), true);
#if defined(__GNUC__) || defined(__clang__)
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
    BOOST_CHECK_EQUAL(get_regex_cache().size(), 1);
#if defined(__GNUC__) || defined(__clang__)
    #pragma GCC diagnostic pop
#endif
    
    // Second use of same pattern - should use cache (size stays at 1)
    BOOST_CHECK_EQUAL(Evaluator::evaluate(R"(matches("text", "t.*t"))", context), true);
#if defined(__GNUC__) || defined(__clang__)
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
    BOOST_CHECK_EQUAL(get_regex_cache().size(), 1);
#if defined(__GNUC__) || defined(__clang__)
    #pragma GCC diagnostic pop
#endif
    
    // Different pattern - should compile and cache
    BOOST_CHECK_EQUAL(Evaluator::evaluate(R"(matches("abc", "a.*c"))", context), true);
#if defined(__GNUC__) || defined(__clang__)
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
    BOOST_CHECK_EQUAL(get_regex_cache().size(), 2);
#if defined(__GNUC__) || defined(__clang__)
    #pragma GCC diagnostic pop
#endif
    
    // Use first pattern again - should still be cached
    BOOST_CHECK_EQUAL(Evaluator::evaluate(R"(matches("toast", "t.*t"))", context), true);
#if defined(__GNUC__) || defined(__clang__)
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
    BOOST_CHECK_EQUAL(get_regex_cache().size(), 2);
#if defined(__GNUC__) || defined(__clang__)
    #pragma GCC diagnostic pop
#endif
}

/**
 * Test LRU eviction behavior when cache is full
 */
BOOST_AUTO_TEST_CASE(test_cache_eviction)
{
    nlohmann::json context = nlohmann::json::object();
    
    // Clear cache and get max size
#if defined(__GNUC__) || defined(__clang__)
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
    get_regex_cache().clear();
    size_t max_size = get_regex_cache().max_size();
#if defined(__GNUC__) || defined(__clang__)
    #pragma GCC diagnostic pop
#endif
    
    // Fill cache to capacity
    for (size_t i = 0; i < max_size; ++i) {
        std::string pattern = "pattern" + std::to_string(i);
        std::string expr = R"(matches("text", ")" + pattern + R"("))";
        auto result = Evaluator::evaluate(expr, context);
        (void)result; // Intentionally unused - just filling cache
    }
    
#if defined(__GNUC__) || defined(__clang__)
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
    BOOST_CHECK_EQUAL(get_regex_cache().size(), max_size);
#if defined(__GNUC__) || defined(__clang__)
    #pragma GCC diagnostic pop
#endif
    
    // Add one more - should evict LRU and stay at max_size
    // Use pattern that actually matches "text" to verify cache works after eviction
    auto overflow_result = Evaluator::evaluate(R"(matches("text", ".*"))", context);
    BOOST_CHECK_EQUAL(overflow_result, true);
#if defined(__GNUC__) || defined(__clang__)
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
    BOOST_CHECK_EQUAL(get_regex_cache().size(), max_size);
#if defined(__GNUC__) || defined(__clang__)
    #pragma GCC diagnostic pop
#endif
}

/**
 * Test warmup_regex_cache function
 */
BOOST_AUTO_TEST_CASE(test_warmup_function)
{
    // Warmup should not throw
    BOOST_CHECK_NO_THROW(warmup_regex_cache());
    
    // Calling multiple times should be safe (idempotent)
    BOOST_CHECK_NO_THROW(warmup_regex_cache());
    BOOST_CHECK_NO_THROW(warmup_regex_cache());
}

/**
 * Test thread safety of matches() function
 */
BOOST_AUTO_TEST_CASE(test_concurrent_matches)
{
    nlohmann::json context = nlohmann::json::object();
    
    // Clear cache
#if defined(__GNUC__) || defined(__clang__)
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
    get_regex_cache().clear();
#if defined(__GNUC__) || defined(__clang__)
    #pragma GCC diagnostic pop
#endif
    
    auto match_func = [&context]([[maybe_unused]] int thread_id) {
        for (int i = 0; i < 100; ++i) {
            // Use a mix of patterns - just execute without logging to avoid output garbling
            auto r1 = Evaluator::evaluate(R"(matches("test", "t.*t"))", context);
            auto r2 = Evaluator::evaluate(R"(matches("abc", "a.*c"))", context);
            auto r3 = Evaluator::evaluate(R"(matches("xyz", "x.*z"))", context);
            // Silent validation - thread safety is what we're testing
            (void)r1; (void)r2; (void)r3;
        }
    };
    
    // Create multiple threads
    std::vector<std::thread> threads;
    for (int i = 0; i < 4; ++i) {
        threads.emplace_back(match_func, i);
    }
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Cache should contain the 3 unique patterns used
#if defined(__GNUC__) || defined(__clang__)
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
    BOOST_CHECK_EQUAL(get_regex_cache().size(), 3);
#if defined(__GNUC__) || defined(__clang__)
    #pragma GCC diagnostic pop
#endif
}

/**
 * Test matches() with context variables
 */
BOOST_AUTO_TEST_CASE(test_matches_with_variables)
{
    nlohmann::json context = {
        {"input_text", "hello world"},
        {"pattern", "hello.*"}
    };
    
    // Use variables from context
    BOOST_CHECK_EQUAL(Evaluator::evaluate(R"(matches(input_text, pattern))", context), true);
    
    // Pattern that doesn't match
    context["pattern"] = "goodbye.*";
    BOOST_CHECK_EQUAL(Evaluator::evaluate(R"(matches(input_text, pattern))", context), false);
}

/**
 * Test matches() with null inputs
 */
BOOST_AUTO_TEST_CASE(test_matches_with_null_inputs)
{
    nlohmann::json context = nlohmann::json::object();
    
    // Null input string - should return null (DMN three-valued logic)
    auto result1 = Evaluator::evaluate(R"(matches(null, "pattern"))", context);
    BOOST_CHECK(result1.is_null());
    
    // Null pattern - should return null
    auto result2 = Evaluator::evaluate(R"(matches("text", null))", context);
    BOOST_CHECK(result2.is_null());
    
    // Both null - should return null
    auto result3 = Evaluator::evaluate(R"(matches(null, null))", context);
    BOOST_CHECK(result3.is_null());
}

/**
 * Test matches() with empty strings
 */
BOOST_AUTO_TEST_CASE(test_matches_with_empty_strings)
{
    nlohmann::json context = nlohmann::json::object();
    
    // Empty input matches empty pattern
    BOOST_CHECK_EQUAL(Evaluator::evaluate(R"(matches("", ""))", context), true);
    
    // Empty input matches zero-or-more quantifier
    BOOST_CHECK_EQUAL(Evaluator::evaluate(R"(matches("", "a*"))", context), true);
    
    // Empty input doesn't match one-or-more quantifier
    BOOST_CHECK_EQUAL(Evaluator::evaluate(R"(matches("", "a+"))", context), false);
    
    // Non-empty input doesn't match empty pattern
    BOOST_CHECK_EQUAL(Evaluator::evaluate(R"(matches("text", ""))", context), false);
}

/**
 * Test matches() with special regex characters
 */
BOOST_AUTO_TEST_CASE(test_matches_with_special_characters)
{
    nlohmann::json context = nlohmann::json::object();
    
    // Literal dot (escaped)
    BOOST_CHECK_EQUAL(Evaluator::evaluate(R"(matches("a.b", "a\\.b"))", context), true);
    BOOST_CHECK_EQUAL(Evaluator::evaluate(R"(matches("axb", "a\\.b"))", context), false);
    
    // Wildcard dot
    BOOST_CHECK_EQUAL(Evaluator::evaluate(R"(matches("axb", "a.b"))", context), true);
    
    // Special characters in character class
    BOOST_CHECK_EQUAL(Evaluator::evaluate(R"(matches("$100", "\\$[0-9]+"))", context), true);
    
    // Word boundaries
    BOOST_CHECK_EQUAL(Evaluator::evaluate(R"(matches("word", "\\bword\\b"))", context), true);
}

BOOST_AUTO_TEST_SUITE_END()

