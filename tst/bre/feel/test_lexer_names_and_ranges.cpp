/*
 * ORION Optimized Rule Integration & Operations Native
 * SPDX-License-Identifier: Apache-2.0
 * SPDX-FileCopyrightText: 2025 ORION contributors
 */

/**
 * @file test_lexer_names_and_ranges.cpp
 * @brief Regression tests for the context-sensitive lexer rules used by FEEL names and ranges
 *
 * Covers:
 * - Hyphenated FEEL names (DMN 1.5 §10.3.1.2) vs. unspaced subtraction
 * - The `[a..b[` / `]a..b]` aliases for exclusive range bounds
 * - Ranges with compound arithmetic endpoints
 */

#include <boost/test/unit_test.hpp>
#include <orion/bre/feel/lexer.hpp>
#include <orion/bre/feel/parser.hpp>
#include <nlohmann/json.hpp>
#include "test_helpers.hpp"

using orion::bre::feel::Lexer;
using orion::bre::feel::Parser;
using orion::bre::feel::TokenType;
using orion::bre::feel::test::get_test_eval_ctx;
using json = nlohmann::json;

namespace {

json parse_and_evaluate(const std::string& expression, const json& input = json::object())
{
    Lexer lexer;
    Parser parser;
    auto ast = parser.parse(lexer.tokenize(expression));
    return ast->evaluate(input, get_test_eval_ctx());
}

} // namespace

BOOST_AUTO_TEST_SUITE(FeelLexerNamesAndRangesSuite)

// =============================================================================
// Hyphenated names vs. subtraction
// =============================================================================

BOOST_AUTO_TEST_CASE(hyphenated_name_is_single_identifier)
{
    Lexer lexer;
    auto tokens = lexer.tokenize("pre-tax-income");

    BOOST_REQUIRE_EQUAL(tokens.size(), 2); // identifier + EOF
    BOOST_CHECK_EQUAL(tokens[0].type, TokenType::IDENTIFIER);
    BOOST_CHECK_EQUAL(tokens[0].text, "pre-tax-income");
}

BOOST_AUTO_TEST_CASE(hyphenated_name_resolves_from_input)
{
    json input = {{"pre-tax-income", 1000}};
    BOOST_CHECK_EQUAL(parse_and_evaluate("pre-tax-income", input).get<double>(), 1000.0);
}

BOOST_AUTO_TEST_CASE(unspaced_subtraction_of_number_is_not_a_name)
{
    // "Age-1" must stay subtraction: a '-' before a digit never continues a name.
    Lexer lexer;
    auto tokens = lexer.tokenize("Age-1");

    BOOST_REQUIRE_EQUAL(tokens.size(), 4); // Age, -, 1, EOF
    BOOST_CHECK_EQUAL(tokens[0].type, TokenType::IDENTIFIER);
    BOOST_CHECK_EQUAL(tokens[0].text, "Age");
    BOOST_CHECK_EQUAL(tokens[1].type, TokenType::OPERATOR);
    BOOST_CHECK_EQUAL(tokens[1].text, "-");
    BOOST_CHECK_EQUAL(tokens[2].type, TokenType::NUMBER);
}

BOOST_AUTO_TEST_CASE(unspaced_subtraction_evaluates)
{
    json input = {{"Age", 40}};
    BOOST_CHECK_EQUAL(parse_and_evaluate("Age-1", input).get<double>(), 39.0);
}

BOOST_AUTO_TEST_CASE(spaced_subtraction_of_identifiers_evaluates)
{
    json input = {{"a", 10}, {"b", 4}};
    BOOST_CHECK_EQUAL(parse_and_evaluate("a - b", input).get<double>(), 6.0);
}

// =============================================================================
// Range bracket aliases
// =============================================================================

BOOST_AUTO_TEST_CASE(range_closing_bracket_alias_is_exclusive)
{
    // [1..10[ is the FEEL alias for [1..10)
    BOOST_CHECK_EQUAL(parse_and_evaluate("9 in [1..10[").get<bool>(), true);
    BOOST_CHECK_EQUAL(parse_and_evaluate("10 in [1..10[").get<bool>(), false);
}

BOOST_AUTO_TEST_CASE(range_opening_bracket_alias_is_exclusive)
{
    // ]1..10] is the FEEL alias for (1..10]
    BOOST_CHECK_EQUAL(parse_and_evaluate("1 in ]1..10]").get<bool>(), false);
    BOOST_CHECK_EQUAL(parse_and_evaluate("2 in ]1..10]").get<bool>(), true);
    BOOST_CHECK_EQUAL(parse_and_evaluate("10 in ]1..10]").get<bool>(), true);
}

BOOST_AUTO_TEST_CASE(filter_bracket_is_not_treated_as_range_alias)
{
    // A '[' that starts a filter must keep its bracket meaning.
    BOOST_CHECK_EQUAL(parse_and_evaluate("[10, 20, 30][2]").get<double>(), 20.0);
}

BOOST_AUTO_TEST_CASE(empty_list_brackets_are_preserved)
{
    BOOST_CHECK_EQUAL(parse_and_evaluate("count([])").get<double>(), 0.0);
}

BOOST_AUTO_TEST_CASE(nested_list_brackets_are_preserved)
{
    auto result = parse_and_evaluate("[[1, 2], [3]]");
    BOOST_REQUIRE(result.is_array());
    BOOST_REQUIRE_EQUAL(result.size(), 2);
    BOOST_CHECK_EQUAL(result[0].size(), 2);
    BOOST_CHECK_EQUAL(result[1].size(), 1);
}

// =============================================================================
// Compound range endpoints
// =============================================================================

BOOST_AUTO_TEST_CASE(range_endpoint_accepts_arithmetic_expression)
{
    json input = {{"n", 9}};
    BOOST_CHECK_EQUAL(parse_and_evaluate("5 in [1..n+1]", input).get<bool>(), true);
    BOOST_CHECK_EQUAL(parse_and_evaluate("10 in [1..n+1]", input).get<bool>(), true);
    BOOST_CHECK_EQUAL(parse_and_evaluate("11 in [1..n+1]", input).get<bool>(), false);
}

BOOST_AUTO_TEST_CASE(for_loop_range_endpoint_accepts_arithmetic_expression)
{
    json input = {{"n", 3}};
    auto result = parse_and_evaluate("for i in 1..n+1 return i", input);

    BOOST_REQUIRE(result.is_array());
    BOOST_REQUIRE_EQUAL(result.size(), 4);
    BOOST_CHECK_EQUAL(result[3].get<double>(), 4.0);
}

BOOST_AUTO_TEST_SUITE_END()
