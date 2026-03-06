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

#include <orion/bre/drg_evaluator.hpp>
#include <orion/bre/contract_violation.hpp>
#include <algorithm>
#include <utility>
#include <queue>
#include <sstream>
#include <stdexcept>

namespace orion::bre
{
    DRGEvaluator::DRGEvaluator(std::vector<Decision> decisions)
        : decisions_(std::move(decisions))  // Move decisions (take ownership)
    {
        // Validate no cycles at construction
        if (has_cycles())
        {
            THROW_CONTRACT_VIOLATION("DRG contains cyclic dependencies");
        }
        
        // Pre-compute and cache the evaluation order (done once, used many times)
        compute_evaluation_order();
    }
    
    void DRGEvaluator::compute_evaluation_order()
    {
        try
        {
            auto graph = build_dependency_graph();
            auto sorted_ids = topological_sort(graph);
            
            // Convert IDs to names for lookup in engine
            cached_evaluation_order_.clear();
            cached_evaluation_order_.reserve(sorted_ids.size());
            for (const auto& id : sorted_ids)
            {
                const Decision* dec = find_decision(id);
                if (dec)
                {
                    cached_evaluation_order_.push_back(dec->name);
                }
            }
        }
        catch (const std::runtime_error&)
        {
            // Should not happen since has_cycles() already passed
            cached_evaluation_order_.clear();
        }
    }

    std::string DRGEvaluator::detect_cycles() const
    {
        if (!has_cycles())
        {
            return "";
        }
        return "Cyclic dependency detected in decision graph";
    }

    const Decision* DRGEvaluator::find_decision(std::string_view id) const
    {
        auto it = std::ranges::find_if(decisions_, [&](const auto& decision) {
            return decision.id == id;
        });
        return it != decisions_.end() ? std::addressof(*it) : nullptr;
    }

    const Decision* DRGEvaluator::find_decision_by_name(std::string_view name) const
    {
        auto it = std::ranges::find_if(decisions_, [&](const auto& decision) {
            return decision.name == name;
        });
        return it != decisions_.end() ? std::addressof(*it) : nullptr;
    }

    std::unordered_map<std::string, std::vector<std::string>> 
    DRGEvaluator::build_dependency_graph() const
    {
        std::unordered_map<std::string, std::vector<std::string>> graph;

        // Initialize all decision nodes
        for (const auto& decision : decisions_)
        {
            graph[decision.id] = {};
        }

        // Build edges from information requirements
        for (const auto& decision : decisions_)
        {
            for (const auto& info_req : decision.informationRequirements)
            {
                // If this decision requires another decision, add edge
                if (!info_req.requiredDecisionId.empty())
                {
                    // Edge: requiredDecision -> decision (dependency direction)
                    graph[info_req.requiredDecisionId].push_back(decision.id);
                }
                // Note: requiredInputId refers to external input data, not decisions
            }
        }

        return graph;
    }

    std::vector<std::string> DRGEvaluator::topological_sort(
        const std::unordered_map<std::string, std::vector<std::string>>& graph) const
    {
        // Calculate in-degrees
        std::unordered_map<std::string, int> in_degree;
        for (const auto& [node, _] : graph)
        {
            in_degree[node] = 0;
        }
        for (const auto& [node, neighbors] : graph)
        {
            for (const auto& neighbor : neighbors)
            {
                in_degree[neighbor]++;
            }
        }

        // Kahn's algorithm
        std::queue<std::string> queue;
        for (const auto& [node, degree] : in_degree)
        {
            if (degree == 0)
            {
                queue.push(node);
            }
        }

        std::vector<std::string> sorted;
        while (!queue.empty())
        {
            std::string current = queue.front();
            queue.pop();
            sorted.push_back(current);

            if (graph.find(current) != graph.end())
            {
                for (const auto& neighbor : graph.at(current))
                {
                    in_degree[neighbor]--;
                    if (in_degree[neighbor] == 0)
                    {
                        queue.push(neighbor);
                    }
                }
            }
        }

        // If sorted contains fewer nodes than graph, there's a cycle
        if (sorted.size() != graph.size())
        {
            throw std::runtime_error("Cycle detected in Decision Requirements Graph");
        }

        return sorted;
    }

    bool DRGEvaluator::has_cycles() const
    {
        try
        {
            auto graph = build_dependency_graph();
            auto sorted = topological_sort(graph);
            // If topological sort succeeded and returned all nodes, no cycles exist
            return sorted.size() != graph.size();
        }
        catch (const std::runtime_error&)
        {
            return true;
        }
    }

    const std::vector<std::string>& DRGEvaluator::get_evaluation_order() const
    {
        return cached_evaluation_order_;
    }

    std::vector<std::string> DRGEvaluator::get_evaluation_order(std::string_view decision_name) const
    {
        // Find the decision
        const Decision* decision = find_decision_by_name(decision_name);
        if (!decision)
        {
            throw std::runtime_error("Decision not found: " + std::string(decision_name));
        }

        // Build full dependency graph
        auto graph = build_dependency_graph();
        
        // Get all dependencies for this decision using DFS
        std::vector<std::string> order;
        std::unordered_set<std::string> visited;
        std::unordered_set<std::string> visiting;
        
        std::function<void(const std::string&)> dfs = [&](const std::string& node_id)
        {
            if (visited.find(node_id) != visited.end())
            {
                return; // Already processed
            }
            
            if (visiting.find(node_id) != visiting.end())
            {
                throw std::runtime_error("Cycle detected");
            }
            
            visiting.insert(node_id);
            
            // Find this node's decision and process dependencies first
            const Decision* node_decision = find_decision(node_id);
            if (node_decision)
            {
                for (const auto& info_req : node_decision->informationRequirements)
                {
                    if (!info_req.requiredDecisionId.empty())
                    {
                        dfs(info_req.requiredDecisionId);
                    }
                }
            }
            
            visiting.erase(node_id);
            visited.insert(node_id);
            order.push_back(node_id);
        };
        
        // Start DFS from the target decision
        dfs(decision->id);
        
        // Convert IDs to names for the result
        std::vector<std::string> result;
        for (const auto& id : order)
        {
            const Decision* dec = find_decision(id);
            if (dec)
            {
                result.push_back(dec->name);
            }
        }
        
        return result;
    }

    nlohmann::json DRGEvaluator::evaluate_decision_recursive(
        std::string_view decision_id,
        const nlohmann::json& input,
        EvaluationContext& eval_ctx,
        std::unordered_map<std::string, nlohmann::json>& memo,
        std::unordered_set<std::string>& visiting) const
    {
        std::string id(decision_id);

        // Check for cycles during evaluation
        if (visiting.find(id) != visiting.end())
        {
            throw std::runtime_error("Cycle detected during evaluation: " + id);
        }

        // Check memoization
        if (auto memo_it = memo.find(id); memo_it != memo.end())
        {
            return memo_it->second;
        }

        // Find decision
        const Decision* decision = find_decision(decision_id);
        if (!decision)
        {
            throw std::runtime_error("Decision not found: " + id);
        }

        // Mark as visiting
        visiting.insert(id);

        // Build augmented context by evaluating dependencies
        nlohmann::json augmented_context = input;

        for (const auto& info_req : decision->informationRequirements)
        {
            if (!info_req.requiredDecisionId.empty())
            {
                // Recursively evaluate required decision
                nlohmann::json dep_result = evaluate_decision_recursive(
                    info_req.requiredDecisionId,
                    input,
                    eval_ctx,
                    memo,
                    visiting);

                // Add result to context using decision name
                const Decision* req_decision = find_decision(info_req.requiredDecisionId);
                if (req_decision)
                {
                    // Use decision name as key in context (per DMN spec)
                    augmented_context[req_decision->name] = std::move(dep_result);
                }
            }
            // requiredInputId: input data is already in context, no action needed
        }

        // Evaluate this decision with augmented context
        nlohmann::json result;

        if (decision->decisionTable.has_value())
        {
            // Evaluate decision table
            result = decision->decisionTable->evaluate(augmented_context, eval_ctx);
        }
        else if (!decision->expression.empty())
        {
            // Simple literal expression
            // For now, just return as string (full FEEL evaluation would be done by LiteralDecision)
            result = decision->expression;
        }
        else
        {
            throw std::runtime_error("Decision has no logic: " + id);
        }

        // Store in memo and return
        auto [it, _] = memo.emplace(std::move(id), std::move(result));

        // Unmark visiting
        visiting.erase(it->first);

        return it->second;
    }

    nlohmann::json DRGEvaluator::evaluate_decision(
        std::string_view decision_id,
        const nlohmann::json& input,
        EvaluationContext& eval_ctx) const
    {
        std::unordered_map<std::string, nlohmann::json> memo;
        std::unordered_set<std::string> visiting;

        return evaluate_decision_recursive(decision_id, input, eval_ctx, memo, visiting);
    }

} // namespace orion::bre
