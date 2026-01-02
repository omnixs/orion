/*
 * ORION Optimized Rule Integration & Operations Native
 * SPDX-License-Identifier: Apache-2.0
 * SPDX-FileCopyrightText: 2025 ORION contributors
 */

#include <boost/test/unit_test.hpp>
#include <orion/bre/feel/evaluator.hpp>
#include <orion/bre/feel/regex_cache.hpp>
#include <nlohmann/json.hpp>
#include "test_helpers.hpp"

using json = nlohmann::json;
using namespace orion::bre;
using orion::bre::feel::test::get_test_eval_ctx;

BOOST_AUTO_TEST_SUITE(math_debug_tests)

BOOST_AUTO_TEST_CASE(test_basic_math_expressions) {
    json input = {};
    // Test simple math from 0105-feel-math
    BOOST_TEST_MESSAGE("Testing basic math expressions from 0105-feel-math:");
    
    auto test_expr = [&](const std::string& expr, double expected) {
        try {
            json result = orion::bre::feel::Evaluator::evaluate(expr, input, get_test_eval_ctx());
            BOOST_TEST_MESSAGE("Expression: " << expr << " -> " << result.dump());
            if (result.is_number()) {
                BOOST_CHECK_CLOSE(result.get<double>(), expected, 0.01);
            } else {
                BOOST_FAIL("Expected numeric result for: " + expr);
            }
        } catch (const std::exception& e) {
            BOOST_TEST_MESSAGE("Expression: " << expr << " -> ERROR: " << e.what());
            BOOST_FAIL("Exception for: " + expr + " - " + e.what());
        }
    };

    auto test_null_expr = [&](const std::string& expr) {
        try {
            json result = orion::bre::feel::Evaluator::evaluate(expr, input, get_test_eval_ctx());
            BOOST_TEST_MESSAGE("Expression: " << expr << " -> " << result.dump());
            BOOST_CHECK(result.is_null());
        } catch (const std::exception& e) {
            BOOST_TEST_MESSAGE("Expression: " << expr << " -> ERROR: " << e.what());
            BOOST_FAIL("Exception for: " + expr + " - " + e.what());
        }
    };

    test_expr("10+5", 15);
    test_expr("-10+-5", -15);
    test_expr("(-10)+(-5)", -15);
    test_expr("10-5", 5);
    test_expr("-10--5", -5);
    test_expr("(-10)-(-5)", -5);
    test_expr("(10+20)-(-5+3)", 32);
    test_expr("10*5", 50);
    test_expr("-10*-5", 50);
    test_expr("(-10)*(-5)", 50);
    test_expr("10/5", 2);
    test_expr("10**5", 100000);
    test_null_expr("10+null");
}

BOOST_AUTO_TEST_CASE(test_not_function) {
    json input;
    
    BOOST_TEST_MESSAGE("Testing not() function from 0107-feel-ternary-logic-not:");
    
    // Test with boolean input
    input["A"] = true;
    json result = orion::bre::feel::Evaluator::evaluate("not(A)", input, get_test_eval_ctx());
    BOOST_TEST_MESSAGE("not(true) -> " << result.dump());
    BOOST_CHECK(result.is_boolean() && !result.get<bool>());
    
    // Test with false
    input["A"] = false;
    result = orion::bre::feel::Evaluator::evaluate("not(A)", input, get_test_eval_ctx());
    BOOST_TEST_MESSAGE("not(false) -> " << result.dump());
    BOOST_CHECK(result.is_boolean() && result.get<bool>());
    
    // Test with null
    input["A"] = nullptr;
    result = orion::bre::feel::Evaluator::evaluate("not(A)", input, get_test_eval_ctx());
    BOOST_TEST_MESSAGE("not(null) -> " << result.dump());
    BOOST_CHECK(result.is_null());
}

BOOST_AUTO_TEST_SUITE_END()