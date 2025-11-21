/*
 * ORION Optimized Rule Integration & Operations Native
 * SPDX-License-Identifier: Apache-2.0
 * SPDX-FileCopyrightText: 2025 ORION contributors
 */

/**
 * @file test_exponentiation.cpp
 * @brief Unit tests for FEEL exponentiation operator (**)
 * 
 * Tests the exponentiation operator required by DMN 1.5 specification.
 * This operator is needed for 0105-feel-math TCK compliance.
 * 
 * DMN 1.5 Reference: Section 10.3.2.7 - Exponentiation has higher precedence
 * than multiplication/division but lower than unary operators.
 */

#include <boost/test/unit_test.hpp>
#include <orion/bre/feel/lexer.hpp>
#include <orion/bre/feel/evaluator.hpp>
#include <nlohmann/json.hpp>
#include <cmath>

using namespace orion::bre;

BOOST_AUTO_TEST_SUITE(test_exponentiation_suite)

// ============================================================================
// Lexer Tests: Verify ** tokenization
// ============================================================================

BOOST_AUTO_TEST_CASE(test_lexer_simple_exponentiation) {
    orion::bre::feel::Lexer lexer;
    auto tokens = lexer.tokenize("10**5");
    
    BOOST_REQUIRE_EQUAL(tokens.size(), 4); // 10, **, 5, EOF
    BOOST_CHECK_EQUAL(tokens[0].type, orion::bre::feel::TokenType::NUMBER);
    BOOST_CHECK_EQUAL(tokens[1].type, orion::bre::feel::TokenType::OPERATOR);
    BOOST_CHECK_EQUAL(tokens[1].text, "**");
    BOOST_CHECK_EQUAL(tokens[2].type, orion::bre::feel::TokenType::NUMBER);
}

BOOST_AUTO_TEST_CASE(test_lexer_exponentiation_with_spaces) {
    orion::bre::feel::Lexer lexer;
    auto tokens = lexer.tokenize("10 ** 5");
    
    BOOST_REQUIRE_EQUAL(tokens.size(), 4);
    BOOST_CHECK_EQUAL(tokens[0].type, orion::bre::feel::TokenType::NUMBER);
    BOOST_CHECK_EQUAL(tokens[1].type, orion::bre::feel::TokenType::OPERATOR);
    BOOST_CHECK_EQUAL(tokens[1].text, "**");
    BOOST_CHECK_EQUAL(tokens[2].type, orion::bre::feel::TokenType::NUMBER);
}

BOOST_AUTO_TEST_CASE(test_lexer_negative_exponent) {
    orion::bre::feel::Lexer lexer;
    auto tokens = lexer.tokenize("10**-5");
    
    BOOST_REQUIRE_EQUAL(tokens.size(), 4);
    BOOST_CHECK_EQUAL(tokens[0].type, orion::bre::feel::TokenType::NUMBER);
    BOOST_CHECK_EQUAL(tokens[1].type, orion::bre::feel::TokenType::OPERATOR);
    BOOST_CHECK_EQUAL(tokens[1].text, "**");
    BOOST_CHECK_EQUAL(tokens[2].type, orion::bre::feel::TokenType::NUMBER);
    BOOST_CHECK_EQUAL(tokens[2].text, "-5");
}

BOOST_AUTO_TEST_CASE(test_lexer_chained_exponentiation) {
    orion::bre::feel::Lexer lexer;
    auto tokens = lexer.tokenize("2**3**2");
    
    BOOST_REQUIRE_EQUAL(tokens.size(), 6); // 2, **, 3, **, 2, EOF
    BOOST_CHECK_EQUAL(tokens[0].type, orion::bre::feel::TokenType::NUMBER);
    BOOST_CHECK_EQUAL(tokens[1].type, orion::bre::feel::TokenType::OPERATOR);
    BOOST_CHECK_EQUAL(tokens[1].text, "**");
    BOOST_CHECK_EQUAL(tokens[2].type, orion::bre::feel::TokenType::NUMBER);
    BOOST_CHECK_EQUAL(tokens[3].type, orion::bre::feel::TokenType::OPERATOR);
    BOOST_CHECK_EQUAL(tokens[3].text, "**");
    BOOST_CHECK_EQUAL(tokens[4].type, orion::bre::feel::TokenType::NUMBER);
}

BOOST_AUTO_TEST_CASE(test_lexer_exponentiation_in_expression) {
    orion::bre::feel::Lexer lexer;
    auto tokens = lexer.tokenize("5+2**5+3");
    
    // Should tokenize: 5, +, 2, **, 5, +, 3, EOF
    BOOST_REQUIRE_EQUAL(tokens.size(), 8);
    BOOST_CHECK_EQUAL(tokens[0].text, "5");
    BOOST_CHECK_EQUAL(tokens[1].text, "+");
    BOOST_CHECK_EQUAL(tokens[2].text, "2");
    BOOST_CHECK_EQUAL(tokens[3].text, "**");
    BOOST_CHECK_EQUAL(tokens[4].text, "5");
    BOOST_CHECK_EQUAL(tokens[5].text, "+");
    BOOST_CHECK_EQUAL(tokens[6].text, "3");
}

// ============================================================================
// Evaluator Tests: Verify correct exponentiation evaluation
// ============================================================================

BOOST_AUTO_TEST_CASE(test_eval_simple_exponentiation) {
    // 10**5 = 100000
    using orion::bre::feel::Evaluator;
    nlohmann::json input = nlohmann::json::object();
    auto result = Evaluator::evaluate("10**5", input);
    
    BOOST_CHECK_EQUAL(result.get<double>(), 100000.0);
}

BOOST_AUTO_TEST_CASE(test_eval_negative_exponent) {
    // 10**-5 = 0.00001
    using orion::bre::feel::Evaluator;
    nlohmann::json input = nlohmann::json::object();
    auto result = Evaluator::evaluate("10**-5", input);
    
    BOOST_CHECK_CLOSE(result.get<double>(), 0.00001, 0.0001);
}

BOOST_AUTO_TEST_CASE(test_eval_exponentiation_with_parentheses) {
    // (5+2)**5 = 7**5 = 16807
    using orion::bre::feel::Evaluator;
    nlohmann::json input = nlohmann::json::object();
    auto result = Evaluator::evaluate("(5+2)**5", input);
    
    BOOST_CHECK_EQUAL(result.get<double>(), 16807.0);
}

BOOST_AUTO_TEST_CASE(test_eval_exponentiation_precedence_over_addition) {
    // 5+2**5 should be 5+(2**5) = 5+32 = 37, not (5+2)**5 = 16807
    using orion::bre::feel::Evaluator;
    nlohmann::json input = nlohmann::json::object();
    auto result = Evaluator::evaluate("5+2**5", input);
    
    BOOST_CHECK_EQUAL(result.get<double>(), 37.0);
}

BOOST_AUTO_TEST_CASE(test_eval_exponentiation_precedence_complex) {
    // 5+2**5+3 should be 5+(2**5)+3 = 5+32+3 = 40
    using orion::bre::feel::Evaluator;
    nlohmann::json input = nlohmann::json::object();
    auto result = Evaluator::evaluate("5+2**5+3", input);
    
    BOOST_CHECK_EQUAL(result.get<double>(), 40.0);
}

BOOST_AUTO_TEST_CASE(test_eval_exponentiation_with_parentheses_in_exponent) {
    // 5+2**(5+3) = 5+(2**8) = 5+256 = 261
    using orion::bre::feel::Evaluator;
    nlohmann::json input = nlohmann::json::object();
    auto result = Evaluator::evaluate("5+2**(5+3)", input);
    
    BOOST_CHECK_EQUAL(result.get<double>(), 261.0);
}

BOOST_AUTO_TEST_CASE(test_eval_chained_exponentiation_right_associative) {
    // 2**3**2 should be 2**(3**2) = 2**9 = 512 (right-associative)
    // NOT (2**3)**2 = 8**2 = 64 (left-associative)
    using orion::bre::feel::Evaluator;
    nlohmann::json input = nlohmann::json::object();
    auto result = Evaluator::evaluate("2**3**2", input);
    
    BOOST_CHECK_EQUAL(result.get<double>(), 512.0);
}

BOOST_AUTO_TEST_CASE(test_eval_exponentiation_with_multiplication) {
    // 1.2*10**3 = 1.2*1000 = 1200
    using orion::bre::feel::Evaluator;
    nlohmann::json input = nlohmann::json::object();
    auto result = Evaluator::evaluate("1.2*10**3", input);
    
    BOOST_CHECK_CLOSE(result.get<double>(), 1200.0, 0.001);
}

BOOST_AUTO_TEST_CASE(test_eval_zero_exponent) {
    // 10**0 = 1
    using orion::bre::feel::Evaluator;
    nlohmann::json input = nlohmann::json::object();
    auto result = Evaluator::evaluate("10**0", input);
    
    BOOST_CHECK_EQUAL(result.get<double>(), 1.0);
}

BOOST_AUTO_TEST_CASE(test_eval_one_exponent) {
    // 10**1 = 10
    using orion::bre::feel::Evaluator;
    nlohmann::json input = nlohmann::json::object();
    auto result = Evaluator::evaluate("10**1", input);
    
    BOOST_CHECK_EQUAL(result.get<double>(), 10.0);
}

BOOST_AUTO_TEST_CASE(test_eval_fractional_base) {
    // 0.5**2 = 0.25
    using orion::bre::feel::Evaluator;
    nlohmann::json input = nlohmann::json::object();
    auto result = Evaluator::evaluate("0.5**2", input);
    
    BOOST_CHECK_CLOSE(result.get<double>(), 0.25, 0.001);
}

BOOST_AUTO_TEST_CASE(test_eval_division_by_zero_returns_null) {
    // (10+20)/0 should return null, not throw exception
    using orion::bre::feel::Evaluator;
    nlohmann::json input = nlohmann::json::object();
    auto result = Evaluator::evaluate("(10+20)/0", input);
    
    BOOST_CHECK(result.is_null());
}

BOOST_AUTO_TEST_SUITE_END()
