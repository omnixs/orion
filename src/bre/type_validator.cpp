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
#include <format>

namespace orion::bre
{
    std::vector<std::string> parse_allowed_values(std::string_view allowed_values)
    {
        std::vector<std::string> values;

        if (allowed_values.empty())
        {
            return values;
        }

        // Parse comma-separated quoted strings: "Active", "Disabled", "Pending"
        bool in_quote = false;
        std::string current;

        for (size_t i = 0; i < allowed_values.length(); ++i)
        {
            char c = allowed_values[i];

            if (c == '"')
            {
                if (in_quote)
                {
                    // End of quoted value
                    values.push_back(current);
                    current.clear();
                    in_quote = false;
                }
                else
                {
                    // Start of quoted value
                    in_quote = true;
                }
            }
            else if (in_quote)
            {
                current += c;
            }
            // Ignore characters outside quotes (commas, whitespace, etc.)
        }

        return values;
    }

    bool validate_type_constraint(const nlohmann::json& value,
                                  const ItemDefinition& item_def)
    {
        // No constraints means any value is valid
        if (!item_def.has_constraints())
        {
            return true;
        }

        // Parse allowed values
        auto allowed = parse_allowed_values(item_def.allowedValues);

        if (allowed.empty())
        {
            // No specific values to check, constraint is satisfied
            return true;
        }

        // Handle collection types (validate each element)
        if (item_def.isCollection && value.is_array())
        {
            for (const auto& element : value)
            {
                // Each element must match one of the allowed values
                if (!element.is_string())
                {
                    return false; // Type mismatch
                }

                const std::string element_str = element.get<std::string>();
                if (!std::ranges::contains(allowed, element_str))
                {
                    return false; // Element not in allowed values
                }
            }
            return true;
        }

        // Handle single value
        if (!value.is_string())
        {
            // For now, only support string enumeration validation
            // Could extend to support number ranges, date ranges, etc.
            return true; // Don't fail for non-string types (lenient)
        }

        const std::string value_str = value.get<std::string>();
        return std::ranges::contains(allowed, value_str);
    }

    std::expected<void, std::string> validate_component(
        const nlohmann::json& comp_value,
        const ItemComponent& component,
        const std::map<std::string, ItemDefinition>& all_definitions,
        int depth
    ) noexcept
    {
        // Check component-level constraints
        if (component.has_constraints())
        {
            const auto allowed = parse_allowed_values(component.allowedValues);

            if (!allowed.empty())
            {
                if (!comp_value.is_string())
                {
                    return std::unexpected(
                        std::format("Component '{}' expected string value", component.name)
                    );
                }

                const std::string value_str = comp_value.get<std::string>();
                if (!std::ranges::contains(allowed, value_str))
                {
                    return std::unexpected(
                        std::format("Component '{}' value '{}' not in allowed values",
                                   component.name, value_str)
                    );
                }
            }
        }

        // Check if typeRef points to another ItemDefinition (complex type)
        if (const auto it = all_definitions.find(component.typeRef); it != all_definitions.end())
        {
            const auto& nested_def = it->second;

            if (nested_def.is_structured_type())
            {
                // Recursive validation for nested complex type
                if (auto result = validate_complex_type(comp_value, nested_def, all_definitions, depth + 1); !result)
                {
                    return result;
                }
            }
            else if (nested_def.has_constraints())
            {
                // Validate against simple type constraints
                if (!validate_type_constraint(comp_value, nested_def))
                {
                    return std::unexpected(
                        std::format("Component '{}' failed type constraint validation",
                                   component.name)
                    );
                }
            }
        }

        // Basic type validation could be added here
        // For now, accept any value if no constraints are violated
        return {};
    }

    std::expected<void, std::string> validate_complex_type(
        const nlohmann::json& value,
        const ItemDefinition& item_def,
        const std::map<std::string, ItemDefinition>& all_definitions,
        int depth
    ) noexcept
    {
        // Complex types must be JSON objects
        if (!value.is_object())
        {
            return std::unexpected(
                std::format("Expected object for structured type '{}', got {}",
                           item_def.name,
                           value.type_name())
            );
        }

        // Validate each component that is present (DMN fields are optional by default)
        for (const auto& component : item_def.itemComponents)
        {
            // Only validate components that are actually present in the input
            if (!value.contains(component.name))
            {
                continue; // Skip missing fields - they are optional
            }

            const auto& comp_value = value[component.name];

            // Handle collections (arrays)
            if (component.isCollection)
            {
                if (!comp_value.is_array())
                {
                    return std::unexpected(
                        std::format("Component '{}' must be an array (isCollection=true)",
                                   component.name)
                    );
                }

                // Validate each element in collection
                for (size_t i = 0; i < comp_value.size(); ++i)
                {
                    if (auto result = validate_component(comp_value[i], component, all_definitions, depth + 1); !result)
                    {
                        return std::unexpected(
                            std::format("Component '{}[{}]': {}",
                                       component.name, i, result.error())
                        );
                    }
                }
            }
            else
            {
                // Validate single value
                if (auto result = validate_component(comp_value, component, all_definitions, depth + 1); !result)
                {
                    return result;
                }
            }
        }

        return {};
    }

} // namespace orion::bre
