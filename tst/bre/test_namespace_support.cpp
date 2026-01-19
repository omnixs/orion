#include <boost/test/unit_test.hpp>
#include <orion/api/engine.hpp>
#include <orion/bre/dmn_parser.hpp>

BOOST_AUTO_TEST_SUITE(namespace_support_tests)

BOOST_AUTO_TEST_CASE(test_dmn_namespace_parsing)
{
    // Test DMN with namespace declaration
    const std::string dmn_with_namespace = R"(
<?xml version="1.0" encoding="UTF-8"?>
<definitions xmlns="http://www.omg.org/spec/DMN/20151101" 
             namespace="http://example.com/dmn"
             name="Test Model" 
             id="test">
  <decision id="decision1" name="TestDecision">
    <literalExpression>
      <text>42</text>
    </literalExpression>
  </decision>
</definitions>
    )";

    // Test DMN without namespace declaration
    const std::string dmn_without_namespace = R"(
<?xml version="1.0" encoding="UTF-8"?>
<definitions xmlns="http://www.omg.org/spec/DMN/20151101" 
             name="Test Model" 
             id="test">
  <decision id="decision1" name="TestDecision">
    <literalExpression>
      <text>42</text>
    </literalExpression>
  </decision>
</definitions>
    )";

    // Test parser directly
    orion::bre::DmnParser parser;
    
    // Test with namespace
    auto model_with_ns = parser.parse(dmn_with_namespace);
    BOOST_CHECK_EQUAL(model_with_ns.namespace_uri, "http://example.com/dmn");
    BOOST_CHECK_EQUAL(model_with_ns.decisions.size(), 1);
    
    // Test without namespace
    auto model_without_ns = parser.parse(dmn_without_namespace);
    BOOST_CHECK(model_without_ns.namespace_uri.empty());
    BOOST_CHECK_EQUAL(model_without_ns.decisions.size(), 1);
}

BOOST_AUTO_TEST_CASE(test_engine_namespace_api)
{
    const std::string dmn_xml = R"(
<?xml version="1.0" encoding="UTF-8"?>
<definitions xmlns="http://www.omg.org/spec/DMN/20151101" 
             namespace="http://test.example/namespace"
             name="Engine Test" 
             id="engine_test">
  <decision id="decision1" name="SimpleDecision">
    <literalExpression>
      <text>100</text>
    </literalExpression>
  </decision>
</definitions>
    )";

    orion::api::BusinessRulesEngine engine;
    
    // Initially no namespace
    BOOST_CHECK(engine.get_namespace().empty());
    
    // Load model with namespace
    auto result = engine.load_dmn_model(dmn_xml);
    BOOST_CHECK(result.has_value());
    
    // Check namespace is stored
    BOOST_CHECK_EQUAL(engine.get_namespace(), "http://test.example/namespace");
    
    // Verify engine still works normally
    auto evaluation_result = engine.evaluate(nlohmann::json::object());
    BOOST_CHECK(!evaluation_result.empty());
    
    // Clear should reset namespace
    engine.clear();
    BOOST_CHECK(engine.get_namespace().empty());
}

BOOST_AUTO_TEST_CASE(test_airline_namespace_integration)
{
    // Test with minimal DMN that has namespace - just enough to test functionality
    const std::string airline_dmn = R"(<?xml version="1.0" encoding="UTF-8"?>
<dmn:definitions xmlns:dmn="http://www.omg.org/spec/DMN/20180521/MODEL/" namespace="http://example.com/dmn">
  <dmn:decision id="test_decision" name="TestDecision">
    <dmn:decisionTable hitPolicy="FIRST">
      <dmn:input><dmn:inputExpression><dmn:text>input1</dmn:text></dmn:inputExpression></dmn:input>
      <dmn:output/>
      <dmn:rule>
        <dmn:inputEntry><dmn:text>"test"</dmn:text></dmn:inputEntry>
        <dmn:outputEntry><dmn:text>42</dmn:text></dmn:outputEntry>
      </dmn:rule>
    </dmn:decisionTable>
  </dmn:decision>
</dmn:definitions>)";

    orion::api::BusinessRulesEngine engine;
    
    // Load the DMN with namespace
    auto result = engine.load_dmn_model(airline_dmn);
    if (!result.has_value()) {
        BOOST_TEST_MESSAGE("Load failed with error: " << result.error());
    }
    BOOST_CHECK(result.has_value());
    
    // Verify namespace extraction
    BOOST_CHECK_EQUAL(engine.get_namespace(), "http://example.com/dmn");
    
    // Verify engine still works for evaluation
    const std::string test_data = R"({"input1": "test"})";
    auto evaluation_result = engine.evaluate(nlohmann::json::parse(test_data));
    BOOST_CHECK(!evaluation_result.empty());
}

BOOST_AUTO_TEST_SUITE_END()