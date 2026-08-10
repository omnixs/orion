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

#include <orion/bre/dmn_parser.hpp>
#include <orion/api/logger.hpp>
#include <orion/bre/business_knowledge_model.hpp>
#include "orion/bre/contract_violation.hpp"
#include <orion/bre/feel/lexer.hpp>
#include <orion/bre/feel/parser.hpp>
#include <orion/bre/type_validator.hpp>
#include <rapidxml/rapidxml.hpp>  // Use RapidXML instead of TinyXML2

#include <algorithm>
#include <string_view>
#include <utility>

using std::string;
using std::unique_ptr;
using std::make_unique;
using std::exception;
using std::vector;

namespace orion::bre
{
    // Import logger
    using orion::api::warn;

    /**
     * @brief Helper function to parse FEEL expression into AST during model load
     * 
     * Attempts to pre-compile FEEL expressions into AST for performance.
     * Returns nullptr if expression is a simple unary test or parsing fails.
     * 
     * @param expression FEEL expression string
     * @return Parsed AST or nullptr if not parseable/simple unary test
     */
    static unique_ptr<ASTNode> tryParseExpressionToAST(const string& expression)
    {
        // Skip AST for simple unary tests (handled by unary_test_matches)
        if (expression == "-" || expression.empty()) {
            return nullptr;
}
            
        // Skip if it looks like a simple comparison or range (unary_test_matches handles these)
        if (expression.find(">=") != string::npos ||
            expression.find("<=") != string::npos ||
            expression.find("..") != string::npos ||
            expression.find('[') != string::npos ||
            expression.find('(') != string::npos) // Ranges and function calls
        {
            return nullptr; // Use unary_test_matches for these
        }
        
        // Try to parse as FEEL expression
        try
        {
            feel::Lexer lexer;
            auto tokens = lexer.tokenize(expression);
            
            feel::Parser parser;
            return parser.parse(tokens);
        }
        catch (const std::runtime_error&)
        {
            // Parsing failed - will use unary_test_matches fallback
            return nullptr;
        }
        catch (const std::invalid_argument&)
        {
            // Invalid expression - will use unary_test_matches fallback
            return nullptr;
        }
    }
    
    static HitPolicy parse_hit_policy(std::string_view hp, CollectAggregation& agg)
    {
    agg = CollectAggregation::NONE; // Default

    if (hp == "FIRST" || hp == "F") { return HitPolicy::FIRST; }
    if (hp == "UNIQUE" || hp == "U") { return HitPolicy::UNIQUE; }
    if (hp == "PRIORITY" || hp == "P") { return HitPolicy::PRIORITY; }
    if (hp == "ANY" || hp == "A") { return HitPolicy::ANY; }
    if (hp == "RULE_ORDER" || hp == "RULE ORDER" || hp == "R") { return HitPolicy::RULE_ORDER; }
    if (hp == "OUTPUT_ORDER" || hp == "OUTPUT ORDER" || hp == "O") { return HitPolicy::OUTPUT_ORDER; }      // Handle collect policies with aggregation
        if (hp == "COLLECT" || hp == "C")
        {
            agg = CollectAggregation::NONE;
            return HitPolicy::COLLECT;
        }
        if (hp == "C+")
        {
            agg = CollectAggregation::SUM;
            return HitPolicy::COLLECT;
        }
        if (hp == "C#")
        {
            agg = CollectAggregation::COUNT;
            return HitPolicy::COLLECT;
        }
        if (hp == "C<")
        {
            agg = CollectAggregation::MIN;
            return HitPolicy::COLLECT;
        }
        if (hp == "C>")
        {
            agg = CollectAggregation::MAX;
            return HitPolicy::COLLECT;
        }

        warn("Unknown hit policy '{}', defaulting to FIRST", hp);
        return HitPolicy::FIRST; // Default fallback
    }

    DecisionTable parse_dmn_decision_table(std::string_view xml)
    {
        rapidxml::xml_document<> doc;
        std::string buf(xml);
        doc.parse<0>(buf.data());

        auto* root = doc.first_node();
        if (root == nullptr) { throw std::runtime_error("DMN: empty document"); }

        rapidxml::xml_node<>* dec = nullptr;
        rapidxml::xml_node<>* table = nullptr;
        for (auto* n = root->first_node(); n != nullptr; n = n->next_sibling())
        {
            if (std::string(n->name()) == "decision")
            {
                dec = n;
                break;
            }
        }
        if (dec != nullptr)
        {
            for (auto* n = dec->first_node(); n != nullptr; n = n->next_sibling())
            {
                if (std::string(n->name()) == "decisionTable")
                {
                    table = n;
                    break;
                }
            }
        }
        if (table == nullptr) { throw std::runtime_error("DMN: decisionTable not found");
}

        DecisionTable dt{};
        if (auto* a = dec->first_attribute("id")) { dt.id = a->value();
}
        if (auto* a = dec->first_attribute("name")) { dt.name = a->value();
}
        if (auto* a = table->first_attribute("hitPolicy"))
        {
            dt.hitPolicy = parse_hit_policy(a->value(), dt.aggregation);
        }

        // Parse aggregation attribute separately (DMN XML format)
        if (auto* a = table->first_attribute("aggregation"))
        {
            std::string agg = a->value();
            if (agg == "SUM") { dt.aggregation = CollectAggregation::SUM;
            } else if (agg == "COUNT") { dt.aggregation = CollectAggregation::COUNT;
            } else if (agg == "MIN") { dt.aggregation = CollectAggregation::MIN;
            } else if (agg == "MAX") { dt.aggregation = CollectAggregation::MAX;
            } else { dt.aggregation = CollectAggregation::NONE;
}
        }

        for (auto* in = table->first_node("input"); in != nullptr; in = in->next_sibling("input"))
        {
            InputClause ic{};
            auto* ie = in->first_node("inputExpression");
            if (ie != nullptr)
            {
                if (auto* a = ie->first_attribute("typeRef")) { ic.typeRef = a->value();
}
                auto* txt = ie->first_node("text");
                if ((txt != nullptr) && (txt->value() != nullptr)) { ic.label = txt->value();
}
            }
            dt.inputs.push_back(std::move(ic));
        }
        for (auto* on = table->first_node("output"); on != nullptr; on = on->next_sibling("output"))
        {
            OutputClause oc{};
            if (auto* a = on->first_attribute("name")) { oc.label = a->value();
}
            if (auto* a = on->first_attribute("typeRef")) { oc.typeRef = a->value();
}

            // Parse output values (priorities for PRIORITY hit policy)
            if (auto* outputValuesNode = on->first_node("outputValues"))
            {
                if (auto* textNode = outputValuesNode->first_node("text"))
                {
                    if (textNode->value() != nullptr)
                    {
                        // Reuse parse_allowed_values from type_validator
                        oc.outputValues = parse_allowed_values(textNode->value());
                    }
                }
            }

            dt.outputs.push_back(std::move(oc));
        }
        for (auto* rn = table->first_node("rule"); rn != nullptr; rn = rn->next_sibling("rule"))
        {
            Rule r{};
            for (auto* ien = rn->first_node("inputEntry"); ien != nullptr; ien = ien->next_sibling("inputEntry"))
            {
                auto* txt = ien->first_node("text");
                string entry_text = (txt != nullptr) && (txt->value() != nullptr) ? txt->value() : "-";
                
                // Store original string
                r.inputEntries.emplace_back(entry_text);
                
                // Phase 3: Pre-parse as AST for performance (cache during model load)
                r.inputEntries_ast.push_back(tryParseExpressionToAST(entry_text));
            }

            // Handle multiple output entries for multi-output tables
            for (auto* oen = rn->first_node("outputEntry"); oen != nullptr; oen = oen->next_sibling("outputEntry"))
            {
                auto* txt = oen->first_node("text");
                string output_text = (txt != nullptr) && (txt->value() != nullptr) ? txt->value() : "{}";
                
                // Store original string
                r.outputEntries.emplace_back(output_text);
                
                // Parse output entry as FEEL expression (for proper type evaluation)
                r.outputEntries_ast.push_back(tryParseExpressionToAST(output_text));
            }

            // Fallback for single output entry (backward compatibility)
            if (r.outputEntries.empty())
            {
                auto* oen = rn->first_node("outputEntry");
                if (oen != nullptr)
                {
                    auto* txt = oen->first_node("text");
                    r.outputEntry = (txt != nullptr) && (txt->value() != nullptr) ? txt->value() : "{}";
                }
            }

            dt.rules.push_back(std::move(r));
        }

        return dt;
    }

    std::pair<std::string, std::string> parse_dmn_literal_decision(std::string_view xml)
    {
        rapidxml::xml_document<> doc;
        std::string buf(xml);
        doc.parse<0>(&buf[0]);
        auto* root = doc.first_node();
        if (root == nullptr) { throw std::runtime_error("DMN: empty document");
}
        for (auto* dec = root->first_node("decision"); dec != nullptr; dec = dec->next_sibling("decision"))
        {
            std::string dname;
            if (const auto* a = dec->first_attribute("name")) { dname = a->value();
            } else if (auto* attribute = dec->first_attribute("id")) { dname = attribute->value();
}
            for (auto* child = dec->first_node(); child != nullptr; child = child->next_sibling())
            {
                if (std::string(child->name()) == "literalExpression")
                {
                    if (auto* txt = child->first_node("text"))
                    {
                        return {dname, (txt->value() != nullptr) ? std::string(txt->value()) : std::string()};
                    }
                }
            }
        }
        throw std::runtime_error("DMN: no literalExpression decision found");
    }

    // Parse Business Knowledge Model from DMN XML
    std::tuple<std::string, std::vector<std::string>, std::string> parse_dmn_business_knowledge_model(
        std::string_view xml, std::string_view bkm_name)
    {
        rapidxml::xml_document<> doc;
        std::string buf(xml);
        doc.parse<0>(&buf[0]);

        auto* root = doc.first_node();
        if (root == nullptr) { throw std::runtime_error("DMN: empty document");
}

        for (auto* bkm = root->first_node("businessKnowledgeModel"); bkm != nullptr; bkm = bkm->next_sibling(
                 "businessKnowledgeModel"))
        {
            std::string name;
            if (auto* a = bkm->first_attribute("name"))
            {
                name = a->value();
            }
            else if (auto* attribute = bkm->first_attribute("id"))
            {
                name = attribute->value();
            }

            if (bkm_name.empty() || name == bkm_name)
            {
                std::vector<std::string> parameters;
                std::string expression;

                // Find encapsulatedLogic
                auto* logic = bkm->first_node("encapsulatedLogic");
                if (logic != nullptr)
                {
                    // Get formal parameters
                    for (auto* param = logic->first_node("formalParameter"); param != nullptr; param = param->next_sibling(
                             "formalParameter"))
                    {
                        if (auto* nameAttr = param->first_attribute("name"))
                        {
                            parameters.push_back(nameAttr->value());
                        }
                    }

                    // Get literal expression
                    auto* litExpr = logic->first_node("literalExpression");
                    if (litExpr != nullptr)
                    {
                        auto* txt = litExpr->first_node("text");
                        if ((txt != nullptr) && (txt->value() != nullptr))
                        {
                            expression = txt->value();
                        }
                    }
                }

                return {name, parameters, expression};
            }
        }

        throw std::runtime_error(std::string("DMN: businessKnowledgeModel '").append(bkm_name) + "' not found");
    }

    // Helper to find XML node with or without namespace prefix
    static rapidxml::xml_node<>* find_node(rapidxml::xml_node<>* parent, const char* name_with_prefix, const char* name_without_prefix)
    {
        auto* node = parent->first_node(name_with_prefix);
        if (node == nullptr)
        {
            node = parent->first_node(name_without_prefix);
        }
        return node;
    }

    // Helper to get an element name without its XML namespace prefix
    [[nodiscard]] static std::string_view local_element_name(rapidxml::xml_node<>* node)
    {
        if (node == nullptr || node->name() == nullptr) return {};
        const std::string_view name = node->name();
        const auto colon = name.find(':');
        return (colon == std::string_view::npos) ? name : name.substr(colon + 1);
    }

<<<<<<< HEAD
    // Helper to render a context entry name as a quoted FEEL string key
    [[nodiscard]] static std::string quote_feel_key(std::string_view name)
    {
        std::string quoted;
        quoted.reserve(name.size() + 2);
        quoted.push_back('"');
        for (const char character : name)
        {
            if (character == '"' || character == '\\') quoted.push_back('\\');
            quoted.push_back(character);
        }
        quoted.push_back('"');
        return quoted;
=======
    // Helper to append a context entry name as a quoted FEEL string key.
    static void append_quoted_feel_key(std::string& output, std::string_view name)
    {
        output.push_back('"');
        for (const char character : name)
        {
            if (character == '"' || character == '\\') output.push_back('\\');
            output.push_back(character);
        }
        output.push_back('"');
>>>>>>> main
    }

    [[nodiscard]] static std::string boxed_context_to_feel(rapidxml::xml_node<>* context_node);

    /**
     * @brief Convert a boxed expression element to equivalent FEEL text
     *
     * Supports literalExpression and (recursively) nested boxed contexts.
     *
     * @param expr_node XML node of the boxed expression
     * @return FEEL expression text, or empty string when the kind is unsupported
     */
    [[nodiscard]] static std::string boxed_expression_to_feel(rapidxml::xml_node<>* expr_node)
    {
        const std::string_view element_name = local_element_name(expr_node);

        if (element_name == "literalExpression")
        {
            auto* text = find_node(expr_node, "dmn:text", "text");
            if (text != nullptr && text->value() != nullptr && text->value()[0] != '\0')
            {
                return text->value();
            }
            return "null";
        }

        if (element_name == "context")
        {
            return boxed_context_to_feel(expr_node);
        }

<<<<<<< HEAD
        if (element_name == "list")
        {
            // DMN boxed list maps to FEEL list literal: [item1, item2, ...]
            std::string feel_list = "[";
            bool first = true;

            for (auto* item = expr_node->first_node(); item != nullptr; item = item->next_sibling())
            {
                const std::string item_expr = boxed_expression_to_feel(item);
                if (item_expr.empty()) return {};
                if (!first) feel_list += ", ";
                first = false;
                feel_list += item_expr;
            }

            feel_list += "]";
            return feel_list;
        }

=======
>>>>>>> main
        return {};
    }

    /**
     * @brief Convert a boxed <context> element into an equivalent FEEL context literal
     *
     * DMN 1.5 §10.2.1.4: the meaning of a boxed context is
     * `{ "Name 1": Value 1, ..., "Name n": Value n }` when no result box is present.
     * When a result box (a contextEntry without a variable) is present, the meaning is
     * `{ "Name 1": Value 1, ..., "result": Result }.result`.
     *
     * @param context_node XML node of the boxed context
     * @return FEEL expression text, or empty string when the context cannot be converted
     */
    [[nodiscard]] static std::string boxed_context_to_feel(rapidxml::xml_node<>* context_node)
    {
        std::vector<std::pair<std::string, std::string>> entries;
        std::string result_expression;
        bool has_result = false;

        for (auto* entry = context_node->first_node(); entry != nullptr; entry = entry->next_sibling())
        {
            if (local_element_name(entry) != "contextEntry") continue;

            std::string entry_name;
            rapidxml::xml_node<>* value_node = nullptr;

            for (auto* child = entry->first_node(); child != nullptr; child = child->next_sibling())
            {
                const std::string_view child_name = local_element_name(child);
                if (child_name == "variable")
                {
                    if (auto* name_attr = child->first_attribute("name"))
                    {
                        entry_name = name_attr->value();
                    }
                }
                else if (child_name != "description" && child_name != "extensionElements" && value_node == nullptr)
                {
                    value_node = child;
                }
            }

            if (value_node == nullptr) return {};

            std::string value_expression = boxed_expression_to_feel(value_node);
            if (value_expression.empty()) return {}; // Unsupported boxed expression kind

            if (entry_name.empty())
            {
                has_result = true;
                result_expression = std::move(value_expression);
            }
            else
            {
                entries.emplace_back(std::move(entry_name), std::move(value_expression));
            }
        }

        if (entries.empty() && !has_result) return {};

        // Pick a result key that cannot collide with a declared entry name
        std::string result_key = "result";
        if (has_result)
        {
            const auto collides = [&entries](const std::string& key) {
                return std::any_of(entries.begin(), entries.end(),
                                   [&key](const auto& entry) { return entry.first == key; });
            };
            while (collides(result_key)) result_key.push_back('_');
        }

<<<<<<< HEAD
        std::string feel_expression = "{";
=======
        std::string feel_expression;
        size_t reserved_size = 2; // "{}"
        for (const auto& [name, expression] : entries)
        {
            reserved_size += name.size() + expression.size() + 6; // quotes, colon+space, separator slack
        }
        if (has_result)
        {
            reserved_size += result_key.size() + result_expression.size() + 10;
        }
        feel_expression.reserve(reserved_size);

        feel_expression.push_back('{');
>>>>>>> main
        bool first_entry = true;
        for (const auto& [name, expression] : entries)
        {
            if (!first_entry) feel_expression += ", ";
            first_entry = false;
<<<<<<< HEAD
            feel_expression += quote_feel_key(name);
=======
            append_quoted_feel_key(feel_expression, name);
>>>>>>> main
            feel_expression += ": ";
            feel_expression += expression;
        }
        if (has_result)
        {
            if (!first_entry) feel_expression += ", ";
            feel_expression += result_key;
            feel_expression += ": ";
            feel_expression += result_expression;
        }
        feel_expression += "}";

        if (has_result)
        {
            feel_expression += ".";
            feel_expression += result_key;
        }
        return feel_expression;
    }

    /**
     * @brief Extract text content from first text child element
     * 
     * Searches for a "text" child element and returns its value, or empty string if not found.
     * Used by ItemDefinition parsers to extract label, description, and allowedValues text content.
     * 
     * @param parent Parent XML node to search for text element
     * @param matches_element Helper function to match element names with namespace awareness
     * @return Text content or empty string
     */
    [[nodiscard]] static std::string extract_text_from_element(
        rapidxml::xml_node<>* parent,
        const std::function<bool(rapidxml::xml_node<>*, const char*)>& matches_element)
    {
        if (parent == nullptr) return "";
        for (auto* text_child = parent->first_node(); text_child != nullptr; text_child = text_child->next_sibling()) {
            if (matches_element(text_child, "text") && text_child->value() != nullptr) {
                return text_child->value();
            }
        }
        return "";
    }

    /**
     * @brief Parse ItemComponent child element with attributes and constraints
     * 
     * Extracts name, typeRef, isCollection, and allowedValues from itemComponent node.
     * Follows DMN 1.5 complex type specification for structured ItemDefinitions.
     * 
     * @param comp_node XML node for itemComponent element
     * @param matches_element Helper function for namespace-aware element matching
     * @return Populated ItemComponent structure
     */
    [[nodiscard]] static ItemComponent parse_item_component(
        rapidxml::xml_node<>* comp_node,
        const std::function<bool(rapidxml::xml_node<>*, const char*)>& matches_element)
    {
        ItemComponent component;
        
        if (auto* name_attr = comp_node->first_attribute("name")) {
            component.name = name_attr->value();
        }
        if (auto* coll_attr = comp_node->first_attribute("isCollection")) {
            std::string val = coll_attr->value();
            component.isCollection = (val == "true" || val == "1");
        }

        for (auto* child = comp_node->first_node(); child != nullptr; child = child->next_sibling()) {
            if (matches_element(child, "typeRef") && child->value() != nullptr) {
                component.typeRef = child->value();
            } else if (matches_element(child, "isCollection") && child->value() != nullptr) {
                std::string val = child->value();
                component.isCollection = (val == "true" || val == "1");
            } else if (matches_element(child, "allowedValues")) {
                component.allowedValues = extract_text_from_element(child, matches_element);
            }
        }
        
        return component;
    }

    /**
     * @brief Parse all child elements of ItemDefinition node
     * 
     * Processes typeRef, label, description, allowedValues, and itemComponent children
     * following DMN 1.5 specification. Updates ItemDefinition structure in-place.
     * 
     * @param item_def_node XML node for itemDefinition element
     * @param item_def ItemDefinition structure to populate
     * @param matches_element Helper function for namespace-aware element matching
     */
    static void parse_item_definition_children(
        rapidxml::xml_node<>* item_def_node,
        ItemDefinition& item_def,
        const std::function<bool(rapidxml::xml_node<>*, const char*)>& matches_element)
    {
        for (auto* child = item_def_node->first_node(); child != nullptr; child = child->next_sibling()) {
            if (matches_element(child, "typeRef") && child->value() != nullptr) {
                item_def.typeRef = child->value();
            } else if (matches_element(child, "label")) {
                item_def.label = extract_text_from_element(child, matches_element);
            } else if (matches_element(child, "description")) {
                item_def.description = extract_text_from_element(child, matches_element);
            } else if (matches_element(child, "allowedValues")) {
                item_def.allowedValues = extract_text_from_element(child, matches_element);
            } else if (matches_element(child, "itemComponent")) {
                auto component = parse_item_component(child, matches_element);
                if (!component.name.empty()) {
                    item_def.itemComponents.push_back(std::move(component));
                }
            }
        }
    }

    DmnModel DmnParser::parse(std::string_view xml)
    {
        rapidxml::xml_document<> doc;
        std::string buf(xml);
        doc.parse<0>(&buf[0]);

        auto* root = doc.first_node();
        if (root == nullptr)
        {
            throw std::runtime_error("DMN: empty document");
        }

        DmnModel model;

        // Parse namespace from definitions element
        if (auto* ns_attr = root->first_attribute("namespace"))
        {
            model.namespace_uri = ns_attr->value();
        }

        // Helper lambda to match element names with or without namespace prefix
        auto matches_element = [](rapidxml::xml_node<>* node, const char* element_name) -> bool {
            if (node == nullptr || node->name() == nullptr) return false;
            std::string_view name = node->name();
            // Match exact name or with any namespace prefix (e.g., "dmn:itemDefinition")
            return name == element_name ||
                   (name.find(':') != std::string_view::npos &&
                    name.substr(name.find(':') + 1) == element_name);
        };

        // Parse ItemDefinitions (custom data types) - must happen before decisions
        for (auto* item_def_node = root->first_node();
             item_def_node != nullptr;
             item_def_node = item_def_node->next_sibling())
        {
            if (!matches_element(item_def_node, "itemDefinition")) continue;
            ItemDefinition item_def;

            // Parse attributes: name, id, label, typeLanguage, isCollection
            if (auto* attr = item_def_node->first_attribute("name")) item_def.name = attr->value();
            if (auto* attr = item_def_node->first_attribute("id")) item_def.id = attr->value();
            if (auto* attr = item_def_node->first_attribute("label")) item_def.label = attr->value();
            if (auto* attr = item_def_node->first_attribute("typeLanguage")) item_def.typeLanguage = attr->value();
            if (auto* attr = item_def_node->first_attribute("isCollection")) {
                std::string val = attr->value();
                item_def.isCollection = (val == "true" || val == "1");
            }

            // Parse child elements (typeRef, label, description, allowedValues, itemComponent)
            parse_item_definition_children(item_def_node, item_def, matches_element);

            // Store ItemDefinition by name
            if (!item_def.name.empty()) {
                model.item_definitions[item_def.name] = std::move(item_def);
            }
        }

        // Parse all decisions in the model (handle both with and without dmn: namespace prefix)
        // Try prefixed first, then unprefixed (for DMN files with default namespace)
        auto* decision_node = find_node(root, "dmn:decision", "decision");
        while (decision_node != nullptr)
        {
            Decision decision;
            decision.id = (decision_node->first_attribute("id") != nullptr) ? decision_node->first_attribute("id")->value() : "";
            decision.name = (decision_node->first_attribute("name") != nullptr)
                                ? decision_node->first_attribute("name")->value()
                                : "";
            // If no id attribute, use name as id (DMN spec allows this)
            if (decision.id.empty() && !decision.name.empty())
            {
                decision.id = decision.name;
            }

            // Parse decision table if present (try both prefixed and unprefixed)
            auto* decision_table = find_node(decision_node, "dmn:decisionTable", "decisionTable");
            if (decision_table != nullptr)
            {
                decision.decisionTable = parse_decision_table_from_node(decision_table, decision_node);
            }

            // Parse literal expression if present (try both prefixed and unprefixed)
            auto* literal_expr = find_node(decision_node, "dmn:literalExpression", "literalExpression");
            if (literal_expr != nullptr)
            {
                auto* text = find_node(literal_expr, "dmn:text", "text");
                if (text != nullptr && text->value() != nullptr)
                {
                    decision.expression = std::string(text->value());
                }
            }

            // Parse boxed context if present (converted to an equivalent FEEL context literal)
            if (!decision.decisionTable.has_value() && decision.expression.empty())
            {
                auto* context_node = find_node(decision_node, "dmn:context", "context");
                if (context_node != nullptr)
                {
                    decision.expression = boxed_context_to_feel(context_node);
<<<<<<< HEAD
                }
            }

            // Parse boxed list if present (converted to equivalent FEEL list literal)
            if (!decision.decisionTable.has_value() && decision.expression.empty())
            {
                auto* list_node = find_node(decision_node, "dmn:list", "list");
                if (list_node != nullptr)
                {
                    decision.expression = boxed_expression_to_feel(list_node);
=======
                    if (decision.expression.empty())
                    {
                        warn("DMN: boxed <context> in decision '{}' could not be converted to a "
                             "FEEL expression (unsupported entry kind); the decision will have no "
                             "logic unless another expression form is present",
                             decision.name.empty() ? decision.id : decision.name);
                    }
>>>>>>> main
                }
            }

            // Parse relation if present (converts to FEEL list-of-contexts expression)
            if (!decision.decisionTable.has_value() && decision.expression.empty())
            {
                auto* relation_node = find_node(decision_node, "dmn:relation", "relation");
                if (relation_node != nullptr)
                {
                    // Collect column names
                    std::vector<std::string> column_names;
                    for (auto* col = relation_node->first_node("column"); col; col = col->next_sibling("column"))
                    {
                        if (auto* name_attr = col->first_attribute("name"))
                            column_names.emplace_back(name_attr->value());
                    }
                    // Also try dmn:column prefix
                    if (column_names.empty())
                    {
                        for (auto* col = relation_node->first_node("dmn:column"); col; col = col->next_sibling("dmn:column"))
                        {
                            if (auto* name_attr = col->first_attribute("name"))
                                column_names.emplace_back(name_attr->value());
                        }
                    }

                    // Build FEEL expression: [{col1: val1, col2: val2}, ...]
                    std::string feel_expr = "[";
                    bool first_row = true;
                    for (auto* row = relation_node->first_node("row"); row; row = row->next_sibling("row"))
                    {
                        if (!first_row) feel_expr += ", ";
                        first_row = false;
                        feel_expr += "{";

                        size_t col_idx = 0;
                        for (auto* cell = row->first_node("literalExpression"); cell; cell = cell->next_sibling("literalExpression"))
                        {
                            if (col_idx > 0) feel_expr += ", ";
                            auto* text = find_node(cell, "dmn:text", "text");
                            std::string cell_text = (text && text->value()) ? text->value() : "null";
                            if (col_idx < column_names.size())
                                feel_expr += column_names[col_idx] + ": " + cell_text;
                            col_idx++;
                        }
                        // Also try dmn:literalExpression prefix
                        if (col_idx == 0)
                        {
                            for (auto* cell = row->first_node("dmn:literalExpression"); cell; cell = cell->next_sibling("dmn:literalExpression"))
                            {
                                if (col_idx > 0) feel_expr += ", ";
                                auto* text = find_node(cell, "dmn:text", "text");
                                std::string cell_text = (text && text->value()) ? text->value() : "null";
                                if (col_idx < column_names.size())
                                    feel_expr += column_names[col_idx] + ": " + cell_text;
                                col_idx++;
                            }
                        }
                        feel_expr += "}";
                    }
                    // Also try dmn:row prefix
                    if (first_row)
                    {
                        for (auto* row = relation_node->first_node("dmn:row"); row; row = row->next_sibling("dmn:row"))
                        {
                            if (!first_row) feel_expr += ", ";
                            first_row = false;
                            feel_expr += "{";

                            size_t col_idx = 0;
                            for (auto* cell = row->first_node("literalExpression"); cell; cell = cell->next_sibling("literalExpression"))
                            {
                                if (col_idx > 0) feel_expr += ", ";
                                auto* text = find_node(cell, "dmn:text", "text");
                                std::string cell_text = (text && text->value()) ? text->value() : "null";
                                if (col_idx < column_names.size())
                                    feel_expr += column_names[col_idx] + ": " + cell_text;
                                col_idx++;
                            }
                            if (col_idx == 0)
                            {
                                for (auto* cell = row->first_node("dmn:literalExpression"); cell; cell = cell->next_sibling("dmn:literalExpression"))
                                {
                                    if (col_idx > 0) feel_expr += ", ";
                                    auto* text = find_node(cell, "dmn:text", "text");
                                    std::string cell_text = (text && text->value()) ? text->value() : "null";
                                    if (col_idx < column_names.size())
                                        feel_expr += column_names[col_idx] + ": " + cell_text;
                                    col_idx++;
                                }
                            }
                            feel_expr += "}";
                        }
                    }
                    feel_expr += "]";
                    decision.expression = feel_expr;
                }
            }

            // Parse information requirements (decision dependencies)
            auto* info_req = find_node(decision_node, "dmn:informationRequirement", "informationRequirement");
            while (info_req != nullptr)
            {
                InformationRequirement requirement;
                
                if (auto* id_attr = info_req->first_attribute("id"))
                {
                    requirement.id = id_attr->value();
                }
                
                // Parse requiredDecision element
                auto* req_decision = find_node(info_req, "dmn:requiredDecision", "requiredDecision");
                if (req_decision != nullptr)
                {
                    if (auto* href_attr = req_decision->first_attribute("href"))
                    {
                        std::string href = href_attr->value();
                        // href format: "#decision_id"
                        if (!href.empty() && href[0] == '#')
                        {
                            requirement.requiredDecisionId = href.substr(1);
                        }
                    }
                }
                
                // Parse requiredInput element (for input data dependencies)
                auto* req_input = find_node(info_req, "dmn:requiredInput", "requiredInput");
                if (req_input != nullptr)
                {
                    if (auto* href_attr = req_input->first_attribute("href"))
                    {
                        std::string href = href_attr->value();
                        // href format: "#input_id"
                        if (!href.empty() && href[0] == '#')
                        {
                            requirement.requiredInputId = href.substr(1);
                        }
                    }
                }
                
                decision.informationRequirements.push_back(std::move(requirement));
                
                // Move to next informationRequirement sibling
                auto* next = info_req->next_sibling("dmn:informationRequirement");
                if (next == nullptr)
                {
                    next = info_req->next_sibling("informationRequirement");
                }
                info_req = next;
            }

            model.decisions.push_back(std::move(decision));
            
            // Move to next decision sibling
            auto* next_decision = decision_node->next_sibling("dmn:decision");
            if (next_decision == nullptr)
            {
                next_decision = decision_node->next_sibling("decision");
            }
            decision_node = next_decision;
        }

        return model;
    }

    // Helper: Parse input clauses from decision table XML
    void DmnParser::parse_input_clauses(rapidxml::xml_node<>* table, DecisionTable& decision_table)
    {
        auto* in = find_node(table, "dmn:input", "input");
        while (in != nullptr)
        {
            InputClause ic{};
            
            // Get label from input element attribute
            if (auto* label_attr = in->first_attribute("label")) {
                ic.label = label_attr->value();
            }
            
            // Parse input expression
            auto* ie = find_node(in, "dmn:inputExpression", "inputExpression");
            if (ie != nullptr)
            {
                if (auto* a = ie->first_attribute("typeRef")) { ic.typeRef = a->value();
}
                // Get expression text from <text> element
                auto* txt = find_node(ie, "dmn:text", "text");
                if ((txt != nullptr) && (txt->value() != nullptr)) { 
                    ic.inputExpression = txt->value();
                    // Pre-parse inputExpression as AST for performance
                    if (!ic.inputExpression.empty())
                    {
                        try
                        {
                            feel::Lexer lexer;
                            auto tokens = lexer.tokenize(ic.inputExpression);
                            feel::Parser parser;
                            ic.inputExpression_ast = parser.parse(tokens);
                        }
                        catch (...)
                        {
                            ic.inputExpression_ast = nullptr;
                        }
                    }
}
            }
            decision_table.inputs.push_back(std::move(ic));
            
            // Move to next input sibling
            auto* next = in->next_sibling("dmn:input");
            if (next == nullptr)
            {
                next = in->next_sibling("input");
            }
            in = next;
        }
    }   // Helper: Parse output clauses from decision table XML (including output values for PRIORITY)
    void DmnParser::parse_output_clauses(rapidxml::xml_node<>* table, DecisionTable& decision_table)
    {
        auto* on = find_node(table, "dmn:output", "output");
        while (on != nullptr)
        {
            OutputClause oc{};
            if (auto* a = on->first_attribute("name")) { oc.label = a->value();
}
            if (auto* a = on->first_attribute("typeRef")) { oc.typeRef = a->value();
}

            // Parse output values (priorities for PRIORITY hit policy)
            auto* outputValuesNode = find_node(on, "dmn:outputValues", "outputValues");
            if (outputValuesNode != nullptr)
            {
                auto* textNode = find_node(outputValuesNode, "dmn:text", "text");
                if (textNode != nullptr && textNode->value() != nullptr)
                {
                    // Reuse parse_allowed_values from type_validator
                    oc.outputValues = parse_allowed_values(textNode->value());
                }
            }

            decision_table.outputs.push_back(std::move(oc));
            
            // Move to next output sibling
            auto* next = on->next_sibling("dmn:output");
            if (next == nullptr)
            {
                next = on->next_sibling("output");
            }
            on = next;
        }
    }   // Helper: Parse rules from decision table XML
    void DmnParser::parse_rules(rapidxml::xml_node<>* table, DecisionTable& decision_table)
    {
        auto* rn = find_node(table, "dmn:rule", "rule");
        while (rn != nullptr)
        {
            Rule r{};
            
            // Parse input entries
            auto* ien = find_node(rn, "dmn:inputEntry", "inputEntry");
            while (ien != nullptr)
            {
                auto* txt = find_node(ien, "dmn:text", "text");
                string entry_text = (txt != nullptr) && (txt->value() != nullptr) ? txt->value() : "-";
                
                // Store original string
                r.inputEntries.emplace_back(entry_text);
                
                // Phase 3: Pre-parse as AST for performance (cache during model load)
                r.inputEntries_ast.push_back(tryParseExpressionToAST(entry_text));
                
                // Move to next inputEntry sibling
                auto* next = ien->next_sibling("dmn:inputEntry");
                if (next == nullptr)
                {
                    next = ien->next_sibling("inputEntry");
                }
                ien = next;
            }

            // Handle multiple output entries for multi-output tables
            auto* oen = find_node(rn, "dmn:outputEntry", "outputEntry");
            while (oen != nullptr)
            {
                auto* txt = find_node(oen, "dmn:text", "text");
                string output_text = (txt != nullptr) && (txt->value() != nullptr) ? txt->value() : "{}";
                
                // Store original string
                r.outputEntries.emplace_back(output_text);
                
                // Parse output entry as FEEL expression (for proper type evaluation)
                r.outputEntries_ast.push_back(tryParseExpressionToAST(output_text));
                
                // Move to next outputEntry sibling
                auto* next_oe = oen->next_sibling("dmn:outputEntry");
                if (next_oe == nullptr)
                {
                    next_oe = oen->next_sibling("outputEntry");
                }
                oen = next_oe;
            }

            // Fallback for single output entry (backward compatibility)
            if (r.outputEntries.empty())
            {
                auto* oe_fallback = find_node(rn, "dmn:outputEntry", "outputEntry");
                if (oe_fallback != nullptr)
                {
                    auto* txt = find_node(oe_fallback, "dmn:text", "text");
                    r.outputEntry = (txt != nullptr) && (txt->value() != nullptr) ? txt->value() : "{}";
                }
            }

            decision_table.rules.push_back(std::move(r));
            
            // Move to next rule sibling
            auto* next = rn->next_sibling("dmn:rule");
            if (next == nullptr)
            {
                next = rn->next_sibling("rule");
            }
            rn = next;
        }
    }   DecisionTable DmnParser::parse_decision_table_from_node(rapidxml::xml_node<>* table,
                                                            rapidxml::xml_node<>* decision_node)
    {
        DecisionTable dt{};
        
        // Parse decision attributes
        if (auto* a = decision_node->first_attribute("id")) { dt.id = a->value();
}
        if (auto* a = decision_node->first_attribute("name")) { dt.name = a->value();
}
        
        // Parse hit policy
        if (auto* a = table->first_attribute("hitPolicy"))
        {
            dt.hitPolicy = parse_hit_policy(a->value(), dt.aggregation);
        }

        // Parse aggregation attribute separately (DMN XML format)
        if (auto* a = table->first_attribute("aggregation"))
        {
            std::string agg = a->value();
            if (agg == "SUM") { dt.aggregation = CollectAggregation::SUM;
            } else if (agg == "COUNT") { dt.aggregation = CollectAggregation::COUNT;
            } else if (agg == "MIN") { dt.aggregation = CollectAggregation::MIN;
            } else if (agg == "MAX") { dt.aggregation = CollectAggregation::MAX;
            } else { dt.aggregation = CollectAggregation::NONE;
}
        }

        // Delegate to helper methods for cleaner organization
        parse_input_clauses(table, dt);
        parse_output_clauses(table, dt);
        parse_rules(table, dt);

        return dt;
    }

    // Enhanced parse_business_knowledge_model with contract enforcement
    unique_ptr<BusinessKnowledgeModel> parse_business_knowledge_model(std::string_view dmn_xml,
                                                                   std::string_view bkm_name,
                                                                   string& error_message)
    {
        if (dmn_xml.empty()) [[unlikely]]
        {
            error_message = "DMN XML cannot be empty";
            return nullptr;
        }

        try
        {
            auto [name, parameters, expression] = parse_dmn_business_knowledge_model(dmn_xml, bkm_name);

            // Validate parsed data
            if (name.empty()) [[unlikely]]
            {
                error_message = "BKM name cannot be empty in DMN XML";
                return nullptr;
            }

            if (expression.empty()) [[unlikely]]
            {
                error_message = "BKM expression cannot be empty in DMN XML for BKM: " + name;
                return nullptr;
            }

            auto bkm = make_unique<BusinessKnowledgeModel>();
            bkm->name = name;
            bkm->parameters = parameters;
            bkm->expression_text = expression;

            return bkm;
        }
        catch (const exception& e)
        {
            error_message = e.what();
            return nullptr;
        }
    }

    // Enhanced parseDecisionTable with contract checking
    unique_ptr<DecisionTable> parseDecisionTable(const string& dmn_xml, string& error_message)
    {
        if (dmn_xml.empty()) [[unlikely]]
        {
            error_message = "DMN XML cannot be empty";
            return nullptr;
        }

        try
        {
            auto dt = make_unique<DecisionTable>(parse_dmn_decision_table(dmn_xml));
            if (dt && dt->name.empty()) [[unlikely]]
            {
                THROW_CONTRACT_VIOLATION("Parsed decision table has empty name");
            }
            return dt;
        }
        catch (const exception& e)
        {
            error_message = e.what();
            return nullptr;
        }
    }

    // Enhanced parseLiteralDecision with contract checking  
    unique_ptr<LiteralDecision> parseLiteralDecision(const string& dmn_xml, string& error_message)
    {
        if (dmn_xml.empty()) [[unlikely]]
        {
            error_message = "DMN XML cannot be empty";
            return nullptr;
        }

        try
        {
            auto [name, expression] = parse_dmn_literal_decision(dmn_xml);
            auto ld = make_unique<LiteralDecision>();
            ld->name = name;
            ld->expression_text = expression;
            
            // Phase 3: Pre-parse expression as AST for performance (cache during model load)
            ld->expression_ast = tryParseExpressionToAST(expression);
            
            if (ld && ld->name.empty()) [[unlikely]]
            {
                THROW_CONTRACT_VIOLATION("Parsed literal decision has empty name");
            }
            return ld;
        }
        catch (const exception& e)
        {
            error_message = e.what();
            return nullptr;
        }
    }
} // namespace orion::bre
