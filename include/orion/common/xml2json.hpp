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
    nlohmann::json parse_xml_value(std::string_view value, std::string_view xsiType);

    /// Parse DMN TCK test XML into structured test cases
    /// @param xml The complete XML content of a DMN test file
    /// @return Vector of parsed test cases with input/output expectations
    std::vector<ParsedCase> parse_test_xml(std::string_view xml);

    /// Extract expected outputs from a specific test case XML node
    /// @param testCaseXml XML content of a single test case
    /// @return Vector of output expectations for this test case
    std::vector<OutputExpectation> parse_output_expectations(std::string_view testCaseXml);

    /// Convert component structure to JSON object
    /// @param componentXml XML content containing component definitions
    /// @return JSON object with component name-value pairs
    nlohmann::json parse_component_structure(std::string_view componentXml);
} // namespace orion::common