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

#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <map>

namespace orion::bre
{
    /**
     * @brief Business Knowledge Model implementation with contract enforcement
     */
    struct BusinessKnowledgeModel
    {
        std::string name;
        std::vector<std::string> parameters;
        std::string expression_text;

        /**
         * @brief Invoke the BKM with given arguments
         * @param args Resolved argument values
         * @param context JSON context for evaluation
         * @param available_bkms Map of available BKMs for recursive calls
         * @return Result of BKM evaluation
         * @throws contract_violation if preconditions are violated
         */
        [[nodiscard]] nlohmann::json invoke(const std::vector<nlohmann::json>& args,
                              const nlohmann::json& context,
                              const std::map<std::string, BusinessKnowledgeModel>& available_bkms) const;

        /**
         * @brief Validate BKM structure
         * @return true if BKM is valid, false otherwise
         */
        [[nodiscard]] bool is_valid() const noexcept
        {
            return !name.empty() && !expression_text.empty();
        }
    };
} // namespace orion::bre
