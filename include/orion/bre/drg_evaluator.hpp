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

/**
 * @file drg_evaluator.hpp
 * @brief Decision Requirements Graph (DRG) evaluation
 * 
 * Implements DMN 1.5 Level 2 support for evaluating decision graphs with dependencies.
 * Handles topological sorting, cycle detection, and cascading evaluation.
 * 
 * @see DMN 1.5 Specification Chapter 6 "Decision Requirements"
 * @see DMN 1.5 Specification Section 6.3 "Decision Requirements Graph metamodel"
 */
#pragma once

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <nlohmann/json.hpp>
#include "dmn_model.hpp"
#include "evaluation_context.hpp"

namespace orion::bre
{
    /**
     * @brief Evaluates Decision Requirements Graphs with dependency resolution
     * 
     * Handles:
     * - Building dependency graph from InformationRequirements
     * - Topological sorting for evaluation order
     * - Cycle detection (throws ContractViolation)
     * - Cascading evaluation with memoization
     * - Context propagation between decisions
     * 
     * @see DMN 1.5 Specification Section 6.3.13 "Information Requirement metamodel"
     */
    class DRGEvaluator
    {
    public:
        /**
         * @brief Default constructor - constructs empty evaluator
         */
        DRGEvaluator() = default;

        /**
         * @brief Construct evaluator with decision model
         * @param decisions Vector of decisions to evaluate (moved)
         */
        explicit DRGEvaluator(std::vector<Decision> decisions);

        /**
         * @brief Detect and describe any cycles in the graph
         * @return Empty string if no cycles, error description if cycles found
         */
        [[nodiscard]] std::string detect_cycles() const;

        /**
         * @brief Evaluate a specific decision with dependency resolution
         * 
         * Automatically evaluates all required decisions and propagates results.
         * 
         * @param decision_id ID of decision to evaluate
         * @param input Input context (JSON object)
         * @param eval_ctx Evaluation context for FEEL expressions
         * @return JSON result of decision evaluation
         * @throws std::runtime_error if decision not found or cycle detected
         */
        [[nodiscard]] nlohmann::json evaluate_decision(
            std::string_view decision_id,
            const nlohmann::json& input,
            EvaluationContext& eval_ctx) const;

        /**
         * @brief Get topologically sorted evaluation order for a specific decision
         * @param decision_name Name of the decision to get evaluation order for
         * @return Vector of decision names in evaluation order
         * @throws std::runtime_error if cycle detected or decision not found
         */
        [[nodiscard]] std::vector<std::string> get_evaluation_order(std::string_view decision_name) const;

        /**
         * @brief Get topologically sorted evaluation order for all decisions
         * @return Vector of decision IDs in evaluation order
         * @throws std::runtime_error if cycle detected
         */
        [[nodiscard]] std::vector<std::string> get_evaluation_order() const;

        /**
         * @brief Check if model contains cycles
         * @return true if cycles detected, false otherwise
         */
        [[nodiscard]] bool has_cycles() const;

    private:
        std::vector<Decision> decisions_; // Own copy of decisions
        
        // Build adjacency list representation of dependency graph
        [[nodiscard]] std::unordered_map<std::string, std::vector<std::string>> 
        build_dependency_graph() const;

        // Topological sort using Kahn's algorithm
        [[nodiscard]] std::vector<std::string> 
        topological_sort(const std::unordered_map<std::string, std::vector<std::string>>& graph) const;

        // Recursive evaluation with memoization
        [[nodiscard]] nlohmann::json evaluate_decision_recursive(
            std::string_view decision_id,
            const nlohmann::json& input,
            EvaluationContext& eval_ctx,
            std::unordered_map<std::string, nlohmann::json>& memo,
            std::unordered_set<std::string>& visiting) const;

        // Find decision by ID or name
        [[nodiscard]] const Decision* find_decision(std::string_view id) const;
        
        // Find decision by name
        [[nodiscard]] const Decision* find_decision_by_name(std::string_view name) const;
    };

} // namespace orion::bre
