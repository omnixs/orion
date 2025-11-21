/*
 * ORION Optimized Rule Integration & Operations Native
 * SPDX-License-Identifier: Apache-2.0
 * SPDX-FileCopyrightText: 2025 ORION contributors
 */

#include <boost/test/unit_test.hpp>
#include <orion/bre/feel/expr.hpp>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

BOOST_AUTO_TEST_SUITE(feel_segfault_debug)

BOOST_AUTO_TEST_CASE(test_and_expression_with_null)
{
    // This is the exact expression causing the segfault in TCK tests
    std::string expr = "A and B";
    json ctx = {{"A", nullptr}, {"B", true}};
    json out;
    std::string err;
    
    // This should not segfault
    bool result = orion::bre::feel::eval_feel_literal(expr, ctx, out, err);
    
    if (result) {
        // In DMN ternary logic: null AND true = null
        BOOST_CHECK(out.is_null());
    } else {
        BOOST_FAIL("Expression evaluation failed: " + err);
    }
}

BOOST_AUTO_TEST_CASE(test_and_expression_variations)
{
    json out;
    std::string err;
    
    // Test various combinations that might cause issues
    struct TestCase {
        std::string expr;
        json ctx;
        bool should_be_null;
    };
    
    std::vector<TestCase> test_cases = {
        {"A and B", {{"A", nullptr}, {"B", true}}, true},
        {"A and B", {{"A", true}, {"B", nullptr}}, true},
        {"A and B", {{"A", nullptr}, {"B", nullptr}}, true},
        {"A and B", {{"A", false}, {"B", nullptr}}, false}, // false AND null = false
        {"A and B", {{"A", nullptr}, {"B", false}}, false}, // null AND false = false
        {"A and B", {{"A", true}, {"B", true}}, false},     // true AND true = true
    };
    
    for (const auto& test_case : test_cases) {
        bool result = orion::bre::feel::eval_feel_literal(test_case.expr, test_case.ctx, out, err);
        BOOST_REQUIRE_MESSAGE(result, "Expression '" + test_case.expr + 
                             "' with context " + test_case.ctx.dump() + 
                             " failed: " + err);
        
        if (test_case.should_be_null) {
            BOOST_CHECK_MESSAGE(out.is_null(), 
                               "Expected null for '" + test_case.expr + 
                               "' with context " + test_case.ctx.dump() + 
                               " but got: " + out.dump());
        }
    }
}

BOOST_AUTO_TEST_CASE(test_or_expression_with_null)
{
    // Test OR expressions as well
    std::string expr = "A or B";
    json ctx = {{"A", nullptr}, {"B", true}};
    json out;
    std::string err;
    
    bool result = orion::bre::feel::eval_feel_literal(expr, ctx, out, err);
    
    if (result) {
        // In DMN ternary logic: null OR true = true
        BOOST_CHECK(out.is_boolean() && out.get<bool>() == true);
    } else {
        BOOST_FAIL("OR expression evaluation failed: " + err);
    }
}

BOOST_AUTO_TEST_SUITE_END()