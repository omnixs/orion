/*
 * ORION Optimized Rule Integration & Operations Native
 * SPDX-License-Identifier: Apache-2.0
 * SPDX-FileCopyrightText: 2025 ORION contributors
 */

#include <boost/test/unit_test.hpp>
#include <orion/api/engine.hpp>
#include <nlohmann/json.hpp>
#include <string>

BOOST_AUTO_TEST_SUITE(test_dmn_abs_named_params, *boost::unit_test::disabled())

using json = nlohmann::json;

// Test the exact DMN structure from TCK test 0050-feel-abs-function
BOOST_AUTO_TEST_CASE(test_abs_positional_basic) {
    std::string dmn_xml = R"(<?xml version="1.0" encoding="UTF-8"?>
<definitions xmlns="https://www.omg.org/spec/DMN/20230324/MODEL/" id="test">
  <decision name="decision001" id="_decision001">
    <variable name="decision001"/>
    <literalExpression>
      <text>abs(1)</text>
    </literalExpression>
  </decision>
</definitions>)";

    std::string input_json = "{}";
    
    orion::api::BusinessRulesEngine engine;
    auto load_result = engine.load_dmn_model(dmn_xml);
    if (!load_result) {
        BOOST_FAIL("Failed to load DMN model: " + load_result.error());
    }
    std::string result = engine.evaluate(input_json);
    auto result_json = json::parse(result);
    
    BOOST_TEST_MESSAGE("Result JSON: " << result);
    BOOST_CHECK(result_json.contains("decision001"));
    if (result_json.contains("decision001")) {
        BOOST_CHECK_EQUAL(result_json["decision001"].get<double>(), 1.0);
    }
}

BOOST_AUTO_TEST_CASE(test_abs_positional_negative) {
    std::string dmn_xml = R"(<?xml version="1.0" encoding="UTF-8"?>
<definitions xmlns="https://www.omg.org/spec/DMN/20230324/MODEL/" id="test">
  <decision name="decision002" id="_decision002">
    <variable name="decision002"/>
    <literalExpression>
      <text>abs(-1)</text>
    </literalExpression>
  </decision>
</definitions>)";

    std::string input_json = "{}";
    
    orion::api::BusinessRulesEngine engine;
    auto load_result = engine.load_dmn_model(dmn_xml);
    if (!load_result) {
        BOOST_FAIL("Failed to load DMN model: " + load_result.error());
    }
    std::string result = engine.evaluate(input_json);
    auto result_json = json::parse(result);
    
    BOOST_TEST_MESSAGE("Result JSON: " << result);
    BOOST_CHECK(result_json.contains("decision002"));
    if (result_json.contains("decision002")) {
        BOOST_CHECK_EQUAL(result_json["decision002"].get<double>(), 1.0);
    }
}

BOOST_AUTO_TEST_CASE(test_abs_named_param_correct) {
    // TCK test decision006: abs(n:-1) should return 1
    std::string dmn_xml = R"(<?xml version="1.0" encoding="UTF-8"?>
<definitions xmlns="https://www.omg.org/spec/DMN/20230324/MODEL/" id="test">
  <decision name="decision006" id="_decision006">
    <variable name="decision006"/>
    <literalExpression>
      <text>abs(n:-1)</text>
    </literalExpression>
  </decision>
</definitions>)";

    std::string input_json = "{}";
    
    orion::api::BusinessRulesEngine engine;
    auto load_result = engine.load_dmn_model(dmn_xml);
    if (!load_result) {
        BOOST_FAIL("Failed to load DMN model: " + load_result.error());
    }
    std::string result = engine.evaluate(input_json);
    auto result_json = json::parse(result);
    
    BOOST_TEST_MESSAGE("Result JSON: " << result);
    BOOST_CHECK(result_json.contains("decision006"));
    if (result_json.contains("decision006")) {
        // Should evaluate to 1 (abs of -1)
        if (result_json["decision006"].is_null()) {
            BOOST_TEST_MESSAGE("ERROR: Got null instead of 1");
            BOOST_CHECK(false);
        } else {
            BOOST_CHECK_EQUAL(result_json["decision006"].get<double>(), 1.0);
        }
    } else {
        BOOST_TEST_MESSAGE("ERROR: decision006 not found in result");
    }
}

BOOST_AUTO_TEST_CASE(test_abs_named_param_wrong_name) {
    // TCK test decision007: abs(number:-1) should return null (wrong param name)
    std::string dmn_xml = R"(<?xml version="1.0" encoding="UTF-8"?>
<definitions xmlns="https://www.omg.org/spec/DMN/20230324/MODEL/" id="test">
  <decision name="decision007" id="_decision007">
    <variable name="decision007"/>
    <literalExpression>
      <text>abs(number:-1)</text>
    </literalExpression>
  </decision>
</definitions>)";

    std::string input_json = "{}";
    
    orion::api::BusinessRulesEngine engine;
    auto load_result = engine.load_dmn_model(dmn_xml);
    if (!load_result) {
        BOOST_FAIL("Failed to load DMN model: " + load_result.error());
    }
    std::string result = engine.evaluate(input_json);
    auto result_json = json::parse(result);
    
    BOOST_TEST_MESSAGE("Result JSON: " << result);
    BOOST_CHECK(result_json.contains("decision007"));
    if (result_json.contains("decision007")) {
        // Should be null because 'number' is not a valid parameter name for abs()
        BOOST_CHECK(result_json["decision007"].is_null());
    }
}

BOOST_AUTO_TEST_CASE(test_abs_no_params) {
    // TCK test decision004: abs() should return null
    std::string dmn_xml = R"(<?xml version="1.0" encoding="UTF-8"?>
<definitions xmlns="https://www.omg.org/spec/DMN/20230324/MODEL/" id="test">
  <decision name="decision004" id="_decision004">
    <variable name="decision004"/>
    <literalExpression>
      <text>abs()</text>
    </literalExpression>
  </decision>
</definitions>)";

    std::string input_json = "{}";
    
    orion::api::BusinessRulesEngine engine;
    auto load_result = engine.load_dmn_model(dmn_xml);
    if (!load_result) {
        BOOST_FAIL("Failed to load DMN model: " + load_result.error());
    }
    std::string result = engine.evaluate(input_json);
    auto result_json = json::parse(result);
    
    BOOST_TEST_MESSAGE("Result JSON: " << result);
    BOOST_CHECK(result_json.contains("decision004"));
    if (result_json.contains("decision004")) {
        BOOST_CHECK(result_json["decision004"].is_null());
    }
}

BOOST_AUTO_TEST_CASE(test_abs_too_many_params) {
    // TCK test decision005: abs(1,1) should return null
    std::string dmn_xml = R"(<?xml version="1.0" encoding="UTF-8"?>
<definitions xmlns="https://www.omg.org/spec/DMN/20230324/MODEL/" id="test">
  <decision name="decision005" id="_decision005">
    <variable name="decision005"/>
    <literalExpression>
      <text>abs(1,1)</text>
    </literalExpression>
  </decision>
</definitions>)";

    std::string input_json = "{}";
    
    orion::api::BusinessRulesEngine engine;
    auto load_result = engine.load_dmn_model(dmn_xml);
    if (!load_result) {
        BOOST_FAIL("Failed to load DMN model: " + load_result.error());
    }
    std::string result = engine.evaluate(input_json);
    auto result_json = json::parse(result);
    
    BOOST_TEST_MESSAGE("Result JSON: " << result);
    BOOST_CHECK(result_json.contains("decision005"));
    if (result_json.contains("decision005")) {
        BOOST_CHECK(result_json["decision005"].is_null());
    }
}

BOOST_AUTO_TEST_SUITE_END()
