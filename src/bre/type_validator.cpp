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
#include <sstream>

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
                
                std::string element_str = element.get<std::string>();
                if (std::find(allowed.begin(), allowed.end(), element_str) == allowed.end())
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
        
        std::string value_str = value.get<std::string>();
        return std::find(allowed.begin(), allowed.end(), value_str) != allowed.end();
    }

} // namespace orion::bre
