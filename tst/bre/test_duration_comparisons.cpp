// test_duration_comparisons.cpp - Test duration comparisons in DMN decision tables
#include <boost/test/unit_test.hpp>
#include <orion/api/engine.hpp>
#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>

using json = nlohmann::json;
using namespace orion::api;

namespace {

std::string read_file(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + filepath);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

json load_json_file(const std::string& filepath) {
    std::string content = read_file(filepath);
    return json::parse(content);
}

} // anonymous namespace

BOOST_AUTO_TEST_SUITE(duration_comparison_dmn_tests)

BOOST_AUTO_TEST_CASE(duration_comparisons_in_decision_table)
{
    // Load DMN model
    std::string dmn_xml = read_file("dat/tst/test_duration_comparisons.dmn");
    
    // Create engine and load model
    BusinessRulesEngine engine;
    auto load_result = engine.load_dmn_model(dmn_xml);
    BOOST_REQUIRE_MESSAGE(load_result.has_value(), 
        "Failed to load DMN model: " + (load_result.has_value() ? "" : load_result.error()));
    
    // Load test cases
    json test_data = load_json_file("dat/tst/test_duration_comparisons.json");
    BOOST_REQUIRE(test_data.contains("test_cases"));
    
    // Run each test case
    for (const auto& test_case : test_data["test_cases"]) {
        std::string test_name = test_case["name"];
        json input = test_case["input"];
        json expected_output = test_case["expected_output"];
        
        // Evaluate decision
        std::string result_str = engine.evaluate(input.dump());
        json result = json::parse(result_str);
        
        // Debug output
        std::cout << "Test: " << test_name << std::endl;
        std::cout << "Input: " << input.dump() << std::endl;
        std::cout << "Result: " << result.dump() << std::endl;
        
        // Verify result
        BOOST_TEST_CONTEXT("Test case: " << test_name) {
            BOOST_REQUIRE(result.contains("Eligibility Decision"));
            BOOST_CHECK_EQUAL(result["Eligibility Decision"], expected_output["Eligibility"]);
        }
    }
}

BOOST_AUTO_TEST_CASE(duration_hour_to_day_equivalence)
{
    // Verify PT48H = P2D equivalence in DMN context
    std::string dmn_xml = read_file("dat/tst/test_duration_comparisons.dmn");
    
    BusinessRulesEngine engine;
    auto load_result = engine.load_dmn_model(dmn_xml);
    BOOST_REQUIRE(load_result.has_value());
    
    // PT120H = P5D (boundary test)
    json input1 = {{"WaitTime", "PT120H"}};
    std::string result1_str = engine.evaluate(input1.dump());
    json result1 = json::parse(result1_str);
    BOOST_CHECK_EQUAL(result1["Eligibility Decision"], "Eligible");
    
    // P5D (boundary test)
    json input2 = {{"WaitTime", "P5D"}};
    std::string result2_str = engine.evaluate(input2.dump());
    json result2 = json::parse(result2_str);
    BOOST_CHECK_EQUAL(result2["Eligibility Decision"], "Eligible");
    
    // Both should produce same result
    BOOST_CHECK_EQUAL(result1["Eligibility Decision"], result2["Eligibility Decision"]);
}

BOOST_AUTO_TEST_CASE(duration_range_comparison)
{
    // Verify range comparisons ["P5D".."P10D"]
    std::string dmn_xml = read_file("dat/tst/test_duration_comparisons.dmn");
    
    BusinessRulesEngine engine;
    auto load_result = engine.load_dmn_model(dmn_xml);
    BOOST_REQUIRE(load_result.has_value());
    
    // Below range
    json input_below = {{"WaitTime", "P3D"}};
    std::string result_below_str = engine.evaluate(input_below.dump());
    json result_below = json::parse(result_below_str);
    BOOST_CHECK_EQUAL(result_below["Eligibility Decision"], "Too Short");
    
    // In range
    json input_in = {{"WaitTime", "P7D"}};
    std::string result_in_str = engine.evaluate(input_in.dump());
    json result_in = json::parse(result_in_str);
    BOOST_CHECK_EQUAL(result_in["Eligibility Decision"], "Eligible");
    
    // Above range
    json input_above = {{"WaitTime", "P15D"}};
    std::string result_above_str = engine.evaluate(input_above.dump());
    json result_above = json::parse(result_above_str);
    BOOST_CHECK_EQUAL(result_above["Eligibility Decision"], "Too Long");
}

BOOST_AUTO_TEST_SUITE_END()
