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

#include <orion/bre/dmn_model.hpp>
#include <orion/api/logger.hpp>
#include <orion/bre/business_knowledge_model.hpp>
#include <orion/bre/bkm_manager.hpp>
#include <algorithm>
#include <stdexcept>

using namespace std;
using json = nlohmann::json;

namespace orion::bre
{
    // Import logger functions
    using orion::api::debug;

    // Helper: Validate input data against allowed values (DMN 1.5 Section 8.2.2)
    void DecisionTable::validate_input_values(const json& context) const
    {
        auto to_string_sv = [](const json& v) -> std::string
        {
            if (v.is_string()) return v.get<std::string>();
            if (v.is_number_integer()) return std::to_string(v.get<int64_t>());
            if (v.is_number_float()) return std::to_string(v.get<double>());
            if (v.is_boolean()) return v.get<bool>() ? "true" : "false";
            if (v.is_null()) return "null";
            return v.dump();
        };

        for (const auto& input : inputs)
        {
            if (!input.inputValues.empty())
            {
                json input_value = detail::get_value_from_label(context, input.label);
                // Only check if value is present in context
                if (!input_value.is_null())
                {
                    bool allowed = false;
                    for (const auto& allowed_val : input.inputValues)
                    {
                        // Compare as string for now (could enhance for FEEL types)
                        if (input_value.is_string())
                        {
                            if (input_value.get<std::string>() == allowed_val)
                            {
                                allowed = true;
                                break;
                            }
                        }
                        else
                        {
                            if (to_string_sv(input_value) == allowed_val)
                            {
                                allowed = true;
                                break;
                            }
                        }
                    }
                    if (!allowed)
                    {
                        throw std::runtime_error(
                            "Input value for '" + input.label + "' not in allowed values: " + to_string_sv(
                                input_value));
                    }
                }
            }
        }
    }

    // Helper: Collect all matching rules based on input conditions
    std::vector<json> DecisionTable::find_matching_rules(const json& context) const
    {
        vector<json> matching_outputs;

        for (const auto& rule : rules)
        {
            bool match = true;

            // Check if all input conditions match
            for (size_t i = 0; i < inputs.size() && i < rule.inputEntries.size(); i++)
            {
                const auto& input = inputs[i];
                const auto& entry = rule.inputEntries[i];

                json input_value = detail::get_value_from_label(context, input.label);

                // Phase 3: Use cached AST if available, otherwise fall back to unary_test_matches
                bool entry_matches_result = false;
                
                if (i < rule.inputEntries_ast.size() && rule.inputEntries_ast[i])
                {
                    // Use pre-parsed AST for complex FEEL expressions
                    try
                    {
                        json ast_result = rule.inputEntries_ast[i]->evaluate(context);
                        
                        // Compare AST result with input value
                        entry_matches_result = (ast_result == input_value);
                    }
                    catch (...)
                    {
                        // AST evaluation failed, fall back to unary_test_matches
                        entry_matches_result = detail::entry_matches(entry, input_value);
                    }
                }
                else
                {
                    // Use legacy unary_test_matches for simple comparisons/ranges
                    entry_matches_result = detail::entry_matches(entry, input_value);
                }

                if (!entry_matches_result)
                {
                    match = false;
                    break;
                }
            }

            if (match)
            {
                // Handle output based on table structure
                if (outputs.size() > 1)
                {
                    // Multi-output table
                    json result = json::object();

                    if (rule.outputEntries.size() >= outputs.size())
                    {
                        for (size_t i = 0; i < outputs.size(); i++)
                        {
                            const auto& output = outputs[i];
                            
                            // Evaluate output entry as FEEL expression using AST
                            json output_value;
                            if (i < rule.outputEntries_ast.size() && rule.outputEntries_ast[i])
                            {
                                // Use pre-parsed AST to evaluate output expression
                                try
                                {
                                    output_value = rule.outputEntries_ast[i]->evaluate(context);
                                }
                                catch (...)
                                {
                                    // Fallback to literal string if evaluation fails
                                    output_value = rule.outputEntries[i];
                                }
                            }
                            else
                            {
                                // Fallback: treat as literal string (remove quotes if present)
                                string literal_value = rule.outputEntries[i];
                                if (!literal_value.empty() && literal_value.front() == '"' && literal_value.back() == '"')
                                {
                                    literal_value = literal_value.substr(1, literal_value.length() - 2);
                                }
                                output_value = literal_value;
                            }

                            result[output.label] = output_value;
                        }

                        // For DMN multi-output tables, return the component structure directly
                        // The engine will wrap it in the decision name
                        matching_outputs.push_back(result);
                    }
                    else
                    {
                        // Fallback handling (legacy single outputEntry)
                        string output_value = rule.outputEntry;
                        if (!output_value.empty() && output_value.front() == '"' && output_value.back() == '"')
                        {
                            output_value = output_value.substr(1, output_value.length() - 2);
                        }

                        json matching_output = json::object();
                        matching_output[outputs[0].label] = output_value;
                        matching_outputs.push_back(matching_output);
                    }
                }
                else
                {
                    // Single output table
                    json output_value;
                    
                    // Try to use AST if available
                    if (!rule.outputEntries_ast.empty() && rule.outputEntries_ast[0])
                    {
                        // Use pre-parsed AST to evaluate output expression
                        try
                        {
                            output_value = rule.outputEntries_ast[0]->evaluate(context);
                        }
                        catch (...)
                        {
                            // Fallback to literal string if evaluation fails
                            string literal_value = !rule.outputEntries.empty() ? rule.outputEntries[0] : rule.outputEntry;
                            if (!literal_value.empty() && literal_value.front() == '"' && literal_value.back() == '"')
                            {
                                literal_value = literal_value.substr(1, literal_value.length() - 2);
                            }
                            output_value = literal_value;
                        }
                    }
                    else
                    {
                        // Fallback: use legacy outputEntry or first outputEntries element
                        string literal_value = rule.outputEntry;
                        if (literal_value.empty() && !rule.outputEntries.empty())
                        {
                            literal_value = rule.outputEntries[0];
                        }
                        
                        // Remove quotes if present
                        if (!literal_value.empty() && literal_value.front() == '"' && literal_value.back() == '"')
                        {
                            literal_value = literal_value.substr(1, literal_value.length() - 2);
                        }
                        output_value = literal_value;
                    }

                    // For collect policies, store as evaluated value for aggregation
                    if (hitPolicy == HitPolicy::COLLECT)
                    {
                        // Output value is already properly typed from AST evaluation
                        matching_outputs.emplace_back(output_value);
                    }
                    else
                    {
                        matching_outputs.push_back(output_value);
                    }
                }

                // For FIRST/UNIQUE/ANY hit policy, return immediately after first match
                // PRIORITY needs to collect all matches to compare priorities
                if (hitPolicy == HitPolicy::FIRST || hitPolicy == HitPolicy::UNIQUE ||
                    hitPolicy == HitPolicy::ANY)
                {
                    break; // Early exit - only first match needed
                }

                // For RULE_ORDER and OUTPUT_ORDER, continue collecting all matches
                // (RULE_ORDER collects in rule definition order, OUTPUT_ORDER sorts by output values)
            }
        }

        return matching_outputs;
    }

    // Helper: Apply COLLECT hit policy aggregations (SUM, COUNT, MIN, MAX, NONE)
    json DecisionTable::apply_collect_aggregation(const std::vector<json>& matching_outputs) const
    {
        switch (aggregation)
        {
        case CollectAggregation::SUM:
            {
                // Sum aggregation (C+)
                double sum = 0.0;
                for (const auto& output : matching_outputs)
                {
                    if (output.is_number())
                    {
                        sum += output.get<double>();
                    }
                    else if (output.is_string())
                    {
                        try
                        {
                            sum += std::stod(output.get<string>());
                        }
                        catch (...)
                        {
                            // Skip non-numeric strings
                        }
                    }
                }

                // For single-output tables, wrap in object with output name
                if (outputs.size() == 1 && !outputs[0].label.empty())
                {
                    json result = json::object();
                    result[outputs[0].label] = sum;
                    return result;
                }
                return json(sum);
            }
        case CollectAggregation::COUNT:
            {
                // Count aggregation (C#)
                double count = static_cast<double>(matching_outputs.size());

                // For single-output tables, wrap in object with output name
                if (outputs.size() == 1 && !outputs[0].label.empty())
                {
                    json result = json::object();
                    result[outputs[0].label] = count;
                    return result;
                }
                return json(count);
            }
        case CollectAggregation::MIN:
            {
                // Minimum aggregation (C<)
                double min_val = std::numeric_limits<double>::max();
                bool found_numeric = false;

                for (const auto& output : matching_outputs)
                {
                    double val = 0.0;
                    if (output.is_number())
                    {
                        val = output.get<double>();
                        found_numeric = true;
                    }
                    else if (output.is_string())
                    {
                        try
                        {
                            val = std::stod(output.get<string>());
                            found_numeric = true;
                        }
                        catch (...)
                        {
                            continue;
                        }
                    }

                    if (found_numeric && val < min_val)
                    {
                        min_val = val;
                    }
                }

                if (found_numeric)
                {
                    // For single-output tables, wrap in object with output name
                    if (outputs.size() == 1 && !outputs[0].label.empty())
                    {
                        json result = json::object();
                        result[outputs[0].label] = min_val;
                        return result;
                    }
                    return json(min_val);
                }
                return matching_outputs[0]; // Fallback
            }
        case CollectAggregation::MAX:
            {
                // Maximum aggregation (C>)
                double max_val = std::numeric_limits<double>::lowest();
                bool found_numeric = false;

                for (const auto& output : matching_outputs)
                {
                    double val = 0.0;
                    if (output.is_number())
                    {
                        val = output.get<double>();
                        found_numeric = true;
                    }
                    else if (output.is_string())
                    {
                        try
                        {
                            val = std::stod(output.get<string>());
                            found_numeric = true;
                        }
                        catch (...)
                        {
                            continue;
                        }
                    }

                    if (found_numeric && val > max_val)
                    {
                        max_val = val;
                    }
                }

                if (found_numeric)
                {
                    // For single-output tables, wrap in object with output name
                    if (outputs.size() == 1 && !outputs[0].label.empty())
                    {
                        json result = json::object();
                        result[outputs[0].label] = max_val;
                        return result;
                    }
                    return json(max_val);
                }
                return matching_outputs[0]; // Fallback
            }
        case CollectAggregation::NONE:
        default:
            {
                // No aggregation (C) - return all matches as array
                // For single-output tables, wrap in object with output name
                if (outputs.size() == 1 && !outputs[0].label.empty())
                {
                    json result = json::object();
                    result[outputs[0].label] = json(matching_outputs);
                    return result;
                }
                else if (outputs.size() == 1)
                {
                    // Fallback if label is empty - return the array directly
                    return json(matching_outputs);
                }
                return json(matching_outputs);
            }
        }
    }

    // Helper: Apply PRIORITY hit policy - select match with highest output priority
    json DecisionTable::apply_priority_policy(const std::vector<json>& matching_outputs) const
    {
        if (matching_outputs.size() == 1)
        {
            return matching_outputs[0];
        }

        // Compare each match to find the one with highest priority
        // For multi-output tables, compare lexicographically (first column, then second, etc.)
        int bestIndex = 0;

        for (size_t i = 1; i < matching_outputs.size(); ++i)
        {
            const json& best = matching_outputs[bestIndex];
            const json& current = matching_outputs[i];

            // Compare each output column according to priority defined in outputValues
            for (size_t colIdx = 0; colIdx < outputs.size(); ++colIdx)
            {
                const auto& outputClause = outputs[colIdx];
                const std::string& colName = outputClause.label;

                // Get values for this output column
                json bestVal = best.contains(colName) ? best[colName] : json{};
                json currentVal = current.contains(colName) ? current[colName] : json{};

                // If no priority list defined, can't prioritize - skip to next column
                if (outputClause.outputValues.empty())
                {
                    continue;
                }

                // Find priority indices (lower index = higher priority)
                int bestPriority = -1;
                int currentPriority = -1;

                for (size_t p = 0; p < outputClause.outputValues.size(); ++p)
                {
                    if (bestVal.is_string() && bestVal.get<std::string>() == outputClause.outputValues[p])
                    {
                        bestPriority = static_cast<int>(p);
                    }
                    if (currentVal.is_string() && currentVal.get<std::string>() == outputClause.outputValues[p])
                    {
                        currentPriority = static_cast<int>(p);
                    }
                }

                // Compare priorities (lower index = higher priority)
                if (currentPriority >= 0 && (bestPriority < 0 || currentPriority < bestPriority))
                {
                    bestIndex = i;
                    break; // Current is better, done with this comparison
                }
                else if (bestPriority >= 0 && (currentPriority < 0 || bestPriority < currentPriority))
                {
                    break; // Best is still better, done with this comparison
                }
                // If equal or both not found, continue to next column
            }
        }

        return matching_outputs[bestIndex];
    }

    // Implementation of DecisionTable::evaluate
    json DecisionTable::evaluate(const json& context) const
    {
        // Step 1: Validate input values against allowed values
        validate_input_values(context);

        // Step 2: Find all matching rules based on input conditions
        vector<json> matching_outputs = find_matching_rules(context);

        // Step 3: If no matches, return empty result
        if (matching_outputs.empty())
        {
            return json::object();
        }

        // Step 4: Handle early exit for FIRST/UNIQUE/ANY policies
        // (findMatchingRules already stopped at first match for these)
        if (hitPolicy == HitPolicy::FIRST || hitPolicy == HitPolicy::UNIQUE ||
            hitPolicy == HitPolicy::ANY)
        {
            return matching_outputs[0];
        }

        // Step 5: Apply hit policy logic
        if (hitPolicy == HitPolicy::COLLECT)
        {
            return apply_collect_aggregation(matching_outputs);
        }
        else if (hitPolicy == HitPolicy::RULE_ORDER)
        {
            // RULE_ORDER: Return all matches in rule definition order (already in order)
            return json(matching_outputs);
        }
        else if (hitPolicy == HitPolicy::OUTPUT_ORDER)
        {
            // OUTPUT_ORDER: Return all matches sorted by output values
            std::sort(matching_outputs.begin(), matching_outputs.end(),
                      [](const json& a, const json& b)
                      {
                          if (a.is_string() && b.is_string())
                          {
                              return a.get<std::string>() < b.get<std::string>();
                          }
                          else if (a.is_number() && b.is_number())
                          {
                              return a.get<double>() < b.get<double>();
                          }
                          else if (a.is_object() && b.is_object())
                          {
                              // For multi-output, compare first available component
                              for (auto it = a.begin(); it != a.end(); ++it)
                              {
                                  auto b_it = b.find(it.key());
                                  if (b_it != b.end())
                                  {
                                      if (it.value().is_string() && b_it.value().is_string())
                                      {
                                          return it.value().get<std::string>() < b_it.value().get<std::string>();
                                      }
                                      else if (it.value().is_number() && b_it.value().is_number())
                                      {
                                          return it.value().get<double>() < b_it.value().get<double>();
                                      }
                                  }
                              }
                          }
                          return false; // Default: maintain original order
                      });

            return json(matching_outputs);
        }
        else if (hitPolicy == HitPolicy::PRIORITY)
        {
            return apply_priority_policy(matching_outputs);
        }
        else
        {
            // Other non-collect hit policies: return first match
            return matching_outputs[0];
        }
    }

    // Implementation of LiteralDecision::evaluate
    json LiteralDecision::evaluate(const json& context,
                                   const std::map<std::string, BusinessKnowledgeModel>& available_bkms) const
    {
        if (expression_text.empty())
        {
            return json{}; // Return null for empty expressions
        }

        // Phase 3: Use cached AST if available for performance
        if (expression_ast)
        {
            try
            {
                json ast_result = expression_ast->evaluate(context);
                debug("LiteralDecision AST result for '{}': {}", expression_text, ast_result.dump());
                return ast_result;
            }
            catch (...)
            {
                // AST evaluation failed, fall back to legacy evaluation
                // This handles BKM calls and complex expressions not yet supported by AST
            }
        }

        // Fallback: Use evaluate_bkm_expression which handles both BKM calls and regular FEEL expressions
        json result = evaluate_bkm_expression(expression_text, context, available_bkms);
        return result;
    }
}
