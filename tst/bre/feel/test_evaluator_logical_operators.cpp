/*
 * ORION Optimized Rule Integration & Operations Native
 * SPDX-License-Identifier: Apache-2.0
 * SPDX-FileCopyrightText: 2025 ORION contributors
 */

#include <boost/test/unit_test.hpp>
#include <orion/bre/feel/evaluator.hpp>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using namespace orion::bre;

BOOST_AUTO_TEST_SUITE(test_logical_operators_debug)

BOOST_AUTO_TEST_CASE(test_logical_operators_with_string_booleans) {
    orion::bre::feel::Evaluator evaluator;
    
    // Test context with string booleans (as seen in failing test output)
    json context = {
        {"VarA", "true"},
        {"VarB", "true"},
        {"A", "true"},
        {"B", "true"}
    };
    
    BOOST_TEST_MESSAGE("Testing logical operators with context: " << context.dump());
    
    // Test expressions that are failing in TCK tests
    struct TestCase {
        std::string expression;
        bool expected_result;
        std::string description;
    };
    
    std::vector<TestCase> test_cases = {
        {"VarA and VarB", true, "AND with string 'true' variables"},
        {"A and B", true, "AND with simple string 'true' variables"}, 
        {"VarA or VarB", true, "OR with string 'true' variables"},
        {"A or B", true, "OR with simple string 'true' variables"}
    };
    
    for (const auto& test_case : test_cases) {
        BOOST_TEST_MESSAGE("Testing: " << test_case.expression);
        auto result = evaluator.evaluate(test_case.expression, context);
        BOOST_TEST_MESSAGE("Result: " << result.dump());
        
        // Check that result is not null
        BOOST_CHECK_MESSAGE(!result.is_null(), 
            "Expression '" << test_case.expression << "' should not return null, got: " << result.dump());
        
        // Check that result is boolean
        if (!result.is_null()) {
            BOOST_CHECK_MESSAGE(result.is_boolean(), 
                "Expression '" << test_case.expression << "' should return boolean, got: " << result.dump());
            
            // Check expected value
            if (result.is_boolean()) {
                BOOST_CHECK_EQUAL(result.get<bool>(), test_case.expected_result);
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(test_logical_not_with_string_booleans) {
    orion::bre::feel::Evaluator evaluator;
    
    // Test context with string booleans
    json context_true = {{"A", "true"}, {"VarA", "true"}};
    json context_false = {{"A", "false"}, {"VarA", "false"}};
    
    // Test NOT operations
    struct NotTestCase {
        std::string expression;
        json context;
        bool expected_result;
        std::string description;
    };
    
    std::vector<NotTestCase> not_test_cases = {
        {"not(A)", context_true, false, "NOT of string 'true'"},
        {"not(VarA)", context_true, false, "NOT of string 'true' variable"},
        {"not(A)", context_false, true, "NOT of string 'false'"},
        {"not(VarA)", context_false, true, "NOT of string 'false' variable"}
    };
    
    for (const auto& test_case : not_test_cases) {
        BOOST_TEST_MESSAGE("Testing: " << test_case.expression << " with context: " << test_case.context.dump());
        auto result = evaluator.evaluate(test_case.expression, test_case.context);
        BOOST_TEST_MESSAGE("Result: " << result.dump());
        
        // Check that result is not null
        BOOST_CHECK_MESSAGE(!result.is_null(), 
            "Expression '" << test_case.expression << "' should not return null, got: " << result.dump());
        
        // Check that result is boolean
        if (!result.is_null()) {
            BOOST_CHECK_MESSAGE(result.is_boolean(), 
                "Expression '" << test_case.expression << "' should return boolean, got: " << result.dump());
            
            // Check expected value
            if (result.is_boolean()) {
                BOOST_CHECK_EQUAL(result.get<bool>(), test_case.expected_result);
            }
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()