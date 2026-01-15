/*
 * ORION Optimized Rule Integration & Operations Native
 * SPDX-License-Identifier: Apache-2.0
 * SPDX-FileCopyrightText: 2025 ORION contributors
 *
 * Test suite for DMN FEEL named parameters feature
 */

#include <boost/test/unit_test.hpp>
#include <orion/bre/feel/evaluator.hpp>
#include <orion/bre/feel/regex_cache.hpp>

using namespace orion::bre::feel;

// Test helper: evaluate with proper EvaluationContext
namespace {
    json eval_feel(std::string_view expression, const json& context = json::object()) {
        static thread_local RegexCache cache(100);
        EvaluationContext eval_ctx;
        eval_ctx.regex_cache = &cache;
        return orion::bre::feel::Evaluator::evaluate(expression, context, eval_ctx);
    }
}

BOOST_AUTO_TEST_SUITE(named_parameters)

// ============================================================================
// Positional Parameter Tests (Baseline - should still work)
// ============================================================================

BOOST_AUTO_TEST_CASE(test_positional_single_param)
{
    BOOST_TEST_MESSAGE("Testing positional parameter with single argument");
    auto result = eval_feel("abs(-42)");
    BOOST_CHECK_EQUAL(result.get<double>(), 42);
}

BOOST_AUTO_TEST_CASE(test_positional_two_params)
{
    BOOST_TEST_MESSAGE("Testing positional parameters with two arguments");
    auto result = eval_feel("modulo(10, 3)");
    BOOST_CHECK_EQUAL(result.get<double>(), 1);
}

BOOST_AUTO_TEST_CASE(test_positional_decimal)
{
    BOOST_TEST_MESSAGE("Testing positional parameters with decimal()");
    auto result = eval_feel("decimal(3.14159, 2)");
    BOOST_CHECK_CLOSE(result.get<double>(), 3.14, 0.01);
}

// ============================================================================
// Named Parameter Tests - Single Parameter
// ============================================================================

BOOST_AUTO_TEST_CASE(test_named_single_param)
{
    BOOST_TEST_MESSAGE("Testing named parameter: abs(n: -42)");
    auto result = eval_feel("abs(n: -42)");
    BOOST_CHECK_EQUAL(result.get<double>(), 42);
}

BOOST_AUTO_TEST_CASE(test_named_sqrt)
{
    BOOST_TEST_MESSAGE("Testing named parameter: sqrt(number: 16)");
    auto result = eval_feel("sqrt(number: 16)");
    BOOST_CHECK_EQUAL(result.get<double>(), 4);
}

// ============================================================================
// Named Parameter Tests - Multiple Parameters
// ============================================================================

BOOST_AUTO_TEST_CASE(test_named_two_params_in_order)
{
    BOOST_TEST_MESSAGE("Testing named parameters in order: modulo(dividend: 10, divisor: 3)");
    auto result = eval_feel("modulo(dividend: 10, divisor: 3)");
    BOOST_CHECK_EQUAL(result.get<double>(), 1);
}

BOOST_AUTO_TEST_CASE(test_named_two_params_out_of_order)
{
    BOOST_TEST_MESSAGE("Testing named parameters out of order: modulo(divisor: 3, dividend: 10)");
    auto result = eval_feel("modulo(divisor: 3, dividend: 10)");
    BOOST_CHECK_EQUAL(result.get<double>(), 1);
}

BOOST_AUTO_TEST_CASE(test_named_decimal_in_order)
{
    BOOST_TEST_MESSAGE("Testing named decimal in order: decimal(n: 3.14159, scale: 2)");
    auto result = eval_feel("decimal(n: 3.14159, scale: 2)");
    BOOST_CHECK_CLOSE(result.get<double>(), 3.14, 0.01);
}

BOOST_AUTO_TEST_CASE(test_named_decimal_out_of_order)
{
    BOOST_TEST_MESSAGE("Testing named decimal out of order: decimal(scale: 2, n: 3.14159)");
    auto result = eval_feel("decimal(scale: 2, n: 3.14159)");
    BOOST_CHECK_CLOSE(result.get<double>(), 3.14, 0.01);
}

BOOST_AUTO_TEST_CASE(test_named_round_functions)
{
    BOOST_TEST_MESSAGE("Testing named parameters with multi-word function names");
    
    // round up
    auto result1 = eval_feel("round up(n: 5.25, scale: 1)");
    BOOST_CHECK_CLOSE(result1.get<double>(), 5.3, 0.01);
    
    // round down (out of order)
    auto result2 = eval_feel("round down(scale: 1, n: 5.25)");
    BOOST_CHECK_CLOSE(result2.get<double>(), 5.2, 0.01);
}

// ============================================================================
// Named Parameters with Expressions
// ============================================================================

BOOST_AUTO_TEST_CASE(test_named_with_literal)
{
    BOOST_TEST_MESSAGE("Testing named parameter with literal: abs(n: -30)");
    auto result = eval_feel("abs(n: -30)");
    BOOST_CHECK_EQUAL(result.get<double>(), 30);
}

// ============================================================================
// Error Cases
// ============================================================================

BOOST_AUTO_TEST_CASE(test_mixed_params_positional_then_named)
{
    BOOST_TEST_MESSAGE("Testing mixed parameters (positional then named) - should throw");
    BOOST_CHECK_THROW((void)eval_feel("modulo(10, divisor: 3)"), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(test_mixed_params_named_then_positional)
{
    BOOST_TEST_MESSAGE("Testing mixed parameters (named then positional) - should throw");
    BOOST_CHECK_THROW((void)eval_feel("modulo(dividend: 10, 3)"), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(test_unknown_parameter_name)
{
    BOOST_TEST_MESSAGE("Testing unknown parameter name - should return null per DMN spec");
    auto result = eval_feel("abs(unknown_param: 42)");
    BOOST_CHECK(result.is_null());
}

BOOST_AUTO_TEST_CASE(test_missing_required_parameter)
{
    BOOST_TEST_MESSAGE("Testing missing required parameter - should return null per DMN spec");
    auto result = eval_feel("modulo(dividend: 10)");
    BOOST_CHECK(result.is_null());
}

BOOST_AUTO_TEST_CASE(test_too_many_positional_params)
{
    BOOST_TEST_MESSAGE("Testing too many positional parameters - should return null per DMN spec");
    auto result = eval_feel("abs(42, 99)");
    BOOST_CHECK(result.is_null());
}

// ============================================================================
// Complex Scenarios
// ============================================================================

BOOST_AUTO_TEST_CASE(test_named_params_in_arithmetic)
{
    BOOST_TEST_MESSAGE("Testing named parameters in arithmetic expression");
    auto result = eval_feel("abs(n: -5) + sqrt(number: 16)");
    BOOST_CHECK_EQUAL(result.get<double>(), 9);  // 5 + 4
}

BOOST_AUTO_TEST_CASE(test_multiple_named_function_calls)
{
    BOOST_TEST_MESSAGE("Testing multiple named function calls");
    auto result = eval_feel("modulo(dividend: 10, divisor: 3) + decimal(n: 2.5, scale: 0)");
    BOOST_CHECK_EQUAL(result.get<double>(), 3);  // 1 + 2
}

BOOST_AUTO_TEST_CASE(test_named_params_with_null)
{
    BOOST_TEST_MESSAGE("Testing named parameters with null value");
    auto result = eval_feel("abs(n: null)");
    BOOST_CHECK(result.is_null());
}

// ============================================================================
// TCK-Style Tests
// ============================================================================

BOOST_AUTO_TEST_CASE(test_tck_style_abs_named)
{
    BOOST_TEST_MESSAGE("Testing TCK-style abs with named parameter");
    
    struct TestCase {
        const char* expr;
        double expected;
    };
    
    TestCase testCases[] = {
        {"abs(n: 10)", 10},
        {"abs(n: -10)", 10},
        {"abs(n: 0)", 0},
        {"abs(n: 1.5)", 1.5},
        {"abs(n: -1.5)", 1.5}
    };
    
    for (const auto& tc : testCases) {
        auto result = eval_feel(tc.expr);
        BOOST_CHECK_CLOSE(result.get<double>(), tc.expected, 0.0001);
    }
}

BOOST_AUTO_TEST_CASE(test_tck_style_sqrt_named)
{
    BOOST_TEST_MESSAGE("Testing TCK-style sqrt with named parameter");
    
    struct TestCase {
        const char* expr;
        double expected;
    };
    
    TestCase testCases[] = {
        {"sqrt(number: 0)", 0},
        {"sqrt(number: 1)", 1},
        {"sqrt(number: 4)", 2},
        {"sqrt(number: 9)", 3},
        {"sqrt(number: 16)", 4}
    };
    
    for (const auto& tc : testCases) {
        auto result = eval_feel(tc.expr);
        BOOST_CHECK_CLOSE(result.get<double>(), tc.expected, 0.0001);
    }
}

BOOST_AUTO_TEST_CASE(test_tck_style_modulo_named_out_of_order)
{
    BOOST_TEST_MESSAGE("Testing TCK-style modulo with named parameters out of order");
    
    struct TestCase {
        const char* expr;
        double expected;
    };
    
    TestCase testCases[] = {
        {"modulo(dividend: 12, divisor: 5)", 2},
        {"modulo(divisor: 5, dividend: 12)", 2},  // Out of order
        {"modulo(dividend: -12, divisor: 5)", 3},
        {"modulo(divisor: 5, dividend: -12)", 3}  // Out of order
    };
    
    for (const auto& tc : testCases) {
        auto result = eval_feel(tc.expr);
        BOOST_CHECK_CLOSE(result.get<double>(), tc.expected, 0.0001);
    }
}

BOOST_AUTO_TEST_SUITE_END()
