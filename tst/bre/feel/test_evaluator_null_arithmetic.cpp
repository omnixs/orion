/*
 * ORION Optimized Rule Integration & Operations Native
 * SPDX-License-Identifier: Apache-2.0
 * SPDX-FileCopyrightText: 2025 ORION contributors
 */

#include <boost/test/unit_test.hpp>
#include <orion/bre/feel/evaluator.hpp>
#include "../../../src/bre/feel/util_internal.hpp"

using json = nlohmann::json;

BOOST_AUTO_TEST_SUITE(feel_null_arithmetic_debug)

BOOST_AUTO_TEST_CASE(test_null_arithmetic_evaluation_path) {
    orion::bre::feel::Evaluator evaluator;
    json context = {};
    
    BOOST_TEST_MESSAGE("\n=== DEBUGGING NULL ARITHMETIC EVALUATION PATH ===");
    
    // Test null arithmetic operations
    struct NullTestCase {
        std::string expression;
        std::string expected;
        std::string description;
    };
    
    std::vector<NullTestCase> null_cases = {
        {"10 - null", "null", "Subtraction with null"},
        {"null - 10", "null", "Null minus number"},
        {"10 * null", "null", "Multiplication with null"},
        {"null * 10", "null", "Null times number"},
        {"null / 10", "null", "Null divided by number"},
        {"10 / null", "null", "Division by null"}
    };
    
    BOOST_TEST_MESSAGE("Testing through orion::bre::feel::Evaluator::evaluate():");
    for (const auto& test_case : null_cases) {
        BOOST_TEST_MESSAGE("Expression: " << test_case.expression);
        json result = evaluator.evaluate(test_case.expression, context);
        BOOST_TEST_MESSAGE("  Result: " << result);
        
        if (result.is_null()) {
            BOOST_TEST_MESSAGE("  ✅ Correctly returns null");
        } else {
            BOOST_TEST_MESSAGE("  ❌ ISSUE: Should return null but got: " << result);
        }
        BOOST_TEST_MESSAGE("---");
    }
    
    // Test direct math expression evaluation
    BOOST_TEST_MESSAGE("\nTesting through detail::eval_math_expression():");
    context["testNull"] = nullptr;
    orion::bre::detail::current_eval_context = &context;
    
    std::vector<std::string> direct_math_tests = {
        "10 - testNull",
        "testNull - 10", 
        "10 * testNull",
        "testNull * 10"
    };
    
    for (const auto& expr : direct_math_tests) {
        BOOST_TEST_MESSAGE("Expression: " << expr);
        json math_result = orion::bre::detail::eval_math_expression(expr);
        BOOST_TEST_MESSAGE("  Math Result: " << math_result);
        
        if (math_result.is_null()) {
            BOOST_TEST_MESSAGE("  ✅ Math evaluator correctly returns null");
        } else {
            BOOST_TEST_MESSAGE("  ❌ ISSUE: Math evaluator should return null but got: " << math_result);
        }
        BOOST_TEST_MESSAGE("---");
    }
    
    orion::bre::detail::current_eval_context = nullptr;
}

BOOST_AUTO_TEST_SUITE_END()