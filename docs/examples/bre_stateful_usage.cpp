// Example usage of the new stateful BRE engine
#include "orion/bre/engine_v2.hpp"
#include <iostream>
#include <fstream>

using namespace orion::bre;

int main() {
    // Example 1: One-time evaluation (similar to old API)
    std::string dmn_xml = R"(<?xml version="1.0" encoding="UTF-8"?>
        <definitions xmlns="https://www.omg.org/spec/DMN/20191111/MODEL/">
            <decision name="Greeting Message">
                <literalExpression>
                    <text>"Hello " + Full Name</text>
                </literalExpression>
            </decision>
        </definitions>)";
    
    std::string data_json = R"({"Full Name": "John Doe"})";
    
    std::string result = evaluate(dmn_xml, data_json);
    std::cout << "One-time evaluation: " << result << std::endl;
    
    // Example 2: Stateful engine for better performance
    BusinessRulesEngine engine;
    std::string error_message;
    
    if (!engine.loadDmnModel(dmn_xml, error_message)) {
        std::cerr << "Failed to load DMN model: " << error_message << std::endl;
        return 1;
    }
    
    // Now we can evaluate multiple times without re-parsing
    std::cout << "Pre-parsed evaluations:" << std::endl;
    
    std::string data1 = R"({"Full Name": "Alice Smith"})";
    std::cout << "  Alice: " << engine.evaluate(data1) << std::endl;
    
    std::string data2 = R"({"Full Name": "Bob Johnson"})";
    std::cout << "  Bob: " << engine.evaluate(data2) << std::endl;
    
    // Example 3: Introspection
    std::cout << "\nLoaded models:" << std::endl;
    
    auto decisions = engine.getLiteralDecisionNames();
    std::cout << "  Literal Decisions: ";
    for (const auto& name : decisions) {
        std::cout << name << " ";
    }
    std::cout << std::endl;
    
    auto tables = engine.getDecisionTableNames();
    std::cout << "  Decision Tables: ";
    for (const auto& name : tables) {
        std::cout << name << " ";
    }
    std::cout << std::endl;
    
    auto bkms = engine.getBusinessKnowledgeModelNames();
    std::cout << "  BKMs: ";
    for (const auto& name : bkms) {
        std::cout << name << " ";
    }
    std::cout << std::endl;
    
    // Example 4: Dynamic model management
    // You could load additional models, remove specific rules, etc.
    
    // Example 5: Rule validation
    auto validation_errors = engine.validateModels();
    if (!validation_errors.empty()) {
        std::cout << "\nValidation errors:" << std::endl;
        for (const auto& error : validation_errors) {
            std::cout << "  - " << error << std::endl;
        }
    }
    
    return 0;
}