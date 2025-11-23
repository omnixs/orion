/*
 * ORION Optimized Rule Integration & Operations Native
 * SPDX-License-Identifier: Apache-2.0
 * SPDX-FileCopyrightText: 2025 ORION contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy at https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Modifications: This file has been modified by ORION contributors. See VCS history.
 */

/**
 * @file dmn_model.hpp
 * @brief Core DMN model structures for Decision Management and Notation
 * 
 * ## DMN Model Overview
 * 
 * This file defines the core structures that represent Decision Model and Notation (DMN) 
 * elements according to the OMG DMN 1.5 specification. These structures work together 
 * to model business decision-making logic.
 * 
 * ## Structure Relationships
 * 
 * ```
 * DecisionTable ──┬─→ Rules (input/output entries)
 *                 └─→ InputClause/OutputClause (column definitions)
 * 
 * LiteralDecision ──→ FEEL Expression (uses BKMs + variables)
 *                      ↓
 * BusinessKnowledgeModel ──→ FEEL Function (reusable logic)
 *                           ↓
 * ASTNode ──→ Parsed expression tree (from ast_node.hpp)
 * ```
 * 
 * ## DMN Element Types
 * 
 * - **DecisionTable**: Tabular decision logic with rules and hit policies
 * - **LiteralDecision**: Decisions using FEEL expressions (can call BKMs)  
 * - **BusinessKnowledgeModel**: Reusable functions encapsulating business logic
 * 
 * ## Integration with Other Headers
 * 
 * - **ast_node.hpp**: Provides AST structures for parsed FEEL expressions
 * - **feel_expr.hpp**: FEEL expression evaluator used by all decision types
 * - **bkm_manager.hpp**: Management and invocation of Business Knowledge Models
 * - **engine.hpp**: Orchestrates evaluation of all decision types
 * 
 * ## Example Usage Flow
 * 
 * 1. **DecisionTable**: Evaluates tabular rules directly against input data
 * 2. **LiteralDecision**: Evaluates FEEL expressions like `"PMT(amount, rate, term) + fee"`
 * 3. **BusinessKnowledgeModel**: Provides `PMT()` function implementation for reuse
 * 
 * @see DMN 1.5 Specification: https://www.omg.org/spec/DMN/
 */
#pragma once
#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <map>
#include <sstream>
#include <nlohmann/json.hpp>
#include "ast_node.hpp"
#include "feel/unary.hpp"

namespace orion::bre
{
    /**
     * @brief Hit policy for decision tables
     * 
     * Determines how multiple matching rules are handled in decision table evaluation.
     * 
     * @see DMN 1.5 Specification Section 8.2.8 "Hit policy"
     * @see DMN 1.5 Specification Section 8.3.4 "Hit policy and result aggregation"
     */
    enum class HitPolicy : std::uint8_t
    {
        UNIQUE,     // Exactly one rule can match (default for most cases)
        FIRST,      // Return result of first matching rule
        PRIORITY,   // Return result of rule with highest priority
        ANY,        // All matching rules must have same output
        COLLECT,    // Collect all outputs (requires aggregation)
        RULE_ORDER, // Return results in rule definition order
        OUTPUT_ORDER // Return results in output value priority order
    };

    /**
     * @brief Aggregation function for COLLECT hit policy
     * 
     * Specifies how to aggregate multiple outputs when using COLLECT hit policy.
     * Only applicable when HitPolicy is COLLECT.
     * 
     * @see DMN 1.5 Specification Section 8.2.8 "Hit policy"
     * @see DMN 1.5 Specification Section 8.3.4 "Hit policy and result aggregation"
     */
    enum class CollectAggregation : std::uint8_t
    {
        NONE,  // No aggregation (return list of results)
        SUM,   // Sum of numeric results
        COUNT, // Count of results
        MIN,   // Minimum value
        MAX    // Maximum value
    };

    /**
     * @brief Table orientation for decision tables
     * 
     * Controls how rules are displayed:
     * - RULE_AS_ROW: Rules displayed as rows (default)
     * - RULE_AS_COLUMN: Rules displayed as columns  
     * - CROSSTAB: Cross-table format
     * 
     * @see DMN 1.5 Specification Section 8.2.2 "Table orientation"
     */
    enum class TableOrientation : std::uint8_t
    {
        RULE_AS_ROW, // Default
        RULE_AS_COLUMN,
        CROSSTAB
    };

    /**
     * @brief Item definition for DMN type system
     * 
     * Defines reusable data types that can be used in decision variables.
     * Supports both simple types and complex structures with collection attribute.
     * 
     * @see DMN 1.5 Specification Section 7.3.2 "ItemDefinition metamodel"
     */
    struct ItemDefinition
    {
        std::string name; // Type name (e.g., "tApproval_1")
        std::string label; // Human-readable label
        std::string typeRef; // Base type reference (e.g., "string")
        bool isCollection = false; // Whether this is a collection type
        std::vector<ItemDefinition> itemComponents; // For complex types
    };

    /**
     * @brief Decision variable definition
     * 
     * Defines the output variable for a decision, including its name and type.
     * The typeRef can reference either built-in types or ItemDefinitions.
     * 
     * @see DMN 1.5 Specification Section 6.2.1.1 "Decision variable"
     */
    struct DecisionVariable
    {
        std::string name; // Variable name (e.g., "Approval")
        std::string typeRef; // Type reference (e.g., "tApproval_1")
        std::string id; // Unique identifier
    };

    /**
     * @brief Input column definition for decision tables
     * 
     * Defines an input column with its label (variable name) and data type.
     * Example: label="Age", typeRef="number"
     * 
     * @see DMN 1.5 Specification Section 8.3.2 "Decision Table Input and Output metamodel"
     * @see DMN 1.5 Specification Section 8.2.4 "Input values"
     */
    struct InputClause
    {
        std::string label;
        std::string typeRef;
        std::string inputExpression; // FEEL expression for input
        std::vector<std::string> inputValues; // Allowed values constraint
    };

    /**
     * @brief Output column definition for decision tables
     * 
     * Defines an output column with its name and data type.
     * Example: label="Risk Level", typeRef="string"
     * 
     * @see DMN 1.5 Specification Section 8.3.2 "Decision Table Input and Output metamodel" 
     * @see DMN 1.5 Specification Section 8.2.11 "Default output values"
     */
    struct OutputClause
    {
        std::string label;
        std::string typeRef;
        std::vector<std::string> outputValues; // Allowed values constraint
        std::string defaultOutputEntry; // Default value if no rules match
    };

    /**
     * @brief Single rule in a decision table
     * 
     * Contains input conditions and corresponding output values.
     * For multi-output tables, outputEntries contains multiple values.
     * Example: inputEntries=[">=18", "Male"], outputEntries=["\"Approved\"", "\"Standard\""]
     * 
     * **Phase 3 Enhancement**: Pre-parsed AST caching
     * - inputEntries: Original string expressions (for debugging and unary tests)
     * - inputEntries_ast: Pre-compiled AST nodes (for complex FEEL expressions)
     * - During evaluation, try AST first, fall back to unary_test_matches if null
     * 
     * @see DMN 1.5 Specification Section 8.3.3 "Decision Rule metamodel"
     * @see DMN 1.5 Specification Section 8.2.7 "Input entries"
     * @see DMN 1.5 Specification Section 8.2.9 "Output entry"
     */
    struct Rule
    {
        std::string id; // Unique rule identifier
        std::vector<std::string> inputEntries; // Original expressions (for debugging/fallback)
        std::vector<std::unique_ptr<ASTNode>> inputEntries_ast; // Pre-parsed AST (nullptr if simple unary test)
        std::vector<std::string> outputEntries; // Support multiple outputs (original FEEL expressions)
        std::vector<std::unique_ptr<ASTNode>> outputEntries_ast; // Pre-parsed output FEEL expressions
        std::string description; // Optional rule description

        // Backward compatibility - deprecated in favor of outputEntries
        std::string outputEntry;
        
        // Move semantics for unique_ptr members
        Rule() = default;
        ~Rule() = default;
        Rule(Rule&&) = default;
        Rule& operator=(Rule&&) = default;
        Rule(const Rule&) = delete;  // Cannot copy unique_ptr
        Rule& operator=(const Rule&) = delete;
    };

    /**
     * @brief Tabular decision logic structure
     * 
     * Represents if-then-else logic as a table with rows (rules) and columns (inputs/outputs).
     * Evaluates input data against rules to determine output using hit policy.
     * 
     * @see DMN 1.5 Specification Section 8.3.1 "Decision Table metamodel"
     * @see DMN 1.5 Specification Section 8.1 "Introduction" (Decision Table concepts)
     * @see DMN 1.5 Specification Section 8.2 "Notation" (Decision Table notation)
     * 
     * **Relationship**: Independent decision type, doesn't use BKMs or FEEL expressions
     */
    struct DecisionTable
    {
        std::string id;
        std::string name;
        HitPolicy hitPolicy{HitPolicy::FIRST};
        CollectAggregation aggregation{CollectAggregation::NONE}; // Only used when hitPolicy is COLLECT
        TableOrientation preferredOrientation{TableOrientation::RULE_AS_ROW};
        std::string outputLabel; // Label for the output column
        std::vector<InputClause> inputs;
        std::vector<OutputClause> outputs;
        std::vector<Rule> rules;

        // Move semantics (contains Rules with unique_ptrs)
        DecisionTable() = default;
        ~DecisionTable() = default;
        DecisionTable(DecisionTable&&) = default;
        DecisionTable& operator=(DecisionTable&&) = default;
        DecisionTable(const DecisionTable&) = delete;
        DecisionTable& operator=(const DecisionTable&) = delete;

        // Evaluate with given context
        [[nodiscard]] nlohmann::json evaluate(const nlohmann::json& context) const;

    private:
        // Helper functions for evaluate() - refactored from 507-line function
        void validate_input_values(const nlohmann::json& context) const;
        [[nodiscard]] std::vector<nlohmann::json> find_matching_rules(const nlohmann::json& context) const;
        [[nodiscard]] nlohmann::json apply_collect_aggregation(const std::vector<nlohmann::json>& matching_outputs) const;
        [[nodiscard]] nlohmann::json apply_priority_policy(const std::vector<nlohmann::json>& matching_outputs) const;
    };

    /**
     * @brief Reusable function encapsulating business logic
     * 
     * A function that can be invoked by LiteralDecisions or other BKMs.
     * Contains parameters, FEEL expression, and optional AST representation.
     * 
     * @see DMN 1.5 Specification Section 6.3.9 "Business Knowledge Model metamodel"
     * @see DMN 1.5 Specification Section 5.3.2 "Decision logic level"
     * @see DMN 1.5 Specification Section 7.3.6 "Invocation metamodel"
     * 
     * **Relationship**: 
     * - Called by LiteralDecision expressions (e.g., "PMT(amount, rate, term)")
     * - Can call other BKMs recursively
     * - Managed by BKMManager for lifecycle and invocation
     */
    struct BusinessKnowledgeModel; // Forward declaration - full definition in business_knowledge_model.hpp

    /**
     * @brief Decision using FEEL expression logic
     * 
     * Represents decisions defined by FEEL expressions that can:
     * - Access input data variables
     * - Invoke BusinessKnowledgeModel functions  
     * - Use arithmetic, logical, and built-in FEEL operations
     * 
     * @see DMN 1.5 Specification Section 7.3.5 "Literal expression metamodel"
     * @see DMN 1.5 Specification Section 10.2.1 "Boxed Expressions"
     * @see DMN 1.5 Specification Section 10.3 "Full FEEL Syntax and Semantics"
     * 
     * **Relationship**:
     * - Uses FEEL expressions from feel_expr.hpp for evaluation
     * - Can invoke BKMs by name (e.g., expression_text = "PMT(Loan.amount, Loan.rate, Loan.term)+fee")
     * - BKMs passed via available_bkms parameter for function resolution
     */
    struct LiteralDecision
    {
        std::string name;
        std::string expression_text; // Store original FEEL expression
        std::unique_ptr<ASTNode> expression_ast;

        // Evaluate with context
        [[nodiscard]] nlohmann::json evaluate(const nlohmann::json& context,
                                const std::map<std::string, BusinessKnowledgeModel>& available_bkms) const;
    };

    /**
     * @brief Information requirement between decisions
     * 
     * Represents dependency where one decision requires input from another decision or input data.
     * Maps to Information Requirements in DMN specification Section 6.3.13.
     * 
     * @see DMN 1.5 Specification Section 6.3.13 "Information Requirement metamodel"
     * @see DMN 1.5 Specification Section 6.2.2.1 "Information Requirement notation"
     */
    struct InformationRequirement
    {
        std::string id;
        std::string requiredDecisionId; // ID of required decision (optional)
        std::string requiredInputId; // ID of required input data (optional)
    };

    /**
     * @brief Knowledge requirement for BKM invocation
     * 
     * Represents dependency where decision logic invokes a Business Knowledge Model.
     * Maps to Knowledge Requirements in DMN specification Section 6.3.14.
     * 
     * @see DMN 1.5 Specification Section 6.3.14 "Knowledge Requirement metamodel" 
     * @see DMN 1.5 Specification Section 6.2.2.2 "Knowledge Requirement notation"
     */
    struct KnowledgeRequirement
    {
        std::string id;
        std::string requiredKnowledgeId; // ID of required BKM or Decision Service
    };

    /**
     * @brief Decision element in DMN model
     * 
     * Represents a single decision with its associated logic.
     * Can contain either a decision table or other expression logic.
     * 
     * @see DMN 1.5 Specification Section 6.3.7 "Decision metamodel"
     * @see DMN 1.5 Specification Section 6.2.1.1 "Decision notation"
     * @see DMN 1.5 Specification Section 5.3.1 "Decision requirements level"
     */
    struct Decision
    {
        std::string id;
        std::string name;
        std::string question; // Natural language question
        std::string allowedAnswers; // Description of allowed answers
        std::optional<DecisionVariable> variable; // Output variable with type info
        std::optional<DecisionTable> decisionTable;
        std::string expression; // For literal expressions like FEEL constants
        std::vector<InformationRequirement> informationRequirements;
        std::vector<KnowledgeRequirement> knowledgeRequirements;

        // Move semantics (contains non-copyable DecisionTable)
        Decision() = default;
        ~Decision() = default;
        Decision(Decision&&) = default;
        Decision& operator=(Decision&&) = default;
        Decision(const Decision&) = delete;
        Decision& operator=(const Decision&) = delete;
    };

    /**
     * @brief Internal utility functions for decision table evaluation
     * 
     * These helper functions support DecisionTable and LiteralDecision evaluation.
     * They are implementation details and should not be considered part of the stable API.
     */
    namespace detail
    {
        /**
         * @brief Retrieve value from JSON context using label with dotted path support
         * 
         * Supports both direct key lookup and dotted path resolution (e.g., "object.property").
         * 
         * @param ctx JSON context object
         * @param label Key or dotted path to resolve
         * @return JSON value at path, or empty JSON if not found
         */
        inline nlohmann::json get_value_from_label(const nlohmann::json& ctx, std::string_view label)
        {
            if (!ctx.is_object()) {
                return {};
            }

            // Direct key lookup
            auto iter = ctx.find(label);
            if (iter != ctx.end()) {
                return *iter;
            }

            // Support dotted path resolution (e.g., "object.property")
            if (label.find('.') != std::string::npos)
            {
                const nlohmann::json* node = &ctx;
                size_t start = 0;

                while (start < label.size())
                {
                    size_t dot = label.find('.', start);
                    std::string part(label.substr(start, dot == std::string::npos ? std::string::npos : dot - start));

                    if (node->is_object())
                    {
                        auto part_iter = node->find(part);
                        if (part_iter == node->end()) {
                            return {};
                        }

                        if (dot == std::string::npos)
                        {
                            return *part_iter; // Final part
                        }
                        
                        node = &*part_iter; // Continue traversal
                    }
                    else
                    {
                        return {};
                    }

                    if (dot == std::string::npos) {
                        break;
                    }
                    start = dot + 1;
                }
            }

            return {};
        }

        /**
         * @brief Check if a decision table entry token matches a given value
         * 
         * Supports DMN unary tests including dash ("-") for any match.
         * Handles both scalar values and arrays.
         * 
         * @param token DMN unary test expression or "-" for any
         * @param value JSON value to test against
         * @return true if token matches value, false otherwise
         */
        inline bool entry_matches(std::string_view token, const nlohmann::json& value)
        {
            if (token == "-" || token.empty()) {
                return true;
            }

            auto to_string_sv = [](const nlohmann::json& val) -> std::string
            {
                if (val.is_string()) {
                    return val.get<std::string>();
                }
                if (val.is_number_float())
                {
                    std::ostringstream oss;
                    oss << val.get<double>();
                    return oss.str();
                }
                if (val.is_number_integer()) {
                    return std::to_string(val.get<long long>());
                }
                if (val.is_number_unsigned()) {
                    return std::to_string(val.get<unsigned long long>());
                }
                if (val.is_boolean()) {
                    return val.get<bool>() ? "true" : "false";
                }
                return val.dump();
            };

            if (value.is_array())
            {
                return std::ranges::any_of(value, [&](const auto& element) {
                    return feel::unary_test_matches(token, to_string_sv(element));
                });
            }

            return feel::unary_test_matches(token, to_string_sv(value));
        }
    } // namespace detail

    // Factory functions for creating parsed components
    std::unique_ptr<DecisionTable> parse_decision_table(std::string_view dmn_xml, std::string& error_message);
    std::unique_ptr<LiteralDecision> parse_literal_decision(std::string_view dmn_xml, std::string& error_message);

} // namespace orion::bre
