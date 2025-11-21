/*
 * ORION Optimized Rule Integration & Operations Native
 * SPDX-License-Identifier: Apache-2.0
 * SPDX-FileCopyrightText: 2025 ORION contributors
 */

/**
 * @file test_property_access.cpp
 * @brief Comprehensive tests for property access (dot notation) in FEEL expressions
 * 
 * Tests AST-based property access implementation including:
 * - Simple property access (obj.field)
 * - Chained property access (obj.field.subfield)
 * - Property access in arithmetic expressions
 * - Property access with missing properties
 * - Property access on non-objects
 * - Property access with DMN naming variations
 */

#include <boost/test/unit_test.hpp>
#include <orion/bre/feel/lexer.hpp>
#include <orion/bre/feel/parser.hpp>
#include "orion/bre/ast_node.hpp"
#include <nlohmann/json.hpp>

using namespace orion::bre;
using json = nlohmann::json;

BOOST_AUTO_TEST_SUITE(test_property_access_suite)

// ============================================================================
// LEXER TESTS - Verify tokenization of property access
// ============================================================================

BOOST_AUTO_TEST_CASE(test_lexer_simple_property_access)
{
    orion::bre::feel::Lexer lexer;
    auto tokens = lexer.tokenize("loan.principal");
    
    BOOST_REQUIRE_EQUAL(tokens.size(), 4); // identifier, dot, identifier, END
    BOOST_CHECK_EQUAL(tokens[0].type, orion::bre::feel::TokenType::IDENTIFIER);
    BOOST_CHECK_EQUAL(tokens[0].text, "loan");
    BOOST_CHECK_EQUAL(tokens[1].type, orion::bre::feel::TokenType::DOT);
    BOOST_CHECK_EQUAL(tokens[1].text, ".");
    BOOST_CHECK_EQUAL(tokens[2].type, orion::bre::feel::TokenType::IDENTIFIER);
    BOOST_CHECK_EQUAL(tokens[2].text, "principal");
}

BOOST_AUTO_TEST_CASE(test_lexer_chained_property_access)
{
    orion::bre::feel::Lexer lexer;
    auto tokens = lexer.tokenize("person.address.city");
    
    BOOST_REQUIRE_EQUAL(tokens.size(), 6); // id, dot, id, dot, id, END
    BOOST_CHECK_EQUAL(tokens[0].text, "person");
    BOOST_CHECK_EQUAL(tokens[1].type, orion::bre::feel::TokenType::DOT);
    BOOST_CHECK_EQUAL(tokens[2].text, "address");
    BOOST_CHECK_EQUAL(tokens[3].type, orion::bre::feel::TokenType::DOT);
    BOOST_CHECK_EQUAL(tokens[4].text, "city");
}

BOOST_AUTO_TEST_CASE(test_lexer_property_in_expression)
{
    orion::bre::feel::Lexer lexer;
    auto tokens = lexer.tokenize("loan.principal * loan.rate");
    
    // loan, dot, principal, *, loan, dot, rate, END
    BOOST_REQUIRE_EQUAL(tokens.size(), 8);
    BOOST_CHECK_EQUAL(tokens[0].text, "loan");
    BOOST_CHECK_EQUAL(tokens[1].type, orion::bre::feel::TokenType::DOT);
    BOOST_CHECK_EQUAL(tokens[2].text, "principal");
    BOOST_CHECK_EQUAL(tokens[3].type, orion::bre::feel::TokenType::OPERATOR);
    BOOST_CHECK_EQUAL(tokens[3].text, "*");
}

// ============================================================================
// PARSER TESTS - Verify AST construction for property access
// ============================================================================

BOOST_AUTO_TEST_CASE(test_parser_simple_property_access)
{
    orion::bre::feel::Lexer lexer;
    auto tokens = lexer.tokenize("loan.principal");
    
    orion::bre::feel::Parser parser;
    auto ast = parser.parse(tokens);
    
    BOOST_REQUIRE(ast != nullptr);
    BOOST_CHECK_EQUAL(ast->type, ASTNodeType::PROPERTY_ACCESS);
    BOOST_CHECK_EQUAL(ast->value, "principal"); // Property name
    BOOST_REQUIRE_EQUAL(ast->children.size(), 1);
    BOOST_CHECK_EQUAL(ast->children[0]->type, ASTNodeType::VARIABLE);
    BOOST_CHECK_EQUAL(ast->children[0]->value, "loan");
}

BOOST_AUTO_TEST_CASE(test_parser_chained_property_access)
{
    orion::bre::feel::Lexer lexer;
    auto tokens = lexer.tokenize("person.address.city");
    
    orion::bre::feel::Parser parser;
    auto ast = parser.parse(tokens);
    
    // Should create nested PROPERTY_ACCESS nodes
    // Top level: .city
    BOOST_REQUIRE(ast != nullptr);
    BOOST_CHECK_EQUAL(ast->type, ASTNodeType::PROPERTY_ACCESS);
    BOOST_CHECK_EQUAL(ast->value, "city");
    
    // Child: .address
    BOOST_REQUIRE_EQUAL(ast->children.size(), 1);
    BOOST_CHECK_EQUAL(ast->children[0]->type, ASTNodeType::PROPERTY_ACCESS);
    BOOST_CHECK_EQUAL(ast->children[0]->value, "address");
    
    // Grandchild: person
    BOOST_REQUIRE_EQUAL(ast->children[0]->children.size(), 1);
    BOOST_CHECK_EQUAL(ast->children[0]->children[0]->type, ASTNodeType::VARIABLE);
    BOOST_CHECK_EQUAL(ast->children[0]->children[0]->value, "person");
}

BOOST_AUTO_TEST_CASE(test_parser_property_in_arithmetic)
{
    orion::bre::feel::Lexer lexer;
    auto tokens = lexer.tokenize("loan.principal * loan.rate");
    
    orion::bre::feel::Parser parser;
    auto ast = parser.parse(tokens);
    
    // Top level should be multiplication
    BOOST_REQUIRE(ast != nullptr);
    BOOST_CHECK_EQUAL(ast->type, ASTNodeType::BINARY_OP);
    BOOST_CHECK_EQUAL(ast->value, "*");
    
    // Left child: loan.principal
    BOOST_REQUIRE_EQUAL(ast->children.size(), 2);
    BOOST_CHECK_EQUAL(ast->children[0]->type, ASTNodeType::PROPERTY_ACCESS);
    BOOST_CHECK_EQUAL(ast->children[0]->value, "principal");
    
    // Right child: loan.rate
    BOOST_CHECK_EQUAL(ast->children[1]->type, ASTNodeType::PROPERTY_ACCESS);
    BOOST_CHECK_EQUAL(ast->children[1]->value, "rate");
}

BOOST_AUTO_TEST_CASE(test_parser_parenthesized_property_access)
{
    orion::bre::feel::Lexer lexer;
    auto tokens = lexer.tokenize("(loan).principal");
    
    orion::bre::feel::Parser parser;
    auto ast = parser.parse(tokens);
    
    BOOST_REQUIRE(ast != nullptr);
    BOOST_CHECK_EQUAL(ast->type, ASTNodeType::PROPERTY_ACCESS);
    BOOST_CHECK_EQUAL(ast->value, "principal");
    BOOST_REQUIRE_EQUAL(ast->children.size(), 1);
    BOOST_CHECK_EQUAL(ast->children[0]->type, ASTNodeType::VARIABLE);
    BOOST_CHECK_EQUAL(ast->children[0]->value, "loan");
}

// ============================================================================
// EVALUATOR TESTS - Verify property access evaluation
// ============================================================================

BOOST_AUTO_TEST_CASE(test_eval_simple_property_access)
{
    orion::bre::feel::Lexer lexer;
    auto tokens = lexer.tokenize("loan.principal");
    
    orion::bre::feel::Parser parser;
    auto ast = parser.parse(tokens);
    
    json context = {
        {"loan", {
            {"principal", 100000},
            {"rate", 0.05}
        }}
    };
    
    json result = ast->evaluate(context);
    BOOST_CHECK_EQUAL(result.get<int>(), 100000);
}

BOOST_AUTO_TEST_CASE(test_eval_chained_property_access)
{
    orion::bre::feel::Lexer lexer;
    auto tokens = lexer.tokenize("person.address.city");
    
    orion::bre::feel::Parser parser;
    auto ast = parser.parse(tokens);
    
    json context = {
        {"person", {
            {"name", "John"},
            {"address", {
                {"street", "123 Main St"},
                {"city", "Boston"},
                {"zip", "02101"}
            }}
        }}
    };
    
    json result = ast->evaluate(context);
    BOOST_CHECK_EQUAL(result.get<std::string>(), "Boston");
}

BOOST_AUTO_TEST_CASE(test_eval_property_in_arithmetic)
{
    orion::bre::feel::Lexer lexer;
    auto tokens = lexer.tokenize("loan.principal * loan.rate");
    
    orion::bre::feel::Parser parser;
    auto ast = parser.parse(tokens);
    
    json context = {
        {"loan", {
            {"principal", 100000},
            {"rate", 0.05}
        }}
    };
    
    json result = ast->evaluate(context);
    BOOST_CHECK_CLOSE(result.get<double>(), 5000.0, 0.001);
}

BOOST_AUTO_TEST_CASE(test_eval_complex_expression_with_properties)
{
    // Expression from TCK test: (loan.principal*loan.rate/12)/(1-(1+loan.rate/12)**-loan.termMonths)
    std::string expr = "(loan.principal * loan.rate / 12) / (1 - (1 + loan.rate / 12) ** -loan.termMonths)";
    
    orion::bre::feel::Lexer lexer;
    auto tokens = lexer.tokenize(expr);
    
    orion::bre::feel::Parser parser;
    auto ast = parser.parse(tokens);
    
    json context = {
        {"loan", {
            {"principal", 100000},
            {"rate", 0.06},
            {"termMonths", 360}
        }}
    };
    
    json result = ast->evaluate(context);
    
    // Monthly payment should be around 599.55
    BOOST_CHECK_CLOSE(result.get<double>(), 599.55, 1.0); // Within 1%
}

BOOST_AUTO_TEST_CASE(test_eval_property_with_decimal_number)
{
    // Ensure we can distinguish property access from decimal numbers
    orion::bre::feel::Lexer lexer;
    auto tokens = lexer.tokenize("obj.value + .872");
    
    orion::bre::feel::Parser parser;
    auto ast = parser.parse(tokens);
    
    json context = {
        {"obj", {
            {"value", 0.128}
        }}
    };
    
    json result = ast->evaluate(context);
    BOOST_CHECK_CLOSE(result.get<double>(), 1.0, 0.001);
}

// ============================================================================
// ERROR HANDLING TESTS
// ============================================================================

BOOST_AUTO_TEST_CASE(test_eval_missing_property)
{
    orion::bre::feel::Lexer lexer;
    auto tokens = lexer.tokenize("loan.missingField");
    
    orion::bre::feel::Parser parser;
    auto ast = parser.parse(tokens);
    
    json context = {
        {"loan", {
            {"principal", 100000}
        }}
    };
    
    BOOST_CHECK_THROW((void)ast->evaluate(context), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(test_eval_property_on_non_object)
{
    orion::bre::feel::Lexer lexer;
    auto tokens = lexer.tokenize("x.field");
    
    orion::bre::feel::Parser parser;
    auto ast = parser.parse(tokens);
    
    json context = {
        {"x", 42} // x is a number, not an object
    };
    
    BOOST_CHECK_THROW((void)ast->evaluate(context), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(test_eval_property_on_null)
{
    orion::bre::feel::Lexer lexer;
    auto tokens = lexer.tokenize("x.field");
    
    orion::bre::feel::Parser parser;
    auto ast = parser.parse(tokens);
    
    json context = {
        {"x", nullptr} // x is null
    };
    
    json result = ast->evaluate(context);
    BOOST_CHECK(result.is_null()); // DMN: null propagation
}

// ============================================================================
// DMN NAMING FLEXIBILITY TESTS
// ============================================================================

BOOST_AUTO_TEST_CASE(test_eval_property_naming_variants)
{
    orion::bre::feel::Lexer lexer;
    auto tokens = lexer.tokenize("person.firstName");
    
    orion::bre::feel::Parser parser;
    auto ast = parser.parse(tokens);
    
    // Test with underscored property name
    json context1 = {
        {"person", {
            {"first_name", "John"}
        }}
    };
    
    json result1 = ast->evaluate(context1);
    BOOST_CHECK_EQUAL(result1.get<std::string>(), "John");
    
    // Test with lowercase property name
    json context2 = {
        {"person", {
            {"firstname", "Jane"}
        }}
    };
    
    json result2 = ast->evaluate(context2);
    BOOST_CHECK_EQUAL(result2.get<std::string>(), "Jane");
}

BOOST_AUTO_TEST_CASE(test_parser_invalid_property_access)
{
    orion::bre::feel::Lexer lexer;
    auto tokens = lexer.tokenize("obj.");
    
    orion::bre::feel::Parser parser;
    BOOST_CHECK_THROW(parser.parse(tokens), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(test_parser_property_access_with_number)
{
    orion::bre::feel::Lexer lexer;
    auto tokens = lexer.tokenize("obj.123");
    
    orion::bre::feel::Parser parser;
    // Should fail because property name must be identifier
    BOOST_CHECK_THROW(parser.parse(tokens), std::runtime_error);
}

BOOST_AUTO_TEST_SUITE_END()
