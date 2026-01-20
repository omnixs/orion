/*
 * ORION Optimized Rule Integration & Operations Native
 * SPDX-License-Identifier: Apache-2.0
 * SPDX-FileCopyrightText: 2025 ORION contributors
 */

#include <boost/test/unit_test.hpp>
#include "orion/bre/feel/parser.hpp"
#include "orion/bre/feel/lexer.hpp"
#include "orion/bre/ast_node.hpp"
#include <orion/bre/feel/evaluator.hpp>
#include <orion/bre/feel/regex_cache.hpp>
#include <nlohmann/json.hpp>
#include "test_helpers.hpp"

using json = nlohmann::json;
using namespace orion::bre::feel;
using namespace orion::bre;
using orion::bre::feel::test::get_test_eval_ctx;

// Helper function to parse and evaluate FEEL expressions
json evaluate_feel(const std::string& expression, const json& input, const orion::bre::EvaluationContext& eval_ctx)
{
    Lexer lexer;
    auto tokens = lexer.tokenize(expression);
    
    Parser parser;
    auto ast = parser.parse(tokens);
    
    return ast->evaluate(input, eval_ctx);
}

BOOST_AUTO_TEST_SUITE(conditional_expressions)

BOOST_AUTO_TEST_CASE(test_simple_true_condition)
{
    auto result = evaluate_feel("if true then 1 else 2", {}, get_test_eval_ctx());
    BOOST_TEST(result == 1);
}

BOOST_AUTO_TEST_CASE(test_simple_false_condition)
{
    auto result = evaluate_feel("if false then 1 else 2", {}, get_test_eval_ctx());
    BOOST_TEST(result == 2);
}

BOOST_AUTO_TEST_CASE(test_null_condition_goes_to_else)
{
    auto result = evaluate_feel("if null then 1 else 2", {}, get_test_eval_ctx());
    BOOST_TEST(result == 2);
}

BOOST_AUTO_TEST_CASE(test_variable_condition)
{
    json input = {{"x", 15}};
    auto result = evaluate_feel(R"(if x > 10 then "high" else "low")", input, get_test_eval_ctx());
    BOOST_TEST(result == "high");
    
    input = {{"x", 5}};
    result = evaluate_feel(R"(if x > 10 then "high" else "low")", input, get_test_eval_ctx());
    BOOST_TEST(result == "low");
}

BOOST_AUTO_TEST_CASE(test_nested_conditionals)
{
    std::string expr = R"(if x > 100 then "high" else if x > 50 then "medium" else "low")";
    
    json input = {{"x", 120}};
    BOOST_TEST(evaluate_feel(expr, input, get_test_eval_ctx()) == "high");
    
    input = {{"x", 75}};
    BOOST_TEST(evaluate_feel(expr, input, get_test_eval_ctx()) == "medium");
    
    input = {{"x", 25}};
    BOOST_TEST(evaluate_feel(expr, input, get_test_eval_ctx()) == "low");
}

BOOST_AUTO_TEST_CASE(test_arithmetic_in_branches)
{
    std::string expr = "if flag then num + 10 else num - 10";
    
    json input = {{"flag", true}, {"num", 5}};
    BOOST_TEST(evaluate_feel(expr, input, get_test_eval_ctx()) == 15);
    
    input = {{"flag", false}, {"num", 5}};
    BOOST_TEST(evaluate_feel(expr, input, get_test_eval_ctx()) == -5);
}

BOOST_AUTO_TEST_CASE(test_string_operations_in_branches)
{
    std::string expr = R"(if len > 5 then "long" else "short")";
    
    json input = {{"len", 10}};
    BOOST_TEST(evaluate_feel(expr, input, get_test_eval_ctx()) == "long");
    
    input = {{"len", 3}};
    BOOST_TEST(evaluate_feel(expr, input, get_test_eval_ctx()) == "short");
}

BOOST_AUTO_TEST_CASE(test_invalid_condition_type_returns_null)
{
    auto result = evaluate_feel("if \"string\" then 1 else 2", {}, get_test_eval_ctx());
    BOOST_TEST(result.is_null());
}

BOOST_AUTO_TEST_CASE(test_condition_with_parentheses)
{
    std::string expr = R"(if (x > 0 and y > 0) then "positive" else "not positive")";
    
    json input = {{"x", 5}, {"y", 3}};
    BOOST_TEST(evaluate_feel(expr, input, get_test_eval_ctx()) == "positive");
    
    input = {{"x", -5}, {"y", 3}};
    BOOST_TEST(evaluate_feel(expr, input, get_test_eval_ctx()) == "not positive");
}

BOOST_AUTO_TEST_CASE(test_multiline_conditional)
{
    std::string expr = R"(
        if score >= 90 then "A"
        else if score >= 80 then "B"
        else if score >= 70 then "C"
        else "F"
    )";
    
    json input = {{"score", 95}};
    BOOST_TEST(evaluate_feel(expr, input, get_test_eval_ctx()) == "A");
    
    input = {{"score", 85}};
    BOOST_TEST(evaluate_feel(expr, input, get_test_eval_ctx()) == "B");
    
    input = {{"score", 75}};
    BOOST_TEST(evaluate_feel(expr, input, get_test_eval_ctx()) == "C");
    
    input = {{"score", 60}};
    BOOST_TEST(evaluate_feel(expr, input, get_test_eval_ctx()) == "F");
}

BOOST_AUTO_TEST_CASE(test_boolean_expressions)
{
    std::string expr = "if x = 10 then true else false";
    
    json input = {{"x", 10}};
    BOOST_TEST(evaluate_feel(expr, input, get_test_eval_ctx()) == true);
    
    input = {{"x", 5}};
    BOOST_TEST(evaluate_feel(expr, input, get_test_eval_ctx()) == false);
}

BOOST_AUTO_TEST_CASE(test_numeric_comparisons)
{
    std::string expr = R"(if price < 100 then "cheap" else if price < 500 then "moderate" else "expensive")";
    
    json input = {{"price", 50}};
    BOOST_TEST(evaluate_feel(expr, input, get_test_eval_ctx()) == "cheap");
    
    input = {{"price", 250}};
    BOOST_TEST(evaluate_feel(expr, input, get_test_eval_ctx()) == "moderate");
    
    input = {{"price", 1000}};
    BOOST_TEST(evaluate_feel(expr, input, get_test_eval_ctx()) == "expensive");
}

BOOST_AUTO_TEST_SUITE_END()
