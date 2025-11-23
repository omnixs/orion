/*
 * ORION Optimized Rule Integration & Operations Native
 * SPDX-License-Identifier: Apache-2.0
 * SPDX-FileCopyrightText: 2025 ORION contributors
 */

#include <boost/test/unit_test.hpp>
#include <orion/bre/feel/evaluator.hpp>
#include <nlohmann/json.hpp>

namespace orion::bre::detail {
    nlohmann::json eval_math_expression(std::string_view expr);
    extern thread_local const nlohmann::json* current_eval_context;
}

BOOST_AUTO_TEST_SUITE(test_math_evaluator_direct)

BOOST_AUTO_TEST_CASE(test_negative_number_math_evaluator_direct) {
    // Test the math evaluator directly to isolate parsing issues
    struct TestCase {
        std::string expression;
        double expected;
        std::string description;
    };
    
    std::vector<TestCase> test_cases = {
        {"20 / -5", -4.0, "Simple division by negative"},
        {"20/-5", -4.0, "Division by negative without spaces"},
        {"-5", -5.0, "Simple negative number"},
        {"20", 20.0, "Simple positive number"},
        {"20 / 5", 4.0, "Simple division"},
        {"10 + 20 / -5 - 3", 3.0, "Complex expression with negative"}
    };
    
    for (const auto& test_case : test_cases) {
        BOOST_TEST_MESSAGE("Testing direct math evaluator: " << test_case.expression);
        
        auto result = orion::bre::detail::eval_math_expression(test_case.expression);
        
        if (result.is_null()) {
            BOOST_TEST_MESSAGE("  Result: null (FAIL - expected: " << test_case.expected << ")");
            BOOST_CHECK_MESSAGE(false, "Expression '" << test_case.expression << "' returned null instead of " << test_case.expected);
        } else if (result.is_number()) {
            double actual = result.get<double>();
            BOOST_TEST_MESSAGE("  Result: " << actual << " (expected: " << test_case.expected << ")");
            BOOST_CHECK_CLOSE(actual, test_case.expected, 0.01);
        } else {
            BOOST_CHECK_MESSAGE(false, "Expression '" << test_case.expression << "' returned unexpected type");
        }
    }
}

BOOST_AUTO_TEST_CASE(test_feel_evaluator_vs_direct_math) {
    using json = nlohmann::json;
    
    orion::bre::feel::Evaluator evaluator;
    json context = json::object();
    
    // Compare orion::bre::feel::Evaluator vs direct math evaluator for expressions that fail
    std::vector<std::string> failing_expressions = {
        "20 / -5",
        "10 + 20/-5 - 3",
        "20/-5"
    };
    
    for (const auto& expr : failing_expressions) {
        BOOST_TEST_MESSAGE("Comparing evaluators for: " << expr);
        
        // Test through orion::bre::feel::Evaluator
        auto feel_result = evaluator.evaluate(expr, context);
        
        // Test through direct math evaluator (no context)
        auto math_result = orion::bre::detail::eval_math_expression(expr);
        
        // Test through direct math evaluator WITH context setup (like orion::bre::feel::Evaluator does)
        orion::bre::detail::current_eval_context = &context;
        auto math_result_with_context = orion::bre::detail::eval_math_expression(expr);
        orion::bre::detail::current_eval_context = nullptr;
        
        BOOST_TEST_MESSAGE("  orion::bre::feel::Evaluator result: " << feel_result);
        BOOST_TEST_MESSAGE("  Math evaluator (no context): " << math_result);
        BOOST_TEST_MESSAGE("  Math evaluator (with context): " << math_result_with_context);
        
        // Check if context setup makes a difference
        if (math_result.is_null() && math_result_with_context.is_null()) {
            BOOST_TEST_MESSAGE("  Both math evaluator calls return null - fundamental parsing issue");
        } else if (math_result.is_number() && math_result_with_context.is_null()) {
            BOOST_TEST_MESSAGE("  Context setup causes math evaluator to fail");
        } else if (math_result.is_null() && math_result_with_context.is_number()) {
            BOOST_TEST_MESSAGE("  Context setup fixes math evaluator");
        } else {
            BOOST_TEST_MESSAGE("  Math evaluator results are consistent");
        }
        
        // Both should return the same result for pure math expressions
        if (feel_result.is_null() && math_result.is_null()) {
            BOOST_TEST_MESSAGE("  Both return null - need to investigate why");
        } else if (feel_result.is_number() && math_result.is_number()) {
            BOOST_CHECK_CLOSE(feel_result.get<double>(), math_result.get<double>(), 0.01);
        } else {
            BOOST_TEST_MESSAGE("  Results differ in type - orion::bre::feel::Evaluator might be using fallback arithmetic");
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()