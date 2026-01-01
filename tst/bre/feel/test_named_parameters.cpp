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

BOOST_AUTO_TEST_SUITE(named_parameters)

// ============================================================================
// Positional Parameter Tests (Baseline - should still work)
// ============================================================================

BOOST_AUTO_TEST_CASE(test_positional_single_param)
{
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    BOOST_TEST_MESSAGE("Testing positional parameter with single argument");
    Evaluator eval;
    nlohmann::json context;
    auto result = eval.evaluate("abs(-42)", context, eval_ctx);
    BOOST_CHECK_EQUAL(result.get<double>(), 42);
}

BOOST_AUTO_TEST_CASE(test_positional_two_params)
{
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    BOOST_TEST_MESSAGE("Testing positional parameters with two arguments");
    Evaluator eval;
    nlohmann::json context;
    auto result = eval.evaluate("modulo(10, 3)", context, eval_ctx);
    BOOST_CHECK_EQUAL(result.get<double>(), 1);
}

BOOST_AUTO_TEST_CASE(test_positional_decimal)
{
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    BOOST_TEST_MESSAGE("Testing positional parameters with decimal()");
    Evaluator eval;
    nlohmann::json context;
    auto result = eval.evaluate("decimal(3.14159, 2)", context, eval_ctx);
    BOOST_CHECK_CLOSE(result.get<double>(), 3.14, 0.01);
}

// ============================================================================
// Named Parameter Tests - Single Parameter
// ============================================================================

BOOST_AUTO_TEST_CASE(test_named_single_param)
{
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    BOOST_TEST_MESSAGE("Testing named parameter: abs(n: -42)");
    Evaluator eval;
    nlohmann::json context;
    auto result = eval.evaluate("abs(n: -42)", context, eval_ctx);
    BOOST_CHECK_EQUAL(result.get<double>(), 42);
}

BOOST_AUTO_TEST_CASE(test_named_sqrt)
{
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    BOOST_TEST_MESSAGE("Testing named parameter: sqrt(number: 16)");
    Evaluator eval;
    nlohmann::json context;
    auto result = eval.evaluate("sqrt(number: 16)", context, eval_ctx);
    BOOST_CHECK_EQUAL(result.get<double>(), 4);
}

// ============================================================================
// Named Parameter Tests - Multiple Parameters
// ============================================================================

BOOST_AUTO_TEST_CASE(test_named_two_params_in_order)
{
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    BOOST_TEST_MESSAGE("Testing named parameters in order: modulo(dividend: 10, divisor: 3)");
    Evaluator eval;
    nlohmann::json context;
    auto result = eval.evaluate("modulo(dividend: 10, divisor: 3)", context, eval_ctx);
    BOOST_CHECK_EQUAL(result.get<double>(), 1);
}

BOOST_AUTO_TEST_CASE(test_named_two_params_out_of_order)
{
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    BOOST_TEST_MESSAGE("Testing named parameters out of order: modulo(divisor: 3, dividend: 10)");
    Evaluator eval;
    nlohmann::json context;
    auto result = eval.evaluate("modulo(divisor: 3, dividend: 10)", context, eval_ctx);
    BOOST_CHECK_EQUAL(result.get<double>(), 1);
}

BOOST_AUTO_TEST_CASE(test_named_decimal_in_order)
{
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    BOOST_TEST_MESSAGE("Testing named decimal in order: decimal(n: 3.14159, scale: 2)");
    Evaluator eval;
    nlohmann::json context;
    auto result = eval.evaluate("decimal(n: 3.14159, scale: 2)", context, eval_ctx);
    BOOST_CHECK_CLOSE(result.get<double>(), 3.14, 0.01);
}

BOOST_AUTO_TEST_CASE(test_named_decimal_out_of_order)
{
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    BOOST_TEST_MESSAGE("Testing named decimal out of order: decimal(scale: 2, n: 3.14159)");
    Evaluator eval;
    nlohmann::json context;
    auto result = eval.evaluate("decimal(scale: 2, n: 3.14159)", context, eval_ctx);
    BOOST_CHECK_CLOSE(result.get<double>(), 3.14, 0.01);
}

BOOST_AUTO_TEST_CASE(test_named_round_functions)
{
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    BOOST_TEST_MESSAGE("Testing named parameters with multi-word function names");
    
    Evaluator eval;
    nlohmann::json context;
    
    // round up
    auto result1 = eval.evaluate("round up(n: 5.25, scale: 1)", context, eval_ctx);
    BOOST_CHECK_CLOSE(result1.get<double>(), 5.3, 0.01);
    
    // round down (out of order)
    auto result2 = eval.evaluate("round down(scale: 1, n: 5.25)", context, eval_ctx);
    BOOST_CHECK_CLOSE(result2.get<double>(), 5.2, 0.01);
}

// ============================================================================
// Named Parameters with Expressions
// ============================================================================

BOOST_AUTO_TEST_CASE(test_named_with_literal)
{
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    BOOST_TEST_MESSAGE("Testing named parameter with literal: abs(n: -30)");
    Evaluator eval;
    nlohmann::json context;
    auto result = eval.evaluate("abs(n: -30)", context, eval_ctx);
    BOOST_CHECK_EQUAL(result.get<double>(), 30);
}

// ============================================================================
// Error Cases
// ============================================================================

BOOST_AUTO_TEST_CASE(test_mixed_params_positional_then_named)
{
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    BOOST_TEST_MESSAGE("Testing mixed parameters (positional then named) - should throw");
    Evaluator eval;
    nlohmann::json context;
    BOOST_CHECK_THROW((void)eval.evaluate("modulo(10, divisor: 3)", context, eval_ctx), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(test_mixed_params_named_then_positional)
{
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    BOOST_TEST_MESSAGE("Testing mixed parameters (named then positional) - should throw");
    Evaluator eval;
    nlohmann::json context;
    BOOST_CHECK_THROW((void)eval.evaluate("modulo(dividend: 10, 3)", context, eval_ctx), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(test_unknown_parameter_name)
{
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    BOOST_TEST_MESSAGE("Testing unknown parameter name - should return null per DMN spec");
    Evaluator eval;
    nlohmann::json context;
    auto result = eval.evaluate("abs(unknown_param: 42)", context, eval_ctx);
    BOOST_CHECK(result.is_null());
}

BOOST_AUTO_TEST_CASE(test_missing_required_parameter)
{
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    BOOST_TEST_MESSAGE("Testing missing required parameter - should return null per DMN spec");
    Evaluator eval;
    nlohmann::json context;
    auto result = eval.evaluate("modulo(dividend: 10)", context, eval_ctx);
    BOOST_CHECK(result.is_null());
}

BOOST_AUTO_TEST_CASE(test_too_many_positional_params)
{
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    BOOST_TEST_MESSAGE("Testing too many positional parameters - should return null per DMN spec");
    Evaluator eval;
    nlohmann::json context;
    auto result = eval.evaluate("abs(42, 99)", context, eval_ctx);
    BOOST_CHECK(result.is_null());
}

// ============================================================================
// Complex Scenarios
// ============================================================================

BOOST_AUTO_TEST_CASE(test_named_params_in_arithmetic)
{
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    BOOST_TEST_MESSAGE("Testing named parameters in arithmetic expression");
    Evaluator eval;
    nlohmann::json context;
    auto result = eval.evaluate("abs(n: -5) + sqrt(number: 16)", context, eval_ctx);
    BOOST_CHECK_EQUAL(result.get<double>(), 9);  // 5 + 4
}

BOOST_AUTO_TEST_CASE(test_multiple_named_function_calls)
{
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    BOOST_TEST_MESSAGE("Testing multiple named function calls");
    Evaluator eval;
    nlohmann::json context;
    auto result = eval.evaluate("modulo(dividend: 10, divisor: 3) + decimal(n: 2.5, scale: 0)", context, eval_ctx);
    BOOST_CHECK_EQUAL(result.get<double>(), 3);  // 1 + 2
}

BOOST_AUTO_TEST_CASE(test_named_params_with_null)
{
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    BOOST_TEST_MESSAGE("Testing named parameters with null value");
    Evaluator eval;
    nlohmann::json context;
    auto result = eval.evaluate("abs(n: null)", context, eval_ctx);
    BOOST_CHECK(result.is_null());
}

// ============================================================================
// TCK-Style Tests
// ============================================================================

BOOST_AUTO_TEST_CASE(test_tck_style_abs_named)
{
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    BOOST_TEST_MESSAGE("Testing TCK-style abs with named parameter");
    Evaluator eval;
    nlohmann::json context;
    
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
        auto result = eval.evaluate(tc.expr, context, eval_ctx);
        BOOST_CHECK_CLOSE(result.get<double>(), tc.expected, 0.0001);
    }
}

BOOST_AUTO_TEST_CASE(test_tck_style_sqrt_named)
{
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    BOOST_TEST_MESSAGE("Testing TCK-style sqrt with named parameter");
    Evaluator eval;
    nlohmann::json context;
    
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
        auto result = eval.evaluate(tc.expr, context, eval_ctx);
        BOOST_CHECK_CLOSE(result.get<double>(), tc.expected, 0.0001);
    }
}

BOOST_AUTO_TEST_CASE(test_tck_style_modulo_named_out_of_order)
{
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    BOOST_TEST_MESSAGE("Testing TCK-style modulo with named parameters out of order");
    Evaluator eval;
    nlohmann::json context;
    
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
        auto result = eval.evaluate(tc.expr, context, eval_ctx);
        BOOST_CHECK_CLOSE(result.get<double>(), tc.expected, 0.0001);
    }
}

BOOST_AUTO_TEST_SUITE_END()
