/*
 * ORION Optimized Rule Integration & Operations Native
 * SPDX-License-Identifier: Apache-2.0
 * SPDX-FileCopyrightText: 2025 ORION contributors
 */

#include <boost/test/unit_test.hpp>
#include <orion/api/engine.hpp>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using namespace orion::api;

BOOST_AUTO_TEST_SUITE(hit_policy_debug_tests)

BOOST_AUTO_TEST_CASE(test_rule_order_hit_policy) {
    BOOST_TEST_MESSAGE("=== DEBUGGING RULE_ORDER HIT POLICY (0109) ===");
    
    // Sample DMN XML with RULE_ORDER hit policy
    std::string dmn_xml = R"(<?xml version="1.0" encoding="UTF-8"?>
<definitions xmlns="https://www.omg.org/spec/DMN/20191111/MODEL/" 
             xmlns:feel="https://www.omg.org/spec/DMN/20191111/FEEL/" 
             id="test-rule-order">
  <decision id="d_Decision" name="Decision">
    <decisionTable id="decisionTable" hitPolicy="R">
      <input id="input1">
        <inputExpression typeRef="number">
          <text>Age</text>
        </inputExpression>
      </input>
      <output id="output1" name="result" typeRef="string"/>
      <rule id="rule1">
        <inputEntry id="inputEntry1">
          <text>&lt; 18</text>
        </inputEntry>
        <outputEntry id="outputEntry1">
          <text>"MINOR"</text>
        </outputEntry>
      </rule>
      <rule id="rule2">
        <inputEntry id="inputEntry2">
          <text>&gt;= 18</text>
        </inputEntry>
        <outputEntry id="outputEntry2">
          <text>"ADULT"</text>
        </outputEntry>
      </rule>
    </decisionTable>
  </decision>
</definitions>)";

    json input = {{"Age", 25}};
    std::string input_json = input.dump();
    
    BOOST_TEST_MESSAGE("Input: " << input_json);
    BOOST_TEST_MESSAGE("Testing RULE_ORDER hit policy (hitPolicy='R')");
    
    try {
        // Use proper BusinessRulesEngine API instead of legacy function
        orion::api::BusinessRulesEngine engine;
        auto load_result = engine.load_dmn_model(dmn_xml);
        if (!load_result) {
            BOOST_FAIL("Failed to load DMN model: " + load_result.error());
        }
        
        std::string result = engine.evaluate(input_json);
        BOOST_TEST_MESSAGE("Result: " << result);
        
        // Parse result to check if it's valid JSON
        json result_json = json::parse(result);
        BOOST_TEST_MESSAGE("Parsed result: " << result_json.dump(2));
        
        // The result should contain the decision
        BOOST_CHECK(result_json.contains("Decision"));
        
    } catch (const std::exception& ex) {
        BOOST_TEST_MESSAGE("❌ Exception caught: " << ex.what());
        BOOST_FAIL("Should not throw exception for RULE_ORDER hit policy");
    }
}

BOOST_AUTO_TEST_CASE(test_output_order_hit_policy) {
    BOOST_TEST_MESSAGE("=== DEBUGGING OUTPUT_ORDER HIT POLICY (0110) ===");
    
    // Sample DMN XML with OUTPUT_ORDER hit policy
    std::string dmn_xml = R"(<?xml version="1.0" encoding="UTF-8"?>
<definitions xmlns="https://www.omg.org/spec/DMN/20191111/MODEL/" 
             xmlns:feel="https://www.omg.org/spec/DMN/20191111/FEEL/" 
             id="test-output-order">
  <decision id="d_Decision" name="Decision">
    <decisionTable id="decisionTable" hitPolicy="O">
      <input id="input1">
        <inputExpression typeRef="number">
          <text>Age</text>
        </inputExpression>
      </input>
      <output id="output1" name="result" typeRef="string"/>
      <rule id="rule1">
        <inputEntry id="inputEntry1">
          <text>&gt;= 18</text>
        </inputEntry>
        <outputEntry id="outputEntry1">
          <text>"ADULT"</text>
        </outputEntry>
      </rule>
      <rule id="rule2">
        <inputEntry id="inputEntry2">
          <text>&gt; 21</text>
        </inputEntry>
        <outputEntry id="outputEntry2">
          <text>"ADULT_DRINKING"</text>
        </outputEntry>
      </rule>
    </decisionTable>
  </decision>
</definitions>)";

    json input = {{"Age", 25}};
    std::string input_json = input.dump();
    
    BOOST_TEST_MESSAGE("Input: " << input_json);
    BOOST_TEST_MESSAGE("Testing OUTPUT_ORDER hit policy (hitPolicy='O')");
    
    try {
        // Use proper BusinessRulesEngine API instead of legacy function
        orion::api::BusinessRulesEngine engine;
        auto load_result = engine.load_dmn_model(dmn_xml);
        if (!load_result) {
            BOOST_FAIL("Failed to load DMN model: " + load_result.error());
        }
        
        std::string result = engine.evaluate(input_json);
        BOOST_TEST_MESSAGE("Result: " << result);
        
        // Parse result to check if it's valid JSON
        json result_json = json::parse(result);
        BOOST_TEST_MESSAGE("Parsed result: " << result_json.dump(2));
        
        // The result should contain the decision
        BOOST_CHECK(result_json.contains("Decision"));
        
    } catch (const std::exception& ex) {
        BOOST_TEST_MESSAGE("❌ Exception caught: " << ex.what());
        BOOST_FAIL("Should not throw exception for OUTPUT_ORDER hit policy");
    }
}

BOOST_AUTO_TEST_SUITE_END()