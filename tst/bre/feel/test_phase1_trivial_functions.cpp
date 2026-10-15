/*
 * ORION Optimized Rule Integration & Operations Native
 * SPDX-License-Identifier: Apache-2.0
 * SPDX-FileCopyrightText: 2025 ORION contributors
 *
 * Test suite for Phase 1 trivial FEEL functions: odd, even, number, string, is
 */

#include <boost/test/unit_test.hpp>
#include <limits>
#include <orion/bre/feel/evaluator.hpp>
#include <orion/bre/feel/regex_cache.hpp>
#include "test_helpers.hpp"

using namespace orion::bre::feel;
using orion::bre::feel::test::get_test_eval_ctx;

BOOST_AUTO_TEST_SUITE(phase1_trivial_functions)

// ========== odd() TESTS ==========

BOOST_AUTO_TEST_CASE(test_odd_true)
{
    auto result = Evaluator::evaluate("odd(5)", {}, get_test_eval_ctx());
    BOOST_CHECK_EQUAL(result.get<bool>(), true);
}

BOOST_AUTO_TEST_CASE(test_odd_false)
{
    auto result = Evaluator::evaluate("odd(2)", {}, get_test_eval_ctx());
    BOOST_CHECK_EQUAL(result.get<bool>(), false);
}

BOOST_AUTO_TEST_CASE(test_odd_zero)
{
    auto result = Evaluator::evaluate("odd(0)", {}, get_test_eval_ctx());
    BOOST_CHECK_EQUAL(result.get<bool>(), false);
}

BOOST_AUTO_TEST_CASE(test_odd_negative_odd)
{
    auto result = Evaluator::evaluate("odd(-5)", {}, get_test_eval_ctx());
    BOOST_CHECK_EQUAL(result.get<bool>(), true);
}

BOOST_AUTO_TEST_CASE(test_odd_negative_even)
{
    auto result = Evaluator::evaluate("odd(-2)", {}, get_test_eval_ctx());
    BOOST_CHECK_EQUAL(result.get<bool>(), false);
}

BOOST_AUTO_TEST_CASE(test_odd_decimal)
{
    auto result = Evaluator::evaluate("odd(5.5)", {}, get_test_eval_ctx());
    BOOST_CHECK(result.is_null());
}

BOOST_AUTO_TEST_CASE(test_odd_null)
{
    auto result = Evaluator::evaluate("odd(null)", {}, get_test_eval_ctx());
    BOOST_CHECK(result.is_null());
}

// ========== even() TESTS ==========

BOOST_AUTO_TEST_CASE(test_even_true)
{
    auto result = Evaluator::evaluate("even(2)", {}, get_test_eval_ctx());
    BOOST_CHECK_EQUAL(result.get<bool>(), true);
}

BOOST_AUTO_TEST_CASE(test_even_false)
{
    auto result = Evaluator::evaluate("even(5)", {}, get_test_eval_ctx());
    BOOST_CHECK_EQUAL(result.get<bool>(), false);
}

BOOST_AUTO_TEST_CASE(test_even_zero)
{
    auto result = Evaluator::evaluate("even(0)", {}, get_test_eval_ctx());
    BOOST_CHECK_EQUAL(result.get<bool>(), true);
}

BOOST_AUTO_TEST_CASE(test_even_negative_even)
{
    auto result = Evaluator::evaluate("even(-2)", {}, get_test_eval_ctx());
    BOOST_CHECK_EQUAL(result.get<bool>(), true);
}

BOOST_AUTO_TEST_CASE(test_even_negative_odd)
{
    auto result = Evaluator::evaluate("even(-5)", {}, get_test_eval_ctx());
    BOOST_CHECK_EQUAL(result.get<bool>(), false);
}

BOOST_AUTO_TEST_CASE(test_even_decimal)
{
    auto result = Evaluator::evaluate("even(2.5)", {}, get_test_eval_ctx());
    BOOST_CHECK(result.is_null());
}

BOOST_AUTO_TEST_CASE(test_even_null)
{
    auto result = Evaluator::evaluate("even(null)", {}, get_test_eval_ctx());
    BOOST_CHECK(result.is_null());
}

// ========== number() TESTS ==========

BOOST_AUTO_TEST_CASE(test_number_us_format)
{
    // US format: comma grouping, period decimal
    auto result = Evaluator::evaluate(R"(number("1,000.05", ",", "."))", {}, get_test_eval_ctx());
    BOOST_CHECK(result.is_number());
    BOOST_CHECK_CLOSE(result.get<double>(), 1000.05, 0.001);
}

BOOST_AUTO_TEST_CASE(test_number_eu_format)
{
    // EU format: period grouping, comma decimal
    auto result = Evaluator::evaluate(R"(number("1.000,05", ".", ","))", {}, get_test_eval_ctx());
    BOOST_CHECK(result.is_number());
    BOOST_CHECK_CLOSE(result.get<double>(), 1000.05, 0.001);
}

BOOST_AUTO_TEST_CASE(test_number_space_grouping)
{
    auto result = Evaluator::evaluate(R"(number("1 000,05", " ", ","))", {}, get_test_eval_ctx());
    BOOST_CHECK(result.is_number());
    BOOST_CHECK_CLOSE(result.get<double>(), 1000.05, 0.001);
}

BOOST_AUTO_TEST_CASE(test_number_no_grouping)
{
    auto result = Evaluator::evaluate(R"(number("1000.05", null, "."))", {}, get_test_eval_ctx());
    BOOST_CHECK(result.is_number());
    BOOST_CHECK_CLOSE(result.get<double>(), 1000.05, 0.001);
}

BOOST_AUTO_TEST_CASE(test_number_null_input)
{
    auto result = Evaluator::evaluate(R"(number(null, ",", "."))", {}, get_test_eval_ctx());
    BOOST_CHECK(result.is_null());
}

BOOST_AUTO_TEST_CASE(test_number_invalid_string)
{
    auto result = Evaluator::evaluate(R"(number("abc", ",", "."))", {}, get_test_eval_ctx());
    BOOST_CHECK(result.is_null());
}

// ========== string() TESTS ==========

BOOST_AUTO_TEST_CASE(test_string_from_number)
{
    auto result = Evaluator::evaluate("string(42)", {}, get_test_eval_ctx());
    BOOST_CHECK(result.is_string());
    BOOST_CHECK_EQUAL(result.get<std::string>(), "42");
}

BOOST_AUTO_TEST_CASE(test_string_from_boolean_true)
{
    auto result = Evaluator::evaluate("string(true)", {}, get_test_eval_ctx());
    BOOST_CHECK(result.is_string());
    BOOST_CHECK_EQUAL(result.get<std::string>(), "true");
}

BOOST_AUTO_TEST_CASE(test_string_from_boolean_false)
{
    auto result = Evaluator::evaluate("string(false)", {}, get_test_eval_ctx());
    BOOST_CHECK(result.is_string());
    BOOST_CHECK_EQUAL(result.get<std::string>(), "false");
}

BOOST_AUTO_TEST_CASE(test_string_from_string)
{
    auto result = Evaluator::evaluate(R"(string("hello"))", {}, get_test_eval_ctx());
    BOOST_CHECK(result.is_string());
    BOOST_CHECK_EQUAL(result.get<std::string>(), "hello");
}

BOOST_AUTO_TEST_CASE(test_string_null)
{
    auto result = Evaluator::evaluate("string(null)", {}, get_test_eval_ctx());
    BOOST_CHECK(result.is_null());
}

BOOST_AUTO_TEST_CASE(test_format_number_picture_string)
{
    auto result = Evaluator::evaluate(R"FEEL(format number(12345.6, "#,###.00"))FEEL", {}, get_test_eval_ctx());
    BOOST_CHECK_EQUAL(result.get<std::string>(), "12,345.60");

    result = Evaluator::evaluate(R"FEEL(format number(1234567, "#,##,###"))FEEL", {}, get_test_eval_ctx());
    BOOST_CHECK_EQUAL(result.get<std::string>(), "12,34,567");

    result = Evaluator::evaluate(R"FEEL(format number(123456789, "#,###,##,##"))FEEL", {}, get_test_eval_ctx());
    BOOST_CHECK_EQUAL(result.get<std::string>(), "12,345,67,89");

    result = Evaluator::evaluate(R"FEEL(format number(2, "E0"))FEEL", {}, get_test_eval_ctx());
    BOOST_CHECK_EQUAL(result.get<std::string>(), "E2");

    result = Evaluator::evaluate(R"FEEL(format number(123.9, "9999"))FEEL", {}, get_test_eval_ctx());
    BOOST_CHECK_EQUAL(result.get<std::string>(), "0124");

    result = Evaluator::evaluate(R"FEEL(format number(0.14, "00%"))FEEL", {}, get_test_eval_ctx());
    BOOST_CHECK_EQUAL(result.get<std::string>(), "14%");

    result = Evaluator::evaluate(R"FEEL(format number(-123.45, "#,##0.0;(#,##0.0)"))FEEL", {}, get_test_eval_ctx());
    BOOST_CHECK_EQUAL(result.get<std::string>(), "(123.4)");

    result = Evaluator::evaluate(R"FEEL(format number(2.5, "0"))FEEL", {}, get_test_eval_ctx());
    BOOST_CHECK_EQUAL(result.get<std::string>(), "2");
    result = Evaluator::evaluate(R"FEEL(format number(3.5, "0"))FEEL", {}, get_test_eval_ctx());
    BOOST_CHECK_EQUAL(result.get<std::string>(), "4");

    result = Evaluator::evaluate(R"FEEL(format number(1234.5, "0.00E0"))FEEL", {}, get_test_eval_ctx());
    BOOST_CHECK_EQUAL(result.get<std::string>(), "1.23E3");

    result = Evaluator::evaluate(R"FEEL(format number(0.234, ".00E0"))FEEL", {}, get_test_eval_ctx());
    BOOST_CHECK_EQUAL(result.get<std::string>(), ".23E0");

    result = Evaluator::evaluate(R"FEEL(format number(999.9, "0.0E0"))FEEL", {}, get_test_eval_ctx());
    BOOST_CHECK_EQUAL(result.get<std::string>(), "1.0E3");

    // The exponent is chosen before rounding, so small values keep their significant digits.
    result = Evaluator::evaluate(R"FEEL(format number(0.000123456, "0.00E0"))FEEL", {}, get_test_eval_ctx());
    BOOST_CHECK_EQUAL(result.get<std::string>(), "1.23E-4");
    result = Evaluator::evaluate(R"FEEL(format number(-0.000123456, "0.00E00"))FEEL", {}, get_test_eval_ctx());
    BOOST_CHECK_EQUAL(result.get<std::string>(), "-1.23E-04");
    result = Evaluator::evaluate(R"FEEL(format number(1234.5, "00.0E0"))FEEL", {}, get_test_eval_ctx());
    BOOST_CHECK_EQUAL(result.get<std::string>(), "12.3E2");

    // Without integer digits the mantissa is scaled into [0.1, 1).
    result = Evaluator::evaluate(R"FEEL(format number(23.4, ".00E0"))FEEL", {}, get_test_eval_ctx());
    BOOST_CHECK_EQUAL(result.get<std::string>(), ".23E2");
    result = Evaluator::evaluate(R"FEEL(format number(0.999, ".00E0"))FEEL", {}, get_test_eval_ctx());
    BOOST_CHECK_EQUAL(result.get<std::string>(), ".10E1");

    result = Evaluator::evaluate(R"FEEL(format number(0, "0.0E0"))FEEL", {}, get_test_eval_ctx());
    BOOST_CHECK_EQUAL(result.get<std::string>(), "0.0E0");

    result = Evaluator::evaluate(R"FEEL(format number(1.2, "'USD '0.##"))FEEL", {}, get_test_eval_ctx());
    BOOST_CHECK_EQUAL(result.get<std::string>(), "USD 1.2");

    result = Evaluator::evaluate(R"FEEL(format number(0.001, "0‰"))FEEL", {}, get_test_eval_ctx());
    BOOST_CHECK_EQUAL(result.get<std::string>(), "1‰");

    result = Evaluator::evaluate("format number(value, \"0\")",
        {{"value", std::numeric_limits<double>::infinity()}}, get_test_eval_ctx());
    BOOST_CHECK_EQUAL(result.get<std::string>(), "Infinity");

    result = Evaluator::evaluate("format number(value, \"0;[0]\")",
        {{"value", -std::numeric_limits<double>::infinity()}}, get_test_eval_ctx());
    BOOST_CHECK_EQUAL(result.get<std::string>(), "[Infinity]");

    result = Evaluator::evaluate("format number(value, \"0\")",
        {{"value", std::numeric_limits<double>::quiet_NaN()}}, get_test_eval_ctx());
    BOOST_CHECK_EQUAL(result.get<std::string>(), "NaN");

    BOOST_CHECK(Evaluator::evaluate(R"FEEL(format number(1, "0..0"))FEEL", {}, get_test_eval_ctx()).is_null());
    BOOST_CHECK(Evaluator::evaluate(R"FEEL(format number(1, "0;"))FEEL", {}, get_test_eval_ctx()).is_null());
    BOOST_CHECK(Evaluator::evaluate(R"FEEL(format number(1, "0%%"))FEEL", {}, get_test_eval_ctx()).is_null());
    BOOST_CHECK(Evaluator::evaluate("format number(1, 2)", {}, get_test_eval_ctx()).is_null());
}

// ========== is() TESTS ==========

BOOST_AUTO_TEST_CASE(test_is_same_number)
{
    auto result = Evaluator::evaluate("is(1, 1)", {}, get_test_eval_ctx());
    BOOST_CHECK_EQUAL(result.get<bool>(), true);
}

BOOST_AUTO_TEST_CASE(test_is_different_numbers)
{
    auto result = Evaluator::evaluate("is(1, 2)", {}, get_test_eval_ctx());
    BOOST_CHECK_EQUAL(result.get<bool>(), false);
}

BOOST_AUTO_TEST_CASE(test_is_same_string)
{
    auto result = Evaluator::evaluate(R"(is("a", "a"))", {}, get_test_eval_ctx());
    BOOST_CHECK_EQUAL(result.get<bool>(), true);
}

BOOST_AUTO_TEST_CASE(test_is_different_strings)
{
    auto result = Evaluator::evaluate(R"(is("a", "b"))", {}, get_test_eval_ctx());
    BOOST_CHECK_EQUAL(result.get<bool>(), false);
}

BOOST_AUTO_TEST_CASE(test_is_both_null)
{
    auto result = Evaluator::evaluate("is(null, null)", {}, get_test_eval_ctx());
    BOOST_CHECK_EQUAL(result.get<bool>(), true);
}

BOOST_AUTO_TEST_CASE(test_is_one_null)
{
    auto result = Evaluator::evaluate("is(1, null)", {}, get_test_eval_ctx());
    BOOST_CHECK_EQUAL(result.get<bool>(), false);
}

BOOST_AUTO_TEST_CASE(test_is_same_boolean)
{
    auto result = Evaluator::evaluate("is(true, true)", {}, get_test_eval_ctx());
    BOOST_CHECK_EQUAL(result.get<bool>(), true);
}

BOOST_AUTO_TEST_SUITE_END()
