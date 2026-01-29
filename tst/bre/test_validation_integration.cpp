/*
 * ORION Optimized Rule Integration & Operations Native
 * SPDX-License-Identifier: Apache-2.0
 * SPDX-FileCopyrightText: 2025 ORION contributors
 */

#include <orion/api/engine.hpp>
#include <orion/bre/dmn_parser.hpp>
#include <boost/test/unit_test.hpp>

using namespace orion::bre;

BOOST_AUTO_TEST_SUITE(validation_integration_tests)

// Test 1: Validation enabled by default
BOOST_AUTO_TEST_CASE(validation_enabled_by_default)
{
    orion::api::BusinessRulesEngine engine;
    BOOST_CHECK_EQUAL(engine.is_validation_enabled(), true);
}

// Test 2: Enable validation
BOOST_AUTO_TEST_CASE(enable_validation)
{
    orion::api::BusinessRulesEngine engine;
    engine.set_validation_enabled(true);
    BOOST_CHECK_EQUAL(engine.is_validation_enabled(), true);
}

// Test 3: Validation passes with valid input
BOOST_AUTO_TEST_CASE(validation_passes_with_valid_input)
{
    std::string_view dmn_xml = R"(<?xml version="1.0"?>
<dmn:definitions xmlns:dmn="http://www.omg.org/spec/DMN/20180521/MODEL/">
  <dmn:itemDefinition name="tStatus">
    <dmn:typeRef>string</dmn:typeRef>
    <dmn:allowedValues>
      <dmn:text>"Active", "Disabled"</dmn:text>
    </dmn:allowedValues>
  </dmn:itemDefinition>
  <dmn:decision name="StatusDecision">
    <dmn:decisionTable>
      <dmn:input>
        <dmn:inputExpression>
          <dmn:text>tStatus</dmn:text>
        </dmn:inputExpression>
      </dmn:input>
      <dmn:output name="result"/>
      <dmn:rule>
        <dmn:inputEntry><dmn:text>"Active"</dmn:text></dmn:inputEntry>
        <dmn:outputEntry><dmn:text>"OK"</dmn:text></dmn:outputEntry>
      </dmn:rule>
    </dmn:decisionTable>
  </dmn:decision>
</dmn:definitions>)";
    
    orion::api::BusinessRulesEngine engine;
    auto load_result = engine.load_dmn_model(dmn_xml);
    BOOST_REQUIRE(load_result.has_value());
    
    engine.set_validation_enabled(true);
    
    nlohmann::json input = {{"tStatus", "Active"}};
    
    // Should not throw - valid input
    std::string result;
    BOOST_CHECK_NO_THROW(result = engine.evaluate(input.dump()));
    BOOST_CHECK(!result.empty());
}

// Test 4: Validation fails with invalid input
BOOST_AUTO_TEST_CASE(validation_fails_with_invalid_input)
{
    std::string_view dmn_xml = R"(<?xml version="1.0"?>
<dmn:definitions xmlns:dmn="http://www.omg.org/spec/DMN/20180521/MODEL/">
  <dmn:itemDefinition name="tStatus">
    <dmn:typeRef>string</dmn:typeRef>
    <dmn:allowedValues>
      <dmn:text>"Active", "Disabled"</dmn:text>
    </dmn:allowedValues>
  </dmn:itemDefinition>
  <dmn:decision name="StatusDecision">
    <dmn:decisionTable>
      <dmn:input>
        <dmn:inputExpression>
          <dmn:text>tStatus</dmn:text>
        </dmn:inputExpression>
      </dmn:input>
      <dmn:output name="result"/>
      <dmn:rule>
        <dmn:inputEntry><dmn:text>"Active"</dmn:text></dmn:inputEntry>
        <dmn:outputEntry><dmn:text>"OK"</dmn:text></dmn:outputEntry>
      </dmn:rule>
    </dmn:decisionTable>
  </dmn:decision>
</dmn:definitions>)";
    
    orion::api::BusinessRulesEngine engine;
    auto load_result = engine.load_dmn_model(dmn_xml);
    BOOST_REQUIRE(load_result.has_value());
    
    engine.set_validation_enabled(true);
    
    nlohmann::json input = {{"tStatus", "InvalidValue"}};
    
    // Should throw - invalid input
    BOOST_CHECK_EXCEPTION(
        engine.evaluate(input.dump()),
        std::runtime_error,
        [](const std::runtime_error& e) {
            std::string msg(e.what());
            return msg.find("Input validation failed") != std::string::npos;
        }
    );
}

// Test 5: Validation skipped when explicitly disabled
BOOST_AUTO_TEST_CASE(validation_skipped_when_disabled)
{
    std::string_view dmn_xml = R"(<?xml version="1.0"?>
<dmn:definitions xmlns:dmn="http://www.omg.org/spec/DMN/20180521/MODEL/">
  <dmn:itemDefinition name="tStatus">
    <dmn:typeRef>string</dmn:typeRef>
    <dmn:allowedValues>
      <dmn:text>"Active", "Disabled"</dmn:text>
    </dmn:allowedValues>
  </dmn:itemDefinition>
  <dmn:decision name="StatusDecision">
    <dmn:decisionTable>
      <dmn:input>
        <dmn:inputExpression>
          <dmn:text>tStatus</dmn:text>
        </dmn:inputExpression>
      </dmn:input>
      <dmn:output name="result"/>
      <dmn:rule>
        <dmn:inputEntry><dmn:text>"Active"</dmn:text></dmn:inputEntry>
        <dmn:outputEntry><dmn:text>"OK"</dmn:text></dmn:outputEntry>
      </dmn:rule>
    </dmn:decisionTable>
  </dmn:decision>
</dmn:definitions>)";
    
    orion::api::BusinessRulesEngine engine;
    auto load_result = engine.load_dmn_model(dmn_xml);
    BOOST_REQUIRE(load_result.has_value());
    
    // Explicitly disable validation
    engine.set_validation_enabled(false);
    BOOST_CHECK_EQUAL(engine.is_validation_enabled(), false);
    
    nlohmann::json input = {{"tStatus", "InvalidValue"}};
    
    // Should NOT throw - validation explicitly disabled
    std::string result;
    BOOST_CHECK_NO_THROW(result = engine.evaluate(input.dump()));
    (void)result; // Suppress unused variable warning
}

// Test 6: Validation with complex types
BOOST_AUTO_TEST_CASE(validation_with_complex_types)
{
    std::string_view dmn_xml = R"(<?xml version="1.0"?>
<dmn:definitions xmlns:dmn="http://www.omg.org/spec/DMN/20180521/MODEL/">
  <dmn:itemDefinition name="tPerson">
    <dmn:itemComponent name="name">
      <dmn:typeRef>string</dmn:typeRef>
    </dmn:itemComponent>
    <dmn:itemComponent name="age">
      <dmn:typeRef>number</dmn:typeRef>
    </dmn:itemComponent>
  </dmn:itemDefinition>
  <dmn:decision name="PersonDecision">
    <dmn:decisionTable>
      <dmn:input>
        <dmn:inputExpression>
          <dmn:text>tPerson.name</dmn:text>
        </dmn:inputExpression>
      </dmn:input>
      <dmn:output name="result"/>
      <dmn:rule>
        <dmn:inputEntry><dmn:text>"John"</dmn:text></dmn:inputEntry>
        <dmn:outputEntry><dmn:text>"OK"</dmn:text></dmn:outputEntry>
      </dmn:rule>
    </dmn:decisionTable>
  </dmn:decision>
</dmn:definitions>)";
    
    orion::api::BusinessRulesEngine engine;
    auto load_result = engine.load_dmn_model(dmn_xml);
    BOOST_REQUIRE(load_result.has_value());
    
    engine.set_validation_enabled(true);
    
    nlohmann::json valid_input = {
        {"tPerson", {{"name", "John"}, {"age", 30}}}
    };
    
    // Should not throw - valid complex type
    BOOST_CHECK_NO_THROW(engine.evaluate(valid_input.dump()));
    
    nlohmann::json partial_input = {
        {"tPerson", {{"name", "John"}}} // Missing 'age' field - should be OK (fields are optional)
    };
    
    // Should NOT throw - missing fields are optional in DMN
    BOOST_CHECK_NO_THROW(engine.evaluate(partial_input.dump()));
}

BOOST_AUTO_TEST_SUITE_END()
