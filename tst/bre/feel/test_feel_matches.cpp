/**
 * @file test_feel_matches.cpp
 * @brief Test suite for FEEL matches() function with PCRE2 regex support
 */

// Suppress deprecation warnings for tests using legacy global cache API
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

#include <boost/test/unit_test.hpp>
#include <orion/bre/feel/evaluator.hpp>
#include <orion/bre/feel/regex_cache.hpp>
#include <nlohmann/json.hpp>
#include <thread>
#include <vector>
#include "test_helpers.hpp"

using namespace orion::bre::feel;
using orion::bre::feel::test::get_test_eval_ctx;

BOOST_AUTO_TEST_SUITE(feel_matches_function_tests)

/**
 * Test cases where matches() should return true
 */
BOOST_AUTO_TEST_CASE(test_matches_true_cases)
{
    
    // XPath/DMN spec: partial (substring) matching is default behavior
    // Per W3C XPath fn:matches: "returns true if input or SOME SUBSTRING matches"
    BOOST_CHECK_EQUAL(Evaluator::evaluate(R"(matches("abcdef", "abc"))", {}, get_test_eval_ctx()), true);  // substring at start
    BOOST_CHECK_EQUAL(Evaluator::evaluate(R"(matches("abcdef", "cde"))", {}, get_test_eval_ctx()), true);  // substring in middle
    BOOST_CHECK_EQUAL(Evaluator::evaluate(R"(matches("abcdef", "def"))", {}, get_test_eval_ctx()), true);  // substring at end
    BOOST_CHECK_EQUAL(Evaluator::evaluate(R"(matches("testing", "test"))", {}, get_test_eval_ctx()), true); // substring match
    
    // Simple patterns
    BOOST_CHECK_EQUAL(Evaluator::evaluate(R"(matches("abc", "a.*"))", {}, get_test_eval_ctx()), true);
    BOOST_CHECK_EQUAL(Evaluator::evaluate(R"(matches("abc", ".*c"))", {}, get_test_eval_ctx()), true);
    BOOST_CHECK_EQUAL(Evaluator::evaluate(R"(matches("abc", "a.c"))", {}, get_test_eval_ctx()), true);
    
    // Exact match
    BOOST_CHECK_EQUAL(Evaluator::evaluate(R"(matches("test", "test"))", {}, get_test_eval_ctx()), true);
    
    // Character classes
    BOOST_CHECK_EQUAL(Evaluator::evaluate(R"(matches("test123", "[a-z]+[0-9]+"))", {}, get_test_eval_ctx()), true);
    BOOST_CHECK_EQUAL(Evaluator::evaluate(R"(matches("ABC", "[A-Z]+"))", {}, get_test_eval_ctx()), true);
    
    // Anchors (PCRE2 in ANCHORED mode matches full string)
    BOOST_CHECK_EQUAL(Evaluator::evaluate(R"(matches("hello", "^hello$"))", {}, get_test_eval_ctx()), true);
    
    // Quantifiers
    BOOST_CHECK_EQUAL(Evaluator::evaluate(R"(matches("aaa", "a+"))", {}, get_test_eval_ctx()), true);
    BOOST_CHECK_EQUAL(Evaluator::evaluate(R"(matches("", "a*"))", {}, get_test_eval_ctx()), true);
    BOOST_CHECK_EQUAL(Evaluator::evaluate(R"(matches("ab", "a?b"))", {}, get_test_eval_ctx()), true);
}

/**
 * Test cases where matches() should return false
 */
BOOST_AUTO_TEST_CASE(test_matches_false_cases)
{
    // Pattern doesn't match
    BOOST_CHECK_EQUAL(Evaluator::evaluate(R"(matches("abc", "x.*"))", {}, get_test_eval_ctx()), false);
    BOOST_CHECK_EQUAL(Evaluator::evaluate(R"(matches("test", "^abc"))", {}, get_test_eval_ctx()), false);
    
    // XPath/DMN spec: partial (substring) matching is default behavior
    // Per W3C XPath: "returns true if input or SOME SUBSTRING matches"
    // To require full-string match, pattern must use anchors: ^pattern$
    BOOST_CHECK_EQUAL(Evaluator::evaluate(R"(matches("abcdef", "^abc$"))", {}, get_test_eval_ctx()), false);
    BOOST_CHECK_EQUAL(Evaluator::evaluate(R"(matches("testing", "^test$"))", {}, get_test_eval_ctx()), false);
    // Character class mismatch
    BOOST_CHECK_EQUAL(Evaluator::evaluate(R"(matches("123", "[a-z]+"))", {}, get_test_eval_ctx()), false);
}

/**
 * Test that invalid regex patterns return null
 */
BOOST_AUTO_TEST_CASE(test_invalid_pattern_returns_null)
{
    // Unclosed bracket
    auto result1 = Evaluator::evaluate(R"(matches("text", "[invalid"))", {}, get_test_eval_ctx());
    BOOST_CHECK(result1.is_null());
    
    // Unclosed parenthesis
    auto result2 = Evaluator::evaluate(R"(matches("text", "(unclosed"))", {}, get_test_eval_ctx());
    BOOST_CHECK(result2.is_null());
    
    // Invalid escape sequence
    auto result3 = Evaluator::evaluate(R"(matches("text", "\\k"))", {}, get_test_eval_ctx());
    BOOST_CHECK(result3.is_null());
    
    // Invalid quantifier
    auto result4 = Evaluator::evaluate(R"(matches("text", "*invalid"))", {}, get_test_eval_ctx());
    BOOST_CHECK(result4.is_null());
}

/**
 * Test matches() with context variables
 */
BOOST_AUTO_TEST_CASE(test_matches_with_variables)
{
    nlohmann::json input = {
        {"input_text", "hello world"},
        {"pattern", "hello.*"}
    };
    
    // Use variables from context
    BOOST_CHECK_EQUAL(Evaluator::evaluate(R"(matches(input_text, pattern))", input, get_test_eval_ctx()), true);
    
    // Pattern that doesn't match
    input["pattern"] = "goodbye.*";
    BOOST_CHECK_EQUAL(Evaluator::evaluate(R"(matches(input_text, pattern))", input, get_test_eval_ctx()), false);
}

/**
 * Test matches() with null inputs
 */
BOOST_AUTO_TEST_CASE(test_matches_with_null_inputs)
{
    // Null input string - should return null (DMN three-valued logic)
    auto result1 = Evaluator::evaluate(R"(matches(null, "pattern"))", {}, get_test_eval_ctx());
    BOOST_CHECK(result1.is_null());
    
    // Null pattern - should return null
    auto result2 = Evaluator::evaluate(R"(matches("text", null))", {}, get_test_eval_ctx());
    BOOST_CHECK(result2.is_null());
    
    // Both null - should return null
    auto result3 = Evaluator::evaluate(R"(matches(null, null))", {}, get_test_eval_ctx());
    BOOST_CHECK(result3.is_null());
}

/**
 * Test matches() with empty strings
 */
BOOST_AUTO_TEST_CASE(test_matches_with_empty_strings)
{
    // Empty input matches empty pattern
    BOOST_CHECK_EQUAL(Evaluator::evaluate(R"(matches("", ""))", {}, get_test_eval_ctx()), true);
    
    // Empty input matches zero-or-more quantifier
    BOOST_CHECK_EQUAL(Evaluator::evaluate(R"(matches("", "a*"))", {}, get_test_eval_ctx()), true);
    
    // Empty input doesn't match one-or-more quantifier
    BOOST_CHECK_EQUAL(Evaluator::evaluate(R"(matches("", "a+"))", {}, get_test_eval_ctx()), false);
    
    // Non-empty input doesn't match empty pattern
    BOOST_CHECK_EQUAL(Evaluator::evaluate(R"(matches("text", ""))", {}, get_test_eval_ctx()), false);
}

/**
 * Test matches() with special regex characters
 */
BOOST_AUTO_TEST_CASE(test_matches_with_special_characters)
{
    // Literal dot (escaped)
    BOOST_CHECK_EQUAL(Evaluator::evaluate(R"(matches("a.b", "a\\.b"))", {}, get_test_eval_ctx()), true);
    BOOST_CHECK_EQUAL(Evaluator::evaluate(R"(matches("axb", "a\\.b"))", {}, get_test_eval_ctx()), false);
    
    // Wildcard dot
    BOOST_CHECK_EQUAL(Evaluator::evaluate(R"(matches("axb", "a.b"))", {}, get_test_eval_ctx()), true);
    
    // Special characters in character class
    BOOST_CHECK_EQUAL(Evaluator::evaluate(R"(matches("$100", "\\$[0-9]+"))", {}, get_test_eval_ctx()), true);
    
    // Word boundaries
    BOOST_CHECK_EQUAL(Evaluator::evaluate(R"(matches("word", "\\bword\\b"))", {}, get_test_eval_ctx()), true);
}

BOOST_AUTO_TEST_SUITE_END()

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif
