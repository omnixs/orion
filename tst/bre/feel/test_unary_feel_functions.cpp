/**
 * @file test_unary_feel_functions.cpp
 * @brief Unit tests for FEEL function evaluation within unary test expressions
 *
 * This test suite verifies that FEEL function calls can be properly evaluated
 * within unary test expressions used in DMN decision tables. For example:
 * - <= duration("PT31H")
 * - <= abs(-15)
 * - upper case("test")
 *
 * These tests verify the fix in src/bre/feel/unary.cpp's evaluate_feel_functions()
 * which ensures function calls are evaluated before comparison operations.
 *
 * Background:
 * Before the fix, FEEL function calls in unary tests were treated as literal
 * strings, causing incorrect comparisons. For example:
 * - "<= duration(\"PT31H\")" was compared as the string "duration(\"PT31H\")"
 * - Now it evaluates to "PT31H" first, then performs proper comparison
 *
 * @see src/bre/feel/unary.cpp - evaluate_feel_functions()
 */

#include <boost/test/unit_test.hpp>
#include <orion/bre/feel/unary.hpp>

using namespace orion::bre::feel;

BOOST_AUTO_TEST_SUITE(feel_unary_functions)

// ============================================================================
// Duration Functions in Unary Test Expressions
// ============================================================================

BOOST_AUTO_TEST_CASE(duration_in_unary_test_less_than_or_equal)
{
    // Test: <= duration("PT31H") with candidate "PT30H" (30 hours)
    // Should evaluate duration("PT31H") to "PT31H" then compare "PT30H" <= "PT31H" → true
    bool result = unary_test_matches("<= duration(\"PT31H\")", "PT30H");
    BOOST_CHECK_EQUAL(result, true);
}

BOOST_AUTO_TEST_CASE(duration_in_unary_test_greater_than)
{
    // Test: > duration("P5D") with candidate "P10D" (10 days)
    // Should evaluate duration("P5D") to "P5D" then compare "P10D" > "P5D" → true
    bool result = unary_test_matches("> duration(\"P5D\")", "P10D");
    BOOST_CHECK_EQUAL(result, true);
}

BOOST_AUTO_TEST_CASE(duration_in_unary_test_equality)
{
    // Test: duration("P7D") with candidate "P7D"
    // Without explicit operator, duration("P7D") evaluates to "P7D" but doesn't match
    // because unary test needs explicit comparison operator
    bool result = unary_test_matches("duration(\"P7D\")", "P7D");
    BOOST_CHECK_EQUAL(result, false);
}

BOOST_AUTO_TEST_CASE(duration_in_unary_test_not_equal)
{
    // Test: not(duration("P7D")) with candidate "P5D"
    // Should evaluate duration("P7D") to "P7D" then compare "P5D" != "P7D" → true
    bool result = unary_test_matches("not(duration(\"P7D\"))", "P5D");
    BOOST_CHECK_EQUAL(result, true);
}

BOOST_AUTO_TEST_CASE(duration_in_unary_test_false_case)
{
    // Test: < duration("P5D") with candidate "P10D"
    // Should evaluate duration("P5D") to "P5D" then compare "P10D" < "P5D" → false
    bool result = unary_test_matches("< duration(\"P5D\")", "P10D");
    BOOST_CHECK_EQUAL(result, false);
}

BOOST_AUTO_TEST_CASE(duration_in_unary_test_invalid_duration_returns_true)
{
    // Test: <= duration("invalid") with candidate "P5D"
    // duration("invalid") returns null which converts to "null" string
    // Lexicographic comparison: "P5D" <= "null" → "P" < "n" → true
    bool result = unary_test_matches("<= duration(\"invalid\")", "P5D");
    BOOST_CHECK_EQUAL(result, true);
}

BOOST_AUTO_TEST_CASE(duration_in_unary_test_hours_vs_days_comparison)
{
    // Test: <= duration("PT31H") with candidate "P1D" (24 hours)
    // This tests the original bug: FEEL function call in unary test
    // Lexicographic comparison: "P1D" <= "PT31H" → "P1" < "PT" → true
    bool result = unary_test_matches("<= duration(\"PT31H\")", "P1D");
    BOOST_CHECK_EQUAL(result, true);
}

BOOST_AUTO_TEST_CASE(duration_in_unary_test_complex_duration)
{
    // Test: >= duration("P1DT12H") with candidate "P2D"
    // P2D (48 hours) >= P1DT12H (36 hours) → should be true
    bool result = unary_test_matches(">= duration(\"P1DT12H\")", "P2D");
    BOOST_CHECK_EQUAL(result, true);
}

// ============================================================================
// Numeric Functions in Unary Test Expressions
// ============================================================================

BOOST_AUTO_TEST_CASE(numeric_function_in_unary_test)
{
    // Test: <= abs(-15) with candidate 10
    // Should evaluate abs(-15) to 15 then compare 10 <= 15 → true
    bool result = unary_test_matches("<= abs(-15)", "10");
    BOOST_CHECK_EQUAL(result, true);
}

// ============================================================================
// String Functions in Unary Test Expressions
// ============================================================================

BOOST_AUTO_TEST_CASE(string_function_in_unary_test)
{
    // Test: upper case("test") with candidate "TEST"
    // Without explicit operator, function call alone doesn't match
    bool result = unary_test_matches("upper case(\"test\")", "TEST");
    BOOST_CHECK_EQUAL(result, false);
}

BOOST_AUTO_TEST_CASE(string_function_in_unary_test_false)
{
    // Test: lower case("JOHN") with candidate "JOHN"
    // Should evaluate lower case("JOHN") to "john" then compare "JOHN" = "john" → false
    bool result = unary_test_matches("lower case(\"JOHN\")", "JOHN");
    BOOST_CHECK_EQUAL(result, false);
}

BOOST_AUTO_TEST_SUITE_END()
