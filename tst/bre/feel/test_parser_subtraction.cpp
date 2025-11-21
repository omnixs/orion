/**
 * @file test_subtraction_parsing.cpp
 * @brief Unit tests for subtraction operator parsing
 * 
 * Tests that subtraction expressions are correctly:
 * - Tokenized by orion::bre::feel::Lexer (distinguishing '-' operator from negative literals)
 * - Parsed by orion::bre::feel::Parser into correct AST
 * - Evaluated correctly
 * 
 * This addresses the bug where expressions like '10-5' failed because
 * the lexer treated '-5' as a negative number instead of subtraction.
 */

#include <boost/test/unit_test.hpp>
#include <orion/bre/feel/lexer.hpp>
#include <orion/bre/feel/parser.hpp>
#include <orion/bre/feel/evaluator.hpp>
#include <nlohmann/json.hpp>

using namespace orion::bre;
using json = nlohmann::json;

BOOST_AUTO_TEST_SUITE(test_subtraction_parsing)

// =============================================================================
// Lexer Tests - Verify correct tokenization of subtraction vs negative numbers
// =============================================================================

BOOST_AUTO_TEST_CASE(test_lexer_simple_subtraction)
{
    // "10-5" should tokenize as: NUMBER(10), OPERATOR(-), NUMBER(5)
    orion::bre::feel::Lexer lexer;
    auto tokens = lexer.tokenize("10-5");
    
    BOOST_REQUIRE_EQUAL(tokens.size(), 4); // 10, -, 5, EOF
    BOOST_CHECK_EQUAL(tokens[0].type, orion::bre::feel::TokenType::NUMBER);
    BOOST_CHECK_EQUAL(tokens[0].text, "10");
    BOOST_CHECK_EQUAL(tokens[1].type, orion::bre::feel::TokenType::OPERATOR);
    BOOST_CHECK_EQUAL(tokens[1].text, "-");
    BOOST_CHECK_EQUAL(tokens[2].type, orion::bre::feel::TokenType::NUMBER);
    BOOST_CHECK_EQUAL(tokens[2].text, "5");
}

BOOST_AUTO_TEST_CASE(test_lexer_subtraction_with_spaces)
{
    // "10 - 5" should tokenize same as "10-5"
    orion::bre::feel::Lexer lexer;
    auto tokens = lexer.tokenize("10 - 5");
    
    BOOST_REQUIRE_EQUAL(tokens.size(), 4);
    BOOST_CHECK_EQUAL(tokens[0].type, orion::bre::feel::TokenType::NUMBER);
    BOOST_CHECK_EQUAL(tokens[0].text, "10");
    BOOST_CHECK_EQUAL(tokens[1].type, orion::bre::feel::TokenType::OPERATOR);
    BOOST_CHECK_EQUAL(tokens[1].text, "-");
    BOOST_CHECK_EQUAL(tokens[2].type, orion::bre::feel::TokenType::NUMBER);
    BOOST_CHECK_EQUAL(tokens[2].text, "5");
}

BOOST_AUTO_TEST_CASE(test_lexer_negative_number_at_start)
{
    // "-42" at start should be negative number literal
    orion::bre::feel::Lexer lexer;
    auto tokens = lexer.tokenize("-42");
    
    BOOST_REQUIRE_EQUAL(tokens.size(), 2); // NUMBER(-42), EOF
    BOOST_CHECK_EQUAL(tokens[0].type, orion::bre::feel::TokenType::NUMBER);
    BOOST_CHECK_EQUAL(tokens[0].text, "-42");
}

BOOST_AUTO_TEST_CASE(test_lexer_negative_after_operator)
{
    // "10+-5" should be: NUMBER(10), OPERATOR(+), NUMBER(-5)
    orion::bre::feel::Lexer lexer;
    auto tokens = lexer.tokenize("10+-5");
    
    BOOST_REQUIRE_EQUAL(tokens.size(), 4);
    BOOST_CHECK_EQUAL(tokens[0].type, orion::bre::feel::TokenType::NUMBER);
    BOOST_CHECK_EQUAL(tokens[0].text, "10");
    BOOST_CHECK_EQUAL(tokens[1].type, orion::bre::feel::TokenType::OPERATOR);
    BOOST_CHECK_EQUAL(tokens[1].text, "+");
    BOOST_CHECK_EQUAL(tokens[2].type, orion::bre::feel::TokenType::NUMBER);
    BOOST_CHECK_EQUAL(tokens[2].text, "-5");
}

BOOST_AUTO_TEST_CASE(test_lexer_double_negatives)
{
    // "-10--5" should be: NUMBER(-10), OPERATOR(-), NUMBER(-5)
    orion::bre::feel::Lexer lexer;
    auto tokens = lexer.tokenize("-10--5");
    
    BOOST_REQUIRE_EQUAL(tokens.size(), 4);
    BOOST_CHECK_EQUAL(tokens[0].type, orion::bre::feel::TokenType::NUMBER);
    BOOST_CHECK_EQUAL(tokens[0].text, "-10");
    BOOST_CHECK_EQUAL(tokens[1].type, orion::bre::feel::TokenType::OPERATOR);
    BOOST_CHECK_EQUAL(tokens[1].text, "-");
    BOOST_CHECK_EQUAL(tokens[2].type, orion::bre::feel::TokenType::NUMBER);
    BOOST_CHECK_EQUAL(tokens[2].text, "-5");
}

BOOST_AUTO_TEST_CASE(test_lexer_negative_after_paren)
{
    // "(-5)" should be: LPAREN, NUMBER(-5), RPAREN
    orion::bre::feel::Lexer lexer;
    auto tokens = lexer.tokenize("(-5)");
    
    BOOST_REQUIRE_EQUAL(tokens.size(), 4);
    BOOST_CHECK_EQUAL(tokens[0].type, orion::bre::feel::TokenType::LPAREN);
    BOOST_CHECK_EQUAL(tokens[1].type, orion::bre::feel::TokenType::NUMBER);
    BOOST_CHECK_EQUAL(tokens[1].text, "-5");
    BOOST_CHECK_EQUAL(tokens[2].type, orion::bre::feel::TokenType::RPAREN);
}

BOOST_AUTO_TEST_CASE(test_lexer_subtraction_in_parentheses)
{
    // "(10-5)" should be: LPAREN, NUMBER(10), OPERATOR(-), NUMBER(5), RPAREN
    orion::bre::feel::Lexer lexer;
    auto tokens = lexer.tokenize("(10-5)");
    
    BOOST_REQUIRE_EQUAL(tokens.size(), 6);
    BOOST_CHECK_EQUAL(tokens[0].type, orion::bre::feel::TokenType::LPAREN);
    BOOST_CHECK_EQUAL(tokens[1].type, orion::bre::feel::TokenType::NUMBER);
    BOOST_CHECK_EQUAL(tokens[1].text, "10");
    BOOST_CHECK_EQUAL(tokens[2].type, orion::bre::feel::TokenType::OPERATOR);
    BOOST_CHECK_EQUAL(tokens[2].text, "-");
    BOOST_CHECK_EQUAL(tokens[3].type, orion::bre::feel::TokenType::NUMBER);
    BOOST_CHECK_EQUAL(tokens[3].text, "5");
    BOOST_CHECK_EQUAL(tokens[4].type, orion::bre::feel::TokenType::RPAREN);
}

BOOST_AUTO_TEST_CASE(test_lexer_complex_expression)
{
    // "(10+20)-(-5+3)" should correctly distinguish all operators
    orion::bre::feel::Lexer lexer;
    auto tokens = lexer.tokenize("(10+20)-(-5+3)");
    
    // (, 10, +, 20, ), -, (, -5, +, 3, ), EOF
    BOOST_REQUIRE_EQUAL(tokens.size(), 12);
    BOOST_CHECK_EQUAL(tokens[0].type, orion::bre::feel::TokenType::LPAREN);
    BOOST_CHECK_EQUAL(tokens[1].text, "10");
    BOOST_CHECK_EQUAL(tokens[2].text, "+");
    BOOST_CHECK_EQUAL(tokens[3].text, "20");
    BOOST_CHECK_EQUAL(tokens[4].type, orion::bre::feel::TokenType::RPAREN);
    BOOST_CHECK_EQUAL(tokens[5].text, "-");
    BOOST_CHECK_EQUAL(tokens[5].type, orion::bre::feel::TokenType::OPERATOR);
    BOOST_CHECK_EQUAL(tokens[6].type, orion::bre::feel::TokenType::LPAREN);
    BOOST_CHECK_EQUAL(tokens[7].text, "-5");
    BOOST_CHECK_EQUAL(tokens[7].type, orion::bre::feel::TokenType::NUMBER);
    BOOST_CHECK_EQUAL(tokens[8].text, "+");
    BOOST_CHECK_EQUAL(tokens[9].text, "3");
    BOOST_CHECK_EQUAL(tokens[10].type, orion::bre::feel::TokenType::RPAREN);
}

// =============================================================================
// Parser + Evaluator Tests - Verify correct evaluation
// =============================================================================

BOOST_AUTO_TEST_CASE(test_eval_simple_subtraction)
{
    // 10-5 = 5
    orion::bre::feel::Lexer lexer;
    auto tokens = lexer.tokenize("10-5");
    
    orion::bre::feel::Parser parser;
    auto ast = parser.parse(tokens);
    
    json context = json::object();
    auto result = ast->evaluate(context);
    
    BOOST_CHECK(result.is_number());
    BOOST_CHECK_EQUAL(result.get<double>(), 5.0);
}

BOOST_AUTO_TEST_CASE(test_eval_subtraction_with_spaces)
{
    // 10 - 5 = 5
    auto result = orion::bre::feel::Evaluator::evaluate("10 - 5", json::object());
    
    BOOST_CHECK(result.is_number());
    BOOST_CHECK_EQUAL(result.get<double>(), 5.0);
}

BOOST_AUTO_TEST_CASE(test_eval_negative_addition)
{
    // 10+-5 = 5
    auto result = orion::bre::feel::Evaluator::evaluate("10+-5", json::object());
    
    BOOST_CHECK(result.is_number());
    BOOST_CHECK_EQUAL(result.get<double>(), 5.0);
}

BOOST_AUTO_TEST_CASE(test_eval_negative_subtraction)
{
    // -10+-5 = -15
    auto result = orion::bre::feel::Evaluator::evaluate("-10+-5", json::object());
    
    BOOST_CHECK(result.is_number());
    BOOST_CHECK_EQUAL(result.get<double>(), -15.0);
}

BOOST_AUTO_TEST_CASE(test_eval_double_negative_subtraction)
{
    // -10--5 = -10 - (-5) = -10 + 5 = -5
    auto result = orion::bre::feel::Evaluator::evaluate("-10--5", json::object());
    
    BOOST_CHECK(result.is_number());
    BOOST_CHECK_EQUAL(result.get<double>(), -5.0);
}

BOOST_AUTO_TEST_CASE(test_eval_parenthesized_negative)
{
    // (-10)+(-5) = -15
    auto result = orion::bre::feel::Evaluator::evaluate("(-10)+(-5)", json::object());
    
    BOOST_CHECK(result.is_number());
    BOOST_CHECK_EQUAL(result.get<double>(), -15.0);
}

BOOST_AUTO_TEST_CASE(test_eval_parenthesized_subtraction)
{
    // (-10)-(-5) = -10 - (-5) = -10 + 5 = -5
    auto result = orion::bre::feel::Evaluator::evaluate("(-10)-(-5)", json::object());
    
    BOOST_CHECK(result.is_number());
    BOOST_CHECK_EQUAL(result.get<double>(), -5.0);
}

BOOST_AUTO_TEST_CASE(test_eval_complex_expression)
{
    // (10+20)-(-5+3) = 30 - (-2) = 30 + 2 = 32
    auto result = orion::bre::feel::Evaluator::evaluate("(10+20)-(-5+3)", json::object());
    
    BOOST_CHECK(result.is_number());
    BOOST_CHECK_EQUAL(result.get<double>(), 32.0);
}

BOOST_AUTO_TEST_CASE(test_eval_division_with_negative)
{
    // 10+20/-5-3 = 10 + (20/(-5)) - 3 = 10 + (-4) - 3 = 10 - 4 - 3 = 3
    auto result = orion::bre::feel::Evaluator::evaluate("10+20/-5-3", json::object());
    
    BOOST_CHECK(result.is_number());
    BOOST_CHECK_EQUAL(result.get<double>(), 3.0);
}

BOOST_AUTO_TEST_CASE(test_eval_chained_subtraction)
{
    // 100-20-10-5 = ((100-20)-10)-5 = 65
    auto result = orion::bre::feel::Evaluator::evaluate("100-20-10-5", json::object());
    
    BOOST_CHECK(result.is_number());
    BOOST_CHECK_EQUAL(result.get<double>(), 65.0);
}

BOOST_AUTO_TEST_SUITE_END()
