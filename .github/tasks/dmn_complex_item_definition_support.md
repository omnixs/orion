---
template: add_dmn_feature.md
agent: general
status: in-progress
category: feature
priority: high
estimated-effort: "6-8 hours"
actual-effort: ""
created: 2026-01-13
---

# Task: Implement DMN 1.5 Complex ItemDefinition Support (itemComponents)

Extend ItemDefinition support to handle complex/structured types with nested components, enabling rich domain models in ORION.

## Context

**Background:**
- Basic ItemDefinition support implemented (simple types + enumerations)
- DMN 1.5 Section 7.3.3 defines itemComponent for structured data modeling
- Real-world DMN models often require complex nested objects (Customer, Order, etc.)
- Need recursive type resolution and validation

**What are ItemComponents?**

From DMN 1.5 Specification (Section 7.3.3.1):
> "An ItemDefinition can be composed of other ItemDefinitions using itemComponent elements. Each itemComponent defines a named property with its own typeRef."

**Examples:**

1. **Simple Structured Type:**
```xml
<dmn:itemDefinition name="tCustomer">
  <dmn:itemComponent name="name">
    <dmn:typeRef>string</dmn:typeRef>
  </dmn:itemComponent>
  <dmn:itemComponent name="age">
    <dmn:typeRef>number</dmn:typeRef>
  </dmn:itemComponent>
</dmn:itemDefinition>
```

2. **Nested Structured Types:**
```xml
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
```

3. **Collections of Complex Types:**
```xml
<dmn:itemDefinition name="tOrder">
  <dmn:itemComponent name="lineItems" isCollection="true">
    <dmn:typeRef>tLineItem</dmn:typeRef>
  </dmn:itemComponent>
</dmn:itemDefinition>
```

## Scope

**Included:**
- [ ] ItemComponent data structure
- [ ] Parse itemComponent XML elements
- [ ] Recursive type resolution (handle typeRef to other ItemDefinitions)
- [ ] Validate JSON objects against complex structure
- [ ] Support nested objects (multiple levels deep)
- [ ] Support collections of complex types
- [ ] Component-level allowedValues constraints
- [ ] Comprehensive test coverage

**Excluded:**
- Function signatures (separate feature)
- Automatic validation during evaluation (separate integration task)

## Implementation Plan

### Step 1: Data Structure Extension

**Extend `ItemDefinition` struct in `dmn_model.hpp`:**

```cpp
struct ItemComponent {
    std::string name;
    std::string typeRef;
    bool isCollection{false};
    std::string allowedValues;  // Component-level constraints
    
    [[nodiscard]] bool has_constraints() const noexcept {
        return !allowedValues.empty();
    }
};

struct ItemDefinition {
    std::string name;
    std::string id;
    std::string label;
    std::string typeRef;
    std::string allowedValues;
    bool isCollection{false};
    
    // NEW: Components for structured types
    std::vector<ItemComponent> itemComponents;
    
    // Existing helper methods
    [[nodiscard]] bool has_constraints() const noexcept;
    [[nodiscard]] bool is_simple_type() const noexcept;
    
    // NEW: Check if this is a structured type
    [[nodiscard]] bool is_structured_type() const noexcept {
        return !itemComponents.empty();
    }
    
    // NEW: Get component by name
    [[nodiscard]] const ItemComponent* get_component(std::string_view name) const;
};
```

### Step 2: Parser Extension

**Update `dmn_parser.cpp` to parse itemComponents:**

```cpp
// Inside ItemDefinition parsing loop (after parsing allowedValues)
for (auto* child = item_def->first_node(); child; child = child->next_sibling()) {
    if (matches_element(child, "itemComponent")) {
        ItemComponent component;
        
        // Parse component attributes
        if (auto* name_attr = child->first_attribute("name")) {
            component.name = name_attr->value();
        }
        if (auto* coll_attr = child->first_attribute("isCollection")) {
            component.isCollection = (std::string_view(coll_attr->value()) == "true");
        }
        
        // Parse component child elements
        for (auto* comp_child = child->first_node(); comp_child; comp_child = comp_child->next_sibling()) {
            if (matches_element(comp_child, "typeRef")) {
                component.typeRef = comp_child->value();
            } else if (matches_element(comp_child, "allowedValues")) {
                // Component-level constraints
                for (auto* text = comp_child->first_node(); text; text = text->next_sibling()) {
                    if (matches_element(text, "text")) {
                        component.allowedValues = text->value();
                        break;
                    }
                }
            }
        }
        
        def.itemComponents.push_back(std::move(component));
    }
}
```

### Step 3: Validation Enhancement

**Add complex type validation to `type_validator.cpp`:**

```cpp
namespace orion::bre {

// Forward declaration for recursion
std::expected<void, std::string> validate_complex_type(
    const nlohmann::json& value,
    const ItemDefinition& item_def,
    const std::map<std::string, ItemDefinition>& all_definitions
);

// Validate a single component value
std::expected<void, std::string> validate_component(
    const nlohmann::json& comp_value,
    const ItemComponent& component,
    const std::map<std::string, ItemDefinition>& all_definitions
) {
    // Check if component has constraints
    if (component.has_constraints()) {
        auto values = parse_allowed_values(component.allowedValues);
        bool valid = false;
        
        if (comp_value.is_string()) {
            valid = std::find(values.begin(), values.end(), comp_value.get<std::string>()) != values.end();
        }
        
        if (!valid) {
            return std::unexpected(
                std::format("Component '{}' value not in allowed values", component.name)
            );
        }
    }
    
    // Check if typeRef points to another ItemDefinition (complex type)
    if (all_definitions.contains(component.typeRef)) {
        const auto& nested_def = all_definitions.at(component.typeRef);
        if (nested_def.is_structured_type()) {
            // Recursive validation for nested complex type
            return validate_complex_type(comp_value, nested_def, all_definitions);
        }
    }
    
    // Basic type validation (string, number, boolean)
    // TODO: Add type checking logic
    
    return {};
}

std::expected<void, std::string> validate_complex_type(
    const nlohmann::json& value,
    const ItemDefinition& item_def,
    const std::map<std::string, ItemDefinition>& all_definitions
) {
    if (!value.is_object()) {
        return std::unexpected(
            std::format("Expected object for structured type '{}', got {}", 
                       item_def.name, value.type_name())
        );
    }
    
    // Validate each component
    for (const auto& component : item_def.itemComponents) {
        if (!value.contains(component.name)) {
            return std::unexpected(
                std::format("Missing required component '{}' in structured type '{}'",
                           component.name, item_def.name)
            );
        }
        
        const auto& comp_value = value[component.name];
        
        // Handle collections
        if (component.isCollection) {
            if (!comp_value.is_array()) {
                return std::unexpected(
                    std::format("Component '{}' must be an array", component.name)
                );
            }
            
            // Validate each element in collection
            for (const auto& element : comp_value) {
                auto result = validate_component(element, component, all_definitions);
                if (!result) {
                    return result;
                }
            }
        } else {
            // Validate single value
            auto result = validate_component(comp_value, component, all_definitions);
            if (!result) {
                return result;
            }
        }
    }
    
    return {};
}

} // namespace orion::bre
```

### Step 4: Testing Strategy

**Create `tst/bre/test_complex_item_definition.cpp`:**

1. **Test Simple Structured Type:**
```cpp
BOOST_AUTO_TEST_CASE(parse_simple_structured_type) {
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
    BOOST_CHECK_EQUAL(person_def.itemComponents.size(), 2);
    BOOST_CHECK_EQUAL(person_def.itemComponents[0].name, "name");
    BOOST_CHECK_EQUAL(person_def.itemComponents[0].typeRef, "string");
    BOOST_CHECK_EQUAL(person_def.itemComponents[1].name, "age");
    BOOST_CHECK_EQUAL(person_def.itemComponents[1].typeRef, "number");
}
```

2. **Test Nested Structured Types:**
```cpp
BOOST_AUTO_TEST_CASE(parse_nested_structured_types) {
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
    const auto* address_comp = customer.get_component("address");
    BOOST_REQUIRE(address_comp != nullptr);
    BOOST_CHECK_EQUAL(address_comp->typeRef, "tAddress");
}
```

3. **Test Collection of Complex Types:**
```cpp
BOOST_AUTO_TEST_CASE(parse_collection_of_complex_types) {
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
    const auto& order = model.item_definitions["tOrder"];
    const auto* items_comp = order.get_component("items");
    BOOST_REQUIRE(items_comp != nullptr);
    BOOST_CHECK(items_comp->isCollection);
    BOOST_CHECK_EQUAL(items_comp->typeRef, "tLineItem");
}
```

4. **Test Validation:**
```cpp
BOOST_AUTO_TEST_CASE(validate_complex_object_valid) {
    nlohmann::json person = {
        {"name", "John Doe"},
        {"age", 30}
    };
    
    ItemDefinition person_def;
    person_def.name = "tPerson";
    person_def.itemComponents = {
        {"name", "string", false, ""},
        {"age", "number", false, ""}
    };
    
    std::map<std::string, ItemDefinition> defs = {{"tPerson", person_def}};
    auto result = validate_complex_type(person, person_def, defs);
    BOOST_CHECK(result.has_value());
}

BOOST_AUTO_TEST_CASE(validate_complex_object_missing_field) {
    nlohmann::json person = {
        {"name", "John Doe"}
        // Missing "age"
    };
    
    ItemDefinition person_def;
    person_def.name = "tPerson";
    person_def.itemComponents = {
        {"name", "string", false, ""},
        {"age", "number", false, ""}
    };
    
    std::map<std::string, ItemDefinition> defs = {{"tPerson", person_def}};
    auto result = validate_complex_type(person, person_def, defs);
    BOOST_CHECK(!result.has_value());
    BOOST_CHECK(result.error().find("Missing required component") != std::string::npos);
}
```

## Success Criteria

- [ ] ItemComponent struct defined and integrated
- [ ] Parse itemComponent XML elements with all attributes
- [ ] Recursive type resolution works (nested ItemDefinitions)
- [ ] Validate JSON objects against complex structure
- [ ] Collection validation works for arrays of complex types
- [ ] Component-level allowedValues constraints enforced
- [ ] All tests pass (target: 15+ new test cases)
- [ ] No regressions in existing ItemDefinition tests
- [ ] Code follows CODING_STANDARDS.md

## Reference Documentation

- DMN 1.5 Specification Section 7.3.3.1 "ItemDefinition metamodel"
- [CODING_STANDARDS.md](../../CODING_STANDARDS.md)
- [Build Instructions](../instructions/build.md)
- [Unit Test Instructions](../instructions/run_unit_tests.md)
- Previous task: [dmn_custom_data_types_feature.md](./dmn_custom_data_types_feature.md)

## Retrospective

(To be filled after completion)

