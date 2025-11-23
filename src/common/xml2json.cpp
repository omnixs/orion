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

#include <orion/common/xml2json.hpp>
#include <rapidxml/rapidxml.hpp>
#include <stdexcept>

namespace orion::common
{
    using json = nlohmann::json;

    nlohmann::json parse_xml_value(std::string_view value, std::string_view xsiType)
        {
            if (value.empty()) { return json{};
}

            // Handle different XSD types
            if (xsiType.find("decimal") != std::string::npos || xsiType.find("double") != std::string::npos || xsiType.
                find("float") != std::string::npos)
            {
                try
                {
                    return json(std::stod(std::string(value)));
                }
                catch (...)
                {
                    return json(value); // fallback to string
                }
            }
            else if (xsiType.find("integer") != std::string::npos || xsiType.find("int") != std::string::npos)
            {
                try
                {
                    return json(std::stoll(std::string(value)));
                }
                catch (...)
                {
                    return json(value); // fallback to string
                }
            }
            else if (xsiType.find("boolean") != std::string::npos)
            {
                if (value == "true") { return json(true);
}
                if (value == "false") { return json(false);
}
                return json(value); // fallback to string
            }
            else if (xsiType.find("string") != std::string::npos)
            {
                return json(value);
            }
            else if (xsiType.find("date") != std::string::npos || xsiType.find("time") != std::string::npos || xsiType.
                find("duration") != std::string::npos)
            {
                return json(value); // Keep dates/times as strings
            }

            // Default case - try to parse as number, fall back to string
            try
            {
                // Try integer first
                if (value.find('.') == std::string::npos)
                {
                    return json(std::stoll(std::string(value)));
                }
                else
                {
                    return json(std::stod(std::string(value)));
                }
            }
            catch (...)
            {
                return json(value);
            }
        }

    // Helper function to parse component nodes into a JSON object
    static nlohmann::json parse_components(rapidxml::xml_node<>* parentNode, const char* componentTag, const char* valueTag)
    {
        nlohmann::json result = nlohmann::json::object();
        
        for (auto* comp = parentNode->first_node(componentTag); comp != nullptr; comp = comp->next_sibling(componentTag))
        {
            const char* compName = (comp->first_attribute("name") != nullptr)
                                       ? comp->first_attribute("name")->value()
                                       : nullptr;
            if (compName != nullptr)
            {
                auto* valNode = comp->first_node(valueTag);
                if (valNode == nullptr && std::string(valueTag) == "tc:value")
                {
                    valNode = comp->first_node("value");
                }
                
                if ((valNode != nullptr) && (valNode->value() != nullptr))
                {
                    std::string xsiType;
                    if (auto* t = valNode->first_attribute("xsi:type"))
                    {
                        xsiType = t->value();
                    }
                    result[compName] = parse_xml_value(valNode->value(), xsiType);
                }
            }
        }
        
        return result;
    }

    // Helper function to parse input nodes (handles both simple values and nested components)
    static void parse_input_node(rapidxml::xml_node<>* inputNode, ParsedCase& pc)
    {
        std::string name;
        if (auto* a = inputNode->first_attribute("name"))
        {
            name = a->value();
        }
        if (name.empty())
        {
            return;
        }

        // Try parsing components without namespace prefix first
        nlohmann::json nestedObject = parse_components(inputNode, "component", "value");
        bool hasComponents = !nestedObject.empty();

        // Try with namespace prefix if no components found
        if (!hasComponents)
        {
            nestedObject = parse_components(inputNode, "tc:component", "tc:value");
            hasComponents = !nestedObject.empty();
        }

        if (hasComponents)
        {
            pc.input[name] = nestedObject;
        }
        else
        {
            // Handle simple value structure
            auto* valNode = inputNode->first_node("value");
            if ((valNode != nullptr) && (valNode->value() != nullptr))
            {
                std::string xsiType;
                if (auto* t = valNode->first_attribute("xsi:type"))
                {
                    xsiType = t->value();
                }
                pc.input[name] = parse_xml_value(valNode->value(), xsiType);
            }
        }
    }

    // Helper function to parse a single value node with nil handling
    static nlohmann::json parse_value_with_nil(rapidxml::xml_node<>* valNode)
    {
        if (valNode == nullptr)
        {
            return nullptr;
        }

        std::string xsiType;
        if (auto* t = valNode->first_attribute("xsi:type"))
        {
            xsiType = t->value();
        }

        // Check for xsi:nil attribute
        if (auto* nilAttr = valNode->first_attribute("xsi:nil"))
        {
            std::string nilValue = nilAttr->value();
            if (nilValue == "true")
            {
                return nullptr;
            }
        }

        // Parse the value
        if (valNode->value() == nullptr)
        {
            if (xsiType.find("string") != std::string::npos)
            {
                return "";
            }
            return nullptr;
        }

        return parse_xml_value(valNode->value(), xsiType);
    }

    // Helper function to parse component-based expected value (object structure)
    static std::string parse_expected_components(rapidxml::xml_node<>* expNode)
    {
        nlohmann::json componentObj = nlohmann::json::object();
        
        for (auto* comp = expNode->first_node("component"); comp != nullptr; comp = comp->next_sibling("component"))
        {
            const char* compName = (comp->first_attribute("name") != nullptr)
                                       ? comp->first_attribute("name")->value()
                                       : nullptr;
            if (compName != nullptr)
            {
                auto* valNode = comp->first_node("value");
                if (valNode != nullptr)
                {
                    componentObj[compName] = parse_value_with_nil(valNode);
                }
            }
        }
        
        return componentObj.dump();
    }

    // Helper function to parse list-based expected value (array structure)
    static std::string parse_expected_list(rapidxml::xml_node<>* listNode)
    {
        nlohmann::json listArray = nlohmann::json::array();
        
        for (auto* item = listNode->first_node("item"); item != nullptr; item = item->next_sibling("item"))
        {
            // Check if this item has a direct value
            auto* directValueNode = item->first_node("value");
            if (directValueNode != nullptr)
            {
                listArray.push_back(parse_value_with_nil(directValueNode));
            }
            else
            {
                // Component-based item (object in array)
                nlohmann::json itemObj = nlohmann::json::object();
                for (auto* comp = item->first_node("component"); comp != nullptr; comp = comp->next_sibling("component"))
                {
                    const char* compName = (comp->first_attribute("name") != nullptr)
                                               ? comp->first_attribute("name")->value()
                                               : nullptr;
                    if (compName != nullptr)
                    {
                        auto* valNode = comp->first_node("value");
                        if (valNode != nullptr)
                        {
                            itemObj[compName] = parse_value_with_nil(valNode);
                        }
                    }
                }
                if (!itemObj.empty())
                {
                    listArray.push_back(itemObj);
                }
            }
        }
        
        return listArray.dump();
    }

    // Helper function to parse expected value node (handles list, component, and simple structures)
    static std::string parse_expected_value(rapidxml::xml_node<>* expNode)
    {
        if (expNode == nullptr)
        {
            return "";
        }

        // Check for list structure first
        auto* listNode = expNode->first_node("list");
        if (listNode != nullptr)
        {
            return parse_expected_list(listNode);
        }

        // Check for component structure
        if (expNode->first_node("component") != nullptr)
        {
            return parse_expected_components(expNode);
        }

        // Simple value structure
        auto* val = expNode->first_node("value");
        if (val == nullptr)
        {
            val = expNode->first_node("tc:value");
        }
        if (val != nullptr)
        {
            return parse_value_with_nil(val).dump();
        }

        return "";
    }

    // Helper function to parse output/result nodes
    static void parse_output_node(rapidxml::xml_node<>* outputNode, ParsedCase& pc)
    {
        std::string outName;
        if (auto* a = outputNode->first_attribute("name"))
        {
            outName = a->value();
        }
        
        std::string outId;
        if (auto* a = outputNode->first_attribute("id"))
        {
            outId = a->value();
        }
        if (outId.empty())
        {
            outId = outName;
        }

        OutputExpectation oe;
        oe.name = outName;
        oe.id = outId;

        auto* exp = outputNode->first_node("expected");
        if (exp == nullptr)
        {
            exp = outputNode->first_node("tc:expected");
        }
        
        oe.expected = parse_expected_value(exp);

        if (!oe.expected.empty())
        {
            pc.outputs.push_back(oe);
        }
    }

    std::vector<ParsedCase> parse_test_xml(std::string_view xml)
        {
            std::vector<ParsedCase> out;
            using namespace rapidxml;
            xml_document<> doc;
            std::string buf(xml);
            buf.push_back('\0');
            try
            {
                doc.parse<0>(&buf[0]);
            }
            catch (...)
            {
                return out;
            }

            auto* root = doc.first_node();
            if (root == nullptr)
            {
                return out;
            }

            // Lambda to process each test case node
            auto for_each_case = [&](rapidxml::xml_node<>* tc)
            {
                ParsedCase pc;
                if (auto* a = tc->first_attribute("id"))
                {
                    pc.id = a->value();
                }
                pc.input = json::object();

                // Parse all child nodes (inputs and outputs)
                for (auto* node = tc->first_node(); node != nullptr; node = node->next_sibling())
                {
                    std::string nodeName = node->name();

                    if (nodeName == "inputNode" || nodeName == "inputData" || 
                        nodeName == "tc:inputNode" || nodeName == "tc:inputData")
                    {
                        parse_input_node(node, pc);
                    }
                    else if (nodeName == "resultNode" || nodeName == "outputNode" || 
                             nodeName == "tc:resultNode" || nodeName == "tc:outputNode")
                    {
                        parse_output_node(node, pc);
                    }
                }

                // Post-processing: set default output ID if needed
                if (pc.outputs.size() == 1 && pc.outputs[0].id.empty())
                {
                    pc.outputs[0].id = pc.id;
                }
                
                if (!pc.outputs.empty())
                {
                    out.push_back(std::move(pc));
                }
            };

            if (std::string(root->name()) == "testCases" || std::string(root->name()) == "tc:testCases")
            {
                for (auto* tc = root->first_node("testCase"); tc != nullptr; tc = tc->next_sibling("testCase")) {
                    for_each_case(tc);
}
                if (out.empty())
                {
                    for (auto* tc = root->first_node("tc:testCase"); tc != nullptr; tc = tc->next_sibling("tc:testCase")) {
                        for_each_case(tc);
}
                }
            }
            else if (std::string(root->name()) == "testCase" || std::string(root->name()) == "tc:testCase")
            {
                for_each_case(root);
            }

            return out;
        }

    std::vector<OutputExpectation> parse_output_expectations(std::string_view testCaseXml)
        {
            // This is a utility function that can extract just the output expectations
            // from a single test case XML fragment - useful for focused testing
            auto cases = parse_test_xml("<testCase>" + std::string(testCaseXml) + "</testCase>");
            if (!cases.empty())
            {
                return cases[0].outputs;
            }
            return {};
        }

    nlohmann::json parse_component_structure(std::string_view componentXml)
        {
            // Parse a component structure XML fragment into JSON
            using namespace rapidxml;
            xml_document<> doc;
            std::string buf(componentXml);
            buf.push_back('\0');

            nlohmann::json componentObj = nlohmann::json::object();

            try
            {
                doc.parse<0>(&buf[0]);
                auto* root = doc.first_node();
                if (root == nullptr) { return componentObj;
}

                for (auto* comp = root->first_node("component"); comp != nullptr; comp = comp->next_sibling("component"))
                {
                    const char* compName = (comp->first_attribute("name") != nullptr)
                                               ? comp->first_attribute("name")->value()
                                               : nullptr;
                    if (compName != nullptr)
                    {
                        auto* valNode = comp->first_node("value");
                        if ((valNode != nullptr) && (valNode->value() != nullptr))
                        {
                            std::string xsiType;
                            if (auto* t = valNode->first_attribute("xsi:type")) { xsiType = t->value();
}
                            componentObj[compName] = parse_xml_value(valNode->value(), xsiType);
                        }
                    }
                }
            }
            catch (...)
            {
                // Return empty object on parse error
            }

            return componentObj;
        }
} // namespace orion::common