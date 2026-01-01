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

using json = nlohmann::json;
using namespace orion::bre::feel;
using namespace orion::bre;

// Helper function to parse and evaluate FEEL expressions
json evaluate_feel(const std::string& expression, const json& context, const EvaluationContext& eval_ctx)
{
    Lexer lexer;
    auto tokens = lexer.tokenize(expression);
    
    Parser parser;
    auto ast = parser.parse(tokens);
    
    return ast->evaluate(context, eval_ctx);
}

BOOST_AUTO_TEST_SUITE(conditional_expressions)

BOOST_AUTO_TEST_CASE(test_simple_true_condition)
{
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    auto result = evaluate_feel("if true then 1 else 2", json::object(), eval_ctx);
    BOOST_TEST(result == 1);
}

BOOST_AUTO_TEST_CASE(test_simple_false_condition)
{
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    auto result = evaluate_feel("if false then 1 else 2", json::object(), eval_ctx);
    BOOST_TEST(result == 2);
}

BOOST_AUTO_TEST_CASE(test_null_condition_goes_to_else)
{
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    auto result = evaluate_feel("if null then 1 else 2", json::object(), eval_ctx);
    BOOST_TEST(result == 2);
}

BOOST_AUTO_TEST_CASE(test_variable_condition)
{
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    json context = {{"x", 15}};
    auto result = evaluate_feel(R"(if x > 10 then "high" else "low")", context, eval_ctx);
    BOOST_TEST(result == "high");
    
    context = {{"x", 5}};
    result = evaluate_feel(R"(if x > 10 then "high" else "low")", context, eval_ctx);
    BOOST_TEST(result == "low");
}

BOOST_AUTO_TEST_CASE(test_nested_conditionals)
{
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    std::string expr = R"(if x > 100 then "high" else if x > 50 then "medium" else "low")";
    
    json context = {{"x", 120}};
    BOOST_TEST(evaluate_feel(expr, context, eval_ctx) == "high");
    
    context = {{"x", 75}};
    BOOST_TEST(evaluate_feel(expr, context, eval_ctx) == "medium");
    
    context = {{"x", 25}};
    BOOST_TEST(evaluate_feel(expr, context, eval_ctx) == "low");
}

BOOST_AUTO_TEST_CASE(test_arithmetic_in_branches)
{
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    std::string expr = "if flag then num + 10 else num - 10";
    
    json context = {{"flag", true}, {"num", 5}};
    BOOST_TEST(evaluate_feel(expr, context, eval_ctx) == 15);
    
    context = {{"flag", false}, {"num", 5}};
    BOOST_TEST(evaluate_feel(expr, context, eval_ctx) == -5);
}

BOOST_AUTO_TEST_CASE(test_string_operations_in_branches)
{
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    std::string expr = R"(if len > 5 then "long" else "short")";
    
    json context = {{"len", 10}};
    BOOST_TEST(evaluate_feel(expr, context, eval_ctx) == "long");
    
    context = {{"len", 3}};
    BOOST_TEST(evaluate_feel(expr, context, eval_ctx) == "short");
}

BOOST_AUTO_TEST_CASE(test_invalid_condition_type_returns_null)
{
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    auto result = evaluate_feel("if \"string\" then 1 else 2", json::object(), eval_ctx);
    BOOST_TEST(result.is_null());
}

BOOST_AUTO_TEST_CASE(test_condition_with_parentheses)
{
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    std::string expr = R"(if (x > 0 and y > 0) then "positive" else "not positive")";
    
    json context = {{"x", 5}, {"y", 3}};
    BOOST_TEST(evaluate_feel(expr, context, eval_ctx) == "positive");
    
    context = {{"x", -5}, {"y", 3}};
    BOOST_TEST(evaluate_feel(expr, context, eval_ctx) == "not positive");
}

BOOST_AUTO_TEST_CASE(test_multiline_conditional)
{
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    std::string expr = R"(
        if score >= 90 then "A"
        else if score >= 80 then "B"
        else if score >= 70 then "C"
        else "F"
    )";
    
    json context = {{"score", 95}};
    BOOST_TEST(evaluate_feel(expr, context, eval_ctx) == "A");
    
    context = {{"score", 85}};
    BOOST_TEST(evaluate_feel(expr, context, eval_ctx) == "B");
    
    context = {{"score", 75}};
    BOOST_TEST(evaluate_feel(expr, context, eval_ctx) == "C");
    
    context = {{"score", 60}};
    BOOST_TEST(evaluate_feel(expr, context, eval_ctx) == "F");
}

BOOST_AUTO_TEST_CASE(test_boolean_expressions)
{
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    std::string expr = "if x = 10 then true else false";
    
    json context = {{"x", 10}};
    BOOST_TEST(evaluate_feel(expr, context, eval_ctx) == true);
    
    context = {{"x", 5}};
    BOOST_TEST(evaluate_feel(expr, context, eval_ctx) == false);
}

BOOST_AUTO_TEST_CASE(test_numeric_comparisons)
{
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    std::string expr = R"(if price < 100 then "cheap" else if price < 500 then "moderate" else "expensive")";
    
    json context = {{"price", 50}};
    BOOST_TEST(evaluate_feel(expr, context, eval_ctx) == "cheap");
    
    context = {{"price", 250}};
    BOOST_TEST(evaluate_feel(expr, context, eval_ctx) == "moderate");
    
    context = {{"price", 1000}};
    BOOST_TEST(evaluate_feel(expr, context, eval_ctx) == "expensive");
}

BOOST_AUTO_TEST_SUITE_END()
