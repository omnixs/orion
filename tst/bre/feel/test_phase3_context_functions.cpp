/*
 * ORION Optimized Rule Integration & Operations Native
 * SPDX-License-Identifier: Apache-2.0
 * SPDX-FileCopyrightText: 2025 ORION contributors
 *
 * Test suite for Phase 3 FEEL context features (DMN 1.5 §10.3.4.6):
 *   get value, get entries, context, context put, context merge,
 *   plus context literal scoping and uniqueness rules (DMN 1.5 §10.2.1.4).
 */

#include <boost/test/unit_test.hpp>
#include <orion/api/engine.hpp>
#include <orion/bre/feel/evaluator.hpp>
#include <orion/bre/feel/regex_cache.hpp>
#include "test_helpers.hpp"

using namespace orion::bre::feel;
using orion::bre::feel::test::get_test_eval_ctx;
using json = nlohmann::json;

namespace
{
    json eval(const std::string& expression)
    {
        return Evaluator::evaluate(expression, {}, get_test_eval_ctx());
    }
}

BOOST_AUTO_TEST_SUITE(phase3_context_functions)

// ========== get value() ==========

BOOST_AUTO_TEST_CASE(test_get_value_existing_key)
{
    auto result = eval(R"(get value({"a": 1, "b": 2}, "b"))");
    BOOST_REQUIRE(result.is_number());
    BOOST_CHECK_EQUAL(result.get<double>(), 2.0);
}

BOOST_AUTO_TEST_CASE(test_get_value_missing_key)
{
    BOOST_CHECK(eval(R"(get value({"a": 1}, "z"))").is_null());
}

BOOST_AUTO_TEST_CASE(test_get_value_empty_context)
{
    BOOST_CHECK(eval(R"(get value({}, "a"))").is_null());
}

BOOST_AUTO_TEST_CASE(test_get_value_null_context)
{
    BOOST_CHECK(eval(R"(get value(null, "a"))").is_null());
}

BOOST_AUTO_TEST_CASE(test_get_value_null_key)
{
    BOOST_CHECK(eval(R"(get value({"a": 1}, null))").is_null());
}

BOOST_AUTO_TEST_CASE(test_get_value_non_string_key)
{
    BOOST_CHECK(eval(R"(get value({"a": 1}, 1))").is_null());
}

BOOST_AUTO_TEST_CASE(test_get_value_non_context_first_argument)
{
    BOOST_CHECK(eval(R"(get value([1, 2], "a"))").is_null());
}

BOOST_AUTO_TEST_CASE(test_get_value_named_parameters)
{
    auto result = eval(R"(get value(m: {"a": 5}, key: "a"))");
    BOOST_REQUIRE(result.is_number());
    BOOST_CHECK_EQUAL(result.get<double>(), 5.0);
}

BOOST_AUTO_TEST_CASE(test_get_value_returns_nested_context)
{
    auto result = eval(R"(get value({"a": {"b": 1}}, "a"))");
    BOOST_REQUIRE(result.is_object());
    BOOST_CHECK_EQUAL(result["b"].get<double>(), 1.0);
}

// ========== get entries() ==========

BOOST_AUTO_TEST_CASE(test_get_entries_basic)
{
    auto result = eval(R"(get entries({"a": 1, "b": 2}))");
    BOOST_REQUIRE(result.is_array());
    BOOST_REQUIRE_EQUAL(result.size(), 2U);
    for (const auto& entry : result)
    {
        BOOST_REQUIRE(entry.is_object());
        BOOST_CHECK(entry.contains("key"));
        BOOST_CHECK(entry.contains("value"));
    }
}

BOOST_AUTO_TEST_CASE(test_get_entries_single_entry)
{
    auto result = eval(R"(get entries({"a": 1}))");
    BOOST_REQUIRE(result.is_array());
    BOOST_REQUIRE_EQUAL(result.size(), 1U);
    BOOST_CHECK_EQUAL(result[0]["key"].get<std::string>(), "a");
    BOOST_CHECK_EQUAL(result[0]["value"].get<double>(), 1.0);
}

BOOST_AUTO_TEST_CASE(test_get_entries_empty_context)
{
    auto result = eval("get entries({})");
    BOOST_REQUIRE(result.is_array());
    BOOST_CHECK_EQUAL(result.size(), 0U);
}

BOOST_AUTO_TEST_CASE(test_get_entries_null)
{
    BOOST_CHECK(eval("get entries(null)").is_null());
}

BOOST_AUTO_TEST_CASE(test_get_entries_non_context)
{
    BOOST_CHECK(eval("get entries([1, 2])").is_null());
}

BOOST_AUTO_TEST_CASE(test_get_entries_round_trip_with_context)
{
    auto result = eval(R"(context(get entries({"a": 1, "b": 2})))");
    BOOST_REQUIRE(result.is_object());
    BOOST_CHECK_EQUAL(result["a"].get<double>(), 1.0);
    BOOST_CHECK_EQUAL(result["b"].get<double>(), 2.0);
}

// ========== context() ==========

BOOST_AUTO_TEST_CASE(test_context_from_entry_list)
{
    auto result = eval(R"(context([{key: "a", value: 1}, {key: "b", value: 2}]))");
    BOOST_REQUIRE(result.is_object());
    BOOST_CHECK_EQUAL(result["a"].get<double>(), 1.0);
    BOOST_CHECK_EQUAL(result["b"].get<double>(), 2.0);
}

BOOST_AUTO_TEST_CASE(test_context_singleton_coercion)
{
    auto result = eval(R"(context({key: "a", value: 1}))");
    BOOST_REQUIRE(result.is_object());
    BOOST_CHECK_EQUAL(result["a"].get<double>(), 1.0);
}

BOOST_AUTO_TEST_CASE(test_context_empty_list)
{
    auto result = eval("context([])");
    BOOST_REQUIRE(result.is_object());
    BOOST_CHECK_EQUAL(result.size(), 0U);
}

BOOST_AUTO_TEST_CASE(test_context_duplicate_keys_is_null)
{
    BOOST_CHECK(eval(R"(context([{key: "a", value: 1}, {key: "a", value: 2}]))").is_null());
}

BOOST_AUTO_TEST_CASE(test_context_entry_missing_value_is_null)
{
    BOOST_CHECK(eval(R"(context([{key: "a"}]))").is_null());
}

BOOST_AUTO_TEST_CASE(test_context_entry_non_string_key_is_null)
{
    BOOST_CHECK(eval(R"(context([{key: 1, value: 1}]))").is_null());
}

BOOST_AUTO_TEST_CASE(test_context_null_is_null)
{
    BOOST_CHECK(eval("context(null)").is_null());
}

BOOST_AUTO_TEST_CASE(test_context_non_entry_element_is_null)
{
    BOOST_CHECK(eval("context([1, 2])").is_null());
}

// ========== context put() ==========

BOOST_AUTO_TEST_CASE(test_context_put_adds_entry)
{
    auto result = eval(R"(context put({}, "a", 1))");
    BOOST_REQUIRE(result.is_object());
    BOOST_CHECK_EQUAL(result["a"].get<double>(), 1.0);
}

BOOST_AUTO_TEST_CASE(test_context_put_overwrites_entry)
{
    auto result = eval(R"(context put({"a": 1}, "a", 2))");
    BOOST_REQUIRE(result.is_object());
    BOOST_CHECK_EQUAL(result.size(), 1U);
    BOOST_CHECK_EQUAL(result["a"].get<double>(), 2.0);
}

BOOST_AUTO_TEST_CASE(test_context_put_preserves_other_entries)
{
    auto result = eval(R"(context put({"a": 1, "b": 2}, "b", 3))");
    BOOST_REQUIRE(result.is_object());
    BOOST_CHECK_EQUAL(result["a"].get<double>(), 1.0);
    BOOST_CHECK_EQUAL(result["b"].get<double>(), 3.0);
}

BOOST_AUTO_TEST_CASE(test_context_put_empty_string_key)
{
    auto result = eval(R"(context put({}, "", 1))");
    BOOST_REQUIRE(result.is_object());
    BOOST_CHECK_EQUAL(result[""].get<double>(), 1.0);
}

BOOST_AUTO_TEST_CASE(test_context_put_null_value_is_stored)
{
    auto result = eval(R"(context put({}, "a", null))");
    BOOST_REQUIRE(result.is_object());
    BOOST_REQUIRE(result.contains("a"));
    BOOST_CHECK(result["a"].is_null());
}

BOOST_AUTO_TEST_CASE(test_context_put_null_key_is_null)
{
    BOOST_CHECK(eval("context put({}, null, 1)").is_null());
}

BOOST_AUTO_TEST_CASE(test_context_put_null_context_is_null)
{
    BOOST_CHECK(eval(R"(context put(null, "a", 1))").is_null());
}

BOOST_AUTO_TEST_CASE(test_context_put_non_context_is_null)
{
    BOOST_CHECK(eval(R"(context put([], "a", 1))").is_null());
}

BOOST_AUTO_TEST_CASE(test_context_put_non_string_key_is_null)
{
    BOOST_CHECK(eval("context put({}, 1, 1)").is_null());
}

BOOST_AUTO_TEST_CASE(test_context_put_named_parameters)
{
    auto result = eval(R"(context put(context: {}, key: "a", value: 1))");
    BOOST_REQUIRE(result.is_object());
    BOOST_CHECK_EQUAL(result["a"].get<double>(), 1.0);
}

BOOST_AUTO_TEST_CASE(test_context_put_nested_existing_path)
{
    auto result = eval(R"(context put({x: 1, y: {a: 0}}, ["y", "a"], 2))");
    BOOST_REQUIRE(result.is_object());
    BOOST_CHECK_EQUAL(result["x"].get<double>(), 1.0);
    BOOST_CHECK_EQUAL(result["y"]["a"].get<double>(), 2.0);
}

BOOST_AUTO_TEST_CASE(test_context_put_nested_new_leaf_key)
{
    auto result = eval(R"(context put({x: 1, y: {a: 0}}, ["y", "b"], 2))");
    BOOST_REQUIRE(result.is_object());
    BOOST_CHECK_EQUAL(result["y"]["a"].get<double>(), 0.0);
    BOOST_CHECK_EQUAL(result["y"]["b"].get<double>(), 2.0);
}

BOOST_AUTO_TEST_CASE(test_context_put_deep_nested_path)
{
    auto result = eval(R"(context put({x: 1, y: {a: {b: {c: 1}}}}, ["y", "a", "b", "c"], 2))");
    BOOST_REQUIRE(result.is_object());
    BOOST_CHECK_EQUAL(result["y"]["a"]["b"]["c"].get<double>(), 2.0);
}

BOOST_AUTO_TEST_CASE(test_context_put_nested_null_key_is_null)
{
    BOOST_CHECK(eval(R"(context put({x: 1, y: {a: 0}}, ["y", null], 2))").is_null());
}

BOOST_AUTO_TEST_CASE(test_context_put_empty_key_list_is_null)
{
    BOOST_CHECK(eval("context put({x: 1, y: {a: 0}}, [], 2)").is_null());
}

BOOST_AUTO_TEST_CASE(test_context_put_does_not_mutate_source)
{
    auto result = eval(R"({original: {a: 1}, copied: context put(original, "a", 2)})");
    BOOST_REQUIRE(result.is_object());
    BOOST_CHECK_EQUAL(result["original"]["a"].get<double>(), 1.0);
    BOOST_CHECK_EQUAL(result["copied"]["a"].get<double>(), 2.0);
}

// ========== context merge() ==========

BOOST_AUTO_TEST_CASE(test_context_merge_two_contexts)
{
    auto result = eval(R"(context merge([{"a": 1}, {"b": 2}]))");
    BOOST_REQUIRE(result.is_object());
    BOOST_CHECK_EQUAL(result["a"].get<double>(), 1.0);
    BOOST_CHECK_EQUAL(result["b"].get<double>(), 2.0);
}

BOOST_AUTO_TEST_CASE(test_context_merge_later_context_wins)
{
    auto result = eval(R"(context merge([{"a": 1}, {"a": 2}]))");
    BOOST_REQUIRE(result.is_object());
    BOOST_CHECK_EQUAL(result.size(), 1U);
    BOOST_CHECK_EQUAL(result["a"].get<double>(), 2.0);
}

BOOST_AUTO_TEST_CASE(test_context_merge_single_context_list)
{
    auto result = eval(R"(context merge([{"a": 1}]))");
    BOOST_REQUIRE(result.is_object());
    BOOST_CHECK_EQUAL(result["a"].get<double>(), 1.0);
}

BOOST_AUTO_TEST_CASE(test_context_merge_empty_list)
{
    auto result = eval("context merge([])");
    BOOST_REQUIRE(result.is_object());
    BOOST_CHECK_EQUAL(result.size(), 0U);
}

BOOST_AUTO_TEST_CASE(test_context_merge_null_is_null)
{
    BOOST_CHECK(eval("context merge(null)").is_null());
}

BOOST_AUTO_TEST_CASE(test_context_merge_non_context_element_is_null)
{
    BOOST_CHECK(eval(R"(context merge([{"a": 1}, 2]))").is_null());
}

// ========== Context literals: scoping and uniqueness (DMN 1.5 §10.2.1.4) ==========

BOOST_AUTO_TEST_CASE(test_context_literal_entry_sees_previous_entry)
{
    auto result = eval("{a: 1 + 2, b: a + 3}");
    BOOST_REQUIRE(result.is_object());
    BOOST_CHECK_EQUAL(result["a"].get<double>(), 3.0);
    BOOST_CHECK_EQUAL(result["b"].get<double>(), 6.0);
}

BOOST_AUTO_TEST_CASE(test_nested_context_literal_sees_outer_entries)
{
    auto result = eval("{a: 1 + 2, b: 3, c: {d: a + b}}");
    BOOST_REQUIRE(result.is_object());
    BOOST_CHECK_EQUAL(result["c"]["d"].get<double>(), 6.0);
}

BOOST_AUTO_TEST_CASE(test_context_literal_duplicate_keys_is_null)
{
    BOOST_CHECK(eval(R"({foo: "bar", foo: "baz"})").is_null());
}

BOOST_AUTO_TEST_CASE(test_context_literal_key_with_additional_name_symbol)
{
    auto result = eval(R"({foo+bar: "foo"})");
    BOOST_REQUIRE(result.is_object());
    BOOST_REQUIRE(result.contains("foo+bar"));
    BOOST_CHECK_EQUAL(result["foo+bar"].get<std::string>(), "foo");
}

BOOST_AUTO_TEST_CASE(test_context_literal_key_with_spaces_still_works)
{
    auto result = eval(R"({foo bar: "foo"})");
    BOOST_REQUIRE(result.is_object());
    BOOST_REQUIRE(result.contains("foo bar"));
    BOOST_CHECK_EQUAL(result["foo bar"].get<std::string>(), "foo");
}

BOOST_AUTO_TEST_CASE(test_context_literal_quoted_key_is_preserved)
{
    auto result = eval(R"({"foo+bar((!!],foo": "foo"})");
    BOOST_REQUIRE(result.is_object());
    BOOST_CHECK(result.contains("foo+bar((!!],foo"));
}

BOOST_AUTO_TEST_CASE(test_context_literal_property_access)
{
    auto result = eval(R"({"a": 4, result: a + 1}.result)");
    BOOST_REQUIRE(result.is_number());
    BOOST_CHECK_EQUAL(result.get<double>(), 5.0);
}

// ========== Boxed contexts (DMN 1.5 §10.2.1.4) ==========

BOOST_AUTO_TEST_CASE(test_boxed_context_decision_evaluates_to_context)
{
    orion::api::BusinessRulesEngine engine;
    const std::string dmn_xml = R"(
    <definitions xmlns="https://www.omg.org/spec/DMN/20230324/MODEL/" namespace="boxed-context-test">
        <decision name="boxed" id="_boxed">
            <variable name="boxed"/>
            <context>
                <contextEntry>
                    <variable name="a"/>
                    <literalExpression><text>1 + 1</text></literalExpression>
                </contextEntry>
                <contextEntry>
                    <variable name="b"/>
                    <literalExpression><text>a * 3</text></literalExpression>
                </contextEntry>
            </context>
        </decision>
    </definitions>
    )";

    BOOST_REQUIRE(engine.load_dmn_model(dmn_xml).has_value());
    json result = engine.evaluate(json::object());
    BOOST_REQUIRE(result.contains("boxed"));
    BOOST_REQUIRE(result["boxed"].is_object());
    BOOST_CHECK_EQUAL(result["boxed"]["a"].get<double>(), 2.0);
    BOOST_CHECK_EQUAL(result["boxed"]["b"].get<double>(), 6.0);
}

BOOST_AUTO_TEST_CASE(test_boxed_context_with_result_box_returns_result_value)
{
    orion::api::BusinessRulesEngine engine;
    const std::string dmn_xml = R"(
    <definitions xmlns="https://www.omg.org/spec/DMN/20230324/MODEL/" namespace="boxed-context-result-test">
        <decision name="boxed" id="_boxed">
            <variable name="boxed"/>
            <context>
                <contextEntry>
                    <variable name="a"/>
                    <literalExpression><text>4</text></literalExpression>
                </contextEntry>
                <contextEntry>
                    <literalExpression><text>a + 1</text></literalExpression>
                </contextEntry>
            </context>
        </decision>
    </definitions>
    )";

    BOOST_REQUIRE(engine.load_dmn_model(dmn_xml).has_value());
    json result = engine.evaluate(json::object());
    BOOST_REQUIRE(result.contains("boxed"));
    BOOST_CHECK_EQUAL(result["boxed"].get<double>(), 5.0);
}

BOOST_AUTO_TEST_CASE(test_nested_boxed_context_decision)
{
    orion::api::BusinessRulesEngine engine;
    const std::string dmn_xml = R"(
    <definitions xmlns="https://www.omg.org/spec/DMN/20230324/MODEL/" namespace="boxed-context-nested-test">
        <decision name="boxed" id="_boxed">
            <variable name="boxed"/>
            <context>
                <contextEntry>
                    <variable name="outer"/>
                    <context>
                        <contextEntry>
                            <variable name="inner"/>
                            <literalExpression><text>7</text></literalExpression>
                        </contextEntry>
                    </context>
                </contextEntry>
            </context>
        </decision>
    </definitions>
    )";

    BOOST_REQUIRE(engine.load_dmn_model(dmn_xml).has_value());
    json result = engine.evaluate(json::object());
    BOOST_REQUIRE(result.contains("boxed"));
    BOOST_REQUIRE(result["boxed"].is_object());
    BOOST_CHECK_EQUAL(result["boxed"]["outer"]["inner"].get<double>(), 7.0);
}

// A boxed context entry using an unsupported boxed expression kind (anything
// other than literalExpression or a nested context, e.g. a <relation>) makes
// the whole context unconvertible. The decision is then registered with no
// expression and no decision table, so the DRG evaluator falls back to a
// null result for it rather than throwing. A warning is logged (see the
// boxed_context_to_feel call site in dmn_parser.cpp) so this is discoverable
// instead of a silent failure.
BOOST_AUTO_TEST_CASE(test_boxed_context_with_unsupported_entry_is_not_converted)
{
    orion::api::BusinessRulesEngine engine;
    const std::string dmn_xml = R"(
    <definitions xmlns="https://www.omg.org/spec/DMN/20230324/MODEL/" namespace="boxed-context-unsupported-test">
        <decision name="boxed" id="_boxed">
            <variable name="boxed"/>
            <context>
                <contextEntry>
                    <variable name="a"/>
                    <literalExpression><text>1 + 1</text></literalExpression>
                </contextEntry>
                <contextEntry>
                    <variable name="b"/>
                    <relation>
                        <column name="col1"/>
                        <row>
                            <literalExpression><text>1</text></literalExpression>
                        </row>
                    </relation>
                </contextEntry>
            </context>
        </decision>
    </definitions>
    )";

    BOOST_REQUIRE(engine.load_dmn_model(dmn_xml).has_value());
    json result = engine.evaluate(json::object());
    BOOST_REQUIRE(result.contains("boxed"));
    BOOST_CHECK(result["boxed"].is_null());
}

BOOST_AUTO_TEST_SUITE_END()
