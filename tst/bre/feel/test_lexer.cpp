/*
 * ORION Optimized Rule Integration & Operations Native
 * SPDX-License-Identifier: Apache-2.0
 * SPDX-FileCopyrightText: 2025 ORION contributors
 */

/**
 * @file test_feel_lexer.cpp
 * @brief Unit tests for FEEL lexer (tokenization)
 */

#include <boost/test/unit_test.hpp>
#include <orion/bre/feel/lexer.hpp>

using namespace orion::bre::feel;

BOOST_AUTO_TEST_SUITE(FeelLexerSuite)

// Helper function to convert token stream to string for easy debugging
std::string tokensToString(const std::vector<Token>& tokens)
{
    std::string result;
    for (const auto& token : tokens)
    {
        if (!result.empty()) result += ", ";
        
        std::string typeStr;
        switch (token.type)
        {
            case TokenType::NUMBER: typeStr = "NUM"; break;
            case TokenType::STRING: typeStr = "STR"; break;
            case TokenType::IDENTIFIER: typeStr = "ID"; break;
            case TokenType::KEYWORD: typeStr = "KW"; break;
            case TokenType::OPERATOR: typeStr = "OP"; break;
            case TokenType::LPAREN: typeStr = "("; break;
            case TokenType::RPAREN: typeStr = ")"; break;
            case TokenType::COMMA: typeStr = ","; break;
            case TokenType::END_OF_INPUT: typeStr = "EOF"; break;
            default: typeStr = "?"; break;
        }
        
        result += typeStr;
        if (!token.text.empty() && token.type != TokenType::END_OF_INPUT)
        {
            result += "[" + token.text + "]";
        }
    }
    return result;
}

BOOST_AUTO_TEST_CASE(TestSimpleNumber)
{
    Lexer lexer;
    auto tokens = lexer.tokenize("42");
    
    BOOST_REQUIRE_EQUAL(tokens.size(), 2); // Number + EOF
    BOOST_CHECK_EQUAL(tokens[0].type, TokenType::NUMBER);
    BOOST_CHECK_EQUAL(tokens[0].text, "42");
    BOOST_CHECK_EQUAL(tokens[1].type, TokenType::END_OF_INPUT);
}

BOOST_AUTO_TEST_CASE(TestDecimalNumber)
{
    Lexer lexer;
    auto tokens = lexer.tokenize("3.14");
    
    BOOST_REQUIRE_EQUAL(tokens.size(), 2);
    BOOST_CHECK_EQUAL(tokens[0].type, TokenType::NUMBER);
    BOOST_CHECK_EQUAL(tokens[0].text, "3.14");
}

BOOST_AUTO_TEST_CASE(TestNegativeNumber)
{
    Lexer lexer;
    auto tokens = lexer.tokenize("-42");
    
    BOOST_REQUIRE_EQUAL(tokens.size(), 2);
    BOOST_CHECK_EQUAL(tokens[0].type, TokenType::NUMBER);
    BOOST_CHECK_EQUAL(tokens[0].text, "-42");
}

BOOST_AUTO_TEST_CASE(TestScientificNotation)
{
    Lexer lexer;
    auto tokens = lexer.tokenize("1.5e-10");
    
    BOOST_REQUIRE_EQUAL(tokens.size(), 2);
    BOOST_CHECK_EQUAL(tokens[0].type, TokenType::NUMBER);
    BOOST_CHECK_EQUAL(tokens[0].text, "1.5e-10");
}

BOOST_AUTO_TEST_CASE(TestStringLiteral)
{
    Lexer lexer;
    auto tokens = lexer.tokenize("\"Hello World\"");
    
    BOOST_REQUIRE_EQUAL(tokens.size(), 2);
    BOOST_CHECK_EQUAL(tokens[0].type, TokenType::STRING);
    BOOST_CHECK_EQUAL(tokens[0].text, "\"Hello World\"");
}

BOOST_AUTO_TEST_CASE(TestIdentifier)
{
    Lexer lexer;
    auto tokens = lexer.tokenize("age");
    
    BOOST_REQUIRE_EQUAL(tokens.size(), 2);
    BOOST_CHECK_EQUAL(tokens[0].type, TokenType::IDENTIFIER);
    BOOST_CHECK_EQUAL(tokens[0].text, "age");
}

BOOST_AUTO_TEST_CASE(TestIdentifierWithSpaces)
{
    Lexer lexer;
    auto tokens = lexer.tokenize("Monthly Salary");
    
    BOOST_REQUIRE_EQUAL(tokens.size(), 2);
    BOOST_CHECK_EQUAL(tokens[0].type, TokenType::IDENTIFIER);
    BOOST_CHECK_EQUAL(tokens[0].text, "Monthly Salary");
}

BOOST_AUTO_TEST_CASE(TestKeywords)
{
    Lexer lexer;
    
    auto tokens1 = lexer.tokenize("true");
    BOOST_CHECK_EQUAL(tokens1[0].type, TokenType::KEYWORD);
    
    auto tokens2 = lexer.tokenize("false");
    BOOST_CHECK_EQUAL(tokens2[0].type, TokenType::KEYWORD);
    
    auto tokens3 = lexer.tokenize("null");
    BOOST_CHECK_EQUAL(tokens3[0].type, TokenType::KEYWORD);
    
    auto tokens4 = lexer.tokenize("and");
    BOOST_CHECK_EQUAL(tokens4[0].type, TokenType::KEYWORD);
    
    auto tokens5 = lexer.tokenize("or");
    BOOST_CHECK_EQUAL(tokens5[0].type, TokenType::KEYWORD);
}

BOOST_AUTO_TEST_CASE(TestArithmeticOperators)
{
    Lexer lexer;
    
    auto tokens = lexer.tokenize("1 + 2 - 3 * 4 / 5");
    
    // 1, +, 2, -, 3, *, 4, /, 5, EOF
    BOOST_REQUIRE_EQUAL(tokens.size(), 10);
    BOOST_CHECK_EQUAL(tokens[0].type, TokenType::NUMBER);
    BOOST_CHECK_EQUAL(tokens[1].type, TokenType::OPERATOR);
    BOOST_CHECK_EQUAL(tokens[1].text, "+");
    BOOST_CHECK_EQUAL(tokens[3].type, TokenType::OPERATOR);
    BOOST_CHECK_EQUAL(tokens[3].text, "-");
    BOOST_CHECK_EQUAL(tokens[5].type, TokenType::OPERATOR);
    BOOST_CHECK_EQUAL(tokens[5].text, "*");
    BOOST_CHECK_EQUAL(tokens[7].type, TokenType::OPERATOR);
    BOOST_CHECK_EQUAL(tokens[7].text, "/");
}

BOOST_AUTO_TEST_CASE(TestExponentiation)
{
    Lexer lexer;
    auto tokens = lexer.tokenize("2 ** 3");
    
    BOOST_REQUIRE_EQUAL(tokens.size(), 4); // 2, **, 3, EOF
    BOOST_CHECK_EQUAL(tokens[1].type, TokenType::OPERATOR);
    BOOST_CHECK_EQUAL(tokens[1].text, "**");
}

BOOST_AUTO_TEST_CASE(TestComparisonOperators)
{
    Lexer lexer;
    
    auto tokens = lexer.tokenize("age >= 18");
    
    // age, >=, 18, EOF
    BOOST_REQUIRE_EQUAL(tokens.size(), 4);
    BOOST_CHECK_EQUAL(tokens[0].type, TokenType::IDENTIFIER);
    BOOST_CHECK_EQUAL(tokens[1].type, TokenType::OPERATOR);
    BOOST_CHECK_EQUAL(tokens[1].text, ">=");
    BOOST_CHECK_EQUAL(tokens[2].type, TokenType::NUMBER);
}

BOOST_AUTO_TEST_CASE(TestParentheses)
{
    Lexer lexer;
    auto tokens = lexer.tokenize("(age + 5) * 2");
    
    // (, age, +, 5, ), *, 2, EOF
    BOOST_REQUIRE_EQUAL(tokens.size(), 8);
    BOOST_CHECK_EQUAL(tokens[0].type, TokenType::LPAREN);
    BOOST_CHECK_EQUAL(tokens[4].type, TokenType::RPAREN);
}

BOOST_AUTO_TEST_CASE(TestComplexExpression)
{
    Lexer lexer;
    auto tokens = lexer.tokenize("age >= 18 and priority > 5");
    
    // age, >=, 18, and, priority, >, 5, EOF
    BOOST_REQUIRE_EQUAL(tokens.size(), 8);
    BOOST_CHECK_EQUAL(tokens[0].type, TokenType::IDENTIFIER);
    BOOST_CHECK_EQUAL(tokens[0].text, "age");
    BOOST_CHECK_EQUAL(tokens[1].type, TokenType::OPERATOR);
    BOOST_CHECK_EQUAL(tokens[1].text, ">=");
    BOOST_CHECK_EQUAL(tokens[2].type, TokenType::NUMBER);
    BOOST_CHECK_EQUAL(tokens[2].text, "18");
    BOOST_CHECK_EQUAL(tokens[3].type, TokenType::KEYWORD);
    BOOST_CHECK_EQUAL(tokens[3].text, "and");
    BOOST_CHECK_EQUAL(tokens[4].type, TokenType::IDENTIFIER);
    BOOST_CHECK_EQUAL(tokens[4].text, "priority");
    BOOST_CHECK_EQUAL(tokens[5].type, TokenType::OPERATOR);
    BOOST_CHECK_EQUAL(tokens[5].text, ">");
    BOOST_CHECK_EQUAL(tokens[6].type, TokenType::NUMBER);
    BOOST_CHECK_EQUAL(tokens[6].text, "5");
}

BOOST_AUTO_TEST_CASE(TestStringConcatenation)
{
    Lexer lexer;
    auto tokens = lexer.tokenize("\"Greeting \" + Name");
    
    // "Greeting ", +, Name, EOF
    BOOST_REQUIRE_EQUAL(tokens.size(), 4);
    BOOST_CHECK_EQUAL(tokens[0].type, TokenType::STRING);
    BOOST_CHECK_EQUAL(tokens[0].text, "\"Greeting \"");
    BOOST_CHECK_EQUAL(tokens[1].type, TokenType::OPERATOR);
    BOOST_CHECK_EQUAL(tokens[1].text, "+");
    BOOST_CHECK_EQUAL(tokens[2].type, TokenType::IDENTIFIER);
    BOOST_CHECK_EQUAL(tokens[2].text, "Name");
}

BOOST_AUTO_TEST_SUITE_END()
