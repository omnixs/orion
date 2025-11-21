#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace orion::common
{
    /// Structure representing a parsed test case expectation
    struct OutputExpectation
        {
            std::string name; ///< Decision name
            std::string id; ///< Test case ID  
            std::string expected; ///< Expected result as JSON string
        };

        /// Structure representing a parsed DMN test case
        struct ParsedCase
        {
            std::string id; ///< Test case ID
            nlohmann::json input; ///< Input data as JSON
            std::vector<OutputExpectation> outputs; ///< Expected outputs
        };

    /// Parse XML value with type information into JSON
    /// @param value The text content of the XML value
    /// @param xsiType The xsi:type attribute value (e.g., "xsd:decimal", "xsd:string")
    /// @return JSON representation of the value
    nlohmann::json parse_xml_value(const std::string& value, const std::string& xsiType);

    /// Parse DMN TCK test XML into structured test cases
    /// @param xml The complete XML content of a DMN test file
    /// @return Vector of parsed test cases with input/output expectations
    std::vector<ParsedCase> parse_test_xml(const std::string& xml);

    /// Extract expected outputs from a specific test case XML node
    /// @param testCaseXml XML content of a single test case
    /// @return Vector of output expectations for this test case
    std::vector<OutputExpectation> parse_output_expectations(const std::string& testCaseXml);

    /// Convert component structure to JSON object
    /// @param componentXml XML content containing component definitions
    /// @return JSON object with component name-value pairs
    nlohmann::json parse_component_structure(const std::string& componentXml);
} // namespace orion::common