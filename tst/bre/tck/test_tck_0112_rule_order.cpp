/*
 * ORION Optimized Rule Integration & Operations Native
 * SPDX-License-Identifier: Apache-2.0
 * SPDX-FileCopyrightText: 2025 ORION contributors
 */

#include <boost/test/unit_test.hpp>
#include <boost/test/data/test_case.hpp>
#include <nlohmann/json.hpp>
#include <string>
#include <orion/api/engine.hpp>

using json = nlohmann::json;
using namespace orion::api;

BOOST_AUTO_TEST_SUITE(rule_order_single_column_debug)

BOOST_AUTO_TEST_CASE(test_0112_rule_order_single_column_exact_output) {
    BOOST_TEST_MESSAGE("=== Testing 0112 RULE ORDER single column exact output format ===");
    
    // Use the exact DMN XML from the 0112 test case
    std::string dmn_xml = R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<definitions exporter="DMN Modeler" exporterVersion="6.1.3" namespace="http://www.trisotech.com/definitions/_a3ebbd98-af09-42f3-9099-4ae2204a1f54" name="0112-ruleOrder-hitpolicy-singleinoutcol" triso:logoChoice="Default" id="_a3ebbd98-af09-42f3-9099-4ae2204a1f54" xmlns="https://www.omg.org/spec/DMN/20230324/MODEL/" xmlns:di="http://www.omg.org/spec/DMN/20180521/DI/" xmlns:dmndi="https://www.omg.org/spec/DMN/20230324/DMNDI/" xmlns:dc="http://www.omg.org/spec/DMN/20180521/DC/" xmlns:triso="http://www.trisotech.com/2015/triso/modeling" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">
    <extensionElements/>
    <itemDefinition isCollection="true" name="tApproval" label="tApproval">
        <itemComponent isCollection="false" name="Status" id="_20dfeb44-1c36-43cf-8819-63ce36819e90">
            <typeRef>string</typeRef>
        </itemComponent>
        <itemComponent isCollection="false" name="Rate" id="_858ef6e6-69ca-4b50-a654-10f4b981f43c">
            <typeRef>string</typeRef>
        </itemComponent>
    </itemDefinition>
    <itemDefinition isCollection="true" name="tApproval_1" label="tApproval_1">
        <typeRef>string</typeRef>
    </itemDefinition>
    <decision name="Approval" id="_3b2953a3-745f-4d2e-b55d-75c8c5ae653c">
        <variable typeRef="tApproval_1" name="Approval" id="_726bba98-e108-4ee4-b22b-9b9f4da43a88"/>
        <informationRequirement id="b9f76e4e-288a-4237-9220-ab5caf2853e5">
            <requiredInput href="#_41effb45-b3c4-46ac-b1da-122b3e428a98"/>
        </informationRequirement>
        <decisionTable hitPolicy="RULE ORDER" outputLabel="Approval" typeRef="tApproval_1" id="_d81c5a51-b7c3-493c-ae8a-07ff798fb1bd">
            <input id="_bf7fc56f-ea82-464e-a541-f3221dc07e78">
                <inputExpression typeRef="number">
                    <text>Age</text>
                </inputExpression>
            </input>
            <output id="_f07f9e98-3a1e-4add-a200-7cee75b550b3"/>
            <rule id="_ca85854c-27a3-4001-b2ac-23a164ca5940">
                <inputEntry id="_ca85854c-27a3-4001-b2ac-23a164ca5940-0">
                    <text>&gt;=18</text>
                </inputEntry>
                <outputEntry id="_ca85854c-27a3-4001-b2ac-23a164ca5940-4">
                    <text>"Best"</text>
                </outputEntry>
            </rule>
            <rule id="_7f03803d-2636-40ab-8346-7fd7f38ab695">
                <inputEntry id="_7f03803d-2636-40ab-8346-7fd7f38ab695-0">
                    <text>&gt;=12</text>
                </inputEntry>
                <outputEntry id="_7f03803d-2636-40ab-8346-7fd7f38ab695-4">
                    <text>"Standard"</text>
                </outputEntry>
            </rule>
            <rule id="_887acecd-40fc-42da-9443-eeba476f5516">
                <inputEntry id="_887acecd-40fc-42da-9443-eeba476f5516-0">
                    <text>&lt;12</text>
                </inputEntry>
                <outputEntry id="_887acecd-40fc-42da-9443-eeba476f5516-4">
                    <text>"Standard"</text>
                </outputEntry>
            </rule>
        </decisionTable>
    </decision>
    <inputData name="Age" id="_41effb45-b3c4-46ac-b1da-122b3e428a98">
        <variable typeRef="number" name="Age" id="_b6681d58-50f4-42a1-9daf-8daa45ac458e"/>
    </inputData>
</definitions>)";

    BusinessRulesEngine engine;
    
    BOOST_TEST_MESSAGE("Loading DMN model...");
    auto load_result = engine.load_dmn_model(dmn_xml);
    if (!load_result) {
        BOOST_FAIL("Failed to load DMN model: " + load_result.error());
    }
    
    // Test case 1: Age = 19 (should match both >=18 and >=12 rules)
    BOOST_TEST_MESSAGE("\n--- Test Case 1: Age = 19 ---");
    json input1 = {{"Age", 19}};
    auto result1 = engine.evaluate(input1.dump());
    
    BOOST_TEST_MESSAGE("Input: " + input1.dump());
    BOOST_TEST_MESSAGE("Output: " + result1);
    
    // Expected: ["Best", "Standard"] in rule order
    BOOST_TEST_MESSAGE("Expected: [\"Best\", \"Standard\"] as collection");
    
    // Test case 2: Age = 13 (should match only >=12 rule)
    BOOST_TEST_MESSAGE("\n--- Test Case 2: Age = 13 ---");
    json input2 = {{"Age", 13}};
    auto result2 = engine.evaluate(input2.dump());
    
    BOOST_TEST_MESSAGE("Input: " + input2.dump());
    BOOST_TEST_MESSAGE("Output: " + result2);
    
    // Expected: ["Standard"]
    BOOST_TEST_MESSAGE("Expected: [\"Standard\"] as collection");
    
    // Test case 3: Age = 10 (should match only <12 rule)  
    BOOST_TEST_MESSAGE("\n--- Test Case 3: Age = 10 ---");
    json input3 = {{"Age", 10}};
    auto result3 = engine.evaluate(input3.dump());
    
    BOOST_TEST_MESSAGE("Input: " + input3.dump());
    BOOST_TEST_MESSAGE("Output: " + result3);
    
    // Expected: ["Standard"]
    BOOST_TEST_MESSAGE("Expected: [\"Standard\"] as collection");
    
    BOOST_TEST_MESSAGE("=== Analysis complete - check output format ===");
}

BOOST_AUTO_TEST_SUITE_END()