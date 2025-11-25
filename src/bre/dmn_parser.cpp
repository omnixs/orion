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
#include <rapidxml/rapidxml.hpp>  // Use RapidXML instead of TinyXML2


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
                        std::string valuesText = textNode->value();
                        // Parse comma-separated quoted values like: "Approved", "Declined"
                        std::vector<std::string> values;
                        bool inQuote = false;
                        std::string current;
                        for (size_t i = 0; i < valuesText.length(); ++i)
                        {
                            char c = valuesText[i];
                            if (c == '"')
                            {
                                if (inQuote)
                                {
                                    // End of quoted value
                                    values.push_back(current);
                                    current.clear();
                                    inQuote = false;
                                }
                                else
                                {
                                    // Start of quoted value
                                    inQuote = true;
                                }
                            }
                            else if (inQuote)
                            {
                                current += c;
                            }
                        }
                        oc.outputValues = values;
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

    DmnModel DmnParser::parse(std::string_view xml)
    {
        rapidxml::xml_document<> doc;
        std::string buf(xml);
        doc.parse<0>(&buf[0]);

        auto* root = doc.first_node();
        if (root == nullptr) { throw std::runtime_error("DMN: empty document");
}

        DmnModel model;

        // Parse all decisions in the model
        for (auto* decision_node = root->first_node("decision"); decision_node != nullptr; decision_node = decision_node->
             next_sibling("decision"))
        {
            Decision decision;
            decision.id = (decision_node->first_attribute("id") != nullptr) ? decision_node->first_attribute("id")->value() : "";
            decision.name = (decision_node->first_attribute("name") != nullptr)
                                ? decision_node->first_attribute("name")->value()
                                : "";

            // Parse decision table if present
            if (auto* decision_table = decision_node->first_node("decisionTable"))
            {
                decision.decisionTable = parse_decision_table_from_node(decision_table, decision_node);
            }

            // Parse literal expression if present
            if (auto* literal_expr = decision_node->first_node("literalExpression"))
            {
                if (auto* text = literal_expr->first_node("text"))
                {
                    if (text->value() != nullptr)
                    {
                        decision.expression = std::string(text->value());
                    }
                }
            }

            model.decisions.push_back(std::move(decision));
        }

        return model;
    }

    // Helper: Parse input clauses from decision table XML
    void DmnParser::parse_input_clauses(rapidxml::xml_node<>* table, DecisionTable& decision_table)
    {
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
        decision_table.inputs.push_back(std::move(ic));
    }
}   // Helper: Parse output clauses from decision table XML (including output values for PRIORITY)
    void DmnParser::parse_output_clauses(rapidxml::xml_node<>* table, DecisionTable& decision_table)
    {
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
                        std::string valuesText = textNode->value();
                        // Parse comma-separated quoted values like: "Approved", "Declined"
                        std::vector<std::string> values;
                        bool inQuote = false;
                        std::string current;
                        for (size_t i = 0; i < valuesText.length(); ++i)
                        {
                            char c = valuesText[i];
                            if (c == '"')
                            {
                                if (inQuote)
                                {
                                    // End of quoted value
                                    values.push_back(current);
                                    current.clear();
                                    inQuote = false;
                                }
                                else
                                {
                                    // Start of quoted value
                                    inQuote = true;
                                }
                            }
                            else if (inQuote)
                            {
                                current += c;
                            }
                        }
                        oc.outputValues = values;
                    }
                }
        }

        decision_table.outputs.push_back(std::move(oc));
    }
}   // Helper: Parse rules from decision table XML
    void DmnParser::parse_rules(rapidxml::xml_node<>* table, DecisionTable& decision_table)
    {
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

        decision_table.rules.push_back(std::move(r));
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
