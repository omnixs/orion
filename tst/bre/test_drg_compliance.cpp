/*
 * ORION Optimized Rule Integration & Operations Native
 * SPDX-License-Identifier: Apache-2.0
 * SPDX-FileCopyrightText: 2025 ORION contributors
 */

#include <boost/test/unit_test.hpp>
#include <orion/api/engine.hpp>
#include <orion/bre/contract_violation.hpp>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using namespace orion::api;

BOOST_AUTO_TEST_SUITE(drg_compliance_tests)

/**
 * @brief Test complex multi-level DRG with 4 levels of dependencies
 * 
 * This test verifies complex Decision Requirements Graph evaluation with:
 * - 4 levels of decision dependencies
 * - Multiple decisions depending on same predecessor (fan-out)
 * - Decision depending on multiple predecessors (fan-in)
 * - Proper evaluation order and context propagation
 * 
 * Structure:
 *   Input → Level1A, Level1B
 *   Level1A, Level1B → Level2A, Level2B
 *   Level2A → Level3
 *   Level2B → Level3
 *   Level3 → Final
 */
BOOST_AUTO_TEST_CASE(test_complex_multilevel_drg)
{
    std::string dmn_xml = R"(<?xml version="1.0" encoding="UTF-8"?>
<definitions xmlns="https://www.omg.org/spec/DMN/20230324/MODEL/" 
             id="complex-drg" 
             name="Complex Multi-Level DRG Test" 
             namespace="http://test.orion/complex-drg">

  <inputData id="input1" name="Base Input"/>

  <!-- Level 1: Two decisions from input -->
  <decision id="level1A" name="Level 1A">
    <variable name="Level 1A" typeRef="number"/>
    <informationRequirement>
      <requiredInput href="#input1"/>
    </informationRequirement>
    <literalExpression>
      <text>Base Input * 2</text>
    </literalExpression>
  </decision>

  <decision id="level1B" name="Level 1B">
    <variable name="Level 1B" typeRef="number"/>
    <informationRequirement>
      <requiredInput href="#input1"/>
    </informationRequirement>
    <literalExpression>
      <text>Base Input + 10</text>
    </literalExpression>
  </decision>

  <!-- Level 2: Decisions depending on Level 1 -->
  <decision id="level2A" name="Level 2A">
    <variable name="Level 2A" typeRef="number"/>
    <informationRequirement>
      <requiredDecision href="#level1A"/>
    </informationRequirement>
    <informationRequirement>
      <requiredDecision href="#level1B"/>
    </informationRequirement>
    <literalExpression>
      <text>Level 1A + Level 1B</text>
    </literalExpression>
  </decision>

  <decision id="level2B" name="Level 2B">
    <variable name="Level 2B" typeRef="number"/>
    <informationRequirement>
      <requiredDecision href="#level1A"/>
    </informationRequirement>
    <literalExpression>
      <text>Level 1A * 3</text>
    </literalExpression>
  </decision>

  <!-- Level 3: Decision depending on Level 2 -->
  <decision id="level3" name="Level 3">
    <variable name="Level 3" typeRef="number"/>
    <informationRequirement>
      <requiredDecision href="#level2A"/>
    </informationRequirement>
    <informationRequirement>
      <requiredDecision href="#level2B"/>
    </informationRequirement>
    <literalExpression>
      <text>Level 2A + Level 2B</text>
    </literalExpression>
  </decision>

  <!-- Final: Decision depending on Level 3 -->
  <decision id="final" name="Final Result">
    <variable name="Final Result" typeRef="number"/>
    <informationRequirement>
      <requiredDecision href="#level3"/>
    </informationRequirement>
    <literalExpression>
      <text>Level 3 * 2</text>
    </literalExpression>
  </decision>

</definitions>)";

    BusinessRulesEngine engine;
    auto load_result = engine.load_dmn_model(dmn_xml);
    BOOST_REQUIRE(load_result.has_value());
    
    json input = {{"Base Input", 5}};
    std::string result = engine.evaluate(input.dump());
    json result_json = json::parse(result);
    
    BOOST_TEST_MESSAGE("Result: " << result_json.dump(2));
    
    // Verify all decisions evaluated correctly:
    // Input: 5
    // Level 1A: 5 * 2 = 10
    // Level 1B: 5 + 10 = 15
    // Level 2A: 10 + 15 = 25
    // Level 2B: 10 * 3 = 30
    // Level 3: 25 + 30 = 55
    // Final Result: 55 * 2 = 110
    
    BOOST_REQUIRE(result_json.contains("Level 1A"));
    BOOST_CHECK_EQUAL(result_json["Level 1A"].get<double>(), 10.0);
    
    BOOST_REQUIRE(result_json.contains("Level 1B"));
    BOOST_CHECK_EQUAL(result_json["Level 1B"].get<double>(), 15.0);
    
    BOOST_REQUIRE(result_json.contains("Level 2A"));
    BOOST_CHECK_EQUAL(result_json["Level 2A"].get<double>(), 25.0);
    
    BOOST_REQUIRE(result_json.contains("Level 2B"));
    BOOST_CHECK_EQUAL(result_json["Level 2B"].get<double>(), 30.0);
    
    BOOST_REQUIRE(result_json.contains("Level 3"));
    BOOST_CHECK_EQUAL(result_json["Level 3"].get<double>(), 55.0);
    
    BOOST_REQUIRE(result_json.contains("Final Result"));
    BOOST_CHECK_EQUAL(result_json["Final Result"].get<double>(), 110.0);
}

/**
 * @brief Test simple two-level DRG
 * 
 * Simplest possible DRG test: Decision B depends on Decision A.
 * This is the baseline for DRG functionality.
 */
BOOST_AUTO_TEST_CASE(test_simple_two_level_drg)
{
    // Create a minimal DMN with two decisions where B depends on A
    std::string dmn_xml = R"(<?xml version="1.0" encoding="UTF-8"?>
<definitions xmlns="https://www.omg.org/spec/DMN/20230324/MODEL/" 
             xmlns:di="http://www.omg.org/spec/DMN/20180521/DI/" 
             xmlns:dmndi="https://www.omg.org/spec/DMN/20230324/DMNDI/" 
             xmlns:dc="http://www.omg.org/spec/DMN/20180521/DC/" 
             id="simple-drg" 
             name="Simple DRG Test" 
             namespace="http://test.orion/simple-drg">

  <inputData id="input1" name="Input Value"/>

  <decision id="decisionA" name="Decision A">
    <variable name="Decision A" typeRef="number"/>
    <informationRequirement>
      <requiredInput href="#input1"/>
    </informationRequirement>
    <literalExpression>
      <text>Input Value * 2</text>
    </literalExpression>
  </decision>

  <decision id="decisionB" name="Decision B">
    <variable name="Decision B" typeRef="number"/>
    <informationRequirement>
      <requiredDecision href="#decisionA"/>
    </informationRequirement>
    <literalExpression>
      <text>Decision A + 10</text>
    </literalExpression>
  </decision>

</definitions>)";

    BusinessRulesEngine engine;
    auto load_result = engine.load_dmn_model(dmn_xml);
    
    if (!load_result) {
        BOOST_TEST_MESSAGE("Load error: " << load_result.error());
    }
    BOOST_REQUIRE(load_result.has_value());
    
    json input = {{"Input Value", 5}};
    std::string result = engine.evaluate(input.dump());
    json result_json = json::parse(result);
    
    BOOST_TEST_MESSAGE("Input: " << input.dump(2));
    BOOST_TEST_MESSAGE("Result: " << result_json.dump(2));
    
    // Expected: Decision A = 5 * 2 = 10, Decision B = 10 + 10 = 20
    BOOST_REQUIRE(result_json.contains("Decision A"));
    BOOST_REQUIRE(result_json.contains("Decision B"));
    BOOST_CHECK_EQUAL(result_json["Decision A"].get<double>(), 10.0);
    BOOST_CHECK_EQUAL(result_json["Decision B"].get<double>(), 20.0);
}

/**
 * @brief Test three-level DRG (diamond pattern)
 * 
 * Tests: A, B both depend on Input, C depends on both A and B.
 * This verifies proper handling of multiple dependencies.
 */
BOOST_AUTO_TEST_CASE(test_diamond_drg_pattern)
{
    std::string dmn_xml = R"(<?xml version="1.0" encoding="UTF-8"?>
<definitions xmlns="https://www.omg.org/spec/DMN/20230324/MODEL/" 
             id="diamond-drg" 
             name="Diamond DRG Test" 
             namespace="http://test.orion/diamond-drg">

  <inputData id="input1" name="Base Value"/>

  <decision id="decisionA" name="Path A">
    <variable name="Path A" typeRef="number"/>
    <informationRequirement>
      <requiredInput href="#input1"/>
    </informationRequirement>
    <literalExpression>
      <text>Base Value + 5</text>
    </literalExpression>
  </decision>

  <decision id="decisionB" name="Path B">
    <variable name="Path B" typeRef="number"/>
    <informationRequirement>
      <requiredInput href="#input1"/>
    </informationRequirement>
    <literalExpression>
      <text>Base Value * 2</text>
    </literalExpression>
  </decision>

  <decision id="decisionC" name="Final Result">
    <variable name="Final Result" typeRef="number"/>
    <informationRequirement>
      <requiredDecision href="#decisionA"/>
    </informationRequirement>
    <informationRequirement>
      <requiredDecision href="#decisionB"/>
    </informationRequirement>
    <literalExpression>
      <text>Path A + Path B</text>
    </literalExpression>
  </decision>

</definitions>)";

    BusinessRulesEngine engine;
    auto load_result = engine.load_dmn_model(dmn_xml);
    BOOST_REQUIRE(load_result.has_value());
    
    json input = {{"Base Value", 10}};
    std::string result = engine.evaluate(input.dump());
    json result_json = json::parse(result);
    
    BOOST_TEST_MESSAGE("Result: " << result_json.dump(2));
    
    // Expected: Path A = 10 + 5 = 15, Path B = 10 * 2 = 20, Final Result = 15 + 20 = 35
    BOOST_REQUIRE(result_json.contains("Final Result"));
    BOOST_CHECK_EQUAL(result_json["Final Result"].get<double>(), 35.0);
}

/**
 * @brief Test self-referencing decision (cyclic dependency)
 * 
 * A decision that references itself should be detected as a cycle.
 */
BOOST_AUTO_TEST_CASE(test_self_referencing_decision)
{
    std::string_view dmn_xml = R"(<?xml version="1.0" encoding="UTF-8"?>
<definitions xmlns="https://www.omg.org/spec/DMN/20230324/MODEL/" 
             id="self-ref-test" 
             name="Self-Referencing Decision Test" 
             namespace="http://test.orion/self-ref">
  <decision id="SelfRef" name="Self Reference">
    <variable name="Self Reference"/>
    <informationRequirement>
      <requiredDecision href="#SelfRef"/>
    </informationRequirement>
    <literalExpression>
      <text>Self Reference * 2</text>
    </literalExpression>
  </decision>
</definitions>)";

    BusinessRulesEngine engine;
    
    // Loading a self-referencing decision should throw ContractViolation (cycle detected)
    BOOST_CHECK_THROW(engine.load_dmn_model(dmn_xml), orion::bre::ContractViolation);
}

/**
 * @brief Test multiple disconnected decision trees
 * 
 * DRG with multiple independent decision chains that don't depend on each other.
 * Tree 1: Input1 → DecisionA
 * Tree 2: Input2 → DecisionB
 * Both should evaluate correctly without interference.
 */
BOOST_AUTO_TEST_CASE(test_disconnected_decision_trees)
{
    std::string_view dmn_xml = R"(<?xml version="1.0" encoding="UTF-8"?>
<definitions xmlns="https://www.omg.org/spec/DMN/20230324/MODEL/" 
             id="disconnected-test" 
             name="Disconnected Trees Test" 
             namespace="http://test.orion/disconnected">
  <inputData id="Input1" name="Input1">
    <variable name="Input1"/>
  </inputData>
  
  <inputData id="Input2" name="Input2">
    <variable name="Input2"/>
  </inputData>
  
  <!-- Tree 1: Input1 → DecisionA -->
  <decision id="DecisionA" name="Decision A">
    <variable name="Decision A"/>
    <informationRequirement>
      <requiredInput href="#Input1"/>
    </informationRequirement>
    <literalExpression>
      <text>Input1 * 3</text>
    </literalExpression>
  </decision>
  
  <!-- Tree 2: Input2 → DecisionB (completely independent) -->
  <decision id="DecisionB" name="Decision B">
    <variable name="Decision B"/>
    <informationRequirement>
      <requiredInput href="#Input2"/>
    </informationRequirement>
    <literalExpression>
      <text>Input2 + 100</text>
    </literalExpression>
  </decision>
</definitions>)";

    BusinessRulesEngine engine;
    auto load_result = engine.load_dmn_model(dmn_xml);
    BOOST_REQUIRE(load_result.has_value());
    
    // Evaluate with both inputs
    nlohmann::json input = {
        {"Input1", 5},
        {"Input2", 25}
    };
    
    std::string result = engine.evaluate(input.dump());
    nlohmann::json result_json = nlohmann::json::parse(result);
    
    BOOST_TEST_MESSAGE("Input: " << input.dump(2));
    BOOST_TEST_MESSAGE("Result: " << result_json.dump(2));
    
    // Tree 1: DecisionA = Input1 * 3 = 5 * 3 = 15
    BOOST_REQUIRE(result_json.contains("Decision A"));
    BOOST_CHECK_EQUAL(result_json["Decision A"].get<double>(), 15.0);
    
    // Tree 2: DecisionB = Input2 + 100 = 25 + 100 = 125
    BOOST_REQUIRE(result_json.contains("Decision B"));
    BOOST_CHECK_EQUAL(result_json["Decision B"].get<double>(), 125.0);
    
    // Both trees evaluated independently
    BOOST_CHECK_EQUAL(result_json.size(), 2);
}

BOOST_AUTO_TEST_SUITE_END()
