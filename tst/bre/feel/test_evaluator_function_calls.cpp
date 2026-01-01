/*
 * ORION Optimized Rule Integration & Operations Native
 * SPDX-License-Identifier: Apache-2.0
 * SPDX-FileCopyrightText: 2025 ORION contributors
 */

#include <boost/test/unit_test.hpp>
#include <orion/bre/feel/parser.hpp>
#include <orion/bre/feel/evaluator.hpp>
#include <orion/bre/feel/regex_cache.hpp>
#include <orion/bre/feel/lexer.hpp>
#include <orion/bre/ast_node.hpp>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using namespace orion::bre;

BOOST_AUTO_TEST_SUITE(test_function_calls_suite)

// Test not() function
BOOST_AUTO_TEST_CASE(test_not_function_basic)
{
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    // not(true) should return false
    orion::bre::feel::Lexer lexer;
    auto tokens = lexer.tokenize("not(true)");
    
    orion::bre::feel::Parser parser;
    auto ast = parser.parse(tokens);
    
    json context = json::object();
    json result = ast->evaluate(context, eval_ctx);
    
    BOOST_TEST(result.is_boolean());
    BOOST_TEST(result == false);
}

BOOST_AUTO_TEST_CASE(test_not_function_false)
{
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    // not(false) should return true
    orion::bre::feel::Lexer lexer;
    auto tokens = lexer.tokenize("not(false)");
    
    orion::bre::feel::Parser parser;
    auto ast = parser.parse(tokens);
    
    json context = json::object();
    json result = ast->evaluate(context, eval_ctx);
    
    BOOST_TEST(result.is_boolean());
    BOOST_TEST(result == true);
}

BOOST_AUTO_TEST_CASE(test_not_function_null_propagation)
{
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    // not(null) should return null (DMN null propagation)
    orion::bre::feel::Lexer lexer;
    auto tokens = lexer.tokenize("not(null)");
    
    orion::bre::feel::Parser parser;
    auto ast = parser.parse(tokens);
    
    json context = json::object();
    json result = ast->evaluate(context, eval_ctx);
    
    BOOST_TEST(result.is_null());
}

BOOST_AUTO_TEST_CASE(test_not_function_with_variable)
{
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    // not(A) where A is boolean variable
    orion::bre::feel::Lexer lexer;
    auto tokens = lexer.tokenize("not(A)");
    
    orion::bre::feel::Parser parser;
    auto ast = parser.parse(tokens);
    
    // Test with A = true
    json context1 = {{"A", true}};
    json result1 = ast->evaluate(context1, eval_ctx);
    BOOST_TEST(result1 == false);
    
    // Test with A = false
    json context2 = {{"A", false}};
    json result2 = ast->evaluate(context2, eval_ctx);
    BOOST_TEST(result2 == true);
}

BOOST_AUTO_TEST_CASE(test_not_function_with_string_boolean)
{
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    // not(A) where A is string "true" or "false" (DMN compatibility)
    orion::bre::feel::Lexer lexer;
    auto tokens = lexer.tokenize("not(A)");
    
    orion::bre::feel::Parser parser;
    auto ast = parser.parse(tokens);
    
    // Test with A = "true" (as string)
    json context1 = {{"A", "true"}};
    json result1 = ast->evaluate(context1, eval_ctx);
    BOOST_TEST(result1 == false);
    
    // Test with A = "false" (as string)
    json context2 = {{"A", "false"}};
    json result2 = ast->evaluate(context2, eval_ctx);
    BOOST_TEST(result2 == true);
}

// Test contains() function
BOOST_AUTO_TEST_CASE(test_contains_function_basic)
{
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    // contains("hello world", "world") should return true
    orion::bre::feel::Lexer lexer;
    auto tokens = lexer.tokenize("contains(\"hello world\", \"world\")");
    
    orion::bre::feel::Parser parser;
    auto ast = parser.parse(tokens);
    
    json context = json::object();
    json result = ast->evaluate(context, eval_ctx);
    
    BOOST_TEST(result.is_boolean());
    BOOST_TEST(result == true);
}

BOOST_AUTO_TEST_CASE(test_contains_function_not_found)
{
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    // contains("hello", "goodbye") should return false
    orion::bre::feel::Lexer lexer;
    auto tokens = lexer.tokenize("contains(\"hello\", \"goodbye\")");
    
    orion::bre::feel::Parser parser;
    auto ast = parser.parse(tokens);
    
    json context = json::object();
    json result = ast->evaluate(context, eval_ctx);
    
    BOOST_TEST(result.is_boolean());
    BOOST_TEST(result == false);
}

BOOST_AUTO_TEST_CASE(test_contains_function_null_propagation)
{
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    // contains(null, "test") should return null
    orion::bre::feel::Lexer lexer;
    auto tokens = lexer.tokenize("contains(null, \"test\")");
    
    orion::bre::feel::Parser parser;
    auto ast = parser.parse(tokens);
    
    json context = json::object();
    json result = ast->evaluate(context, eval_ctx);
    
    BOOST_TEST(result.is_null());
}

// Test all() function
BOOST_AUTO_TEST_CASE(test_all_function_all_true)
{
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    // Create an AST manually since we don't have list literal syntax yet
    auto funcNode = std::make_unique<ASTNode>(ASTNodeType::FUNCTION_CALL, "all");
    
    // Create a parameter with a variable that will be a list in context
    auto varNode = std::make_unique<ASTNode>(ASTNodeType::VARIABLE, "list");
    FunctionParameter param;
    param.name = ""; // Positional parameter
    param.valueExpr = std::move(varNode);
    funcNode->parameters.push_back(std::move(param));
    
    json context = {{"list", json::array({true, true, true})}};
    json result = funcNode->evaluate(context, eval_ctx);
    
    BOOST_TEST(result.is_boolean());
    BOOST_TEST(result == true);
}

BOOST_AUTO_TEST_CASE(test_all_function_has_false)
{
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    auto funcNode = std::make_unique<ASTNode>(ASTNodeType::FUNCTION_CALL, "all");
    auto varNode = std::make_unique<ASTNode>(ASTNodeType::VARIABLE, "list");
    FunctionParameter param;
    param.name = ""; // Positional parameter
    param.valueExpr = std::move(varNode);
    funcNode->parameters.push_back(std::move(param));
    
    json context = {{"list", json::array({true, false, true})}};
    json result = funcNode->evaluate(context, eval_ctx);
    
    BOOST_TEST(result.is_boolean());
    BOOST_TEST(result == false);
}

BOOST_AUTO_TEST_CASE(test_all_function_empty_list)
{
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    // all([]) should return true (vacuous truth)
    auto funcNode = std::make_unique<ASTNode>(ASTNodeType::FUNCTION_CALL, "all");
    auto varNode = std::make_unique<ASTNode>(ASTNodeType::VARIABLE, "list");
    FunctionParameter param;
    param.name = ""; // Positional parameter
    param.valueExpr = std::move(varNode);
    funcNode->parameters.push_back(std::move(param));
    
    json context = {{"list", json::array()}};
    json result = funcNode->evaluate(context, eval_ctx);
    
    BOOST_TEST(result.is_boolean());
    BOOST_TEST(result == true);
}

// Test any() function
BOOST_AUTO_TEST_CASE(test_any_function_has_true)
{
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    auto funcNode = std::make_unique<ASTNode>(ASTNodeType::FUNCTION_CALL, "any");
    auto varNode = std::make_unique<ASTNode>(ASTNodeType::VARIABLE, "list");
    FunctionParameter param;
    param.name = ""; // Positional parameter
    param.valueExpr = std::move(varNode);
    funcNode->parameters.push_back(std::move(param));
    
    json context = {{"list", json::array({false, true, false})}};
    json result = funcNode->evaluate(context, eval_ctx);
    
    BOOST_TEST(result.is_boolean());
    BOOST_TEST(result == true);
}

BOOST_AUTO_TEST_CASE(test_any_function_all_false)
{
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    auto funcNode = std::make_unique<ASTNode>(ASTNodeType::FUNCTION_CALL, "any");
    auto varNode = std::make_unique<ASTNode>(ASTNodeType::VARIABLE, "list");
    FunctionParameter param;
    param.name = ""; // Positional parameter
    param.valueExpr = std::move(varNode);
    funcNode->parameters.push_back(std::move(param));
    
    json context = {{"list", json::array({false, false, false})}};
    json result = funcNode->evaluate(context, eval_ctx);
    
    BOOST_TEST(result.is_boolean());
    BOOST_TEST(result == false);
}

BOOST_AUTO_TEST_CASE(test_any_function_empty_list)
{
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    // any([]) should return false
    auto funcNode = std::make_unique<ASTNode>(ASTNodeType::FUNCTION_CALL, "any");
    auto varNode = std::make_unique<ASTNode>(ASTNodeType::VARIABLE, "list");
    FunctionParameter param;
    param.name = ""; // Positional parameter
    param.valueExpr = std::move(varNode);
    funcNode->parameters.push_back(std::move(param));
    
    json context = {{"list", json::array()}};
    json result = funcNode->evaluate(context, eval_ctx);
    
    BOOST_TEST(result.is_boolean());
    BOOST_TEST(result == false);
}

// Test error cases
BOOST_AUTO_TEST_CASE(test_not_function_wrong_argument_count)
{
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    // not() with no arguments should return null (DMN spec: invalid args → null)
    auto funcNode = std::make_unique<ASTNode>(ASTNodeType::FUNCTION_CALL, "not");
    json context = json::object();
    
    auto result = funcNode->evaluate(context, eval_ctx);
    BOOST_CHECK(result.is_null());
}

BOOST_AUTO_TEST_CASE(test_contains_wrong_argument_count)
{
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    // contains() with wrong number of arguments should return null (DMN spec)
    auto funcNode = std::make_unique<ASTNode>(ASTNodeType::FUNCTION_CALL, "contains");
    
    // Create parameter with wrong count
    FunctionParameter param;
    param.name = "";  // positional
    param.valueExpr = std::make_unique<ASTNode>(ASTNodeType::LITERAL_STRING, "test");
    funcNode->parameters.push_back(std::move(param));
    
    json context = json::object();
    auto result = funcNode->evaluate(context, eval_ctx);
    BOOST_CHECK(result.is_null());
}

BOOST_AUTO_TEST_CASE(test_unknown_function)
{
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    // Unknown function should throw
    auto funcNode = std::make_unique<ASTNode>(ASTNodeType::FUNCTION_CALL, "unknownFunc");
    json context = json::object();
    
    BOOST_CHECK_THROW((void)funcNode->evaluate(context, eval_ctx), std::runtime_error);
}

BOOST_AUTO_TEST_SUITE_END()
