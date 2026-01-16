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

BOOST_AUTO_TEST_SUITE(test_list_operations_suite)

// Test: Empty list literal
BOOST_AUTO_TEST_CASE(test_empty_list_literal)
{
    std::string expression = "[]";
    orion::bre::feel::Evaluator evaluator;
    json result = evaluator.evaluate(expression, {}, get_test_eval_ctx());
    
    BOOST_CHECK(result.is_array());
    BOOST_CHECK_EQUAL(result.size(), 0);
}

// Test: Simple number list
BOOST_AUTO_TEST_CASE(test_number_list_literal)
{
    std::string expression = "[1, 2, 3]";
    orion::bre::feel::Evaluator evaluator;
    json result = evaluator.evaluate(expression, {}, get_test_eval_ctx());
    
    BOOST_CHECK(result.is_array());
    BOOST_CHECK_EQUAL(result.size(), 3);
    BOOST_CHECK_EQUAL(result[0], 1);
    BOOST_CHECK_EQUAL(result[1], 2);
    BOOST_CHECK_EQUAL(result[2], 3);
}

// Test: String list
BOOST_AUTO_TEST_CASE(test_string_list_literal)
{
    std::string expression = "[\"apple\", \"banana\", \"cherry\"]";
    orion::bre::feel::Evaluator evaluator;
    json result = evaluator.evaluate(expression, {}, get_test_eval_ctx());
    
    BOOST_CHECK(result.is_array());
    BOOST_CHECK_EQUAL(result.size(), 3);
    BOOST_CHECK_EQUAL(result[0], "apple");
    BOOST_CHECK_EQUAL(result[1], "banana");
    BOOST_CHECK_EQUAL(result[2], "cherry");
}

// Test: Mixed type list
BOOST_AUTO_TEST_CASE(test_mixed_type_list)
{
    std::string expression = "[1, \"hello\", true, null]";
    orion::bre::feel::Evaluator evaluator;
    json result = evaluator.evaluate(expression, {}, get_test_eval_ctx());
    
    BOOST_CHECK(result.is_array());
    BOOST_CHECK_EQUAL(result.size(), 4);
    BOOST_CHECK_EQUAL(result[0], 1);
    BOOST_CHECK_EQUAL(result[1], "hello");
    BOOST_CHECK_EQUAL(result[2], true);
    BOOST_CHECK(result[3].is_null());
}

// Test: List with expressions
BOOST_AUTO_TEST_CASE(test_list_with_expressions)
{
    std::string expression = "[1 + 2, 3 * 4, 10 - 5]";
    orion::bre::feel::Evaluator evaluator;
    json result = evaluator.evaluate(expression, {}, get_test_eval_ctx());
    
    BOOST_CHECK(result.is_array());
    BOOST_CHECK_EQUAL(result.size(), 3);
    BOOST_CHECK_EQUAL(result[0], 3);
    BOOST_CHECK_EQUAL(result[1], 12);
    BOOST_CHECK_EQUAL(result[2], 5);
}

// Test: List with variables
BOOST_AUTO_TEST_CASE(test_list_with_variables)
{
    std::string expression = "[x, y, z]";
    json input = {
        {"x", 10},
        {"y", 20},
        {"z", 30}
    };
    
    orion::bre::feel::Evaluator evaluator;
    json result = evaluator.evaluate(expression, input, get_test_eval_ctx());
    
    BOOST_CHECK(result.is_array());
    BOOST_CHECK_EQUAL(result.size(), 3);
    BOOST_CHECK_EQUAL(result[0], 10);
    BOOST_CHECK_EQUAL(result[1], 20);
    BOOST_CHECK_EQUAL(result[2], 30);
}

// Test: Nested lists
BOOST_AUTO_TEST_CASE(test_nested_lists)
{
    std::string expression = "[[1, 2], [3, 4], [5, 6]]";
    orion::bre::feel::Evaluator evaluator;
    json result = evaluator.evaluate(expression, {}, get_test_eval_ctx());
    
    BOOST_CHECK(result.is_array());
    BOOST_CHECK_EQUAL(result.size(), 3);
    BOOST_CHECK(result[0].is_array());
    BOOST_CHECK_EQUAL(result[0].size(), 2);
    BOOST_CHECK_EQUAL(result[0][0], 1);
    BOOST_CHECK_EQUAL(result[0][1], 2);
    BOOST_CHECK_EQUAL(result[1][0], 3);
    BOOST_CHECK_EQUAL(result[1][1], 4);
    BOOST_CHECK_EQUAL(result[2][0], 5);
    BOOST_CHECK_EQUAL(result[2][1], 6);
}

// Test: List in function call
BOOST_AUTO_TEST_CASE(test_list_in_function_call)
{
    std::string expression = "all([true, true, true])";
    orion::bre::feel::Evaluator evaluator;
    json result = evaluator.evaluate(expression, {}, get_test_eval_ctx());
    
    BOOST_CHECK(result.is_boolean());
    BOOST_CHECK_EQUAL(result, true);
}

// Test: Single element list
BOOST_AUTO_TEST_CASE(test_single_element_list)
{
    std::string expression = "[42]";
    orion::bre::feel::Evaluator evaluator;
    json result = evaluator.evaluate(expression, {}, get_test_eval_ctx());
    
    BOOST_CHECK(result.is_array());
    BOOST_CHECK_EQUAL(result.size(), 1);
    BOOST_CHECK_EQUAL(result[0], 42);
}

// Test: List with decimal numbers
BOOST_AUTO_TEST_CASE(test_decimal_list)
{
    std::string expression = "[1.5, 2.7, 3.14]";
    orion::bre::feel::Evaluator evaluator;
    json result = evaluator.evaluate(expression, {}, get_test_eval_ctx());
    
    BOOST_CHECK(result.is_array());
    BOOST_CHECK_EQUAL(result.size(), 3);
    BOOST_CHECK_CLOSE(result[0].get<double>(), 1.5, 0.001);
    BOOST_CHECK_CLOSE(result[1].get<double>(), 2.7, 0.001);
    BOOST_CHECK_CLOSE(result[2].get<double>(), 3.14, 0.001);
}

// Test: List with negative numbers
BOOST_AUTO_TEST_CASE(test_negative_number_list)
{
    std::string expression = "[-1, -2, -3]";
    orion::bre::feel::Evaluator evaluator;
    json result = evaluator.evaluate(expression, {}, get_test_eval_ctx());
    
    BOOST_CHECK(result.is_array());
    BOOST_CHECK_EQUAL(result.size(), 3);
    BOOST_CHECK_EQUAL(result[0], -1);
    BOOST_CHECK_EQUAL(result[1], -2);
    BOOST_CHECK_EQUAL(result[2], -3);
}

// Test: List passed to any() function
BOOST_AUTO_TEST_CASE(test_any_with_list_literal)
{
    std::string expression = "any([false, false, true])";
    orion::bre::feel::Evaluator evaluator;
    json result = evaluator.evaluate(expression, {}, get_test_eval_ctx());
    
    BOOST_CHECK(result.is_boolean());
    BOOST_CHECK_EQUAL(result, true);
}

BOOST_AUTO_TEST_SUITE_END()


// Test: Empty list literal
