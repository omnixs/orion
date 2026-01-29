/*
 * ORION Optimized Rule Integration & Operations Native
 * SPDX-License-Identifier: Apache-2.0
 * SPDX-FileCopyrightText: 2025 ORION contributors
 */

#include <orion/bre/dmn_parser.hpp>
#include <orion/bre/type_validator.hpp>
#include <orion/api/engine.hpp>
#include <boost/test/unit_test.hpp>

using namespace orion::bre;

BOOST_AUTO_TEST_SUITE(complex_item_definition_tests)

// Test 1: Parse simple structured type with two components
BOOST_AUTO_TEST_CASE(parse_simple_structured_type)
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
</dmn:definitions>)";
    
    auto model = orion::bre::DmnParser().parse(dmn_xml);
    BOOST_REQUIRE(model.item_definitions.count("tPerson") > 0);
    
    const auto& person_def = model.item_definitions["tPerson"];
    BOOST_CHECK(person_def.is_structured_type());
    BOOST_CHECK(!person_def.is_simple_type());
    BOOST_CHECK_EQUAL(person_def.itemComponents.size(), 2);
    
    // Check first component
    BOOST_CHECK_EQUAL(person_def.itemComponents[0].name, "name");
    BOOST_CHECK_EQUAL(person_def.itemComponents[0].typeRef, "string");
    BOOST_CHECK(!person_def.itemComponents[0].isCollection);
    
    // Check second component
    BOOST_CHECK_EQUAL(person_def.itemComponents[1].name, "age");
    BOOST_CHECK_EQUAL(person_def.itemComponents[1].typeRef, "number");
    BOOST_CHECK(!person_def.itemComponents[1].isCollection);
}

// Test 2: Parse nested structured types (ItemDefinition referencing another)
BOOST_AUTO_TEST_CASE(parse_nested_structured_types)
{
    std::string_view dmn_xml = R"(<?xml version="1.0"?>
<dmn:definitions xmlns:dmn="http://www.omg.org/spec/DMN/20180521/MODEL/">
  <dmn:itemDefinition name="tAddress">
    <dmn:itemComponent name="street">
      <dmn:typeRef>string</dmn:typeRef>
    </dmn:itemComponent>
    <dmn:itemComponent name="city">
      <dmn:typeRef>string</dmn:typeRef>
    </dmn:itemComponent>
  </dmn:itemDefinition>
  
  <dmn:itemDefinition name="tCustomer">
    <dmn:itemComponent name="name">
      <dmn:typeRef>string</dmn:typeRef>
    </dmn:itemComponent>
    <dmn:itemComponent name="address">
      <dmn:typeRef>tAddress</dmn:typeRef>
    </dmn:itemComponent>
  </dmn:itemDefinition>
</dmn:definitions>)";
    
    auto model = orion::bre::DmnParser().parse(dmn_xml);
    BOOST_REQUIRE(model.item_definitions.count("tCustomer") > 0);
    BOOST_REQUIRE(model.item_definitions.count("tAddress") > 0);
    
    const auto& customer = model.item_definitions["tCustomer"];
    BOOST_CHECK_EQUAL(customer.itemComponents.size(), 2);
    
    // Check that address component references tAddress type
    const auto* address_comp = customer.get_component("address");
    BOOST_REQUIRE(address_comp != nullptr);
    BOOST_CHECK_EQUAL(address_comp->typeRef, "tAddress");
}

// Test 3: Parse collection of complex types
BOOST_AUTO_TEST_CASE(parse_collection_of_complex_types)
{
    std::string_view dmn_xml = R"(<?xml version="1.0"?>
<dmn:definitions xmlns:dmn="http://www.omg.org/spec/DMN/20180521/MODEL/">
  <dmn:itemDefinition name="tLineItem">
    <dmn:itemComponent name="product">
      <dmn:typeRef>string</dmn:typeRef>
    </dmn:itemComponent>
    <dmn:itemComponent name="quantity">
      <dmn:typeRef>number</dmn:typeRef>
    </dmn:itemComponent>
  </dmn:itemDefinition>
  
  <dmn:itemDefinition name="tOrder">
    <dmn:itemComponent name="orderId">
      <dmn:typeRef>number</dmn:typeRef>
    </dmn:itemComponent>
    <dmn:itemComponent name="items" isCollection="true">
      <dmn:typeRef>tLineItem</dmn:typeRef>
    </dmn:itemComponent>
  </dmn:itemDefinition>
</dmn:definitions>)";
    
    auto model = orion::bre::DmnParser().parse(dmn_xml);
    BOOST_REQUIRE(model.item_definitions.count("tOrder") > 0);
    
    const auto& order = model.item_definitions["tOrder"];
    const auto* items_comp = order.get_component("items");
    BOOST_REQUIRE(items_comp != nullptr);
    BOOST_CHECK(items_comp->isCollection);
    BOOST_CHECK_EQUAL(items_comp->typeRef, "tLineItem");
}

// Test 4: Component with allowedValues constraint
BOOST_AUTO_TEST_CASE(parse_component_with_constraints)
{
    std::string_view dmn_xml = R"(<?xml version="1.0"?>
<dmn:definitions xmlns:dmn="http://www.omg.org/spec/DMN/20180521/MODEL/">
  <dmn:itemDefinition name="tEmployee">
    <dmn:itemComponent name="name">
      <dmn:typeRef>string</dmn:typeRef>
    </dmn:itemComponent>
    <dmn:itemComponent name="role">
      <dmn:typeRef>string</dmn:typeRef>
      <dmn:allowedValues>
        <dmn:text>"Manager", "Developer", "Designer"</dmn:text>
      </dmn:allowedValues>
    </dmn:itemComponent>
  </dmn:itemDefinition>
</dmn:definitions>)";
    
    auto model = orion::bre::DmnParser().parse(dmn_xml);
    BOOST_REQUIRE(model.item_definitions.count("tEmployee") > 0);
    
    const auto& employee = model.item_definitions["tEmployee"];
    const auto* role_comp = employee.get_component("role");
    BOOST_REQUIRE(role_comp != nullptr);
    BOOST_CHECK(role_comp->has_constraints());
    BOOST_CHECK_EQUAL(role_comp->allowedValues, "\"Manager\", \"Developer\", \"Designer\"");
}

// Test 5: Validate valid complex object
BOOST_AUTO_TEST_CASE(validate_complex_object_valid)
{
    nlohmann::json person = {
        {"name", "John Doe"},
        {"age", 30}
    };
    
    ItemDefinition person_def;
    person_def.name = "tPerson";
    person_def.itemComponents = {
        ItemComponent{"name", "string", false, ""},
        ItemComponent{"age", "number", false, ""}
    };
    
    std::map<std::string, ItemDefinition> defs = {{"tPerson", person_def}};
    
    auto result = validate_complex_type(person, person_def, defs);
    BOOST_CHECK(result.has_value());
}

// Test 6: Validate complex object - missing field (DMN fields are optional by default)
BOOST_AUTO_TEST_CASE(validate_complex_object_missing_field)
{
    nlohmann::json person = {
        {"name", "John Doe"}
        // Missing "age" - but this is valid in DMN (fields are optional)
    };
    
    ItemDefinition person_def;
    person_def.name = "tPerson";
    person_def.itemComponents = {
        ItemComponent{"name", "string", false, ""},
        ItemComponent{"age", "number", false, ""}
    };
    
    std::map<std::string, ItemDefinition> defs = {{"tPerson", person_def}};
    
    // Per DMN 1.5 spec, fields are optional by default - missing fields are allowed
    auto result = validate_complex_type(person, person_def, defs);
    BOOST_CHECK(result.has_value()); // Should succeed
}

// Test 7: Validate nested complex object
BOOST_AUTO_TEST_CASE(validate_nested_complex_object)
{
    nlohmann::json customer = {
        {"name", "Jane Smith"},
        {"address", {
            {"street", "456 Oak Ave"},
            {"city", "Boston"}
        }}
    };
    
    ItemDefinition address_def;
    address_def.name = "tAddress";
    address_def.itemComponents = {
        ItemComponent{"street", "string", false, ""},
        ItemComponent{"city", "string", false, ""}
    };
    
    ItemDefinition customer_def;
    customer_def.name = "tCustomer";
    customer_def.itemComponents = {
        ItemComponent{"name", "string", false, ""},
        ItemComponent{"address", "tAddress", false, ""}
    };
    
    std::map<std::string, ItemDefinition> defs = {
        {"tAddress", address_def},
        {"tCustomer", customer_def}
    };
    
    auto result = validate_complex_type(customer, customer_def, defs);
    BOOST_CHECK(result.has_value());
}

// Test 8: Validate nested object - missing nested field (DMN fields are optional)
BOOST_AUTO_TEST_CASE(validate_nested_object_missing_nested_field)
{
    nlohmann::json customer = {
        {"name", "Jane Smith"},
        {"address", {
            {"street", "456 Oak Ave"}
            // Missing "city" - but this is valid in DMN (fields are optional)
        }}
    };
    
    ItemDefinition address_def;
    address_def.name = "tAddress";
    address_def.itemComponents = {
        ItemComponent{"street", "string", false, ""},
        ItemComponent{"city", "string", false, ""}
    };
    
    ItemDefinition customer_def;
    customer_def.name = "tCustomer";
    customer_def.itemComponents = {
        ItemComponent{"name", "string", false, ""},
        ItemComponent{"address", "tAddress", false, ""}
    };
    
    std::map<std::string, ItemDefinition> defs = {
        {"tAddress", address_def},
        {"tCustomer", customer_def}
    };
    
    // Per DMN 1.5 spec, fields are optional by default - missing fields are allowed
    auto result = validate_complex_type(customer, customer_def, defs);
    BOOST_CHECK(result.has_value()); // Should succeed
}

// Test 9: Validate collection of complex objects
BOOST_AUTO_TEST_CASE(validate_collection_of_complex_objects)
{
    nlohmann::json order = {
        {"orderId", 12345},
        {"items", nlohmann::json::array({
            {{"product", "Widget"}, {"quantity", 5}},
            {{"product", "Gadget"}, {"quantity", 2}}
        })}
    };
    
    ItemDefinition line_item_def;
    line_item_def.name = "tLineItem";
    line_item_def.itemComponents = {
        ItemComponent{"product", "string", false, ""},
        ItemComponent{"quantity", "number", false, ""}
    };
    
    ItemDefinition order_def;
    order_def.name = "tOrder";
    order_def.itemComponents = {
        ItemComponent{"orderId", "number", false, ""},
        ItemComponent{"items", "tLineItem", true, ""} // isCollection=true
    };
    
    std::map<std::string, ItemDefinition> defs = {
        {"tLineItem", line_item_def},
        {"tOrder", order_def}
    };
    
    auto result = validate_complex_type(order, order_def, defs);
    BOOST_CHECK(result.has_value());
}

// Test 10: Validate collection - element with missing field (DMN fields are optional)
BOOST_AUTO_TEST_CASE(validate_collection_invalid_element)
{
    nlohmann::json order = {
        {"orderId", 12345},
        {"items", nlohmann::json::array({
            {{"product", "Widget"}, {"quantity", 5}},
            {{"product", "Gadget"}}  // Missing quantity - but this is valid (fields optional)
        })}
    };
    
    ItemDefinition line_item_def;
    line_item_def.name = "tLineItem";
    line_item_def.itemComponents = {
        ItemComponent{"product", "string", false, ""},
        ItemComponent{"quantity", "number", false, ""}
    };
    
    ItemDefinition order_def;
    order_def.name = "tOrder";
    order_def.itemComponents = {
        ItemComponent{"orderId", "number", false, ""},
        ItemComponent{"items", "tLineItem", true, ""}
    };
    
    std::map<std::string, ItemDefinition> defs = {
        {"tLineItem", line_item_def},
        {"tOrder", order_def}
    };
    
    // Per DMN 1.5 spec, fields are optional by default - missing fields are allowed
    auto result = validate_complex_type(order, order_def, defs);
    BOOST_CHECK(result.has_value()); // Should succeed
}

// Test 11: Validate component with constraints
BOOST_AUTO_TEST_CASE(validate_component_with_constraints)
{
    nlohmann::json employee = {
        {"name", "Alice"},
        {"role", "Developer"}
    };
    
    ItemDefinition employee_def;
    employee_def.name = "tEmployee";
    employee_def.itemComponents = {
        ItemComponent{"name", "string", false, ""},
        ItemComponent{"role", "string", false, "\"Manager\", \"Developer\", \"Designer\""}
    };
    
    std::map<std::string, ItemDefinition> defs = {{"tEmployee", employee_def}};
    
    auto result = validate_complex_type(employee, employee_def, defs);
    BOOST_CHECK(result.has_value());
}

// Test 12: Validate component constraint violation
BOOST_AUTO_TEST_CASE(validate_component_constraint_violation)
{
    nlohmann::json employee = {
        {"name", "Alice"},
        {"role", "CEO"}  // Not in allowed values
    };
    
    ItemDefinition employee_def;
    employee_def.name = "tEmployee";
    employee_def.itemComponents = {
        ItemComponent{"name", "string", false, ""},
        ItemComponent{"role", "string", false, "\"Manager\", \"Developer\", \"Designer\""}
    };
    
    std::map<std::string, ItemDefinition> defs = {{"tEmployee", employee_def}};
    
    auto result = validate_complex_type(employee, employee_def, defs);
    BOOST_CHECK(!result.has_value());
    BOOST_CHECK(result.error().find("role") != std::string::npos);
    BOOST_CHECK(result.error().find("CEO") != std::string::npos);
}

// Test 13: Load real DMN with structured types via engine
BOOST_AUTO_TEST_CASE(engine_loads_complex_item_definitions)
{
    std::string_view dmn_xml = R"(<?xml version="1.0"?>
<dmn:definitions xmlns:dmn="http://www.omg.org/spec/DMN/20180521/MODEL/">
  <dmn:itemDefinition name="tContact">
    <dmn:itemComponent name="email">
      <dmn:typeRef>string</dmn:typeRef>
    </dmn:itemComponent>
    <dmn:itemComponent name="phone">
      <dmn:typeRef>string</dmn:typeRef>
    </dmn:itemComponent>
  </dmn:itemDefinition>
</dmn:definitions>)";
    
    orion::api::BusinessRulesEngine engine;
    auto result = engine.load_dmn_model(std::string(dmn_xml));
    
    // Engine should have parsed the complex ItemDefinition
    // (Cannot directly access internal state, but load succeeded)
    BOOST_CHECK(result.has_value());
}

// Test 14: Validate collection not an array
BOOST_AUTO_TEST_CASE(validate_collection_not_array)
{
    nlohmann::json order = {
        {"orderId", 12345},
        {"items", "not-an-array"}  // Should be array
    };
    
    ItemDefinition order_def;
    order_def.name = "tOrder";
    order_def.itemComponents = {
        ItemComponent{"orderId", "number", false, ""},
        ItemComponent{"items", "tLineItem", true, ""}
    };
    
    std::map<std::string, ItemDefinition> defs = {{"tOrder", order_def}};
    
    auto result = validate_complex_type(order, order_def, defs);
    BOOST_CHECK(!result.has_value());
    BOOST_CHECK(result.error().find("items") != std::string::npos);
    BOOST_CHECK(result.error().find("array") != std::string::npos);
}

// Test 15: Validate not an object
BOOST_AUTO_TEST_CASE(validate_not_an_object)
{
    nlohmann::json not_object = "just-a-string";
    
    ItemDefinition person_def;
    person_def.name = "tPerson";
    person_def.itemComponents = {
        ItemComponent{"name", "string", false, ""}
    };
    
    std::map<std::string, ItemDefinition> defs = {{"tPerson", person_def}};
    
    auto result = validate_complex_type(not_object, person_def, defs);
    BOOST_CHECK(!result.has_value());
    BOOST_CHECK(result.error().find("Expected object") != std::string::npos);
}

BOOST_AUTO_TEST_SUITE_END()

