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

#include <orion/bre/bkm_manager.hpp>
#include <orion/api/logger.hpp>
#include <orion/bre/dmn_parser.hpp>
#include <orion/bre/feel/expr.hpp>
#include <orion/bre/feel/evaluator.hpp>
#include <orion/bre/contract_violation.hpp>
#include "../common/util.hpp"  // Reuse existing utility functions
#include "feel/util_internal.hpp"
#include <ctre.hpp>
#include <stdexcept>
#include <algorithm>
#include <set>       // For std::set

using json = nlohmann::json;
using std::string;
using std::vector;
using std::map;
using std::exception;

namespace orion::bre
{
    // Import logger functions
    using orion::api::error;
    using orion::api::debug;

    bool BKMManager::load_bkm_from_dmn(std::string_view dmn_xml, string& error_message, std::string_view bkm_name)
    {
        if (dmn_xml.empty()) [[unlikely]]
        {
            error_message = "DMN XML cannot be empty";
            return false;
        }

        try
        {
            // If bkm_name is empty, just try to parse the first available BKM
            auto bkm = parse_business_knowledge_model(dmn_xml, bkm_name, error_message);
            if (bkm) [[likely]]
            {
                add_bkm(std::move(bkm));
                return true;
            }
        return false;
    }
    catch (const exception& e)
    {
        error_message = e.what();
        error("BKM Manager: Exception loading BKM: {}", error_message);
        return false;
    }
}   void BKMManager::add_bkm(std::unique_ptr<BusinessKnowledgeModel> bkm)
    {
        if (!bkm) [[unlikely]]
        {
            THROW_CONTRACT_VIOLATION("BKM cannot be null");
        }
        if (bkm->name.empty()) [[unlikely]]
        {
            THROW_CONTRACT_VIOLATION("BKM name cannot be empty");
        }
        bkms_[bkm->name] = std::move(bkm);
    }

    json BKMManager::invoke_bkm(std::string_view bkm_name,
                               const vector<json>& args,
                                 const json& input,
                                 const EvaluationContext& eval_ctx) const
    {
        if (bkm_name.empty()) [[unlikely]]
        {
            THROW_CONTRACT_VIOLATION("BKM name cannot be empty");
        }

        auto bkm_iterator = bkms_.find(bkm_name);
        if (bkm_iterator == bkms_.end()) [[unlikely]]
        {
            throw std::runtime_error(std::string("BKM not found: ").append(bkm_name));
        }

        // Create BKM map for recursive calls
        auto bkm_map = create_bkm_map();

        return bkm_iterator->second->invoke(args, input, bkm_map, eval_ctx);
    }

    bool BKMManager::has_bkm(std::string_view bkm_name) const
    {
        return bkms_.find(bkm_name) != bkms_.end();
    }

    const BusinessKnowledgeModel* BKMManager::get_bkm(std::string_view bkm_name) const
    {
        if (bkm_name.empty()) [[unlikely]]
        {
            THROW_CONTRACT_VIOLATION("BKM name cannot be empty");
        }
        auto bkm_iterator = bkms_.find(bkm_name);
        return (bkm_iterator != bkms_.end()) ? bkm_iterator->second.get() : nullptr;
    }

    vector<string> BKMManager::get_bkm_names() const
    {
        vector<string> names;
        names.reserve(bkms_.size());
        for (const auto& name_bkm_pair : bkms_)
        {
            names.push_back(name_bkm_pair.first);
        }
        return names;
    }

    bool BKMManager::remove_bkm(std::string_view bkm_name)
    {
        if (bkm_name.empty()) [[unlikely]]
        {
            THROW_CONTRACT_VIOLATION("BKM name cannot be empty for removal");
        }
        auto it = bkms_.find(bkm_name);
        if (it == bkms_.end()) return false;
        bkms_.erase(it);
        return true;
    }

    void BKMManager::clear()
    {
        bkms_.clear();
    }

    map<string, const BusinessKnowledgeModel*, std::less<>> BKMManager::create_bkm_map() const
    {
        map<string, const BusinessKnowledgeModel*, std::less<>> bkm_map;
        for (const auto& bkm_pair : bkms_)
        {
            const string& name = bkm_pair.first;
            const auto& bkm_ptr = bkm_pair.second;
            if (bkm_ptr)
            {
                bkm_map[name] = bkm_ptr.get();
            }
        }
        return bkm_map;
    }

    // Helper function to get FEEL built-in functions
    static const std::set<std::string>& get_builtin_functions()
    {
        static const std::set<std::string> builtin_functions = {
            "all", "any", "sum", "count", "min", "max", "mean", "median", "mode", "stddev",
            "contains", "starts with", "ends with", "matches", "replace", "split",
            "substring", "string length", "upper case", "lower case",
            "abs", "ceiling", "floor", "round", "sqrt", "log", "exp",
            "date", "time", "date and time", "duration", "now", "today",
            "number", "string", "boolean", "list contains", "append", "concatenate",
            "not" // Add logical NOT function
        };
        return builtin_functions;
    }

    // Helper function to handle arithmetic operations on BKM results
    static nlohmann::json handle_arithmetic_remainder(const nlohmann::json& bkm_result, 
                                                       const std::string& remainder,
                                                       const nlohmann::json& input)
    {
        if (remainder.starts_with('+') && remainder.length() > 1)
        {
            std::string add_var = remainder.substr(1);
            // Trim whitespace
            add_var.erase(0, add_var.find_first_not_of(' '));
            add_var.erase(add_var.find_last_not_of(' ') + 1);
            
            debug("Extracted variable name: '{}'", add_var);

            if (input.contains(add_var) && input[add_var].is_number() && bkm_result.is_number())
            {
                double bkm_val = bkm_result.get<double>();
                double add_val = input[add_var].get<double>();
                double sum = bkm_val + add_val;
                nlohmann::json result = sum;  // Simplified construction
                
                return result;
            }
        }
        return bkm_result;
    }

    // Helper function to process BKM function call
    static nlohmann::json process_bkm_call(std::string_view func_name,
                                            std::string_view args_str,
                                            const nlohmann::json& input,
                                            const std::map<std::string, const BusinessKnowledgeModel*, std::less<>>& available_bkms,
                                            const EvaluationContext& eval_ctx)
    {
        // Check if this BKM exists
        auto bkm_it = available_bkms.find(func_name);
        if (bkm_it == available_bkms.end())
        {
            return feel::Evaluator::evaluate(std::string(func_name) + "(" + std::string(args_str) + ")", input, eval_ctx);
        }

        const BusinessKnowledgeModel& bkm = *bkm_it->second;

        // Parse arguments using existing utility function
        std::vector<std::string> args = orion::common::split(args_str, ',');

        // Trim whitespace from each argument
        for (auto& arg : args)
        {
            arg = orion::common::trim(arg);
        }

        // Resolve argument values
        std::vector<nlohmann::json> arg_values;
        for (const auto& arg : args)
        {
            nlohmann::json arg_val = detail::resolve_argument(arg, input);
            arg_values.push_back(arg_val);
        }

        // Invoke BKM
        return bkm.invoke(arg_values, input, available_bkms, eval_ctx);
    }

    // BKM-specific expression evaluator  
    json evaluate_bkm_expression(std::string_view expression,
                               const json& input,
                               const map<string, const BusinessKnowledgeModel*, std::less<>>& available_bkms,
                               const EvaluationContext& eval_ctx)
    {
        // Pattern: PMT(Loan.amount, Loan.rate, Loan.term)+fee
        // First, check if this contains a BKM function call
        // CTRE compile-time regex for BKM call pattern
        if (auto match = ctre::search<R"(\b([A-Za-z][A-Za-z0-9_]*)\s*\(\s*([^)]*)\s*\))">(expression))
        {
            std::string func_name = match.get<1>().to_string();
            std::string args_str = match.get<2>().to_string();
        
            debug("Found function call: {} with args: {}", func_name, args_str);

            // Check if this is a built-in function - if so, fall back to FEEL evaluation
            const auto& builtin_functions = get_builtin_functions();
            if (builtin_functions.find(func_name) != builtin_functions.end())
            {
                debug("Using FEEL evaluator for builtin function: {}", func_name);
                return feel::Evaluator::evaluate(expression, input, eval_ctx);
            }
            
            // Process BKM call
            nlohmann::json bkm_result = process_bkm_call(func_name, args_str, input, available_bkms, eval_ctx);

            // Check if there's additional arithmetic (e.g., +fee)
            std::string_view full_match = match.get<0>().to_view();
            size_t match_start = full_match.data() - expression.data();
            size_t match_end = match_start + full_match.size();

            if (match_end < expression.length())
            {
                std::string remainder(expression.substr(match_end));
                debug("Processing arithmetic remainder: '{}'", remainder);
                json final_result = handle_arithmetic_remainder(bkm_result, remainder, input);
                debug("Final result after arithmetic: {}", final_result.dump());
                return final_result;
            }

            return bkm_result;
        }

        // Use the full feel::Evaluator for logical and other complex expressions
        nlohmann::json result = feel::Evaluator::evaluate(expression, input, eval_ctx);
        if (!result.is_null())
        {
            return result;
        }

        // Fallback to basic FEEL evaluation for simple literals
        std::string error;
        if (feel::eval_feel_literal(expression, input, result, error, eval_ctx))
        {
            return result;
        }

        return nlohmann::json{};
    }
} // namespace orion::bre
