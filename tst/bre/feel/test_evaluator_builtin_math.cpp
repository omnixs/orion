/*
 * ORION Optimized Rule Integration & Operations Native
 * SPDX-License-Identifier: Apache-2.0
 * SPDX-FileCopyrightText: 2025 ORION contributors
 *
 * Test suite for DMN FEEL built-in math functions
 */

#include <boost/test/unit_test.hpp>
#include <orion/bre/feel/evaluator.hpp>
#include <cmath>

using namespace orion::bre::feel;

BOOST_AUTO_TEST_SUITE(builtin_math_functions)

// ========== abs() TESTS ==========

BOOST_AUTO_TEST_CASE(test_abs_positive)
{
    BOOST_TEST_MESSAGE("Testing abs() with positive number");
    auto result = Evaluator::evaluate("abs(42)");
    BOOST_CHECK(result.is_number());
    BOOST_CHECK_EQUAL(result.get<double>(), 42.0);
}

BOOST_AUTO_TEST_CASE(test_abs_negative)
{
    BOOST_TEST_MESSAGE("Testing abs() with negative number");
    auto result = Evaluator::evaluate("abs(-42)");
    BOOST_CHECK(result.is_number());
    BOOST_CHECK_EQUAL(result.get<double>(), 42.0);
}

BOOST_AUTO_TEST_CASE(test_abs_zero)
{
    BOOST_TEST_MESSAGE("Testing abs() with zero");
    auto result = Evaluator::evaluate("abs(0)");
    BOOST_CHECK(result.is_number());
    BOOST_CHECK_EQUAL(result.get<double>(), 0.0);
}

BOOST_AUTO_TEST_CASE(test_abs_decimal)
{
    BOOST_TEST_MESSAGE("Testing abs() with decimal");
    auto result = Evaluator::evaluate("abs(-3.14159)");
    BOOST_CHECK(result.is_number());
    BOOST_CHECK_CLOSE(result.get<double>(), 3.14159, 0.00001);
}

BOOST_AUTO_TEST_CASE(test_abs_null)
{
    BOOST_TEST_MESSAGE("Testing abs() with null");
    auto result = Evaluator::evaluate("abs(null)");
    BOOST_CHECK(result.is_null());
}

// ========== sqrt() TESTS ==========

BOOST_AUTO_TEST_CASE(test_sqrt_positive)
{
    BOOST_TEST_MESSAGE("Testing sqrt() with positive number");
    auto result = Evaluator::evaluate("sqrt(16)");
    BOOST_CHECK(result.is_number());
    BOOST_CHECK_EQUAL(result.get<double>(), 4.0);
}

BOOST_AUTO_TEST_CASE(test_sqrt_zero)
{
    BOOST_TEST_MESSAGE("Testing sqrt() with zero");
    auto result = Evaluator::evaluate("sqrt(0)");
    BOOST_CHECK(result.is_number());
    BOOST_CHECK_EQUAL(result.get<double>(), 0.0);
}

BOOST_AUTO_TEST_CASE(test_sqrt_negative)
{
    BOOST_TEST_MESSAGE("Testing sqrt() with negative number - should return null");
    auto result = Evaluator::evaluate("sqrt(-1)");
    BOOST_CHECK(result.is_null());
}

BOOST_AUTO_TEST_CASE(test_sqrt_decimal)
{
    BOOST_TEST_MESSAGE("Testing sqrt() with decimal");
    auto result = Evaluator::evaluate("sqrt(2)");
    BOOST_CHECK(result.is_number());
    BOOST_CHECK_CLOSE(result.get<double>(), std::sqrt(2.0), 0.00001);
}

BOOST_AUTO_TEST_CASE(test_sqrt_null)
{
    BOOST_TEST_MESSAGE("Testing sqrt() with null");
    auto result = Evaluator::evaluate("sqrt(null)");
    BOOST_CHECK(result.is_null());
}

// ========== floor() TESTS ==========

BOOST_AUTO_TEST_CASE(test_floor_positive)
{
    BOOST_TEST_MESSAGE("Testing floor() with positive decimal");
    auto result = Evaluator::evaluate("floor(1.5)");
    BOOST_CHECK(result.is_number());
    BOOST_CHECK_EQUAL(result.get<double>(), 1.0);
}

BOOST_AUTO_TEST_CASE(test_floor_negative)
{
    BOOST_TEST_MESSAGE("Testing floor() with negative decimal");
    auto result = Evaluator::evaluate("floor(-1.5)");
    BOOST_CHECK(result.is_number());
    BOOST_CHECK_EQUAL(result.get<double>(), -2.0);
}

BOOST_AUTO_TEST_CASE(test_floor_integer)
{
    BOOST_TEST_MESSAGE("Testing floor() with integer");
    auto result = Evaluator::evaluate("floor(5)");
    BOOST_CHECK(result.is_number());
    BOOST_CHECK_EQUAL(result.get<double>(), 5.0);
}

BOOST_AUTO_TEST_CASE(test_floor_null)
{
    BOOST_TEST_MESSAGE("Testing floor() with null");
    auto result = Evaluator::evaluate("floor(null)");
    BOOST_CHECK(result.is_null());
}

// ========== ceiling() TESTS ==========

BOOST_AUTO_TEST_CASE(test_ceiling_positive)
{
    BOOST_TEST_MESSAGE("Testing ceiling() with positive decimal");
    auto result = Evaluator::evaluate("ceiling(1.5)");
    BOOST_CHECK(result.is_number());
    BOOST_CHECK_EQUAL(result.get<double>(), 2.0);
}

BOOST_AUTO_TEST_CASE(test_ceiling_negative)
{
    BOOST_TEST_MESSAGE("Testing ceiling() with negative decimal");
    auto result = Evaluator::evaluate("ceiling(-1.5)");
    BOOST_CHECK(result.is_number());
    BOOST_CHECK_EQUAL(result.get<double>(), -1.0);
}

BOOST_AUTO_TEST_CASE(test_ceiling_integer)
{
    BOOST_TEST_MESSAGE("Testing ceiling() with integer");
    auto result = Evaluator::evaluate("ceiling(5)");
    BOOST_CHECK(result.is_number());
    BOOST_CHECK_EQUAL(result.get<double>(), 5.0);
}

BOOST_AUTO_TEST_CASE(test_ceiling_null)
{
    BOOST_TEST_MESSAGE("Testing ceiling() with null");
    auto result = Evaluator::evaluate("ceiling(null)");
    BOOST_CHECK(result.is_null());
}

// ========== exp() TESTS ==========

BOOST_AUTO_TEST_CASE(test_exp_zero)
{
    BOOST_TEST_MESSAGE("Testing exp() with zero");
    auto result = Evaluator::evaluate("exp(0)");
    BOOST_CHECK(result.is_number());
    BOOST_CHECK_CLOSE(result.get<double>(), 1.0, 0.00001);
}

BOOST_AUTO_TEST_CASE(test_exp_one)
{
    BOOST_TEST_MESSAGE("Testing exp() with one");
    auto result = Evaluator::evaluate("exp(1)");
    BOOST_CHECK(result.is_number());
    BOOST_CHECK_CLOSE(result.get<double>(), std::exp(1.0), 0.00001);
}

BOOST_AUTO_TEST_CASE(test_exp_negative)
{
    BOOST_TEST_MESSAGE("Testing exp() with negative");
    auto result = Evaluator::evaluate("exp(-1)");
    BOOST_CHECK(result.is_number());
    BOOST_CHECK_CLOSE(result.get<double>(), std::exp(-1.0), 0.00001);
}

BOOST_AUTO_TEST_CASE(test_exp_null)
{
    BOOST_TEST_MESSAGE("Testing exp() with null");
    auto result = Evaluator::evaluate("exp(null)");
    BOOST_CHECK(result.is_null());
}

// ========== log() TESTS ==========

BOOST_AUTO_TEST_CASE(test_log_one)
{
    BOOST_TEST_MESSAGE("Testing log() with one");
    auto result = Evaluator::evaluate("log(1)");
    BOOST_CHECK(result.is_number());
    BOOST_CHECK_SMALL(result.get<double>(), 0.00001);
}

BOOST_AUTO_TEST_CASE(test_log_e)
{
    BOOST_TEST_MESSAGE("Testing log() with e");
    auto result = Evaluator::evaluate("log(2.718281828)");
    BOOST_CHECK(result.is_number());
    BOOST_CHECK_CLOSE(result.get<double>(), 1.0, 0.0001);
}

BOOST_AUTO_TEST_CASE(test_log_zero)
{
    BOOST_TEST_MESSAGE("Testing log() with zero - should return null");
    auto result = Evaluator::evaluate("log(0)");
    BOOST_CHECK(result.is_null());
}

BOOST_AUTO_TEST_CASE(test_log_negative)
{
    BOOST_TEST_MESSAGE("Testing log() with negative - should return null");
    auto result = Evaluator::evaluate("log(-1)");
    BOOST_CHECK(result.is_null());
}

BOOST_AUTO_TEST_CASE(test_log_null)
{
    BOOST_TEST_MESSAGE("Testing log() with null");
    auto result = Evaluator::evaluate("log(null)");
    BOOST_CHECK(result.is_null());
}

// ========== modulo() TESTS ==========

BOOST_AUTO_TEST_CASE(test_modulo_basic)
{
    BOOST_TEST_MESSAGE("Testing modulo() with basic case");
    auto result = Evaluator::evaluate("modulo(12, 5)");
    BOOST_CHECK(result.is_number());
    BOOST_CHECK_EQUAL(result.get<double>(), 2.0);
}

BOOST_AUTO_TEST_CASE(test_modulo_negative_dividend)
{
    BOOST_TEST_MESSAGE("Testing modulo() with negative dividend");
    auto result = Evaluator::evaluate("modulo(-12, 5)");
    BOOST_CHECK(result.is_number());
    BOOST_CHECK_CLOSE(result.get<double>(), 3.0, 0.00001);
}

BOOST_AUTO_TEST_CASE(test_modulo_decimal)
{
    BOOST_TEST_MESSAGE("Testing modulo() with decimals");
    auto result = Evaluator::evaluate("modulo(10.1, 4.5)");
    BOOST_CHECK(result.is_number());
    BOOST_CHECK_CLOSE(result.get<double>(), 1.1, 0.00001);
}

BOOST_AUTO_TEST_CASE(test_modulo_null)
{
    BOOST_TEST_MESSAGE("Testing modulo() with null");
    auto result = Evaluator::evaluate("modulo(10, null)");
    BOOST_CHECK(result.is_null());
}

// ========== decimal() TESTS ==========

BOOST_AUTO_TEST_CASE(test_decimal_basic)
{
    BOOST_TEST_MESSAGE("Testing decimal() with basic case");
    auto result = Evaluator::evaluate("decimal(1/3, 2)");
    BOOST_CHECK(result.is_number());
    BOOST_CHECK_CLOSE(result.get<double>(), 0.33, 0.01);
}

BOOST_AUTO_TEST_CASE(test_decimal_half_even_up)
{
    BOOST_TEST_MESSAGE("Testing decimal() with half-even rounding (rounds to even)");
    auto result = Evaluator::evaluate("decimal(1.5, 0)");
    BOOST_CHECK(result.is_number());
    BOOST_CHECK_EQUAL(result.get<double>(), 2.0);
}

BOOST_AUTO_TEST_CASE(test_decimal_half_even_down)
{
    BOOST_TEST_MESSAGE("Testing decimal() with half-even rounding (rounds to even)");
    auto result = Evaluator::evaluate("decimal(2.5, 0)");
    BOOST_CHECK(result.is_number());
    BOOST_CHECK_EQUAL(result.get<double>(), 2.0);
}

// ========== round() TESTS ==========

BOOST_AUTO_TEST_CASE(test_round_basic)
{
    BOOST_TEST_MESSAGE("Testing round() with basic case");
    auto result = Evaluator::evaluate("round(5.5, 0)");
    BOOST_CHECK(result.is_number());
    BOOST_CHECK_EQUAL(result.get<double>(), 6.0);
}

BOOST_AUTO_TEST_CASE(test_round_half_even)
{
    BOOST_TEST_MESSAGE("Testing round() with half-even (banker's rounding)");
    auto result = Evaluator::evaluate("round(2.5, 0)");
    BOOST_CHECK(result.is_number());
    BOOST_CHECK_EQUAL(result.get<double>(), 2.0);
}

BOOST_AUTO_TEST_CASE(test_round_precision)
{
    BOOST_TEST_MESSAGE("Testing round() with precision");
    auto result = Evaluator::evaluate("round(1.121, 2)");
    BOOST_CHECK(result.is_number());
    BOOST_CHECK_CLOSE(result.get<double>(), 1.12, 0.001);
}

// ========== round up() TESTS ==========

BOOST_AUTO_TEST_CASE(test_round_up_positive)
{
    BOOST_TEST_MESSAGE("Testing round up() with positive");
    auto result = Evaluator::evaluate("round up(5.5, 0)");
    BOOST_CHECK(result.is_number());
    BOOST_CHECK_EQUAL(result.get<double>(), 6.0);
}

BOOST_AUTO_TEST_CASE(test_round_up_negative)
{
    BOOST_TEST_MESSAGE("Testing round up() with negative (away from zero)");
    auto result = Evaluator::evaluate("round up(-5.5, 0)");
    BOOST_CHECK(result.is_number());
    BOOST_CHECK_EQUAL(result.get<double>(), -6.0);
}

BOOST_AUTO_TEST_CASE(test_round_up_precision)
{
    BOOST_TEST_MESSAGE("Testing round up() with precision");
    auto result = Evaluator::evaluate("round up(1.121, 2)");
    BOOST_CHECK(result.is_number());
    BOOST_CHECK_CLOSE(result.get<double>(), 1.13, 0.001);
}

// ========== round down() TESTS ==========

BOOST_AUTO_TEST_CASE(test_round_down_positive)
{
    BOOST_TEST_MESSAGE("Testing round down() with positive");
    auto result = Evaluator::evaluate("round down(5.5, 0)");
    BOOST_CHECK(result.is_number());
    BOOST_CHECK_EQUAL(result.get<double>(), 5.0);
}

BOOST_AUTO_TEST_CASE(test_round_down_negative)
{
    BOOST_TEST_MESSAGE("Testing round down() with negative (toward zero)");
    auto result = Evaluator::evaluate("round down(-5.5, 0)");
    BOOST_CHECK(result.is_number());
    BOOST_CHECK_EQUAL(result.get<double>(), -5.0);
}

BOOST_AUTO_TEST_CASE(test_round_down_precision)
{
    BOOST_TEST_MESSAGE("Testing round down() with precision");
    auto result = Evaluator::evaluate("round down(1.129, 2)");
    BOOST_CHECK(result.is_number());
    BOOST_CHECK_CLOSE(result.get<double>(), 1.12, 0.001);
}

// ========== round half up() TESTS ==========

BOOST_AUTO_TEST_CASE(test_round_half_up_positive)
{
    BOOST_TEST_MESSAGE("Testing round half up() with positive");
    auto result = Evaluator::evaluate("round half up(5.5, 0)");
    BOOST_CHECK(result.is_number());
    BOOST_CHECK_EQUAL(result.get<double>(), 6.0);
}

BOOST_AUTO_TEST_CASE(test_round_half_up_negative)
{
    BOOST_TEST_MESSAGE("Testing round half up() with negative");
    auto result = Evaluator::evaluate("round half up(-5.5, 0)");
    BOOST_CHECK(result.is_number());
    BOOST_CHECK_EQUAL(result.get<double>(), -6.0);
}

BOOST_AUTO_TEST_CASE(test_round_half_up_precision)
{
    BOOST_TEST_MESSAGE("Testing round half up() with precision");
    auto result = Evaluator::evaluate("round half up(1.125, 2)");
    BOOST_CHECK(result.is_number());
    BOOST_CHECK_CLOSE(result.get<double>(), 1.13, 0.001);
}

// ========== round half down() TESTS ==========

BOOST_AUTO_TEST_CASE(test_round_half_down_positive)
{
    BOOST_TEST_MESSAGE("Testing round half down() with positive");
    auto result = Evaluator::evaluate("round half down(5.5, 0)");
    BOOST_CHECK(result.is_number());
    BOOST_CHECK_EQUAL(result.get<double>(), 5.0);
}

BOOST_AUTO_TEST_CASE(test_round_half_down_negative)
{
    BOOST_TEST_MESSAGE("Testing round half down() with negative");
    auto result = Evaluator::evaluate("round half down(-5.5, 0)");
    BOOST_CHECK(result.is_number());
    BOOST_CHECK_EQUAL(result.get<double>(), -5.0);
}

BOOST_AUTO_TEST_CASE(test_round_half_down_precision)
{
    BOOST_TEST_MESSAGE("Testing round half down() with precision");
    auto result = Evaluator::evaluate("round half down(1.125, 2)");
    BOOST_CHECK(result.is_number());
    BOOST_CHECK_CLOSE(result.get<double>(), 1.12, 0.001);
}

BOOST_AUTO_TEST_SUITE_END()
