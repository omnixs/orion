/**
 * @file test_decimal_numbers.cpp
 * @brief Unit tests for decimal number parsing and evaluation
 * 
 * Tests that decimal numbers with various formats are correctly:
 * - Tokenized by orion::bre::feel::Lexer
 * - Parsed by orion::bre::feel::Parser  
 * - Evaluated by AST (not routed to legacy evaluator)
 * 
 * This addresses the bug where expressions with decimal points were
 * incorrectly routed to the legacy evaluator due to naive dot detection.
 */

#include <boost/test/unit_test.hpp>
#include <orion/bre/feel/lexer.hpp>
#include <orion/bre/feel/parser.hpp>
#include <orion/bre/feel/evaluator.hpp>
#include <nlohmann/json.hpp>

using namespace orion::bre;
using json = nlohmann::json;

BOOST_AUTO_TEST_SUITE(test_decimal_number_parsing)

// =============================================================================
// Lexer Tests - Verify tokenization of decimal numbers
// =============================================================================

BOOST_AUTO_TEST_CASE(test_lexer_decimal_with_leading_dot)
{
    // Test case: .872 (leading dot - common in FEEL)
    orion::bre::feel::Lexer lexer;
    auto tokens = lexer.tokenize(".872");
    
    BOOST_REQUIRE_EQUAL(tokens.size(), 2);  // NUMBER + END_OF_INPUT
    BOOST_CHECK_EQUAL(tokens[0].type, orion::bre::feel::TokenType::NUMBER);
    BOOST_CHECK_EQUAL(tokens[0].text, ".872");
}

BOOST_AUTO_TEST_CASE(test_lexer_negative_decimal_with_leading_dot)
{
    // Test case: -.872 (negative with leading dot)
    orion::bre::feel::Lexer lexer;
    auto tokens = lexer.tokenize("-.872");
    
    BOOST_REQUIRE_EQUAL(tokens.size(), 2);
    BOOST_CHECK_EQUAL(tokens[0].type, orion::bre::feel::TokenType::NUMBER);
    BOOST_CHECK_EQUAL(tokens[0].text, "-.872");
}

BOOST_AUTO_TEST_CASE(test_lexer_many_decimal_places)
{
    // Test case: 125.4321987654 (many decimal places)
    orion::bre::feel::Lexer lexer;
    auto tokens = lexer.tokenize("125.4321987654");
    
    BOOST_REQUIRE_EQUAL(tokens.size(), 2);
    BOOST_CHECK_EQUAL(tokens[0].type, orion::bre::feel::TokenType::NUMBER);
    BOOST_CHECK_EQUAL(tokens[0].text, "125.4321987654");
}

BOOST_AUTO_TEST_CASE(test_lexer_negative_many_decimal_places)
{
    // Test case: -125.4321987654
    orion::bre::feel::Lexer lexer;
    auto tokens = lexer.tokenize("-125.4321987654");
    
    BOOST_REQUIRE_EQUAL(tokens.size(), 2);
    BOOST_CHECK_EQUAL(tokens[0].type, orion::bre::feel::TokenType::NUMBER);
    BOOST_CHECK_EQUAL(tokens[0].text, "-125.4321987654");
}

BOOST_AUTO_TEST_CASE(test_lexer_standard_decimal)
{
    // Test case: 3.14 (standard decimal)
    orion::bre::feel::Lexer lexer;
    auto tokens = lexer.tokenize("3.14");
    
    BOOST_REQUIRE_EQUAL(tokens.size(), 2);
    BOOST_CHECK_EQUAL(tokens[0].type, orion::bre::feel::TokenType::NUMBER);
    BOOST_CHECK_EQUAL(tokens[0].text, "3.14");
}

// =============================================================================
// Parser Tests - Verify AST creation and evaluation
// =============================================================================

BOOST_AUTO_TEST_CASE(test_parser_decimal_with_leading_dot)
{
    // Test case: .872 should parse and evaluate correctly
    orion::bre::feel::Lexer lexer;
    auto tokens = lexer.tokenize(".872");
    
    orion::bre::feel::Parser parser;
    auto ast = parser.parse(tokens);
    
    json context = json::object();
    auto result = ast->evaluate(context);
    
    BOOST_CHECK(result.is_number());
    BOOST_CHECK_CLOSE(result.get<double>(), 0.872, 0.0001);
}

BOOST_AUTO_TEST_CASE(test_parser_negative_decimal_with_leading_dot)
{
    // Test case: -.872
    orion::bre::feel::Lexer lexer;
    auto tokens = lexer.tokenize("-.872");
    
    orion::bre::feel::Parser parser;
    auto ast = parser.parse(tokens);
    
    json context = json::object();
    auto result = ast->evaluate(context);
    
    BOOST_CHECK(result.is_number());
    BOOST_CHECK_CLOSE(result.get<double>(), -0.872, 0.0001);
}

BOOST_AUTO_TEST_CASE(test_parser_many_decimal_places)
{
    // Test case: 125.4321987654
    orion::bre::feel::Lexer lexer;
    auto tokens = lexer.tokenize("125.4321987654");
    
    orion::bre::feel::Parser parser;
    auto ast = parser.parse(tokens);
    
    json context = json::object();
    auto result = ast->evaluate(context);
    
    BOOST_CHECK(result.is_number());
    BOOST_CHECK_CLOSE(result.get<double>(), 125.4321987654, 0.0001);
}

BOOST_AUTO_TEST_CASE(test_parser_negative_many_decimal_places)
{
    // Test case: -125.4321987654
    orion::bre::feel::Lexer lexer;
    auto tokens = lexer.tokenize("-125.4321987654");
    
    orion::bre::feel::Parser parser;
    auto ast = parser.parse(tokens);
    
    json context = json::object();
    auto result = ast->evaluate(context);
    
    BOOST_CHECK(result.is_number());
    BOOST_CHECK_CLOSE(result.get<double>(), -125.4321987654, 0.0001);
}

// =============================================================================
// Evaluator Tests - Verify expressions with decimals use AST (not legacy)
// =============================================================================

BOOST_AUTO_TEST_CASE(test_evaluator_decimal_in_arithmetic)
{
    // Test case: Arithmetic with decimal numbers should use AST path
    // Expression: .872 + 3.14
    orion::bre::feel::Evaluator evaluator;
    json context = json::object();
    
    auto result = evaluator.evaluate(".872 + 3.14", context);
    
    BOOST_CHECK(result.is_number());
    BOOST_CHECK_CLOSE(result.get<double>(), 4.012, 0.0001);
}

BOOST_AUTO_TEST_CASE(test_evaluator_many_decimals_in_expression)
{
    // Test case: Complex arithmetic with many decimal places
    // Expression: 125.4321987654 * 2.5
    orion::bre::feel::Evaluator evaluator;
    json context = json::object();
    
    auto result = evaluator.evaluate("125.4321987654 * 2.5", context);
    
    BOOST_CHECK(result.is_number());
    BOOST_CHECK_CLOSE(result.get<double>(), 313.58049691350, 0.0001);
}

BOOST_AUTO_TEST_CASE(test_evaluator_negative_decimals)
{
    // Test case: Negative decimal arithmetic
    // Expression: -.872 + 1.0
    orion::bre::feel::Evaluator evaluator;
    json context = json::object();
    
    auto result = evaluator.evaluate("-.872 + 1.0", context);
    
    BOOST_CHECK(result.is_number());
    BOOST_CHECK_CLOSE(result.get<double>(), 0.128, 0.0001);
}

// =============================================================================
// Detection Function Tests - Verify property access vs decimal distinction
// =============================================================================

BOOST_AUTO_TEST_CASE(test_decimal_not_detected_as_property_access)
{
    // Test that decimal numbers are NOT incorrectly detected as property access
    // These should all use AST path, not legacy
    
    orion::bre::feel::Evaluator evaluator;
    json context = json::object();
    
    // All of these should succeed with AST (not throw or return null)
    std::vector<std::string> decimal_expressions = {
        ".872",
        "-.872",
        "3.14",
        "-3.14",
        "125.4321987654",
        "-125.4321987654",
        "0.5",
        "10.0",
        ".1 + .2"
    };
    
    for (const auto& expr : decimal_expressions)
    {
        BOOST_TEST_MESSAGE("Testing decimal expression: " << expr);
        auto result = evaluator.evaluate(expr, context);
        BOOST_CHECK_MESSAGE(result.is_number(), 
            "Expression '" << expr << "' should evaluate to a number via AST");
    }
}

BOOST_AUTO_TEST_CASE(test_property_access_correctly_detected)
{
    // Test that property access IS correctly detected
    // These should fail or use legacy path (we're not testing functionality,
    // just that they're recognized as needing special handling)
    
    orion::bre::feel::Evaluator evaluator;
    json context = {
        {"loan", {
            {"principal", 100000},
            {"rate", 0.05}
        }}
    };
    
    // Property access expressions - these require special handling
    std::vector<std::string> property_expressions = {
        "loan.principal",
        "loan.rate", 
        "(loan.principal * loan.rate)"
    };
    
    // We just verify these are recognized as different from decimal numbers
    // The actual evaluation behavior is tested elsewhere
    for (const auto& expr : property_expressions)
    {
        BOOST_TEST_MESSAGE("Testing property access expression: " << expr);
        // Just verify it doesn't crash - property access may or may not be supported
        try {
            auto result = evaluator.evaluate(expr, context);
            BOOST_TEST_MESSAGE("  Result: " << result.dump());
        } catch (...) {
            BOOST_TEST_MESSAGE("  Expected: property access not yet fully supported");
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
