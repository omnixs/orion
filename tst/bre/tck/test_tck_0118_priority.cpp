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

BOOST_AUTO_TEST_SUITE(debug_0118_priority_hitpolicy)

BOOST_AUTO_TEST_CASE(test_0118_case_001_debug) {
    BOOST_TEST_MESSAGE("=== DEBUG: 0118 PRIORITY Hit Policy Test Case 001 ===");
    
    // Load the 0118 DMN model
    std::ifstream file("dat/dmn-tck/TestCases/compliance-level-2/0118-multi-priority-hitpolicy/0118-multi-priority-hitpolicy.dmn");
    BOOST_REQUIRE_MESSAGE(file.is_open(), "Could not open 0118 DMN file");
    
    std::ostringstream buffer;
    buffer << file.rdbuf();
    std::string dmn_xml = buffer.str();
    
    // Create engine and load model
    BusinessRulesEngine engine;
    
    auto load_result = engine.load_dmn_model(dmn_xml);
    if (!load_result) {
        BOOST_FAIL("Failed to load DMN model: " + load_result.error());
    }
    
    // Test case 001: Age=17, RiskCategory=High, isAffordable=true
    // Expected: Approved/Standard (Rule 3 wins over Rule 2 due to "Approved" > "Declined" priority)
    // Rule 2: Age<18 → Declined/Standard
    // Rule 3: RiskCategory=High → Approved/Standard
    json input = {
        {"Age", 17},
        {"RiskCategory", "High"},
        {"isAffordable", true}
    };
    std::string input_str = input.dump();
    
    BOOST_TEST_MESSAGE("Input: " + input_str);
    
    // Get engine result
    std::string result = engine.evaluate(input_str);
    BOOST_TEST_MESSAGE("Engine Result: " + result);
    
    // Parse result
    json result_json = json::parse(result);
    BOOST_TEST_MESSAGE("Parsed Result Type: " + std::string(result_json.type_name()));
    
    // Expected result structure
    json expected = {
        {"Approval Status", {
            {"Approved/Declined", "Approved"},
            {"Rate", "Standard"}
        }}
    };
    BOOST_TEST_MESSAGE("Expected: " + expected.dump());
    
    // Check actual structure
    if (result_json.contains("Approval Status")) {
        json approval_status = result_json["Approval Status"];
        BOOST_TEST_MESSAGE("Approval Status Type: " + std::string(approval_status.type_name()));
        BOOST_TEST_MESSAGE("Approval Status Value: " + approval_status.dump());
        
        if (approval_status.is_object()) {
            if (approval_status.contains("Approved/Declined")) {
                std::string status = approval_status["Approved/Declined"].get<std::string>();
                BOOST_TEST_MESSAGE("Approved/Declined: " + status);
                BOOST_TEST_MESSAGE("Expected: Approved, Got: " + status);
            }
            if (approval_status.contains("Rate")) {
                std::string rate = approval_status["Rate"].get<std::string>();
                BOOST_TEST_MESSAGE("Rate: " + rate);
                BOOST_TEST_MESSAGE("Expected: Standard, Got: " + rate);
            }
        }
    }
    
    // Manual comparison
    bool matches = (result_json == expected);
    BOOST_TEST_MESSAGE("Result matches expected: " + std::string(matches ? "YES" : "NO"));
}

BOOST_AUTO_TEST_CASE(test_0118_case_003_debug) {
    BOOST_TEST_MESSAGE("=== DEBUG: 0118 PRIORITY Hit Policy Test Case 003 ===");
    
    // Load the 0118 DMN model
    std::ifstream file("dat/dmn-tck/TestCases/compliance-level-2/0118-multi-priority-hitpolicy/0118-multi-priority-hitpolicy.dmn");
    BOOST_REQUIRE_MESSAGE(file.is_open(), "Could not open 0118 DMN file");
    
    std::ostringstream buffer;
    buffer << file.rdbuf();
    std::string dmn_xml = buffer.str();
    
    // Create engine and load model
    BusinessRulesEngine engine;
    
    auto load_result = engine.load_dmn_model(dmn_xml);
    if (!load_result) {
        BOOST_FAIL("Failed to load DMN model: " + load_result.error());
    }
    
    // Test case 001: Age=17, RiskCategory=Low, isAffordable=true
    // Expected: Declined/Standard
    // Should match Rule 2: Age<18 → Declined/Standard
    json input = {
        {"Age", 10},
        {"RiskCategory", "Low"},
        {"isAffordable", true}
    };
    std::string input_str = input.dump();
    
    BOOST_TEST_MESSAGE("Input: " + input_str);
    
    // Get engine result
    std::string result = engine.evaluate(input_str);
    BOOST_TEST_MESSAGE("Engine Result: " + result);
    
    // Parse result
    json result_json = json::parse(result);
    BOOST_TEST_MESSAGE("Parsed Result Type: " + std::string(result_json.type_name()));
    
    // Expected result structure
    json expected = {
        {"Approval Status", {
            {"Approved/Declined", "Declined"},
            {"Rate", "Standard"}
        }}
    };
    BOOST_TEST_MESSAGE("Expected: " + expected.dump());
    
    // Check actual structure
    if (result_json.contains("Approval Status")) {
        json approval_status = result_json["Approval Status"];
        BOOST_TEST_MESSAGE("Approval Status Type: " + std::string(approval_status.type_name()));
        BOOST_TEST_MESSAGE("Approval Status Value: " + approval_status.dump());
        
        if (approval_status.is_object()) {
            if (approval_status.contains("Approved/Declined")) {
                std::string status = approval_status["Approved/Declined"].get<std::string>();
                BOOST_TEST_MESSAGE("Approved/Declined: " + status);
                BOOST_TEST_MESSAGE("Expected: Declined, Got: " + status);
            }
            if (approval_status.contains("Rate")) {
                std::string rate = approval_status["Rate"].get<std::string>();
                BOOST_TEST_MESSAGE("Rate: " + rate);
                BOOST_TEST_MESSAGE("Expected: Standard, Got: " + rate);
            }
        }
    }
    
    // Manual comparison
    bool matches = (result_json == expected);
    BOOST_TEST_MESSAGE("Result matches expected: " + std::string(matches ? "YES" : "NO"));
}

BOOST_AUTO_TEST_SUITE_END()
