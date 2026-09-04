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

#include <orion/bre/business_knowledge_model.hpp>
#include <orion/api/logger.hpp>
#include <orion/bre/bkm_manager.hpp>
#include <orion/bre/feel/evaluator.hpp>
#include "orion/bre/contract_violation.hpp"


using json = nlohmann::json;
using std::string;
using std::vector;
using std::map;

namespace orion::bre
{
    // Import logger functions
    using orion::api::debug;

    static bool matches_builtin_type(const json& value, std::string type_ref)
    {
        const auto separator = type_ref.rfind(':');
        if (separator != std::string::npos) type_ref = type_ref.substr(separator + 1);
        if (type_ref == "number") return value.is_number();
        if (type_ref == "string") return value.is_string();
        if (type_ref == "boolean") return value.is_boolean();
        if (type_ref == "date" || type_ref == "time" || type_ref == "date and time" ||
            type_ref == "duration") return value.is_string();
        return value.is_object();
    }

    static json coerce_bkm_result(json result, const BusinessKnowledgeModel& bkm)
    {
        if (bkm.result_type_ref.empty() || result.is_null()) return result;
        if (bkm.result_is_collection)
        {
            if (!result.is_array()) return nullptr;
            if (!bkm.result_element_type_ref.empty())
            {
                for (const auto& element : result)
                {
                    if (!matches_builtin_type(element, bkm.result_element_type_ref)) return nullptr;
                }
            }
            return result;
        }
        if (matches_builtin_type(result, bkm.result_type_ref)) return result;
        if (result.is_array() && result.size() == 1 &&
            matches_builtin_type(result.front(), bkm.result_type_ref))
        {
            return result.front();
        }
        return nullptr;
    }

    nlohmann::json BusinessKnowledgeModel::invoke(const std::vector<nlohmann::json>& args,
                                                  const nlohmann::json& input,
                                                  const std::map<std::string, const BusinessKnowledgeModel*, std::less<>>& available_bkms,
                                                  const EvaluationContext& eval_ctx)
    const
    {
        // Contract: BKM must have a name
        if (name.empty()) [[unlikely]]
        {
            THROW_CONTRACT_VIOLATION("BKM name cannot be empty during invocation");
        }

        // Contract: Expression must be non-empty
        if (expression_text.empty()) [[unlikely]]
        {
            THROW_CONTRACT_VIOLATION("BKM expression cannot be empty");
        }

        // DMN 1.5 flexible parameter handling: BKMs can accept variable arguments
        // Some DMN TCK tests may have BKMs with flexible parameter counts
        if (!parameters.empty() && args.size() != parameters.size()) [[unlikely]]
        {
            // Log warning but don't fail for DMN TCK compatibility
            debug(
                "BKM '{}': argument count ({}) differs from parameter count ({}), proceeding with available arguments",
                name, args.size(), parameters.size());
        }

        // Create parameter bindings for BKM evaluation
        nlohmann::json bkm_context = input;

        // Bind arguments to parameters if parameters are defined
        if (!parameters.empty()) [[likely]]
        {
            for (size_t i = 0; i < parameters.size() && i < args.size(); ++i)
            {
                bkm_context[parameters[i]] = args[i];
            }
        }

        EvaluationContext bkm_eval_ctx(eval_ctx.regex_cache);
        bkm_eval_ctx.bkm_map = &available_bkms;
        return coerce_bkm_result(
            feel::Evaluator::evaluate(expression_text, bkm_context, bkm_eval_ctx), *this);
    }
} // namespace orion::bre
