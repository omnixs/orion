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
 * @file type_validator.hpp
 * @brief DMN ItemDefinition type validation
 * 
 * Provides validation of values against DMN ItemDefinition constraints,
 * including allowedValues enumeration validation.
 * 
 * @see DMN 1.5 Specification Section 7.3.3 "ItemDefinition metamodel"
 */
#pragma once

#include "dmn_model.hpp"
#include <nlohmann/json.hpp>
#include <string_view>

namespace orion::bre
{
    /**
     * @brief Validates a value against ItemDefinition constraints
     * 
     * Checks if the given value satisfies the allowedValues constraint of the ItemDefinition.
     * For enumeration constraints like "Active", "Disabled", the value must match one of the
     * allowed string literals.
     * 
     * @param value Value to validate (JSON type)
     * @param item_def ItemDefinition with constraints
     * @return true if value is valid, false otherwise
     * 
     * @note Currently supports string enumeration validation only
     * @note Handles collection types (isCollection) by validating each element
     */
    [[nodiscard]] bool validate_type_constraint(const nlohmann::json& value,
                                                const ItemDefinition& item_def);

    /**
     * @brief Parse enumeration values from allowedValues string
     * 
     * Parses comma-separated quoted strings from allowedValues.
     * Example: "Active", "Disabled" -> ["Active", "Disabled"]
     * 
     * @param allowed_values String from ItemDefinition.allowedValues
     * @return Vector of allowed string values
     */
    [[nodiscard]] std::vector<std::string> parse_allowed_values(std::string_view allowed_values);

} // namespace orion::bre
