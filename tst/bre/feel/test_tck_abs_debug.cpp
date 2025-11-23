/*
 * ORION Optimized Rule Integration & Operations Native
 * SPDX-License-Identifier: Apache-2.0
 * SPDX-FileCopyrightText: 2025 ORION contributors
 */

#include <boost/test/unit_test.hpp>
#include <orion/api/engine.hpp>
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>

BOOST_AUTO_TEST_SUITE(test_tck_abs_debug, *boost::unit_test::disabled())

using json = nlohmann::json;
namespace fs = std::filesystem;

BOOST_AUTO_TEST_CASE(test_abs_with_actual_tck_file) {
    // Read the actual TCK DMN file
    fs::path dmn_path = "dat/dmn-tck/TestCases/compliance-level-3/0050-feel-abs-function/0050-feel-abs-function.dmn";
    
    if (!fs::exists(dmn_path)) {
        BOOST_TEST_MESSAGE("TCK file not found: " << dmn_path);
        BOOST_CHECK(false);
        return;
    }
    
    std::ifstream file(dmn_path);
    std::string dmn_xml((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    
    std::string input_json = "{}";
    
    BOOST_TEST_MESSAGE("Evaluating TCK DMN file...");
    // Use proper BusinessRulesEngine API
    orion::api::BusinessRulesEngine engine;
    auto load_result = engine.load_dmn_model(dmn_xml);
    if (!load_result) {
        BOOST_FAIL("Failed to load DMN model: " + load_result.error());
    }
    std::string result = engine.evaluate(input_json);
    BOOST_TEST_MESSAGE("Raw result: " << result);
    
    auto j = json::parse(result);
    BOOST_TEST_MESSAGE("Parsed result JSON:");
    BOOST_TEST_MESSAGE(j.dump(2));
    
    // Check for decision001 (abs(1) should return 1)
    if (j.contains("decision001")) {
        BOOST_TEST_MESSAGE("decision001 found: " << j["decision001"]);
        if (j["decision001"].is_number()) {
            BOOST_CHECK_EQUAL(j["decision001"].get<double>(), 1.0);
        } else {
            BOOST_TEST_MESSAGE("ERROR: decision001 is not a number, it's: " << j["decision001"].type_name());
        }
    } else {
        BOOST_TEST_MESSAGE("ERROR: decision001 not found in result");
        BOOST_TEST_MESSAGE("Available keys: ");
        for (auto& el : j.items()) {
            BOOST_TEST_MESSAGE("  - " << el.key());
        }
    }
    
    // Check for decision006 (abs(n:-1) should return 1)
    if (j.contains("decision006")) {
        BOOST_TEST_MESSAGE("decision006 found: " << j["decision006"]);
        if (j["decision006"].is_number()) {
            BOOST_CHECK_EQUAL(j["decision006"].get<double>(), 1.0);
        } else {
            BOOST_TEST_MESSAGE("ERROR: decision006 is not a number");
        }
    } else {
        BOOST_TEST_MESSAGE("ERROR: decision006 not found - named parameters not working?");
    }
}

BOOST_AUTO_TEST_SUITE_END()
