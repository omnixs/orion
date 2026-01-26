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

#include <orion/bre/type_validator.hpp>
#include <algorithm>
#include <ranges>
#include <sstream>

namespace orion::bre
{
    namespace {
        // Helper: Extract quoted value from position, returns (value, next_position)
        [[nodiscard]] std::pair<std::string, size_t> extract_quoted_value(
            std::string_view allowed_values, size_t start_quote_pos)
        {
            std::string value;
            size_t i = start_quote_pos + 1; // Skip opening quote
            
            while (i < allowed_values.length() && allowed_values[i] != '"')
            {
                value += allowed_values[i];
                ++i;
            }

            return {value, i + 1};
        }

        // Helper: Check if value is in allowed list (uses std::ranges)
        [[nodiscard]] bool is_value_allowed(
            const nlohmann::json& value,
            std::string_view allowed_values) noexcept
        {
            if (!value.is_string()) return true; // Lenient
            const auto allowed = parse_allowed_values(allowed_values);
            return allowed.empty() || 
                   std::ranges::contains(allowed, value.get<std::string>());
        }

        // Helper: Check if array elements are in allowed list
        [[nodiscard]] bool are_array_values_allowed(
            const nlohmann::json& array,
            std::string_view allowed_values) noexcept
        {
            if (!array.is_array()) return false;
            const auto allowed = parse_allowed_values(allowed_values);
            if (allowed.empty()) return true;

            return std::ranges::all_of(array, 
                [&allowed](const nlohmann::json& elem) {
                    return elem.is_string() && 
                           std::ranges::contains(allowed, elem.get<std::string>());
                });
        }
    } // anonymous namespace

    [[nodiscard]] std::vector<std::string> parse_allowed_values(std::string_view allowed_values)
    {
        std::vector<std::string> values;
        if (allowed_values.empty()) {
            return values;
        }

        // Parse comma-separated quoted strings: "Active", "Disabled", "Pending"
        for (size_t i = 0; i < allowed_values.length(); ++i)
        {
            if (allowed_values[i] == '"')
            {
                auto [value, next_pos] = extract_quoted_value(allowed_values, i);
                values.push_back(value);
                i = next_pos;
            }
        }

        return values;
    }

    [[nodiscard]] bool validate_type_constraint(const nlohmann::json& value,
                                                const ItemDefinition& item_def)
    {
        // No constraints means any value is valid
        if (!item_def.has_constraints()) {
            return true;
        }

        // Use helper functions for constraint validation
        return item_def.isCollection 
            ? are_array_values_allowed(value, item_def.allowedValues)
            : is_value_allowed(value, item_def.allowedValues);
    }

    [[nodiscard]] std::expected<void, std::string> validate_component(
        const nlohmann::json& comp_value,
        const ItemComponent& component,
        const std::map<std::string, ItemDefinition>& all_definitions,
        int depth
    ) noexcept
    {
        // Validate component-level allowed values (if present)
        if (component.has_constraints() && !is_value_allowed(comp_value, component.allowedValues))
        {
            std::ostringstream oss;
            oss << "Component '" << component.name << "' value not in allowed values";
            return std::unexpected(oss.str());
        }

        // Validate against typeRef definition (if it references another ItemDefinition)
        if (const auto it = all_definitions.find(component.typeRef); 
            it != all_definitions.end())
        {
            const auto& nested_def = it->second;
            
            // Recursive validation for complex types
            if (nested_def.is_structured_type())
            {
                return validate_complex_type(comp_value, nested_def, all_definitions, depth + 1);
            }
            
            // Validate against simple type constraints
            if (nested_def.has_constraints() && !validate_type_constraint(comp_value, nested_def))
            {
                std::ostringstream oss;
                oss << "Component '" << component.name << "' failed type constraint";
                return std::unexpected(oss.str());
            }
        }

        return {};
    }

    [[nodiscard]] std::expected<void, std::string> validate_complex_type(
        const nlohmann::json& value,
        const ItemDefinition& item_def,
        const std::map<std::string, ItemDefinition>& all_definitions,
        int depth
    ) noexcept
    {
        // Complex types must be JSON objects
        if (!value.is_object())
        {
            std::ostringstream oss;
            oss << "Expected object for structured type '" << item_def.name << "', got " << value.type_name();
            return std::unexpected(oss.str());
        }

        // Validate each component that is present (DMN fields are optional)
        for (const auto& component : item_def.itemComponents)
        {
            if (!value.contains(component.name)) {
                continue; // Skip missing fields - they are optional
            }

            const auto& comp_value = value[component.name];

            // Handle collections (arrays)
            if (component.isCollection)
            {
                if (!comp_value.is_array())
                {
                    std::ostringstream oss;
                    oss << "Component '" << component.name << "' must be an array";
                    return std::unexpected(oss.str());
                }

                // Validate each element in collection
                for (size_t i = 0; i < comp_value.size(); ++i)
                {
                    if (auto result = validate_component(comp_value[i], component, 
                                                        all_definitions, depth + 1); !result)
                    {
                        std::ostringstream oss;
                        oss << "Component '" << component.name << "[" << i << "]: " << result.error();
                        return std::unexpected(oss.str());
                    }
                }
            }
            else if (auto result = validate_component(comp_value, component, 
                                                      all_definitions, depth + 1); !result)
            {
                return result;
            }
        }

        return {};
    }

} // namespace orion::bre
