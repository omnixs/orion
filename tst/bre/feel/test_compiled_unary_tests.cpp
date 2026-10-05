/*
 * ORION Optimized Rule Integration & Operations Native
 * SPDX-License-Identifier: Apache-2.0
 * SPDX-FileCopyrightText: 2025 ORION contributors
 */

#include <boost/test/unit_test.hpp>
#include <orion/bre/feel/unary.hpp>
#include <orion/bre/feel/evaluator.hpp>
#include <orion/bre/feel/regex_cache.hpp>
#include "test_helpers.hpp"

#include <array>
#include <string>
#include <string_view>

using namespace orion::bre::feel;
using orion::bre::feel::test::get_test_eval_ctx;

BOOST_AUTO_TEST_SUITE(feel_compiled_unary_tests)

BOOST_AUTO_TEST_CASE(compile_rejects_non_list_tests)
{
    BOOST_TEST(!CompiledUnaryTests::compile("\"AAA\"").has_value());
    BOOST_TEST(!CompiledUnaryTests::compile("-").has_value());
    BOOST_TEST(!CompiledUnaryTests::compile("> 5").has_value());
    BOOST_TEST(!CompiledUnaryTests::compile("not(\"AAA\",\"BBB\")").has_value());
    BOOST_TEST(!CompiledUnaryTests::compile("not(1),2").has_value());
    BOOST_TEST(CompiledUnaryTests::compile("\"AAA\",\"BBB\"").has_value());
}

BOOST_AUTO_TEST_CASE(compiled_lists_match_like_unary_test_matches)
{
    constexpr auto tests = std::to_array<std::string_view>({
        "\"AAA\",\"BBB\",\"CCC\"",
        " \"AAA\" , \"BBB\" ",
        "1,2,3.5",
        "1.0, 2",
        "true,false",
        "\"2024-01-15\",\"2024-02-01\"",
        "2024-01-15,2024-02-01",
        "\"10:30:00\",\"11:00\"",
        "\"2024-01-15T10:30:00\",\"2024-01-15T10:30:00Z\"",
        "\"P3D\",\"PT72H\",\"-P4Y\"",
        "<5,>10",
        "[1..3],(7..9]",
        "\"A\",-",
        "\"A\",,\"B\"",
        "2,not(1)",
        "'single',\"double\"",
        "\"a b\",c d",
    });
    constexpr auto candidates = std::to_array<std::string_view>({
        "AAA", "BBB", " AAA ", "XX", "\"AAA\"", "1", "1.0", "2", "3.5", "4", "11", "12", "8", "9",
        "true", "false", "TRUE", "2024-01-15", "2024-02-01", "2024-03-01", "10:30:00", "11:00",
        "11:00:00", "2024-01-15T10:30:00", "2024-01-15T10:30:00Z", "P3D", "PT72H", "-P4Y", "P5D",
        "A", "B", "", "single", "double", "a b", "c d", "null",
    });

    for (const auto test : tests)
    {
        const auto compiled = CompiledUnaryTests::compile(test);
        BOOST_TEST_REQUIRE(compiled.has_value(), "list should compile: " << test);
        for (const auto candidate : candidates)
        {
            const bool expected = unary_test_matches(test, candidate);
            const bool actual = compiled->matches(UnaryCandidate(std::string(candidate)));
            BOOST_TEST(actual == expected, "test '" << test << "' candidate '" << candidate << "'");
        }
    }
}

BOOST_AUTO_TEST_CASE(evaluator_compile_matches_evaluate)
{
    const nlohmann::json input = {{"a", 2}, {"b", 3}, {"name", "Orion"}, {"list", {1, 2, 3}}};
    constexpr auto expressions = std::to_array<std::string_view>({
        "a + b * 2",
        "if a > 1 then \"big\" else \"small\"",
        "upper case(name)",
        "count(list) + sum(list)",
        "{x: a, y: x + b}.y",
        "for i in list return i * a",
    });
    for (const auto expression : expressions)
    {
        const auto ast = Evaluator::compile(expression);
        BOOST_TEST_REQUIRE(ast != nullptr, "should compile: " << expression);
        BOOST_TEST(ast->evaluate(input, get_test_eval_ctx()) == Evaluator::evaluate(expression, input, get_test_eval_ctx()),
                   "expression: " << expression);
    }
}

BOOST_AUTO_TEST_CASE(evaluator_compile_leaves_text_forms_to_evaluate)
{
    BOOST_TEST(Evaluator::compile("sort(list, function(x, y) x < y)") == nullptr);
    BOOST_TEST(Evaluator::compile("list[item > 1]") == nullptr);
    BOOST_TEST(Evaluator::compile("[{a: 1}, {a: 2}].a") == nullptr);
    BOOST_TEST(Evaluator::compile("replace(\"abc\", \"b\", \"x\")") == nullptr);
    BOOST_TEST(Evaluator::compile("function(x) x + 1") == nullptr);
    BOOST_TEST(Evaluator::compile("1 +") == nullptr);
}

BOOST_AUTO_TEST_SUITE_END()
