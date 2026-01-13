#include <boost/test/unit_test.hpp>
#include <orion/api/engine.hpp>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

BOOST_AUTO_TEST_SUITE(native_api_tests)

BOOST_AUTO_TEST_CASE(test_evaluate_native_json)
{
    orion::api::BusinessRulesEngine engine;
    
    // Simple decision table
    std::string dmn_xml = R"(
    <definitions xmlns="https://www.omg.org/spec/DMN/20191111/MODEL/" namespace="test">
        <decision name="SimpleDecision" id="d_simple">
            <decisionTable hitPolicy="UNIQUE">
                <input id="i_input">
                    <inputExpression typeRef="string">
                        <text>Input</text>
                    </inputExpression>
                </input>
                <output id="o_output" name="Result" typeRef="string"/>
                <rule id="r1">
                    <inputEntry id="r1_i1">
                        <text>"Test"</text>
                    </inputEntry>
                    <outputEntry id="r1_o1">
                        <text>"Success"</text>
                    </outputEntry>
                </rule>
            </decisionTable>
        </decision>
    </definitions>
    )";

    auto result_load = engine.load_dmn_model(dmn_xml);
    BOOST_CHECK(result_load.has_value());

    // Prepare input as nlohmann::json
    json input_context;
    input_context["Input"] = "Test";

    // CALL THE NEW API
    json result = engine.evaluate(input_context);

    // Verify result is a json object, not string
    BOOST_CHECK(result.is_object());
    BOOST_CHECK(result.contains("SimpleDecision"));
    
    // Verify result is correct (Single output column returns scalar)
    BOOST_CHECK_EQUAL(result["SimpleDecision"], "Success");
}

BOOST_AUTO_TEST_CASE(test_evaluate_legacy_string)
{
    orion::api::BusinessRulesEngine engine;
    
    // Simple decision table
    std::string dmn_xml = R"(
    <definitions xmlns="https://www.omg.org/spec/DMN/20191111/MODEL/" namespace="test">
        <decision name="SimpleDecision" id="d_simple">
            <decisionTable hitPolicy="UNIQUE">
                <input id="i_input">
                    <inputExpression typeRef="string">
                        <text>Input</text>
                    </inputExpression>
                </input>
                <output id="o_output" name="Result" typeRef="string"/>
                <rule id="r1">
                    <inputEntry id="r1_i1">
                        <text>"Test"</text>
                    </inputEntry>
                    <outputEntry id="r1_o1">
                        <text>"Success"</text>
                    </outputEntry>
                </rule>
            </decisionTable>
        </decision>
    </definitions>
    )";

    auto result_load = engine.load_dmn_model(dmn_xml);
    BOOST_CHECK(result_load.has_value());

    // Prepare input as string
    std::string input_json = R"({"Input": "Test"})";

    // CALL API (parsing required)
    json result = engine.evaluate(json::parse(input_json));

    // Verify result
    BOOST_CHECK(!result.empty());
    BOOST_CHECK(result.contains("SimpleDecision"));
    BOOST_CHECK_EQUAL(result["SimpleDecision"], "Success");
}

BOOST_AUTO_TEST_SUITE_END()
