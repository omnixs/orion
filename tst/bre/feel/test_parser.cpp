/*
 * ORION Optimized Rule Integration & Operations Native
 * SPDX-License-Identifier: Apache-2.0
 * SPDX-FileCopyrightText: 2025 ORION contributors
 */

#include <boost/test/unit_test.hpp>

#include <orion/bre/feel/parser.hpp>
#include <orion/bre/feel/lexer.hpp>
#include "orion/bre/ast_node.hpp"
#include <orion/bre/feel/evaluator.hpp>
#include <orion/bre/feel/regex_cache.hpp>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using namespace orion::bre::feel;

/**
 * Helper function to parse and evaluate an expression
 */
json parse_and_evaluate(const std::string& expression, const json& context, const EvaluationContext& eval_ctx)
{
    Lexer lexer;
    auto tokens = lexer.tokenize(expression);
    
    Parser parser;
    auto ast = parser.parse(tokens);
    
    return ast->evaluate(context, eval_ctx);
}

BOOST_AUTO_TEST_SUITE(test_feel_parser_suite)

// ============================================================================
// Literal Tests
// ============================================================================

BOOST_AUTO_TEST_CASE(TestParseLiteralInteger)
{
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    auto result = parse_and_evaluate("42", json::object(), eval_ctx);
    BOOST_CHECK(result.is_number_integer());
    BOOST_CHECK_EQUAL(result.get<int>(), 42);
}

BOOST_AUTO_TEST_CASE(TestParseLiteralDecimal)
{
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    auto result = parse_and_evaluate("3.14", json::object(), eval_ctx);
    BOOST_CHECK(result.is_number_float());
    BOOST_CHECK_CLOSE(result.get<double>(), 3.14, 0.001);
}

BOOST_AUTO_TEST_CASE(TestParseLiteralString)
{
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    auto result = parse_and_evaluate("\"Hello World\"", json::object(), eval_ctx);
    BOOST_CHECK(result.is_string());
    BOOST_CHECK_EQUAL(result.get<std::string>(), "Hello World");
}

BOOST_AUTO_TEST_CASE(TestParseLiteralTrue)
{
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    auto result = parse_and_evaluate("true", json::object(), eval_ctx);
    BOOST_CHECK(result.is_boolean());
    BOOST_CHECK_EQUAL(result.get<bool>(), true);
}

BOOST_AUTO_TEST_CASE(TestParseLiteralFalse)
{
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    auto result = parse_and_evaluate("false", json::object(), eval_ctx);
    BOOST_CHECK(result.is_boolean());
    BOOST_CHECK_EQUAL(result.get<bool>(), false);
}

BOOST_AUTO_TEST_CASE(TestParseLiteralNull)
{
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    auto result = parse_and_evaluate("null", json::object(), eval_ctx);
    BOOST_CHECK(result.is_null());
}

// ============================================================================
// Variable Tests
// ============================================================================

BOOST_AUTO_TEST_CASE(TestParseVariable)
{
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    json context = {{"age", 25}};
    auto result = parse_and_evaluate("age", context, eval_ctx);
    BOOST_CHECK(result.is_number_integer());
    BOOST_CHECK_EQUAL(result.get<int>(), 25);
}

BOOST_AUTO_TEST_CASE(TestParseVariableWithSpaces)
{
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    json context = {{"Monthly Salary", 10000}};
    auto result = parse_and_evaluate("Monthly Salary", context, eval_ctx);
    BOOST_CHECK(result.is_number_integer());
    BOOST_CHECK_EQUAL(result.get<int>(), 10000);
}

BOOST_AUTO_TEST_CASE(TestParseVariableWithUnderscore)
{
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    json context = {{"Monthly_Salary", 10000}};
    auto result = parse_and_evaluate("Monthly Salary", context, eval_ctx);
    BOOST_CHECK(result.is_number_integer());
    BOOST_CHECK_EQUAL(result.get<int>(), 10000);
}

BOOST_AUTO_TEST_CASE(TestParseUndefinedVariable)
{
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    json context = json::object();
    BOOST_CHECK_THROW(parse_and_evaluate("undefined_var", context, eval_ctx), std::runtime_error);
}

// ============================================================================
// Arithmetic Operator Tests
// ============================================================================

BOOST_AUTO_TEST_CASE(TestParseAddition)
{
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    auto result = parse_and_evaluate("5 + 3", json::object(), eval_ctx);
    BOOST_CHECK(result.is_number());
    BOOST_CHECK_EQUAL(result.get<double>(), 8.0);
}

BOOST_AUTO_TEST_CASE(TestParseSubtraction)
{
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    auto result = parse_and_evaluate("10 - 4", json::object(), eval_ctx);
    BOOST_CHECK(result.is_number());
    BOOST_CHECK_EQUAL(result.get<double>(), 6.0);
}

BOOST_AUTO_TEST_CASE(TestParseMultiplication)
{
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    auto result = parse_and_evaluate("6 * 7", json::object(), eval_ctx);
    BOOST_CHECK(result.is_number());
    BOOST_CHECK_EQUAL(result.get<double>(), 42.0);
}

BOOST_AUTO_TEST_CASE(TestParseDivision)
{
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    auto result = parse_and_evaluate("20 / 4", json::object(), eval_ctx);
    BOOST_CHECK(result.is_number());
    BOOST_CHECK_EQUAL(result.get<double>(), 5.0);
}

BOOST_AUTO_TEST_CASE(TestParseDivisionByZero)
{
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    // Division by zero should return null per DMN 1.5 spec, not throw exception
    auto result = parse_and_evaluate("10 / 0", json::object(), eval_ctx);
    BOOST_CHECK(result.is_null());
}

BOOST_AUTO_TEST_CASE(TestParseExponentiation)
{
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    auto result = parse_and_evaluate("2 ** 8", json::object(), eval_ctx);
    BOOST_CHECK(result.is_number());
    BOOST_CHECK_EQUAL(result.get<double>(), 256.0);
}

// ============================================================================
// Operator Precedence Tests
// ============================================================================

BOOST_AUTO_TEST_CASE(TestPrecedenceMultiplicationBeforeAddition)
{
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    auto result = parse_and_evaluate("1 + 2 * 3", json::object(), eval_ctx);
    BOOST_CHECK_EQUAL(result.get<double>(), 7.0); // Not 9
}

BOOST_AUTO_TEST_CASE(TestPrecedenceExponentiationBeforeMultiplication)
{
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    auto result = parse_and_evaluate("2 * 3 ** 2", json::object(), eval_ctx);
    BOOST_CHECK_EQUAL(result.get<double>(), 18.0); // Not 36
}

BOOST_AUTO_TEST_CASE(TestPrecedenceParenthesesOverride)
{
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    auto result = parse_and_evaluate("(1 + 2) * 3", json::object(), eval_ctx);
    BOOST_CHECK_EQUAL(result.get<double>(), 9.0); // Not 7
}

BOOST_AUTO_TEST_CASE(TestPrecedenceComplexExpression)
{
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    auto result = parse_and_evaluate("2 + 3 * 4 - 5", json::object(), eval_ctx);
    BOOST_CHECK_EQUAL(result.get<double>(), 9.0); // 2 + 12 - 5
}

// ============================================================================
// Comparison Operator Tests
// ============================================================================

BOOST_AUTO_TEST_CASE(TestParseLessThan)
{
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    auto result = parse_and_evaluate("5 < 10", json::object(), eval_ctx);
    BOOST_CHECK(result.is_boolean());
    BOOST_CHECK_EQUAL(result.get<bool>(), true);
}

BOOST_AUTO_TEST_CASE(TestParseGreaterThan)
{
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    auto result = parse_and_evaluate("15 > 10", json::object(), eval_ctx);
    BOOST_CHECK(result.is_boolean());
    BOOST_CHECK_EQUAL(result.get<bool>(), true);
}

BOOST_AUTO_TEST_CASE(TestParseLessOrEqual)
{
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    auto result1 = parse_and_evaluate("5 <= 10", json::object(), eval_ctx);
    BOOST_CHECK_EQUAL(result1.get<bool>(), true);
    
    auto result2 = parse_and_evaluate("10 <= 10", json::object(), eval_ctx);
    BOOST_CHECK_EQUAL(result2.get<bool>(), true);
}

BOOST_AUTO_TEST_CASE(TestParseGreaterOrEqual)
{
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    auto result1 = parse_and_evaluate("15 >= 10", json::object(), eval_ctx);
    BOOST_CHECK_EQUAL(result1.get<bool>(), true);
    
    auto result2 = parse_and_evaluate("10 >= 10", json::object(), eval_ctx);
    BOOST_CHECK_EQUAL(result2.get<bool>(), true);
}

BOOST_AUTO_TEST_CASE(TestParseEquality)
{
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    auto result1 = parse_and_evaluate("10 = 10", json::object(), eval_ctx);
    BOOST_CHECK_EQUAL(result1.get<bool>(), true);
    
    auto result2 = parse_and_evaluate("10 == 10", json::object(), eval_ctx);
    BOOST_CHECK_EQUAL(result2.get<bool>(), true);
    
    auto result3 = parse_and_evaluate("10 = 5", json::object(), eval_ctx);
    BOOST_CHECK_EQUAL(result3.get<bool>(), false);
}

BOOST_AUTO_TEST_CASE(TestParseInequality)
{
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    auto result1 = parse_and_evaluate("10 != 5", json::object(), eval_ctx);
    BOOST_CHECK_EQUAL(result1.get<bool>(), true);
    
    auto result2 = parse_and_evaluate("10 != 10", json::object(), eval_ctx);
    BOOST_CHECK_EQUAL(result2.get<bool>(), false);
}

// ============================================================================
// Logical Operator Tests
// ============================================================================

BOOST_AUTO_TEST_CASE(TestParseLogicalAnd)
{
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    auto result1 = parse_and_evaluate("true and true", json::object(), eval_ctx);
    BOOST_CHECK_EQUAL(result1.get<bool>(), true);
    
    auto result2 = parse_and_evaluate("true and false", json::object(), eval_ctx);
    BOOST_CHECK_EQUAL(result2.get<bool>(), false);
    
    auto result3 = parse_and_evaluate("false and false", json::object(), eval_ctx);
    BOOST_CHECK_EQUAL(result3.get<bool>(), false);
}

BOOST_AUTO_TEST_CASE(TestParseLogicalOr)
{
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    auto result1 = parse_and_evaluate("true or false", json::object(), eval_ctx);
    BOOST_CHECK_EQUAL(result1.get<bool>(), true);
    
    auto result2 = parse_and_evaluate("false or true", json::object(), eval_ctx);
    BOOST_CHECK_EQUAL(result2.get<bool>(), true);
    
    auto result3 = parse_and_evaluate("false or false", json::object(), eval_ctx);
    BOOST_CHECK_EQUAL(result3.get<bool>(), false);
}

BOOST_AUTO_TEST_CASE(TestParseLogicalAndPrecedence)
{
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    // 'and' has higher precedence than 'or'
    auto result = parse_and_evaluate("false or true and false", json::object(), eval_ctx);
    BOOST_CHECK_EQUAL(result.get<bool>(), false); // false or (true and false) = false or false = false
}

// ============================================================================
// Unary Operator Tests
// ============================================================================

BOOST_AUTO_TEST_CASE(TestParseUnaryMinus)
{
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    auto result = parse_and_evaluate("-42", json::object(), eval_ctx);
    BOOST_CHECK(result.is_number());
    BOOST_CHECK_EQUAL(result.get<double>(), -42.0);
}

BOOST_AUTO_TEST_CASE(TestParseUnaryMinusExpression)
{
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    auto result = parse_and_evaluate("-(5 + 3)", json::object(), eval_ctx);
    BOOST_CHECK_EQUAL(result.get<double>(), -8.0);
}

// ============================================================================
// String Concatenation Tests
// ============================================================================

BOOST_AUTO_TEST_CASE(TestParseStringConcatenation)
{
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    auto result = parse_and_evaluate("\"Hello \" + \"World\"", json::object(), eval_ctx);
    BOOST_CHECK(result.is_string());
    BOOST_CHECK_EQUAL(result.get<std::string>(), "Hello World");
}

BOOST_AUTO_TEST_CASE(TestParseStringNumberConcatenation)
{
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    auto result = parse_and_evaluate("\"Age: \" + 25", json::object(), eval_ctx);
    BOOST_CHECK(result.is_string());
    BOOST_CHECK_EQUAL(result.get<std::string>(), "Age: 25");
}

// ============================================================================
// Complex Expression Tests
// ============================================================================

BOOST_AUTO_TEST_CASE(TestParseComplexLogicalExpression)
{
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    json context = {{"age", 25}, {"priority", 7}};
    auto result = parse_and_evaluate("age >= 18 and priority > 5", context, eval_ctx);
    BOOST_CHECK(result.is_boolean());
    BOOST_CHECK_EQUAL(result.get<bool>(), true);
}

BOOST_AUTO_TEST_CASE(TestParseComplexArithmeticWithVariables)
{
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    json context = {{"salary", 10000}, {"bonus", 2000}};
    auto result = parse_and_evaluate("(salary + bonus) * 12", context, eval_ctx);
    BOOST_CHECK(result.is_number());
    BOOST_CHECK_EQUAL(result.get<double>(), 144000.0);
}

BOOST_AUTO_TEST_CASE(TestParseNestedParentheses)
{
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    auto result = parse_and_evaluate("((2 + 3) * (4 + 5))", json::object(), eval_ctx);
    BOOST_CHECK_EQUAL(result.get<double>(), 45.0);
}

BOOST_AUTO_TEST_CASE(TestParseDMNLikeExpression)
{
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    json context = {
        {"Monthly Salary", 10000},
        {"Monthly Expenses", 3000}
    };
    auto result = parse_and_evaluate("Monthly Salary - Monthly Expenses > 5000", context, eval_ctx);
    BOOST_CHECK(result.is_boolean());
    BOOST_CHECK_EQUAL(result.get<bool>(), true); // 10000 - 3000 = 7000 > 5000
}

// ============================================================================
// Error Handling Tests
// ============================================================================

BOOST_AUTO_TEST_CASE(TestParseInvalidExpression)
{
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    BOOST_CHECK_THROW(parse_and_evaluate("5 + + 3", json::object(), eval_ctx), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(TestParseUnmatchedParenthesis)
{
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    BOOST_CHECK_THROW(parse_and_evaluate("(5 + 3", json::object(), eval_ctx), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(TestParseEmptyExpression)
{
    orion::bre::feel::RegexCache regex_cache;
    orion::bre::feel::EvaluationContext eval_ctx;
    eval_ctx.regex_cache = &regex_cache;
    BOOST_CHECK_THROW(parse_and_evaluate("", json::object(), eval_ctx), std::runtime_error);
}

BOOST_AUTO_TEST_SUITE_END()
