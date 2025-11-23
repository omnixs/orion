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

#include <string>
#include <nlohmann/json.hpp>

namespace orion::bre::feel {
    using json = nlohmann::json;

    /**
     * @brief FEEL Expression Evaluator
     * 
     * Evaluates FEEL expressions using AST-based parsing.
     * All FEEL features are now handled through the AST parser for optimal performance.
     */
    class Evaluator
    {
    public:
        /**
         * @brief Evaluate a FEEL expression
         * @param expression The FEEL expression to evaluate
         * @param context The evaluation context with variable bindings
         * @return The result of evaluation as JSON
         * @throws std::runtime_error if evaluation fails
         */
        [[nodiscard]] static json evaluate(std::string_view expression, const json& context = json::object());
    };
}
