/*
 * ORION Optimized Rule Integration & Operations Native
 * SPDX-License-Identifier: Apache-2.0
 * SPDX-FileCopyrightText: 2025 ORION contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Modifications: This file has been modified by ORION contributors. See VCS history.
 */

#include <boost/test/unit_test.hpp>
#include <orion/bre/dmn_parser.hpp>
#include <orion/bre/type_validator.hpp>
#include <orion/api/engine.hpp>
#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>

using namespace orion::bre;
using namespace orion::api;
using json = nlohmann::json;

BOOST_AUTO_TEST_SUITE(item_definition_tests)

BOOST_AUTO_TEST_CASE(parse_simple_item_definition_with_allowed_values)
{
    std::string_view dmn_xml = R"(
        <?xml version="1.0" encoding="UTF-8"?>
        <dmn:definitions xmlns:dmn="http://www.omg.org/spec/DMN/20180521/MODEL/" 
                        xmlns="http://example.com/dmn" 
                        id="Definitions_1" 
                        name="Sample">
            <dmn:itemDefinition id="_11015BBD" name="tStatus" isCollection="false">
                <dmn:typeRef>string</dmn:typeRef>
                <dmn:allowedValues id="_8736916F">
                    <dmn:text>"Active", "Disabled"</dmn:text>
                </dmn:allowedValues>
            </dmn:itemDefinition>
        </dmn:definitions>
    )";
    
    DmnParser parser;
    auto model = parser.parse(dmn_xml);
    
    BOOST_REQUIRE(model.item_definitions.count("tStatus") > 0);
    
    const auto& status_type = model.item_definitions.at("tStatus");
    BOOST_CHECK_EQUAL(status_type.name, "tStatus");
    BOOST_CHECK_EQUAL(status_type.typeRef, "string");
    BOOST_CHECK_EQUAL(status_type.isCollection, false);
    BOOST_CHECK(!status_type.allowedValues.empty());
    BOOST_CHECK(status_type.is_simple_type());
    BOOST_CHECK(status_type.has_constraints());
}

BOOST_AUTO_TEST_CASE(parse_allowed_values_enumeration)
{
    std::string_view allowed_values = R"("Active", "Disabled", "Pending")";
    
    auto values = parse_allowed_values(allowed_values);
    
    BOOST_REQUIRE_EQUAL(values.size(), 3);
    BOOST_CHECK_EQUAL(values[0], "Active");
    BOOST_CHECK_EQUAL(values[1], "Disabled");
    BOOST_CHECK_EQUAL(values[2], "Pending");
}

BOOST_AUTO_TEST_CASE(parse_allowed_values_with_extra_whitespace)
{
    std::string_view allowed_values = R"(  "CH_EK"  ,  "CH_L"  ,  "CH_O"  )";
    
    auto values = parse_allowed_values(allowed_values);
    
    BOOST_REQUIRE_EQUAL(values.size(), 3);
    BOOST_CHECK_EQUAL(values[0], "CH_EK");
    BOOST_CHECK_EQUAL(values[1], "CH_L");
    BOOST_CHECK_EQUAL(values[2], "CH_O");
}

BOOST_AUTO_TEST_CASE(validate_string_against_allowed_values)
{
    ItemDefinition item_def;
    item_def.name = "tStatus";
    item_def.typeRef = "string";
    item_def.allowedValues = R"("Active", "Disabled")";
    
    json valid_value = "Active";
    json invalid_value = "Unknown";
    
    BOOST_CHECK(validate_type_constraint(valid_value, item_def));
    BOOST_CHECK(!validate_type_constraint(invalid_value, item_def));
}

BOOST_AUTO_TEST_CASE(validate_without_constraints_allows_all)
{
    ItemDefinition item_def;
    item_def.name = "tAnyString";
    item_def.typeRef = "string";
    // No allowedValues constraint
    
    json value = "AnyValueIsValid";
    
    BOOST_CHECK(validate_type_constraint(value, item_def));
}

BOOST_AUTO_TEST_CASE(parse_multiple_item_definitions)
{
    std::string_view dmn_xml = R"(
        <?xml version="1.0" encoding="UTF-8"?>
        <dmn:definitions xmlns:dmn="http://www.omg.org/spec/DMN/20180521/MODEL/">
            <dmn:itemDefinition name="tStatus">
                <dmn:typeRef>string</dmn:typeRef>
                <dmn:allowedValues>
                    <dmn:text>"Active", "Disabled"</dmn:text>
                </dmn:allowedValues>
            </dmn:itemDefinition>
            <dmn:itemDefinition name="tSeatCode">
                <dmn:typeRef>string</dmn:typeRef>
                <dmn:allowedValues>
                    <dmn:text>"CH_EK", "CH_L", "CH_O"</dmn:text>
                </dmn:allowedValues>
            </dmn:itemDefinition>
        </dmn:definitions>
    )";
    
    DmnParser parser;
    auto model = parser.parse(dmn_xml);
    
    BOOST_REQUIRE_EQUAL(model.item_definitions.size(), 2);
    BOOST_CHECK(model.item_definitions.count("tStatus") > 0);
    BOOST_CHECK(model.item_definitions.count("tSeatCode") > 0);
    
    // Validate tStatus
    const auto& status = model.item_definitions.at("tStatus");
    auto status_values = parse_allowed_values(status.allowedValues);
    BOOST_REQUIRE_EQUAL(status_values.size(), 2);
    
    // Validate tSeatCode
    const auto& seat = model.item_definitions.at("tSeatCode");
    auto seat_values = parse_allowed_values(seat.allowedValues);
    BOOST_REQUIRE_EQUAL(seat_values.size(), 3);
}

BOOST_AUTO_TEST_CASE(engine_loads_item_definitions_from_dmn)
{
    std::string_view dmn_xml = R"(
        <?xml version="1.0" encoding="UTF-8"?>
        <dmn:definitions xmlns:dmn="http://www.omg.org/spec/DMN/20180521/MODEL/"
                        id="test_definitions">
            <dmn:itemDefinition name="tApprovalStatus">
                <dmn:typeRef>string</dmn:typeRef>
                <dmn:allowedValues>
                    <dmn:text>"Approved", "Declined", "Pending"</dmn:text>
                </dmn:allowedValues>
            </dmn:itemDefinition>
            <dmn:decision id="dec1" name="TestDecision">
                <dmn:variable name="result" typeRef="string"/>
                <dmn:literalExpression>
                    <dmn:text>"test"</dmn:text>
                </dmn:literalExpression>
            </dmn:decision>
        </dmn:definitions>
    )";
    
    BusinessRulesEngine engine;
    auto result = engine.load_dmn_model(dmn_xml);
    
    BOOST_CHECK(result.has_value());
    // ItemDefinitions are now stored in the engine
    // They can be used for validation in future enhancements
}

BOOST_AUTO_TEST_CASE(load_seat_bundle_rules_file)
{
    // Try to load the actual seat bundle rules file
    std::ifstream file("D:\\Workspace\\orion2\\temp\\00_SeatBundleRules.dmn");
    
    if (!file.is_open())
    {
        // File doesn't exist, skip test
        BOOST_TEST_MESSAGE("Seat bundle rules file not found, skipping test");
        return;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string dmn_xml = buffer.str();
    
    DmnParser parser;
    auto model = parser.parse(dmn_xml);
    
    // Verify ItemDefinitions were parsed
    BOOST_CHECK_GT(model.item_definitions.size(), 0);
    
    // Check for specific types we expect
    BOOST_CHECK(model.item_definitions.count("tStatus") > 0);
    BOOST_CHECK(model.item_definitions.count("tSeatCode") > 0);
    BOOST_CHECK(model.item_definitions.count("tChangeOfferDetailsOnly") > 0);
    
    // Verify tStatus constraints
    const auto& status_type = model.item_definitions.at("tStatus");
    BOOST_CHECK_EQUAL(status_type.typeRef, "string");
    BOOST_CHECK(!status_type.allowedValues.empty());
    
    auto status_values = parse_allowed_values(status_type.allowedValues);
    BOOST_REQUIRE_EQUAL(status_values.size(), 2);
    BOOST_CHECK_EQUAL(status_values[0], "Active");
    BOOST_CHECK_EQUAL(status_values[1], "Disabled");
    
    // Verify tSeatCode constraints
    const auto& seat_type = model.item_definitions.at("tSeatCode");
    auto seat_values = parse_allowed_values(seat_type.allowedValues);
    BOOST_REQUIRE_EQUAL(seat_values.size(), 5);
    BOOST_CHECK_EQUAL(seat_values[0], "CH_EK");
    BOOST_CHECK_EQUAL(seat_values[4], "CH");
    
    // Verify decisions were parsed
    BOOST_CHECK_GT(model.decisions.size(), 0);
    
    // Try to load in engine
    BusinessRulesEngine engine;
    auto load_result = engine.load_dmn_model(dmn_xml);
    BOOST_CHECK(load_result.has_value());
}

BOOST_AUTO_TEST_CASE(validate_collection_type_constraints)
{
    ItemDefinition list_def;
    list_def.name = "tStatusList";
    list_def.typeRef = "string";
    list_def.allowedValues = R"("Active", "Disabled")";
    list_def.isCollection = true;
    
    json valid_list = json::array({"Active", "Disabled", "Active"});
    json invalid_list = json::array({"Active", "Unknown"});
    json empty_list = json::array();
    
    BOOST_CHECK(validate_type_constraint(valid_list, list_def));
    BOOST_CHECK(!validate_type_constraint(invalid_list, list_def));
    BOOST_CHECK(validate_type_constraint(empty_list, list_def));
}

BOOST_AUTO_TEST_SUITE_END()
