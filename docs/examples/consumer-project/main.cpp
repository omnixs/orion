#include <orion/api/engine.hpp>
#include <nlohmann/json.hpp>
#include <iostream>
#include <string>

int main() {
    std::cout << "=== Orion Consumer Example ===" << std::endl;
    
    // Create the business rules engine
    orion::api::BusinessRulesEngine engine;
    std::cout << "Engine created successfully!" << std::endl;
    
    // Example DMN XML (simple decision table)
    const std::string dmn_xml = R"(<?xml version="1.0" encoding="UTF-8"?>
<definitions xmlns="https://www.omg.org/spec/DMN/20191111/MODEL/"
             xmlns:dmndi="https://www.omg.org/spec/DMN/20191111/DMNDI/"
             xmlns:dc="http://www.omg.org/spec/DMN/20180521/DC/"
             id="definitions_001"
             name="Simple Decision"
             namespace="http://camunda.org/schema/1.0/dmn">
  <decision id="decision_001" name="Age Category">
    <decisionTable id="decisionTable_001" hitPolicy="FIRST">
      <input id="input_1" label="Age">
        <inputExpression id="inputExpression_1" typeRef="number">
          <text>age</text>
        </inputExpression>
      </input>
      <output id="output_1" label="Category" name="category" typeRef="string"/>
      <rule id="rule_1">
        <inputEntry id="inputEntry_1_1">
          <text>&lt; 18</text>
        </inputEntry>
        <outputEntry id="outputEntry_1_1">
          <text>"Minor"</text>
        </outputEntry>
      </rule>
      <rule id="rule_2">
        <inputEntry id="inputEntry_2_1">
          <text>[18..65)</text>
        </inputEntry>
        <outputEntry id="outputEntry_2_1">
          <text>"Adult"</text>
        </outputEntry>
      </rule>
      <rule id="rule_3">
        <inputEntry id="inputEntry_3_1">
          <text>&gt;= 65</text>
        </inputEntry>
        <outputEntry id="outputEntry_3_1">
          <text>"Senior"</text>
        </outputEntry>
      </rule>
    </decisionTable>
  </decision>
</definitions>)";
    
    // Load DMN model
    auto load_result = engine.load_dmn_model(dmn_xml);
    if (!load_result) {
        std::cerr << "Failed to load DMN model: " << load_result.error() << std::endl;
        return 1;
    }
    std::cout << "DMN model loaded successfully!" << std::endl;
    
    // Test cases
    std::vector<int> test_ages = {10, 25, 70};
    
    for (int age : test_ages) {
        std::cout << "\n--- Testing age: " << age << " ---" << std::endl;
        
        // Create input context
        nlohmann::json context = {
            {"age", age}
        };
        
        try {
            // Evaluate the decision
            std::string result_json = engine.evaluate(context.dump());
            
            std::cout << "Result: " << result_json << std::endl;
            
            // Parse result to extract category
            auto result = nlohmann::json::parse(result_json);
            if (result.is_object() && result.contains("category")) {
                std::cout << "Category: " << result["category"] << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "Error evaluating decision: " << e.what() << std::endl;
        }
    }
    
    std::cout << "\n=== Example completed ===" << std::endl;
    return 0;
}
