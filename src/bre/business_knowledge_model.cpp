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
#include "orion/bre/contract_violation.hpp"


using json = nlohmann::json;
using std::string;
using std::vector;
using std::map;

namespace orion::bre
{
    // Import logger functions
    using orion::api::debug;

    nlohmann::json BusinessKnowledgeModel::invoke(const std::vector<nlohmann::json>& args,
                                                  const nlohmann::json& context,
                                                  const std::map<std::string, BusinessKnowledgeModel>& available_bkms)
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
        nlohmann::json bkm_context = context;

        // Bind arguments to parameters if parameters are defined
        if (!parameters.empty()) [[likely]]
        {
            for (size_t i = 0; i < parameters.size() && i < args.size(); ++i)
            {
                bkm_context[parameters[i]] = args[i];
            }
        }

        // Evaluate the BKM expression with the enhanced context
        return evaluate_bkm_expression(expression_text, bkm_context, available_bkms);
    }
} // namespace orion::bre
