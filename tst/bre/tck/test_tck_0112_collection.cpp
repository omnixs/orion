/*
 * ORION Optimized Rule Integration & Operations Native
 * SPDX-License-Identifier: Apache-2.0
 * SPDX-FileCopyrightText: 2025 ORION contributors
 */

#include <boost/test/unit_test.hpp>
#include <nlohmann/json.hpp>
#include <orion/api/engine.hpp>
#include <fstream>
#include <sstream>

using json = nlohmann::json;
using namespace orion::api;

BOOST_AUTO_TEST_SUITE(debug_0112_collection_output)

BOOST_AUTO_TEST_CASE(test_0112_collection_direct_comparison) {
    BOOST_TEST_MESSAGE("=== DEBUG: 0112 Collection Output Format Direct Test ===");
    
    // Load the exact 0112 DMN model
    std::ifstream file("dat/dmn-tck/TestCases/compliance-level-2/0112-ruleOrder-hitpolicy-singleinoutcol/0112-ruleOrder-hitpolicy-singleinoutcol.dmn");
    BOOST_REQUIRE_MESSAGE(file.is_open(), "Could not open 0112 DMN file");
    
    std::ostringstream buffer;
    buffer << file.rdbuf();
    std::string dmn_xml = buffer.str();
    
    // Create engine and load model
    BusinessRulesEngine engine;
    
    auto load_result = engine.load_dmn_model(dmn_xml);
    BOOST_REQUIRE_MESSAGE(load_result.has_value(), "Failed to load DMN model: " + load_result.error());
    
    // Test case 1: Age = 19 (should trigger multiple rules)
    json input = {{"Age", 19}};
    std::string input_str = input.dump();
    
    BOOST_TEST_MESSAGE("Input: " + input_str);
    
    // Get engine result
    std::string result = engine.evaluate(input_str);
    BOOST_TEST_MESSAGE("Engine Result: " + result);
    
    // Parse result and analyze structure
    json result_json = json::parse(result);
    BOOST_TEST_MESSAGE("Parsed Result Type: " + std::string(result_json.type_name()));
    
    if (result_json.is_array()) {
        BOOST_TEST_MESSAGE("Result is array with " + std::to_string(result_json.size()) + " elements");
        BOOST_TEST_MESSAGE("Array content: " + result_json.dump());
    } else if (result_json.is_object()) {
        BOOST_TEST_MESSAGE("Result is object with keys:");
        for (auto& [key, value] : result_json.items()) {
            BOOST_TEST_MESSAGE("  Key: " + key + ", Value Type: " + std::string(value.type_name()));
            if (value.is_array()) {
                BOOST_TEST_MESSAGE("    Array content: " + value.dump());
            }
        }
    }
    
    // Expected according to TCK: ["Best", "Standard"]
    json expected = {"Best", "Standard"};
    BOOST_TEST_MESSAGE("Expected: " + expected.dump());
    
    // Analyze what we need to fix
    bool matches_expected = false;
    if (result_json.is_array()) {
        matches_expected = (result_json == expected);
    } else if (result_json.is_object() && result_json.contains("Approval")) {
        matches_expected = (result_json["Approval"] == expected);
    }
    
    BOOST_TEST_MESSAGE("Matches expected format: " + std::string(matches_expected ? "YES" : "NO"));
}

BOOST_AUTO_TEST_SUITE_END()