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
        
        json result = engine.evaluate(input);
        BOOST_TEST_MESSAGE("Result: " << result.dump());
        BOOST_TEST_MESSAGE("Parsed result: " << result.dump(2));
        
        // The result should contain the decision
        BOOST_CHECK(result.contains("Decision"));
        
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
        
        json result = engine.evaluate(input);
        BOOST_TEST_MESSAGE("Result: " << result.dump());
        BOOST_TEST_MESSAGE("Parsed result: " << result.dump(2));
        
        // The result should contain the decision
        BOOST_CHECK(result.contains("Decision"));
        
    } catch (const std::exception& ex) {
        BOOST_TEST_MESSAGE("❌ Exception caught: " << ex.what());
        BOOST_FAIL("Should not throw exception for OUTPUT_ORDER hit policy");
    }
}

BOOST_AUTO_TEST_CASE(test_collect_multi_output_no_match_and_spaced_not) {
    const std::string dmn_xml = R"(<?xml version="1.0" encoding="UTF-8"?>
<definitions xmlns="https://www.omg.org/spec/DMN/20240513/MODEL/" namespace="test" id="collect_no_match">
  <inputData id="i_country" name="country"><variable name="country" typeRef="string"/></inputData>
  <decision id="d_taxes" name="Taxes">
    <variable name="Taxes"/>
    <informationRequirement><requiredInput href="#i_country"/></informationRequirement>
    <decisionTable id="dt" hitPolicy="COLLECT">
      <input id="in1"><inputExpression typeRef="string"><text>country</text></inputExpression></input>
      <output id="o1" name="code" typeRef="string"/>
      <output id="o2" name="rate" typeRef="number"/>
      <rule id="r1"><inputEntry><text>"CR"</text></inputEntry><outputEntry><text>"N8"</text></outputEntry><outputEntry><text>1</text></outputEntry></rule>
      <rule id="r2"><inputEntry><text>not ("FR", "DE")</text></inputEntry><outputEntry><text>"OU"</text></outputEntry><outputEntry><text>5</text></outputEntry></rule>
    </decisionTable>
  </decision>
  <decision id="d_count" name="TaxCount">
    <variable name="TaxCount" typeRef="number"/>
    <informationRequirement><requiredDecision href="#d_taxes"/></informationRequirement>
    <literalExpression><text>count(Taxes)</text></literalExpression>
  </decision>
</definitions>)";

    BusinessRulesEngine engine;
    const auto load_result = engine.load_dmn_model(dmn_xml);
    BOOST_TEST_REQUIRE(load_result.has_value());

    const json both = engine.evaluate({{"country", "CR"}});
    BOOST_TEST_REQUIRE(both.at("Taxes").is_array());
    BOOST_TEST_REQUIRE(both.at("Taxes").size() == 2U);
    BOOST_TEST(both.at("Taxes")[0].at("code") == "N8");
    BOOST_TEST(both.at("Taxes")[1].at("code") == "OU");
    BOOST_TEST(both.at("TaxCount") == 2);

    const json negated = engine.evaluate({{"country", "NL"}});
    BOOST_TEST_REQUIRE(negated.at("Taxes").size() == 1U);
    BOOST_TEST(negated.at("Taxes")[0].at("code") == "OU");

    for (const auto* country : {"FR", "DE"}) {
        const json none = engine.evaluate({{"country", country}});
        BOOST_TEST(none.at("Taxes") == json::array(), country);
        BOOST_TEST(none.at("TaxCount") == 0, country);
    }
}

BOOST_AUTO_TEST_CASE(test_rule_order_no_match_returns_empty_list) {
    const std::string dmn_xml = R"(<?xml version="1.0" encoding="UTF-8"?>
<definitions xmlns="https://www.omg.org/spec/DMN/20240513/MODEL/" namespace="test" id="rule_order_no_match">
  <decision id="d" name="Decision">
    <decisionTable id="dt" hitPolicy="RULE ORDER">
      <input id="in1"><inputExpression typeRef="number"><text>Age</text></inputExpression></input>
      <output id="o1" name="result" typeRef="string"/>
      <output id="o2" name="rank" typeRef="number"/>
      <rule id="r1"><inputEntry><text>&lt; 18</text></inputEntry><outputEntry><text>"MINOR"</text></outputEntry><outputEntry><text>1</text></outputEntry></rule>
    </decisionTable>
  </decision>
</definitions>)";

    BusinessRulesEngine engine;
    BOOST_TEST_REQUIRE(engine.load_dmn_model(dmn_xml).has_value());
    BOOST_TEST(engine.evaluate({{"Age", 30}}).at("Decision") == json::array());
}

BOOST_AUTO_TEST_CASE(test_first_no_match_remains_empty_context) {
    const std::string dmn_xml = R"(<?xml version="1.0" encoding="UTF-8"?>
<definitions xmlns="https://www.omg.org/spec/DMN/20240513/MODEL/" namespace="test" id="first_no_match">
  <decision id="d" name="Decision">
    <decisionTable id="dt" hitPolicy="FIRST">
      <input id="in1"><inputExpression typeRef="number"><text>Age</text></inputExpression></input>
      <output id="o1" name="result" typeRef="string"/>
      <output id="o2" name="rank" typeRef="number"/>
      <rule id="r1"><inputEntry><text>&lt; 18</text></inputEntry><outputEntry><text>"MINOR"</text></outputEntry><outputEntry><text>1</text></outputEntry></rule>
    </decisionTable>
  </decision>
</definitions>)";

    BusinessRulesEngine engine;
    BOOST_TEST_REQUIRE(engine.load_dmn_model(dmn_xml).has_value());
    BOOST_TEST(engine.evaluate({{"Age", 30}}).at("Decision") == json::object());
}

BOOST_AUTO_TEST_SUITE_END()