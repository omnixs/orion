/**
 * @file test_feel_matches.cpp
 * @brief Test suite for FEEL matches() function with PCRE2 regex support
 */

// Suppress deprecation warnings for tests using legacy global cache API
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

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
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    nlohmann::json context = nlohmann::json::object();
    
    // XPath/DMN spec: partial (substring) matching is default behavior
    // Per W3C XPath fn:matches: "returns true if input or SOME SUBSTRING matches"
    BOOST_CHECK_EQUAL(Evaluator::evaluate(R"(matches("abcdef", "abc"))", context, eval_ctx), true);  // substring at start
    BOOST_CHECK_EQUAL(Evaluator::evaluate(R"(matches("abcdef", "cde"))", context, eval_ctx), true);  // substring in middle
    BOOST_CHECK_EQUAL(Evaluator::evaluate(R"(matches("abcdef", "def"))", context, eval_ctx), true);  // substring at end
    BOOST_CHECK_EQUAL(Evaluator::evaluate(R"(matches("testing", "test"))", context, eval_ctx), true); // substring match
    
    // Simple patterns
    BOOST_CHECK_EQUAL(Evaluator::evaluate(R"(matches("abc", "a.*"))", context, eval_ctx), true);
    BOOST_CHECK_EQUAL(Evaluator::evaluate(R"(matches("abc", ".*c"))", context, eval_ctx), true);
    BOOST_CHECK_EQUAL(Evaluator::evaluate(R"(matches("abc", "a.c"))", context, eval_ctx), true);
    
    // Exact match
    BOOST_CHECK_EQUAL(Evaluator::evaluate(R"(matches("test", "test"))", context, eval_ctx), true);
    
    // Character classes
    BOOST_CHECK_EQUAL(Evaluator::evaluate(R"(matches("test123", "[a-z]+[0-9]+"))", context, eval_ctx), true);
    BOOST_CHECK_EQUAL(Evaluator::evaluate(R"(matches("ABC", "[A-Z]+"))", context, eval_ctx), true);
    
    // Anchors (PCRE2 in ANCHORED mode matches full string)
    BOOST_CHECK_EQUAL(Evaluator::evaluate(R"(matches("hello", "^hello$"))", context, eval_ctx), true);
    
    // Quantifiers
    BOOST_CHECK_EQUAL(Evaluator::evaluate(R"(matches("aaa", "a+"))", context, eval_ctx), true);
    BOOST_CHECK_EQUAL(Evaluator::evaluate(R"(matches("", "a*"))", context, eval_ctx), true);
    BOOST_CHECK_EQUAL(Evaluator::evaluate(R"(matches("ab", "a?b"))", context, eval_ctx), true);
}

/**
 * Test cases where matches() should return false
 */
BOOST_AUTO_TEST_CASE(test_matches_false_cases)
{
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    nlohmann::json context = nlohmann::json::object();
    
    // Pattern doesn't match
    BOOST_CHECK_EQUAL(Evaluator::evaluate(R"(matches("abc", "x.*"))", context, eval_ctx), false);
    BOOST_CHECK_EQUAL(Evaluator::evaluate(R"(matches("test", "^abc"))", context, eval_ctx), false);
    
    // XPath/DMN spec: partial (substring) matching is default behavior
    // Per W3C XPath: "returns true if input or SOME SUBSTRING matches"
    // To require full-string match, pattern must use anchors: ^pattern$
    BOOST_CHECK_EQUAL(Evaluator::evaluate(R"(matches("abcdef", "^abc$"))", context, eval_ctx), false);
    BOOST_CHECK_EQUAL(Evaluator::evaluate(R"(matches("testing", "^test$"))", context, eval_ctx), false);
    // Character class mismatch
    BOOST_CHECK_EQUAL(Evaluator::evaluate(R"(matches("123", "[a-z]+"))", context, eval_ctx), false);
}

/**
 * Test that invalid regex patterns return null
 */
BOOST_AUTO_TEST_CASE(test_invalid_pattern_returns_null)
{
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    nlohmann::json context = nlohmann::json::object();
    
    // Unclosed bracket
    auto result1 = Evaluator::evaluate(R"(matches("text", "[invalid"))", context, eval_ctx);
    BOOST_CHECK(result1.is_null());
    
    // Unclosed parenthesis
    auto result2 = Evaluator::evaluate(R"(matches("text", "(unclosed"))", context, eval_ctx);
    BOOST_CHECK(result2.is_null());
    
    // Invalid escape sequence
    auto result3 = Evaluator::evaluate(R"(matches("text", "\\k"))", context, eval_ctx);
    BOOST_CHECK(result3.is_null());
    
    // Invalid quantifier
    auto result4 = Evaluator::evaluate(R"(matches("text", "*invalid"))", context, eval_ctx);
    BOOST_CHECK(result4.is_null());
}

/**
 * Test matches() with context variables
 */
BOOST_AUTO_TEST_CASE(test_matches_with_variables)
{
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    nlohmann::json context = {
        {"input_text", "hello world"},
        {"pattern", "hello.*"}
    };
    
    // Use variables from context
    BOOST_CHECK_EQUAL(Evaluator::evaluate(R"(matches(input_text, pattern))", context, eval_ctx), true);
    
    // Pattern that doesn't match
    context["pattern"] = "goodbye.*";
    BOOST_CHECK_EQUAL(Evaluator::evaluate(R"(matches(input_text, pattern))", context, eval_ctx), false);
}

/**
 * Test matches() with null inputs
 */
BOOST_AUTO_TEST_CASE(test_matches_with_null_inputs)
{
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    nlohmann::json context = nlohmann::json::object();
    
    // Null input string - should return null (DMN three-valued logic)
    auto result1 = Evaluator::evaluate(R"(matches(null, "pattern"))", context, eval_ctx);
    BOOST_CHECK(result1.is_null());
    
    // Null pattern - should return null
    auto result2 = Evaluator::evaluate(R"(matches("text", null))", context, eval_ctx);
    BOOST_CHECK(result2.is_null());
    
    // Both null - should return null
    auto result3 = Evaluator::evaluate(R"(matches(null, null))", context, eval_ctx);
    BOOST_CHECK(result3.is_null());
}

/**
 * Test matches() with empty strings
 */
BOOST_AUTO_TEST_CASE(test_matches_with_empty_strings)
{
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    nlohmann::json context = nlohmann::json::object();
    
    // Empty input matches empty pattern
    BOOST_CHECK_EQUAL(Evaluator::evaluate(R"(matches("", ""))", context, eval_ctx), true);
    
    // Empty input matches zero-or-more quantifier
    BOOST_CHECK_EQUAL(Evaluator::evaluate(R"(matches("", "a*"))", context, eval_ctx), true);
    
    // Empty input doesn't match one-or-more quantifier
    BOOST_CHECK_EQUAL(Evaluator::evaluate(R"(matches("", "a+"))", context, eval_ctx), false);
    
    // Non-empty input doesn't match empty pattern
    BOOST_CHECK_EQUAL(Evaluator::evaluate(R"(matches("text", ""))", context, eval_ctx), false);
}

/**
 * Test matches() with special regex characters
 */
BOOST_AUTO_TEST_CASE(test_matches_with_special_characters)
{
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    nlohmann::json context = nlohmann::json::object();
    
    // Literal dot (escaped)
    BOOST_CHECK_EQUAL(Evaluator::evaluate(R"(matches("a.b", "a\\.b"))", context, eval_ctx), true);
    BOOST_CHECK_EQUAL(Evaluator::evaluate(R"(matches("axb", "a\\.b"))", context, eval_ctx), false);
    
    // Wildcard dot
    BOOST_CHECK_EQUAL(Evaluator::evaluate(R"(matches("axb", "a.b"))", context, eval_ctx), true);
    
    // Special characters in character class
    BOOST_CHECK_EQUAL(Evaluator::evaluate(R"(matches("$100", "\\$[0-9]+"))", context, eval_ctx), true);
    
    // Word boundaries
    BOOST_CHECK_EQUAL(Evaluator::evaluate(R"(matches("word", "\\bword\\b"))", context, eval_ctx), true);
}

BOOST_AUTO_TEST_SUITE_END()

#pragma GCC diagnostic pop
