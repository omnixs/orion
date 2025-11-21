/*
 * ORION Optimized Rule Integration & Operations Native
 * SPDX-License-Identifier: Apache-2.0
 * SPDX-FileCopyrightText: 2025 ORION contributors
 *
 * Debug test for not() function parameter binding
 */

#include <boost/test/unit_test.hpp>
#include <orion/bre/feel/evaluator.hpp>
#include <orion/bre/feel/lexer.hpp>
#include <orion/bre/feel/parser.hpp>

using json = nlohmann::json;

BOOST_AUTO_TEST_SUITE(debug_not_function)

BOOST_AUTO_TEST_CASE(test_not_with_evaluator)
{
    // Test not() using Evaluator (should work)
    using orion::bre::feel::Evaluator;
    
    json result1 = Evaluator::evaluate("not(true)");
    BOOST_TEST(result1.is_boolean());
    BOOST_TEST(result1 == false);
    
    json result2 = Evaluator::evaluate("not(false)");
    BOOST_TEST(result2.is_boolean());
    BOOST_TEST(result2 == true);
}

BOOST_AUTO_TEST_CASE(test_not_with_parser_directly)
{
    // Test not() using Parser directly (this is what the old tests do)
    orion::bre::feel::Lexer lexer;
    auto tokens = lexer.tokenize("not(true)");
    
    orion::bre::feel::Parser parser;
    auto ast = parser.parse(tokens);
    
    // Check the AST structure
    BOOST_TEST_MESSAGE("AST type: " << static_cast<int>(ast->type));
    BOOST_TEST_MESSAGE("AST value: " << ast->value);
    BOOST_TEST_MESSAGE("Parameters size: " << ast->parameters.size());
    
    for (size_t i = 0; i < ast->parameters.size(); ++i)
    {
        BOOST_TEST_MESSAGE("  Parameter " << i << " name: '" << ast->parameters[i].name << "'");
        if (ast->parameters[i].valueExpr)
        {
            BOOST_TEST_MESSAGE("  Parameter " << i << " has valueExpr");
        }
    }
    
    json context = json::object();
    json result = ast->evaluate(context);
    
    BOOST_TEST(result.is_boolean());
    BOOST_TEST(result == false);
}

BOOST_AUTO_TEST_CASE(test_abs_with_parser_directly)
{
    // Test abs() for comparison (this should work based on our unit tests)
    orion::bre::feel::Lexer lexer;
    auto tokens = lexer.tokenize("abs(-42)");
    
    orion::bre::feel::Parser parser;
    auto ast = parser.parse(tokens);
    
    BOOST_TEST_MESSAGE("ABS AST type: " << static_cast<int>(ast->type));
    BOOST_TEST_MESSAGE("ABS AST value: " << ast->value);
    BOOST_TEST_MESSAGE("ABS Parameters size: " << ast->parameters.size());
    
    for (size_t i = 0; i < ast->parameters.size(); ++i)
    {
        BOOST_TEST_MESSAGE("  ABS Parameter " << i << " name: '" << ast->parameters[i].name << "'");
    }
    
    json context = json::object();
    json result = ast->evaluate(context);
    
    BOOST_TEST(result == 42);
}

BOOST_AUTO_TEST_SUITE_END()
