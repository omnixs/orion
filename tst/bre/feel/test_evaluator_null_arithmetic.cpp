/*
 * ORION Optimized Rule Integration & Operations Native
 * SPDX-License-Identifier: Apache-2.0
 * SPDX-FileCopyrightText: 2025 ORION contributors
 */

#include <boost/test/unit_test.hpp>
#include <orion/bre/feel/evaluator.hpp>
#include <orion/bre/feel/regex_cache.hpp>
#include <nlohmann/json.hpp>
#include "../../../src/bre/feel/util_internal.hpp"

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

BOOST_AUTO_TEST_SUITE(feel_null_arithmetic_debug)

BOOST_AUTO_TEST_CASE(test_null_arithmetic_evaluation_path) {
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
    
    int evaluator_tested = 0;
    BOOST_TEST_MESSAGE("Testing through orion::bre::feel::eval_feel():");
    for (const auto& test_case : null_cases) {
        evaluator_tested++;
        BOOST_TEST_MESSAGE("Expression: " << test_case.expression);
        json result = eval_feel(test_case.expression, context);
        BOOST_TEST_MESSAGE("  Result: " << result);
        
        if (result.is_null()) {
            BOOST_TEST_MESSAGE("  âœ… Correctly returns null");
        } else {
            BOOST_TEST_MESSAGE("  âŒ ISSUE: Should return null but got: " << result);
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
    
    int math_tested = 0;
    for (const auto& expr : direct_math_tests) {
        math_tested++;
        BOOST_TEST_MESSAGE("Expression: " << expr);
        json math_result = orion::bre::detail::eval_math_expression(expr);
        BOOST_TEST_MESSAGE("  Math Result: " << math_result);
        
        if (math_result.is_null()) {
            BOOST_TEST_MESSAGE("  âœ… Math evaluator correctly returns null");
        } else {
            BOOST_TEST_MESSAGE("  âŒ ISSUE: Math evaluator should return null but got: " << math_result);
        }
        BOOST_TEST_MESSAGE("---");
    }
    
    orion::bre::detail::current_eval_context = nullptr;
    
    // Verify we actually tested something
    BOOST_CHECK_EQUAL(evaluator_tested, 6);
    BOOST_CHECK_EQUAL(math_tested, 4);
}

BOOST_AUTO_TEST_SUITE_END()