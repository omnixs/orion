/*
 * ORION Optimized Rule Integration & Operations Native
 * SPDX-License-Identifier: Apache-2.0
 * SPDX-FileCopyrightText: 2025 ORION contributors
 */

#include <boost/test/unit_test.hpp>

#include <orion/bre/feel/parser.hpp>
#include <orion/bre/feel/lexer.hpp>
#include "orion/bre/ast_node.hpp"
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using namespace orion::bre::feel;

/**
 * Helper function to parse and evaluate an expression
 */
json parse_and_evaluate(const std::string& expression, const json& context = json::object())
{
    Lexer lexer;
    auto tokens = lexer.tokenize(expression);
    
    Parser parser;
    auto ast = parser.parse(tokens);
    
    return ast->evaluate(context);
}

BOOST_AUTO_TEST_SUITE(test_feel_parser_suite)

// ============================================================================
// Literal Tests
// ============================================================================

BOOST_AUTO_TEST_CASE(TestParseLiteralInteger)
{
    auto result = parse_and_evaluate("42");
    BOOST_CHECK(result.is_number_integer());
    BOOST_CHECK_EQUAL(result.get<int>(), 42);
}

BOOST_AUTO_TEST_CASE(TestParseLiteralDecimal)
{
    auto result = parse_and_evaluate("3.14");
    BOOST_CHECK(result.is_number_float());
    BOOST_CHECK_CLOSE(result.get<double>(), 3.14, 0.001);
}

BOOST_AUTO_TEST_CASE(TestParseLiteralString)
{
    auto result = parse_and_evaluate("\"Hello World\"");
    BOOST_CHECK(result.is_string());
    BOOST_CHECK_EQUAL(result.get<std::string>(), "Hello World");
}

BOOST_AUTO_TEST_CASE(TestParseLiteralTrue)
{
    auto result = parse_and_evaluate("true");
    BOOST_CHECK(result.is_boolean());
    BOOST_CHECK_EQUAL(result.get<bool>(), true);
}

BOOST_AUTO_TEST_CASE(TestParseLiteralFalse)
{
    auto result = parse_and_evaluate("false");
    BOOST_CHECK(result.is_boolean());
    BOOST_CHECK_EQUAL(result.get<bool>(), false);
}

BOOST_AUTO_TEST_CASE(TestParseLiteralNull)
{
    auto result = parse_and_evaluate("null");
    BOOST_CHECK(result.is_null());
}

// ============================================================================
// Variable Tests
// ============================================================================

BOOST_AUTO_TEST_CASE(TestParseVariable)
{
    json context = {{"age", 25}};
    auto result = parse_and_evaluate("age", context);
    BOOST_CHECK(result.is_number_integer());
    BOOST_CHECK_EQUAL(result.get<int>(), 25);
}

BOOST_AUTO_TEST_CASE(TestParseVariableWithSpaces)
{
    json context = {{"Monthly Salary", 10000}};
    auto result = parse_and_evaluate("Monthly Salary", context);
    BOOST_CHECK(result.is_number_integer());
    BOOST_CHECK_EQUAL(result.get<int>(), 10000);
}

BOOST_AUTO_TEST_CASE(TestParseVariableWithUnderscore)
{
    json context = {{"Monthly_Salary", 10000}};
    auto result = parse_and_evaluate("Monthly Salary", context);
    BOOST_CHECK(result.is_number_integer());
    BOOST_CHECK_EQUAL(result.get<int>(), 10000);
}

BOOST_AUTO_TEST_CASE(TestParseUndefinedVariable)
{
    json context = json::object();
    BOOST_CHECK_THROW(parse_and_evaluate("undefined_var", context), std::runtime_error);
}

// ============================================================================
// Arithmetic Operator Tests
// ============================================================================

BOOST_AUTO_TEST_CASE(TestParseAddition)
{
    auto result = parse_and_evaluate("5 + 3");
    BOOST_CHECK(result.is_number());
    BOOST_CHECK_EQUAL(result.get<double>(), 8.0);
}

BOOST_AUTO_TEST_CASE(TestParseSubtraction)
{
    auto result = parse_and_evaluate("10 - 4");
    BOOST_CHECK(result.is_number());
    BOOST_CHECK_EQUAL(result.get<double>(), 6.0);
}

BOOST_AUTO_TEST_CASE(TestParseMultiplication)
{
    auto result = parse_and_evaluate("6 * 7");
    BOOST_CHECK(result.is_number());
    BOOST_CHECK_EQUAL(result.get<double>(), 42.0);
}

BOOST_AUTO_TEST_CASE(TestParseDivision)
{
    auto result = parse_and_evaluate("20 / 4");
    BOOST_CHECK(result.is_number());
    BOOST_CHECK_EQUAL(result.get<double>(), 5.0);
}

BOOST_AUTO_TEST_CASE(TestParseDivisionByZero)
{
    // Division by zero should return null per DMN 1.5 spec, not throw exception
    auto result = parse_and_evaluate("10 / 0");
    BOOST_CHECK(result.is_null());
}

BOOST_AUTO_TEST_CASE(TestParseExponentiation)
{
    auto result = parse_and_evaluate("2 ** 8");
    BOOST_CHECK(result.is_number());
    BOOST_CHECK_EQUAL(result.get<double>(), 256.0);
}

// ============================================================================
// Operator Precedence Tests
// ============================================================================

BOOST_AUTO_TEST_CASE(TestPrecedenceMultiplicationBeforeAddition)
{
    auto result = parse_and_evaluate("1 + 2 * 3");
    BOOST_CHECK_EQUAL(result.get<double>(), 7.0); // Not 9
}

BOOST_AUTO_TEST_CASE(TestPrecedenceExponentiationBeforeMultiplication)
{
    auto result = parse_and_evaluate("2 * 3 ** 2");
    BOOST_CHECK_EQUAL(result.get<double>(), 18.0); // Not 36
}

BOOST_AUTO_TEST_CASE(TestPrecedenceParenthesesOverride)
{
    auto result = parse_and_evaluate("(1 + 2) * 3");
    BOOST_CHECK_EQUAL(result.get<double>(), 9.0); // Not 7
}

BOOST_AUTO_TEST_CASE(TestPrecedenceComplexExpression)
{
    auto result = parse_and_evaluate("2 + 3 * 4 - 5");
    BOOST_CHECK_EQUAL(result.get<double>(), 9.0); // 2 + 12 - 5
}

// ============================================================================
// Comparison Operator Tests
// ============================================================================

BOOST_AUTO_TEST_CASE(TestParseLessThan)
{
    auto result = parse_and_evaluate("5 < 10");
    BOOST_CHECK(result.is_boolean());
    BOOST_CHECK_EQUAL(result.get<bool>(), true);
}

BOOST_AUTO_TEST_CASE(TestParseGreaterThan)
{
    auto result = parse_and_evaluate("15 > 10");
    BOOST_CHECK(result.is_boolean());
    BOOST_CHECK_EQUAL(result.get<bool>(), true);
}

BOOST_AUTO_TEST_CASE(TestParseLessOrEqual)
{
    auto result1 = parse_and_evaluate("5 <= 10");
    BOOST_CHECK_EQUAL(result1.get<bool>(), true);
    
    auto result2 = parse_and_evaluate("10 <= 10");
    BOOST_CHECK_EQUAL(result2.get<bool>(), true);
}

BOOST_AUTO_TEST_CASE(TestParseGreaterOrEqual)
{
    auto result1 = parse_and_evaluate("15 >= 10");
    BOOST_CHECK_EQUAL(result1.get<bool>(), true);
    
    auto result2 = parse_and_evaluate("10 >= 10");
    BOOST_CHECK_EQUAL(result2.get<bool>(), true);
}

BOOST_AUTO_TEST_CASE(TestParseEquality)
{
    auto result1 = parse_and_evaluate("10 = 10");
    BOOST_CHECK_EQUAL(result1.get<bool>(), true);
    
    auto result2 = parse_and_evaluate("10 == 10");
    BOOST_CHECK_EQUAL(result2.get<bool>(), true);
    
    auto result3 = parse_and_evaluate("10 = 5");
    BOOST_CHECK_EQUAL(result3.get<bool>(), false);
}

BOOST_AUTO_TEST_CASE(TestParseInequality)
{
    auto result1 = parse_and_evaluate("10 != 5");
    BOOST_CHECK_EQUAL(result1.get<bool>(), true);
    
    auto result2 = parse_and_evaluate("10 != 10");
    BOOST_CHECK_EQUAL(result2.get<bool>(), false);
}

// ============================================================================
// Logical Operator Tests
// ============================================================================

BOOST_AUTO_TEST_CASE(TestParseLogicalAnd)
{
    auto result1 = parse_and_evaluate("true and true");
    BOOST_CHECK_EQUAL(result1.get<bool>(), true);
    
    auto result2 = parse_and_evaluate("true and false");
    BOOST_CHECK_EQUAL(result2.get<bool>(), false);
    
    auto result3 = parse_and_evaluate("false and false");
    BOOST_CHECK_EQUAL(result3.get<bool>(), false);
}

BOOST_AUTO_TEST_CASE(TestParseLogicalOr)
{
    auto result1 = parse_and_evaluate("true or false");
    BOOST_CHECK_EQUAL(result1.get<bool>(), true);
    
    auto result2 = parse_and_evaluate("false or true");
    BOOST_CHECK_EQUAL(result2.get<bool>(), true);
    
    auto result3 = parse_and_evaluate("false or false");
    BOOST_CHECK_EQUAL(result3.get<bool>(), false);
}

BOOST_AUTO_TEST_CASE(TestParseLogicalAndPrecedence)
{
    // 'and' has higher precedence than 'or'
    auto result = parse_and_evaluate("false or true and false");
    BOOST_CHECK_EQUAL(result.get<bool>(), false); // false or (true and false) = false or false = false
}

// ============================================================================
// Unary Operator Tests
// ============================================================================

BOOST_AUTO_TEST_CASE(TestParseUnaryMinus)
{
    auto result = parse_and_evaluate("-42");
    BOOST_CHECK(result.is_number());
    BOOST_CHECK_EQUAL(result.get<double>(), -42.0);
}

BOOST_AUTO_TEST_CASE(TestParseUnaryMinusExpression)
{
    auto result = parse_and_evaluate("-(5 + 3)");
    BOOST_CHECK_EQUAL(result.get<double>(), -8.0);
}

// ============================================================================
// String Concatenation Tests
// ============================================================================

BOOST_AUTO_TEST_CASE(TestParseStringConcatenation)
{
    auto result = parse_and_evaluate("\"Hello \" + \"World\"");
    BOOST_CHECK(result.is_string());
    BOOST_CHECK_EQUAL(result.get<std::string>(), "Hello World");
}

BOOST_AUTO_TEST_CASE(TestParseStringNumberConcatenation)
{
    auto result = parse_and_evaluate("\"Age: \" + 25");
    BOOST_CHECK(result.is_string());
    BOOST_CHECK_EQUAL(result.get<std::string>(), "Age: 25");
}

// ============================================================================
// Complex Expression Tests
// ============================================================================

BOOST_AUTO_TEST_CASE(TestParseComplexLogicalExpression)
{
    json context = {{"age", 25}, {"priority", 7}};
    auto result = parse_and_evaluate("age >= 18 and priority > 5", context);
    BOOST_CHECK(result.is_boolean());
    BOOST_CHECK_EQUAL(result.get<bool>(), true);
}

BOOST_AUTO_TEST_CASE(TestParseComplexArithmeticWithVariables)
{
    json context = {{"salary", 10000}, {"bonus", 2000}};
    auto result = parse_and_evaluate("(salary + bonus) * 12", context);
    BOOST_CHECK(result.is_number());
    BOOST_CHECK_EQUAL(result.get<double>(), 144000.0);
}

BOOST_AUTO_TEST_CASE(TestParseNestedParentheses)
{
    auto result = parse_and_evaluate("((2 + 3) * (4 + 5))");
    BOOST_CHECK_EQUAL(result.get<double>(), 45.0);
}

BOOST_AUTO_TEST_CASE(TestParseDMNLikeExpression)
{
    json context = {
        {"Monthly Salary", 10000},
        {"Monthly Expenses", 3000}
    };
    auto result = parse_and_evaluate("Monthly Salary - Monthly Expenses > 5000", context);
    BOOST_CHECK(result.is_boolean());
    BOOST_CHECK_EQUAL(result.get<bool>(), true); // 10000 - 3000 = 7000 > 5000
}

// ============================================================================
// Error Handling Tests
// ============================================================================

BOOST_AUTO_TEST_CASE(TestParseInvalidExpression)
{
    BOOST_CHECK_THROW(parse_and_evaluate("5 + + 3"), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(TestParseUnmatchedParenthesis)
{
    BOOST_CHECK_THROW(parse_and_evaluate("(5 + 3"), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(TestParseEmptyExpression)
{
    BOOST_CHECK_THROW(parse_and_evaluate(""), std::runtime_error);
}

BOOST_AUTO_TEST_SUITE_END()
