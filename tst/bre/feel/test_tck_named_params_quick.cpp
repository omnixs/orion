/*
 * ORION Optimized Rule Integration & Operations Native
 * SPDX-License-Identifier: Apache-2.0
 * SPDX-FileCopyrightText: 2025 ORION contributors
 */

/*
 * Quick verification that named parameters work for TCK tests
 */

#include <boost/test/unit_test.hpp>
#include <orion/bre/feel/evaluator.hpp>
#include <orion/bre/feel/regex_cache.hpp>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using namespace orion::bre;

// Test helper: evaluate with proper EvaluationContext
namespace {
    json eval_feel(std::string_view expression, const json& context = json::object()) {
        static thread_local orion::bre::feel::RegexCache cache(100);
        orion::bre::feel::EvaluationContext eval_ctx;
        eval_ctx.regex_cache = &cache;
        return orion::bre::feel::Evaluator::evaluate(expression, context, eval_ctx);
    }
}

BOOST_AUTO_TEST_SUITE(tck_named_params_verification)

BOOST_AUTO_TEST_CASE(test_abs_with_named_param)
{
    // TCK test 0050 decision006: abs(n:-1) should return 1
    orion::bre::feel::json result = eval_feel("abs(n:-1)");
    BOOST_TEST(result == 1);
}

BOOST_AUTO_TEST_CASE(test_abs_with_wrong_param_name)
{
    // TCK test 0050 decision007: abs(number:-1) should return null (wrong param name)
    
    try {
        json result = eval_feel("abs(number:-1)");
        // Should either throw or return null
        BOOST_TEST(result.is_null());
    } catch (const std::exception&) {
        // Expected - wrong parameter name should cause error
        BOOST_TEST(true);
    }
}

BOOST_AUTO_TEST_CASE(test_sqrt_with_named_param)
{
    // sqrt(number:16) should return 4
    orion::bre::feel::json result = eval_feel("sqrt(number:16)");
    BOOST_TEST(result == 4);
}

BOOST_AUTO_TEST_CASE(test_modulo_out_of_order)
{
    // modulo(divisor:3, dividend:10) should return 1 (parameters out of order)
    orion::bre::feel::json result = eval_feel("modulo(divisor:3, dividend:10)");
    BOOST_TEST(result == 1);
}

BOOST_AUTO_TEST_CASE(test_decimal_out_of_order)
{
    // decimal(scale:2, n:3.14159) should return 3.14 (parameters out of order)
    orion::bre::feel::json result = eval_feel("decimal(scale:2, n:3.14159)");
    BOOST_TEST(result == 3.14);
}

BOOST_AUTO_TEST_SUITE_END()
