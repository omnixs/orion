// Test to verify regex cache memory cleanup with engine-scoped cache
#include <orion/api/engine.hpp>
#include <iostream>

int main() {
    std::cout << "Testing engine-scoped regex cache cleanup..." << std::endl;
    
    // Create and destroy 10 engine instances
    // Each should have its own cache that gets cleaned up
    for (int i = 0; i < 10; ++i) {
        orion::api::BusinessRulesEngine engine;
        
        // Load a DMN model that uses matches() function
        std::string dmn_xml = R"(<?xml version="1.0" encoding="UTF-8"?>
<definitions xmlns="https://www.omg.org/spec/DMN/20191111/MODEL/" 
             xmlns:feel="https://www.omg.org/spec/DMN/20191111/FEEL/"
             id="test" name="Test" namespace="http://test">
  <decision id="d1" name="TestDecision">
    <decisionTable id="dt1" hitPolicy="FIRST">
      <input id="i1">
        <inputExpression typeRef="string">
          <text>input</text>
        </inputExpression>
      </input>
      <output id="o1" typeRef="boolean"/>
      <rule id="r1">
        <inputEntry>
          <text>matches("test.*")</text>
        </inputEntry>
        <outputEntry>
          <text>true</text>
        </outputEntry>
      </rule>
    </decisionTable>
  </decision>
</definitions>)";
        
        auto result = engine.load_dmn_model(dmn_xml);
        if (!result) {
            std::cerr << "Failed to load model: " << result.error() << std::endl;
            return 1;
        }
        
        // Evaluate to trigger regex compilation
        std::string eval_result = engine.evaluate(R"({"input": "test123"})");
        std::cout << "  Engine " << i+1 << " evaluation: " << eval_result << std::endl;
        
        // Engine destructor will clean up regex_cache_ here
    }
    
    std::cout << "\nAll 10 engines created and destroyed successfully." << std::endl;
    std::cout << "Each engine's regex cache should have been cleaned up." << std::endl;
    std::cout << "If you see PCRE2 leak reports, the fix didn't work." << std::endl;
    std::cout << "If no leak reports, the engine-scoped cache architecture works!" << std::endl;
    
    return 0;
}
