/*
 * ORION Optimized Rule Integration & Operations Native
 * SPDX-License-Identifier: Apache-2.0
 * SPDX-FileCopyrightText: 2025 ORION contributors
 */

/**
 * @file test_decision_semantics.cpp
 * @brief Model-level FEEL semantics: variable resolution, three-valued logic, null propagation
 *
 * Replaces the former dat/tst/dmn-tck-extra models 0120-0126, which the TCK runners only reported
 * and never asserted.
 */

#include <boost/test/unit_test.hpp>
#include <orion/api/engine.hpp>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

using json = nlohmann::json;
using orion::api::BusinessRulesEngine;

namespace {
    void load(BusinessRulesEngine& engine, const std::string& dmn_xml)
    {
        const auto result = engine.load_dmn_model(dmn_xml);
        BOOST_TEST_REQUIRE(result.has_value(), (result ? std::string() : result.error()));
    }

    const std::string LOGIC_MODEL = R"(<?xml version="1.0" encoding="UTF-8"?>
<definitions xmlns="https://www.omg.org/spec/DMN/20230324/MODEL/" namespace="test" id="logic" name="logic">
  <inputData id="i_a" name="VarA"><variable name="VarA" typeRef="boolean"/></inputData>
  <inputData id="i_b" name="VarB"><variable name="VarB" typeRef="boolean"/></inputData>
  <decision id="d_and" name="LogicalAnd">
    <variable name="LogicalAnd" typeRef="boolean"/>
    <informationRequirement><requiredInput href="#i_a"/></informationRequirement>
    <informationRequirement><requiredInput href="#i_b"/></informationRequirement>
    <literalExpression><text>VarA and VarB</text></literalExpression>
  </decision>
  <decision id="d_or" name="LogicalOr">
    <variable name="LogicalOr" typeRef="boolean"/>
    <informationRequirement><requiredInput href="#i_a"/></informationRequirement>
    <informationRequirement><requiredInput href="#i_b"/></informationRequirement>
    <literalExpression><text>VarA or VarB</text></literalExpression>
  </decision>
  <decision id="d_not" name="LogicalNot">
    <variable name="LogicalNot" typeRef="boolean"/>
    <informationRequirement><requiredInput href="#i_a"/></informationRequirement>
    <literalExpression><text>not(VarA)</text></literalExpression>
  </decision>
</definitions>)";

    const std::string ARITHMETIC_MODEL = R"(<?xml version="1.0" encoding="UTF-8"?>
<definitions xmlns="https://www.omg.org/spec/DMN/20230324/MODEL/" namespace="test" id="arithmetic" name="arithmetic">
  <inputData id="i_a" name="A"><variable name="A" typeRef="number"/></inputData>
  <inputData id="i_b" name="B"><variable name="B" typeRef="number"/></inputData>
  <decision id="d_add" name="Add">
    <informationRequirement><requiredInput href="#i_a"/></informationRequirement>
    <informationRequirement><requiredInput href="#i_b"/></informationRequirement>
    <literalExpression><text>A + B</text></literalExpression>
  </decision>
  <decision id="d_sub" name="Subtract">
    <informationRequirement><requiredInput href="#i_a"/></informationRequirement>
    <informationRequirement><requiredInput href="#i_b"/></informationRequirement>
    <literalExpression><text>A - B</text></literalExpression>
  </decision>
  <decision id="d_mul" name="Multiply">
    <informationRequirement><requiredInput href="#i_a"/></informationRequirement>
    <informationRequirement><requiredInput href="#i_b"/></informationRequirement>
    <literalExpression><text>A * B</text></literalExpression>
  </decision>
  <decision id="d_div" name="Divide">
    <informationRequirement><requiredInput href="#i_a"/></informationRequirement>
    <informationRequirement><requiredInput href="#i_b"/></informationRequirement>
    <literalExpression><text>A / B</text></literalExpression>
  </decision>
</definitions>)";
}

BOOST_AUTO_TEST_SUITE(decision_semantics)

BOOST_AUTO_TEST_CASE(input_data_names_with_spaces_resolve_in_expressions)
{
    // DMN 1.5 Section 10.3.1.4: names may contain spaces
    BusinessRulesEngine engine;
    load(engine, R"(<?xml version="1.0" encoding="UTF-8"?>
<definitions xmlns="https://www.omg.org/spec/DMN/20230324/MODEL/" namespace="test" id="names" name="names">
  <inputData id="i_spaces" name="Input With Spaces"><variable name="Input With Spaces" typeRef="number"/></inputData>
  <inputData id="i_number" name="Number"><variable name="Number" typeRef="number"/></inputData>
  <decision id="d_result" name="Result">
    <variable name="Result" typeRef="number"/>
    <informationRequirement><requiredInput href="#i_spaces"/></informationRequirement>
    <literalExpression><text>2 * Input With Spaces</text></literalExpression>
  </decision>
  <decision id="d_scaled" name="Scaled">
    <variable name="Scaled" typeRef="number"/>
    <informationRequirement><requiredInput href="#i_number"/></informationRequirement>
    <literalExpression><text>Number * 10</text></literalExpression>
  </decision>
</definitions>)");

    const json result = engine.evaluate({{"Input With Spaces", 21}, {"Number", 5}});
    BOOST_TEST(result.at("Result") == 42);
    BOOST_TEST(result.at("Scaled") == 50);
}

BOOST_AUTO_TEST_CASE(logical_operators_follow_three_valued_logic)
{
    // DMN 1.5 Section 10.3.2.15, Table 50: conjunction and disjunction with null
    struct Case { json a; json b; json conjunction; json disjunction; };
    const std::vector<Case> cases = {
        {true, true, true, true},       {true, false, false, true},     {true, nullptr, nullptr, true},
        {false, true, false, true},     {false, false, false, false},   {false, nullptr, false, nullptr},
        {nullptr, true, nullptr, true}, {nullptr, false, false, nullptr}, {nullptr, nullptr, nullptr, nullptr},
    };

    BusinessRulesEngine engine;
    load(engine, LOGIC_MODEL);
    for (const auto& c : cases) {
        const json result = engine.evaluate({{"VarA", c.a}, {"VarB", c.b}});
        const std::string label = c.a.dump() + ", " + c.b.dump();
        BOOST_TEST(result.at("LogicalAnd") == c.conjunction, label);
        BOOST_TEST(result.at("LogicalOr") == c.disjunction, label);
        BOOST_TEST(result.at("LogicalNot") == (c.a.is_null() ? json() : json(!c.a.get<bool>())), label);
    }
}

BOOST_AUTO_TEST_CASE(arithmetic_with_null_operand_is_null)
{
    BusinessRulesEngine engine;
    load(engine, ARITHMETIC_MODEL);

    const json numbers = engine.evaluate({{"A", 6}, {"B", 3}});
    BOOST_TEST(numbers.at("Add") == 9);
    BOOST_TEST(numbers.at("Subtract") == 3);
    BOOST_TEST(numbers.at("Multiply") == 18);
    BOOST_TEST(numbers.at("Divide") == 2);

    for (const json& input : {json{{"A", 1}, {"B", nullptr}}, json{{"A", nullptr}, {"B", 2}},
                              json{{"A", nullptr}, {"B", nullptr}}}) {
        const json result = engine.evaluate(input);
        for (const auto* decision : {"Add", "Subtract", "Multiply", "Divide"}) {
            BOOST_TEST(result.at(decision).is_null(), decision << " with " << input.dump());
        }
    }
}

BOOST_AUTO_TEST_CASE(arithmetic_does_not_convert_strings_or_booleans)
{
    // DMN 1.5 Section 10.3.2.9.4: no implicit string/boolean to number conversion
    BusinessRulesEngine engine;
    load(engine, ARITHMETIC_MODEL);

    BOOST_TEST(engine.evaluate({{"A", "123.45"}, {"B", 1.55}}).at("Add").is_null());
    BOOST_TEST(engine.evaluate({{"A", true}, {"B", 2}}).at("Add").is_null());
}

BOOST_AUTO_TEST_CASE(null_input_entries_match_null_inputs)
{
    BusinessRulesEngine engine;
    load(engine, R"(<?xml version="1.0" encoding="UTF-8"?>
<definitions xmlns="https://www.omg.org/spec/DMN/20230324/MODEL/" namespace="test" id="null_entries" name="null_entries">
  <decision id="d_presence" name="Presence">
    <decisionTable id="dt" hitPolicy="UNIQUE">
      <input id="in_a"><inputExpression typeRef="boolean"><text>A</text></inputExpression></input>
      <input id="in_b"><inputExpression typeRef="boolean"><text>B</text></inputExpression></input>
      <output id="out" typeRef="string"/>
      <rule id="r1"><inputEntry><text>true</text></inputEntry><inputEntry><text>true</text></inputEntry><outputEntry><text>"both"</text></outputEntry></rule>
      <rule id="r2"><inputEntry><text>true</text></inputEntry><inputEntry><text>null</text></inputEntry><outputEntry><text>"first only"</text></outputEntry></rule>
      <rule id="r3"><inputEntry><text>null</text></inputEntry><inputEntry><text>true</text></inputEntry><outputEntry><text>"second only"</text></outputEntry></rule>
      <rule id="r4"><inputEntry><text>null</text></inputEntry><inputEntry><text>null</text></inputEntry><outputEntry><text>"neither"</text></outputEntry></rule>
    </decisionTable>
  </decision>
</definitions>)");

    BOOST_TEST(engine.evaluate({{"A", true}, {"B", true}}).at("Presence") == "both");
    BOOST_TEST(engine.evaluate({{"A", true}, {"B", nullptr}}).at("Presence") == "first only");
    BOOST_TEST(engine.evaluate({{"A", nullptr}, {"B", true}}).at("Presence") == "second only");
    BOOST_TEST(engine.evaluate({{"A", nullptr}, {"B", nullptr}}).at("Presence") == "neither");
    BOOST_TEST(engine.evaluate({{"A", false}, {"B", true}}).at("Presence").is_null());
}

BOOST_AUTO_TEST_SUITE_END()
