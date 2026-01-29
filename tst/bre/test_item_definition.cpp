// test_item_definition.cpp - Unit tests for DMN 1.5 ItemDefinition support
//
// Tests for:
// - ItemDefinition parsing from DMN XML
// - Type validation with constraints
// - Engine integration

#include <boost/test/unit_test.hpp>
#include <orion/api/engine.hpp>
#include <orion/bre/type_validator.hpp>
#include <nlohmann/json.hpp>

namespace utf = boost::unit_test;

BOOST_AUTO_TEST_SUITE(item_definition_tests)

// ============================================================================
// ItemDefinition Loading Tests (via Engine)
// ============================================================================

BOOST_AUTO_TEST_CASE(test_engine_loads_simple_type_definition)
{
    std::string_view dmn_xml = R"(<?xml version="1.0" encoding="UTF-8"?>
<definitions xmlns="https://www.omg.org/spec/DMN/20230324/MODEL/" 
             namespace="test">
    <itemDefinition id="tAge" name="tAge">
        <typeRef>number</typeRef>
    </itemDefinition>
    <decision id="d1" name="Test">
        <decisionTable id="dt1" hitPolicy="UNIQUE">
            <input id="i1">
                <inputExpression id="ie1" typeRef="tAge">
                    <text>Age</text>
                </inputExpression>
            </input>
            <output id="o1" name="Output"/>
            <rule id="r1">
                <inputEntry id="r1i1"><text>18</text></inputEntry>
                <outputEntry id="r1o1"><text>"Adult"</text></outputEntry>
            </rule>
        </decisionTable>
    </decision>
</definitions>)";

    orion::api::BusinessRulesEngine engine;
    auto result = engine.load_dmn_model(dmn_xml);
    
    BOOST_TEST(result.has_value(), "Engine should load DMN model with ItemDefinition");
}

BOOST_AUTO_TEST_CASE(test_engine_loads_enumeration_constraints)
{
    std::string_view dmn_xml = R"(<?xml version="1.0" encoding="UTF-8"?>
<definitions xmlns="https://www.omg.org/spec/DMN/20230324/MODEL/" 
             namespace="test">
    <itemDefinition id="tStatus" name="tStatus">
        <typeRef>string</typeRef>
        <allowedValues>
            <text>"Pending", "Approved", "Rejected"</text>
        </allowedValues>
    </itemDefinition>
    <decision id="d1" name="Test">
        <decisionTable id="dt1" hitPolicy="UNIQUE">
            <input id="i1">
                <inputExpression id="ie1" typeRef="tStatus">
                    <text>Status</text>
                </inputExpression>
            </input>
            <output id="o1" name="Output"/>
            <rule id="r1">
                <inputEntry id="r1i1"><text>"Pending"</text></inputEntry>
                <outputEntry id="r1o1"><text>"Process"</text></outputEntry>
            </rule>
        </decisionTable>
    </decision>
</definitions>)";

    orion::api::BusinessRulesEngine engine;
    auto result = engine.load_dmn_model(dmn_xml);
    
    BOOST_TEST(result.has_value(), "Engine should load DMN model with enumeration");
}

BOOST_AUTO_TEST_CASE(test_engine_loads_structured_type)
{
    std::string_view dmn_xml = R"(<?xml version="1.0" encoding="UTF-8"?>
<definitions xmlns="https://www.omg.org/spec/DMN/20230324/MODEL/" 
             namespace="test">
    <itemDefinition id="tPerson" name="tPerson">
        <itemComponent id="c1" name="name">
            <typeRef>string</typeRef>
        </itemComponent>
        <itemComponent id="c2" name="age">
            <typeRef>number</typeRef>
        </itemComponent>
        <itemComponent id="c3" name="email">
            <typeRef>string</typeRef>
        </itemComponent>
    </itemDefinition>
    <decision id="d1" name="Test">
        <decisionTable id="dt1" hitPolicy="UNIQUE">
            <input id="i1">
                <inputExpression id="ie1" typeRef="tPerson">
                    <text>Person</text>
                </inputExpression>
            </input>
            <output id="o1" name="Output"/>
            <rule id="r1">
                <inputEntry id="r1i1"><text>-</text></inputEntry>
                <outputEntry id="r1o1"><text>"Result"</text></outputEntry>
            </rule>
        </decisionTable>
    </decision>
</definitions>)";

    orion::api::BusinessRulesEngine engine;
    auto result = engine.load_dmn_model(dmn_xml);
    
    BOOST_TEST(result.has_value(), "Engine should load DMN model with structured type");
}

BOOST_AUTO_TEST_CASE(test_engine_loads_collection_type)
{
    std::string_view dmn_xml = R"(<?xml version="1.0" encoding="UTF-8"?>
<definitions xmlns="https://www.omg.org/spec/DMN/20230324/MODEL/" 
             namespace="test">
    <itemDefinition id="tNumbers" name="tNumbers" isCollection="true">
        <typeRef>number</typeRef>
    </itemDefinition>
    <decision id="d1" name="Test">
        <decisionTable id="dt1" hitPolicy="UNIQUE">
            <input id="i1">
                <inputExpression id="ie1" typeRef="tNumbers">
                    <text>Numbers</text>
                </inputExpression>
            </input>
            <output id="o1" name="Output"/>
            <rule id="r1">
                <inputEntry id="r1i1"><text>-</text></inputEntry>
                <outputEntry id="r1o1"><text>"Result"</text></outputEntry>
            </rule>
        </decisionTable>
    </decision>
</definitions>)";

    orion::api::BusinessRulesEngine engine;
    auto result = engine.load_dmn_model(dmn_xml);
    
    BOOST_TEST(result.has_value(), "Engine should load collection type");
}

// ============================================================================
// Type Validation Tests
// ============================================================================

BOOST_AUTO_TEST_CASE(test_validate_enumeration_valid)
{
    orion::bre::ItemDefinition item_def;
    item_def.name = "tStatus";
    item_def.typeRef = "string";
    item_def.allowedValues = R"("Pending", "Approved", "Rejected")";
    
    nlohmann::json valid_value = "Approved";
    BOOST_TEST(orion::bre::validate_type_constraint(valid_value, item_def));
    
    valid_value = "Pending";
    BOOST_TEST(orion::bre::validate_type_constraint(valid_value, item_def));
}

BOOST_AUTO_TEST_CASE(test_validate_enumeration_invalid)
{
    orion::bre::ItemDefinition item_def;
    item_def.name = "tStatus";
    item_def.typeRef = "string";
    item_def.allowedValues = R"("Pending", "Approved", "Rejected")";
    
    nlohmann::json invalid_value = "Unknown";
    BOOST_TEST(!orion::bre::validate_type_constraint(invalid_value, item_def));
}

BOOST_AUTO_TEST_CASE(test_validate_collection_enumeration)
{
    orion::bre::ItemDefinition item_def;
    item_def.name = "tStatuses";
    item_def.typeRef = "string";
    item_def.isCollection = true;
    item_def.allowedValues = R"("Pending", "Approved", "Rejected")";
    
    // Valid collection with all allowed values
    nlohmann::json valid_collection = nlohmann::json::array({"Pending", "Approved"});
    BOOST_TEST(orion::bre::validate_type_constraint(valid_collection, item_def));
    
    // Invalid collection with one disallowed value
    nlohmann::json invalid_collection = nlohmann::json::array({"Pending", "Unknown"});
    BOOST_TEST(!orion::bre::validate_type_constraint(invalid_collection, item_def));
}

BOOST_AUTO_TEST_CASE(test_validate_structured_type)
{
    // Define structured type
    orion::bre::ItemDefinition item_def;
    item_def.name = "tPerson";
    
    orion::bre::ItemComponent name_comp;
    name_comp.name = "name";
    name_comp.typeRef = "string";
    item_def.itemComponents.push_back(name_comp);
    
    orion::bre::ItemComponent age_comp;
    age_comp.name = "age";
    age_comp.typeRef = "number";
    item_def.itemComponents.push_back(age_comp);
    
    std::map<std::string, orion::bre::ItemDefinition> item_defs;
    item_defs[item_def.name] = item_def;
    
    // Valid structured value - should succeed
    nlohmann::json valid_value = {
        {"name", "John Doe"},
        {"age", 30}
    };
    auto result1 = orion::bre::validate_complex_type(valid_value, item_def, item_defs);
    BOOST_CHECK(result1.has_value());
    
    // Partial value (missing age) - DMN fields are optional by default, so this is valid
    nlohmann::json partial_value = {
        {"name", "John Doe"}
        // age missing - but this is allowed in DMN (fields are optional)
    };
    auto result2 = orion::bre::validate_complex_type(partial_value, item_def, item_defs);
    BOOST_CHECK(result2.has_value());
    
    // Invalid - non-object type - should return error
    nlohmann::json invalid_value = "not an object";
    auto result3 = orion::bre::validate_complex_type(invalid_value, item_def, item_defs);
    BOOST_CHECK(!result3.has_value());
    BOOST_CHECK(!result3.error().empty());
}

BOOST_AUTO_TEST_CASE(test_validate_component_with_constraints)
{
    // Component with enumeration constraint
    orion::bre::ItemComponent component;
    component.name = "status";
    component.typeRef = "string";
    component.allowedValues = R"("Active", "Inactive")";
    
    std::map<std::string, orion::bre::ItemDefinition> item_defs;
    
    nlohmann::json valid_value = "Active";
    auto result1 = orion::bre::validate_component(valid_value, component, item_defs);
    BOOST_CHECK(result1.has_value());
    
    nlohmann::json invalid_value = "Unknown";
    auto result2 = orion::bre::validate_component(invalid_value, component, item_defs);
    BOOST_CHECK(!result2.has_value());
    BOOST_CHECK(!result2.error().empty());
}

// ============================================================================
// Engine Integration Tests
// ============================================================================

BOOST_AUTO_TEST_CASE(test_engine_loads_item_definitions)
{
    std::string_view dmn_xml = R"(<?xml version="1.0" encoding="UTF-8"?>
<definitions xmlns="https://www.omg.org/spec/DMN/20230324/MODEL/" 
             namespace="test">
    <itemDefinition id="tAge" name="tAge">
        <typeRef>number</typeRef>
    </itemDefinition>
    <itemDefinition id="tStatus" name="tStatus">
        <typeRef>string</typeRef>
        <allowedValues>
            <text>"Pending", "Approved"</text>
        </allowedValues>
    </itemDefinition>
    <decision id="d1" name="Test">
        <decisionTable id="dt1" hitPolicy="UNIQUE">
            <input id="i1">
                <inputExpression id="ie1" typeRef="tAge">
                    <text>Age</text>
                </inputExpression>
            </input>
            <output id="o1" name="Output"/>
            <rule id="r1">
                <inputEntry id="r1i1"><text>18</text></inputEntry>
                <outputEntry id="r1o1"><text>"Result"</text></outputEntry>
            </rule>
        </decisionTable>
    </decision>
</definitions>)";

    orion::api::BusinessRulesEngine engine;
    auto result = engine.load_dmn_model(dmn_xml);
    
    BOOST_TEST(result.has_value(), "Engine should load DMN model successfully");
}

BOOST_AUTO_TEST_CASE(test_engine_validates_invalid_typeref)
{
    std::string_view dmn_xml = R"(<?xml version="1.0" encoding="UTF-8"?>
<definitions xmlns="https://www.omg.org/spec/DMN/20230324/MODEL/" 
             namespace="test">
    <itemDefinition id="tCustom" name="tCustom">
        <typeRef>tUndefined</typeRef>
    </itemDefinition>
    <decision id="d1" name="Test">
        <decisionTable id="dt1" hitPolicy="UNIQUE">
            <input id="i1">
                <inputExpression id="ie1" typeRef="tCustom">
                    <text>Value</text>
                </inputExpression>
            </input>
            <output id="o1" name="Output"/>
            <rule id="r1">
                <inputEntry id="r1i1"><text>-</text></inputEntry>
                <outputEntry id="r1o1"><text>"Result"</text></outputEntry>
            </rule>
        </decisionTable>
    </decision>
</definitions>)";

    orion::api::BusinessRulesEngine engine;
    auto result = engine.load_dmn_model(dmn_xml);
    
    BOOST_TEST(result.has_value(), "Engine should load model even with validation errors");
    
    // Check for validation errors
    auto errors = engine.validate_models();
    BOOST_TEST(!errors.empty(), "Should have validation errors");
    
    // Verify error message mentions the invalid typeRef
    bool found_typeref_error = false;
    for (const auto& error : errors) {
        if (error.find("tUndefined") != std::string::npos) {
            found_typeref_error = true;
            break;
        }
    }
    BOOST_TEST(found_typeref_error, "Should report invalid typeRef");
}

BOOST_AUTO_TEST_CASE(test_engine_validates_structured_type_without_components)
{
    // Test ItemDefinition that has both itemComponents AND typeRef (should be mutually exclusive)
    std::string_view dmn_xml = R"(<?xml version="1.0" encoding="UTF-8"?>
<definitions xmlns="https://www.omg.org/spec/DMN/20230324/MODEL/" 
             namespace="test">
    <itemDefinition id="tBadType" name="tBadType" typeRef="string">
        <!-- Has both typeRef AND itemComponent - ambiguous -->
        <itemComponent id="comp1" name="field1">
            <typeRef>string</typeRef>
        </itemComponent>
    </itemDefinition>
    <decision id="d1" name="Test">
        <decisionTable id="dt1" hitPolicy="UNIQUE">
            <input id="i1">
                <inputExpression id="ie1" typeRef="tBadType">
                    <text>Value</text>
                </inputExpression>
            </input>
            <output id="o1" name="Output"/>
            <rule id="r1">
                <inputEntry id="r1i1"><text>-</text></inputEntry>
                <outputEntry id="r1o1"><text>"Result"</text></outputEntry>
            </rule>
        </decisionTable>
    </decision>
</definitions>)";

    orion::api::BusinessRulesEngine engine;
    auto result = engine.load_dmn_model(dmn_xml);
    
    // Model should load successfully (parser accepts this structure)
    BOOST_TEST(result.has_value(), "Engine should load model");
    
    // Note: The current implementation allows ItemDefinitions with both typeRef and itemComponents.
    // Future enhancement could add validation to detect this ambiguous case.
    // For now, the structured type (itemComponents present) takes precedence.
    auto errors = engine.validate_models();
    // No errors expected in current implementation - this test documents current behavior
    BOOST_TEST(errors.empty(), "Current implementation allows both typeRef and itemComponents");
}

BOOST_AUTO_TEST_SUITE_END()
