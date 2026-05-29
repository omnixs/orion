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

#include <orion/bre/feel/functions.hpp>
#include <orion/bre/feel/types.hpp>
#include <orion/bre/feel/evaluator.hpp>
#include <orion/bre/feel/regex_cache.hpp>
#include <orion/bre/contract_violation.hpp>
#include <cmath>
#include <stdexcept>
#include <string>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <iostream>

namespace orion::bre::feel {

json evaluate_not_function(const std::vector<json>& args)
{
    // Validate argument count
    if (args.size() != 1)
    {
        throw std::runtime_error("Function 'not' requires exactly 1 argument, got " + 
                                 std::to_string(args.size()));
    }

    const auto& arg = args[0];

    // DMN 1.5 Section 10.3.2.15: Null propagation
    // not(null) returns null
    if (arg.is_null())
    {
        return nullptr;
    }

    // Type validation - argument must be boolean
    if (!arg.is_boolean())
    {
        // Try to convert string representations to boolean
        if (arg.is_string())
        {
            std::string str = arg.get<std::string>();
            if (str == "true")
            {
                return false;
            }
            if (str == "false")
            {
                return true;
            }
        }
        
        throw std::runtime_error("Function 'not' requires boolean argument, got " + 
                                 std::string(arg.type_name()));
    }

    // Return negated value
    return !arg.get<bool>();
}

json evaluate_all_function(const std::vector<json>& args)
{
    // Validate argument count
    if (args.size() != 1)
    {
        throw std::runtime_error("Function 'all' requires exactly 1 argument, got " + 
                                 std::to_string(args.size()));
    }

    const auto& list = args[0];

    // DMN null propagation
    if (list.is_null())
    {
        return nullptr;
    }

    // Type validation - argument must be array
    if (!list.is_array())
    {
        throw std::runtime_error("Function 'all' requires array argument, got " + 
                                 std::string(list.type_name()));
    }

    // Empty list case: all([]) = true (vacuous truth)
    if (list.empty())
    {
        return true;
    }

    // Check all elements
    for (const auto& elem : list)
    {
        // Skip null elements (they don't affect result)
        if (elem.is_null())
        {
            continue;
        }

        // Type validation for each element
        if (!elem.is_boolean())
        {
            // Try to convert string representations
            if (elem.is_string())
            {
                std::string str = elem.get<std::string>();
                if (str == "false") { return false;
}
                if (str != "true")
                {
                    throw std::runtime_error("Function 'all' requires array of booleans, got string: " + str);
                }
                continue; // "true" doesn't affect all() result
            }
            
            throw std::runtime_error("Function 'all' requires array of booleans, got " + 
                                     std::string(elem.type_name()));
        }

        // If any element is false, return false
        if (!elem.get<bool>())
        {
            return false;
        }
    }

    // All elements are true
    return true;
}

json evaluate_any_function(const std::vector<json>& args)
{
    // Validate argument count
    if (args.size() != 1)
    {
        throw std::runtime_error("Function 'any' requires exactly 1 argument, got " + 
                                 std::to_string(args.size()));
    }

    const auto& list = args[0];

    // DMN null propagation
    if (list.is_null())
    {
        return nullptr;
    }

    // Type validation - argument must be array
    if (!list.is_array())
    {
        throw std::runtime_error("Function 'any' requires array argument, got " + 
                                 std::string(list.type_name()));
    }

    // Empty list case: any([]) = false
    if (list.empty())
    {
        return false;
    }

    // Check any element
    for (const auto& elem : list)
    {
        // Skip null elements
        if (elem.is_null())
        {
            continue;
        }

        // Type validation for each element
        if (!elem.is_boolean())
        {
            // Try to convert string representations
            if (elem.is_string())
            {
                std::string str = elem.get<std::string>();
                if (str == "true")
                {
                    return true;
                }
                if (str != "false")
                {
                    throw std::runtime_error("Function 'any' requires array of booleans, got string: " + str);
                }
                continue; // "false" doesn't affect any() result
            }
            
            throw std::runtime_error("Function 'any' requires array of booleans, got " + 
                                     std::string(elem.type_name()));
        }

        // If any element is true, return true
        if (elem.get<bool>())
        {
            return true;
        }
    }

    // No elements are true
    return false;
}

json evaluate_contains_function(const std::vector<json>& args)
{
    // Validate argument count
    if (args.size() != 2)
    {
        throw std::runtime_error("Function 'contains' requires exactly 2 arguments, got " + 
                                 std::to_string(args.size()));
    }

    const auto& str = args[0];
    const auto& substr = args[1];

    // DMN null propagation - if either argument is null, return null
    if (str.is_null() || substr.is_null())
    {
        return nullptr;
    }

    // Type validation - both arguments must be strings
    if (!str.is_string())
    {
        throw std::runtime_error("Function 'contains' requires string as first argument, got " + 
                                 std::string(str.type_name()));
    }

    if (!substr.is_string())
    {
        throw std::runtime_error("Function 'contains' requires string as second argument, got " + 
                                 std::string(substr.type_name()));
    }

    // Perform substring search
    std::string string_val = str.get<std::string>();
    std::string substring_val = substr.get<std::string>();

    return string_val.find(substring_val) != std::string::npos;
}

// ========== MATH FUNCTIONS IMPLEMENTATION ==========

json evaluate_abs_function(const std::vector<json>& args)
{
    // Validate argument count (already handled by parameter binder, but kept for safety)
    if (args.size() != 1)
    {
        return nullptr; // DMN spec: return null for invalid argument count
    }

    const auto& arg = args[0];

    // DMN null propagation
    if (arg.is_null())
    {
        return nullptr;
    }

    // Type validation - must be numeric (return null per DMN spec, don't throw)
    if (!arg.is_number())
    {
        return nullptr; // DMN spec: return null for invalid argument type
    }

    double value = arg.get<double>();
    return std::abs(value);
}

json evaluate_sqrt_function(const std::vector<json>& args)
{
    // Validate argument count
    if (args.size() != 1)
    {
        return nullptr; // DMN spec: return null for invalid argument count
    }

    const auto& arg = args[0];

    // DMN null propagation
    if (arg.is_null())
    {
        return nullptr;
    }

    // Type validation - must be numeric
    if (!arg.is_number())
    {
        return nullptr; // DMN spec: return null for invalid argument type
    }

    double value = arg.get<double>();
    
    // Square root of negative number is undefined
    if (value < 0.0)
    {
        return nullptr;
    }

    return std::sqrt(value);
}

json evaluate_floor_function(const std::vector<json>& args)
{
    // Validate argument count
    if (args.size() != 1)
    {
        throw std::runtime_error("Function 'floor' requires exactly 1 argument, got " + 
                                 std::to_string(args.size()));
    }

    const auto& arg = args[0];

    // DMN null propagation
    if (arg.is_null())
    {
        return nullptr;
    }

    // Type validation - must be numeric
    if (!arg.is_number())
    {
        throw std::runtime_error("Function 'floor' requires numeric argument, got " + 
                                 std::string(arg.type_name()));
    }

    double value = arg.get<double>();
    return std::floor(value);
}

json evaluate_ceiling_function(const std::vector<json>& args)
{
    // Validate argument count
    if (args.size() != 1)
    {
        throw std::runtime_error("Function 'ceiling' requires exactly 1 argument, got " + 
                                 std::to_string(args.size()));
    }

    const auto& arg = args[0];

    // DMN null propagation
    if (arg.is_null())
    {
        return nullptr;
    }

    // Type validation - must be numeric
    if (!arg.is_number())
    {
        throw std::runtime_error("Function 'ceiling' requires numeric argument, got " + 
                                 std::string(arg.type_name()));
    }

    double value = arg.get<double>();
    return std::ceil(value);
}

json evaluate_exp_function(const std::vector<json>& args)
{
    // Validate argument count
    if (args.size() != 1)
    {
        throw std::runtime_error("Function 'exp' requires exactly 1 argument, got " + 
                                 std::to_string(args.size()));
    }

    const auto& arg = args[0];

    // DMN null propagation
    if (arg.is_null())
    {
        return nullptr;
    }

    // Type validation - must be numeric
    if (!arg.is_number())
    {
        throw std::runtime_error("Function 'exp' requires numeric argument, got " + 
                                 std::string(arg.type_name()));
    }

    double value = arg.get<double>();
    return std::exp(value);
}

json evaluate_log_function(const std::vector<json>& args)
{
    // Validate argument count
    if (args.size() != 1)
    {
        throw std::runtime_error("Function 'log' requires exactly 1 argument, got " + 
                                 std::to_string(args.size()));
    }

    const auto& arg = args[0];

    // DMN null propagation
    if (arg.is_null())
    {
        return nullptr;
    }

    // Type validation - must be numeric
    if (!arg.is_number())
    {
        throw std::runtime_error("Function 'log' requires numeric argument, got " + 
                                 std::string(arg.type_name()));
    }

    double value = arg.get<double>();
    
    // Log of non-positive number is undefined
    if (value <= 0.0)
    {
        return nullptr;
    }

    return std::log(value);
}

json evaluate_modulo_function(const std::vector<json>& args)
{
    // Validate argument count
    if (args.size() != 2)
    {
        throw std::runtime_error("Function 'modulo' requires exactly 2 arguments, got " + 
                                 std::to_string(args.size()));
    }

    const auto& dividend = args[0];
    const auto& divisor = args[1];

    // DMN null propagation
    if (dividend.is_null() || divisor.is_null())
    {
        return nullptr;
    }

    // Type validation - both must be numeric
    if (!dividend.is_number())
    {
        throw std::runtime_error("Function 'modulo' requires numeric first argument, got " + 
                                 std::string(dividend.type_name()));
    }

    if (!divisor.is_number())
    {
        throw std::runtime_error("Function 'modulo' requires numeric second argument, got " + 
                                 std::string(divisor.type_name()));
    }

    double div_value = dividend.get<double>();
    double divisor_value = divisor.get<double>();
    
    // Modulo by zero is undefined
    if (divisor_value == 0.0)
    {
        return nullptr;
    }

    // DMN 1.5 spec: modulo(dividend, divisor) = dividend - divisor * floor(dividend / divisor)
    double result = div_value - divisor_value * std::floor(div_value / divisor_value);
    return result;
}

json evaluate_decimal_function(const std::vector<json>& args)
{
    // Validate argument count
    if (args.size() != 2)
    {
        throw std::runtime_error("Function 'decimal' requires exactly 2 arguments, got " + 
                                 std::to_string(args.size()));
    }

    const auto& num = args[0];
    const auto& scale = args[1];

    // DMN null propagation
    if (num.is_null() || scale.is_null())
    {
        return nullptr;
    }

    // Type validation - both must be numeric
    if (!num.is_number())
    {
        throw std::runtime_error("Function 'decimal' requires numeric first argument, got " + 
                                 std::string(num.type_name()));
    }

    if (!scale.is_number())
    {
        throw std::runtime_error("Function 'decimal' requires numeric second argument, got " + 
                                 std::string(scale.type_name()));
    }

    double value = num.get<double>();
    int scale_value = static_cast<int>(scale.get<double>());

    // Use half-even rounding (banker's rounding)
    double multiplier = std::pow(10.0, scale_value);
    double scaled = value * multiplier;
    double rounded = std::nearbyint(scaled);  // nearbyint uses current rounding mode (default is half-even)
    return rounded / multiplier;
}

json evaluate_round_function(const std::vector<json>& args)
{
    // Validate argument count
    if (args.size() != 2)
    {
        throw std::runtime_error("Function 'round' requires exactly 2 arguments, got " + 
                                 std::to_string(args.size()));
    }

    const auto& num = args[0];
    const auto& scale = args[1];

    // DMN null propagation
    if (num.is_null() || scale.is_null())
    {
        return nullptr;
    }

    // Type validation - both must be numeric
    if (!num.is_number())
    {
        throw std::runtime_error("Function 'round' requires numeric first argument, got " + 
                                 std::string(num.type_name()));
    }

    if (!scale.is_number())
    {
        throw std::runtime_error("Function 'round' requires numeric second argument, got " + 
                                 std::string(scale.type_name()));
    }

    double value = num.get<double>();
    int scale_value = static_cast<int>(scale.get<double>());

    // Use half-even rounding (banker's rounding)
    double multiplier = std::pow(10.0, scale_value);
    double scaled = value * multiplier;
    double rounded = std::nearbyint(scaled);  // nearbyint uses current rounding mode (default is half-even)
    return rounded / multiplier;
}

json evaluate_round_up_function(const std::vector<json>& args)
{
    // Validate argument count
    if (args.size() != 2)
    {
        throw std::runtime_error("Function 'round up' requires exactly 2 arguments, got " + 
                                 std::to_string(args.size()));
    }

    const auto& num = args[0];
    const auto& scale = args[1];

    // DMN null propagation
    if (num.is_null() || scale.is_null())
    {
        return nullptr;
    }

    // Type validation - both must be numeric
    if (!num.is_number())
    {
        throw std::runtime_error("Function 'round up' requires numeric first argument, got " + 
                                 std::string(num.type_name()));
    }

    if (!scale.is_number())
    {
        throw std::runtime_error("Function 'round up' requires numeric second argument, got " + 
                                 std::string(scale.type_name()));
    }

    double value = num.get<double>();
    int scale_value = static_cast<int>(scale.get<double>());

    // Round away from zero
    double multiplier = std::pow(10.0, scale_value);
    double scaled = value * multiplier;
    double rounded = (value >= 0.0) ? std::ceil(scaled) : std::floor(scaled);
    return rounded / multiplier;
}

json evaluate_round_down_function(const std::vector<json>& args)
{
    // Validate argument count
    if (args.size() != 2)
    {
        throw std::runtime_error("Function 'round down' requires exactly 2 arguments, got " + 
                                 std::to_string(args.size()));
    }

    const auto& num = args[0];
    const auto& scale = args[1];

    // DMN null propagation
    if (num.is_null() || scale.is_null())
    {
        return nullptr;
    }

    // Type validation - both must be numeric
    if (!num.is_number())
    {
        throw std::runtime_error("Function 'round down' requires numeric first argument, got " + 
                                 std::string(num.type_name()));
    }

    if (!scale.is_number())
    {
        throw std::runtime_error("Function 'round down' requires numeric second argument, got " + 
                                 std::string(scale.type_name()));
    }

    double value = num.get<double>();
    int scale_value = static_cast<int>(scale.get<double>());

    // Round toward zero
    double multiplier = std::pow(10.0, scale_value);
    double scaled = value * multiplier;
    double rounded = (value >= 0.0) ? std::floor(scaled) : std::ceil(scaled);
    return rounded / multiplier;
}

json evaluate_round_half_up_function(const std::vector<json>& args)
{
    // Validate argument count
    if (args.size() != 2)
    {
        throw std::runtime_error("Function 'round half up' requires exactly 2 arguments, got " + 
                                 std::to_string(args.size()));
    }

    const auto& num = args[0];
    const auto& scale = args[1];

    // DMN null propagation
    if (num.is_null() || scale.is_null())
    {
        return nullptr;
    }

    // Type validation - both must be numeric
    if (!num.is_number())
    {
        throw std::runtime_error("Function 'round half up' requires numeric first argument, got " + 
                                 std::string(num.type_name()));
    }

    if (!scale.is_number())
    {
        throw std::runtime_error("Function 'round half up' requires numeric second argument, got " + 
                                 std::string(scale.type_name()));
    }

    double value = num.get<double>();
    int scale_value = static_cast<int>(scale.get<double>());

    // Standard rounding: 0.5 rounds away from zero
    double multiplier = std::pow(10.0, scale_value);
    double scaled = value * multiplier;
    double rounded = (value >= 0.0) ? std::floor(scaled + 0.5) : std::ceil(scaled - 0.5);
    return rounded / multiplier;
}

json evaluate_round_half_down_function(const std::vector<json>& args)
{
    // Validate argument count
    if (args.size() != 2)
    {
        throw std::runtime_error("Function 'round half down' requires exactly 2 arguments, got " + 
                                 std::to_string(args.size()));
    }

    const auto& num = args[0];
    const auto& scale = args[1];

    // DMN null propagation
    if (num.is_null() || scale.is_null())
    {
        return nullptr;
    }

    // Type validation - both must be numeric
    if (!num.is_number())
    {
        throw std::runtime_error("Function 'round half down' requires numeric first argument, got " + 
                                 std::string(num.type_name()));
    }

    if (!scale.is_number())
    {
        throw std::runtime_error("Function 'round half down' requires numeric second argument, got " + 
                                 std::string(scale.type_name()));
    }

    double value = num.get<double>();
    int scale_value = static_cast<int>(scale.get<double>());

    // Half-down rounding: 0.5 rounds toward zero
    double multiplier = std::pow(10.0, scale_value);
    double scaled = value * multiplier;
    double rounded = (value >= 0.0) ? std::ceil(scaled - 0.5) : std::floor(scaled + 0.5);
    return rounded / multiplier;
}

// ========== STRING FUNCTIONS IMPLEMENTATION ==========

json evaluate_substring_before_function(const std::vector<json>& args)
{
    // Validate argument count
    if (args.size() != 2)
    {
        return nullptr; // DMN spec: return null for invalid argument count
    }

    const auto& str = args[0];
    const auto& match = args[1];

    // DMN null propagation
    if (str.is_null() || match.is_null())
    {
        return nullptr;
    }

    // Type validation - both must be strings
    if (!str.is_string() || !match.is_string())
    {
        return nullptr; // DMN spec: return null for invalid argument type
    }

    std::string string_val = str.get<std::string>();
    std::string match_val = match.get<std::string>();

    // Find first occurrence of match string
    size_t pos = string_val.find(match_val);
    
    // If not found, return empty string per DMN spec
    if (pos == std::string::npos)
    {
        return std::string("");
    }

    // Return substring before the match
    return string_val.substr(0, pos);
}

json evaluate_substring_after_function(const std::vector<json>& args)
{
    // Validate argument count
    if (args.size() != 2)
    {
        return nullptr; // DMN spec: return null for invalid argument count
    }

    const auto& str = args[0];
    const auto& match = args[1];

    // DMN null propagation
    if (str.is_null() || match.is_null())
    {
        return nullptr;
    }

    // Type validation - both must be strings
    if (!str.is_string() || !match.is_string())
    {
        return nullptr; // DMN spec: return null for invalid argument type
    }

    std::string string_val = str.get<std::string>();
    std::string match_val = match.get<std::string>();

    // Find first occurrence of match string
    size_t pos = string_val.find(match_val);
    
    // If not found, return empty string per DMN spec
    if (pos == std::string::npos)
    {
        return std::string("");
    }

    // Return substring after the match
    return string_val.substr(pos + match_val.length());
}

json evaluate_substring_function(const std::vector<json>& args)
{
    // Validate argument count (2 or 3 arguments - length is optional)
    if (args.size() < 2 || args.size() > 3)
    {
        return nullptr; // DMN spec: return null for invalid argument count
    }

    const auto& str = args[0];
    const auto& start_pos = args[1];

    // DMN null propagation
    if (str.is_null() || start_pos.is_null())
    {
        return nullptr;
    }

    // Type validation
    if (!str.is_string() || !start_pos.is_number())
    {
        return nullptr; // DMN spec: return null for invalid argument type
    }

    std::string string_val = str.get<std::string>();
    
    // DMN uses 1-based indexing
    int start_index = static_cast<int>(start_pos.get<double>()) - 1;

    // Handle negative start position (from end)
    if (start_index < 0)
    {
        start_index = static_cast<int>(string_val.length()) + start_index + 1;
    }

    // Validate bounds
    if (start_index < 0 || start_index >= static_cast<int>(string_val.length()))
    {
        return std::string(""); // Return empty string for out of bounds
    }

    // If length is provided
    if (args.size() == 3)
    {
        const auto& length = args[2];
        
        if (length.is_null())
        {
            // If length is null, return substring from start to end
            return string_val.substr(start_index);
        }

        if (!length.is_number())
        {
            return nullptr; // DMN spec: return null for invalid argument type
        }

        int len = static_cast<int>(length.get<double>());
        
        // Negative length returns empty string
        if (len < 0)
        {
            return std::string("");
        }

        return string_val.substr(start_index, len);
    }

    // No length provided - return from start to end
    return string_val.substr(start_index);
}

json evaluate_string_length_function(const std::vector<json>& args)
{
    // Validate argument count
    if (args.size() != 1)
    {
        return nullptr; // DMN spec: return null for invalid argument count
    }

    const auto& str = args[0];

    // DMN null propagation
    if (str.is_null())
    {
        return nullptr;
    }

    // Type validation - must be string
    if (!str.is_string())
    {
        return nullptr; // DMN spec: return null for invalid argument type
    }

    std::string string_val = str.get<std::string>();
    return static_cast<int>(string_val.length());
}

json evaluate_upper_case_function(const std::vector<json>& args)
{
    // Validate argument count
    if (args.size() != 1)
    {
        return nullptr; // DMN spec: return null for invalid argument count
    }

    const auto& str = args[0];

    // DMN null propagation
    if (str.is_null())
    {
        return nullptr;
    }

    // Type validation - must be string
    if (!str.is_string())
    {
        return nullptr; // DMN spec: return null for invalid argument type
    }

    std::string string_val = str.get<std::string>();
    std::transform(string_val.begin(), string_val.end(), string_val.begin(), ::toupper);
    return string_val;
}

json evaluate_lower_case_function(const std::vector<json>& args)
{
    // Validate argument count
    if (args.size() != 1)
    {
        return nullptr; // DMN spec: return null for invalid argument count
    }

    const auto& str = args[0];

    // DMN null propagation
    if (str.is_null())
    {
        return nullptr;
    }

    // Type validation - must be string
    if (!str.is_string())
    {
        return nullptr; // DMN spec: return null for invalid argument type
    }

    std::string string_val = str.get<std::string>();
    std::transform(string_val.begin(), string_val.end(), string_val.begin(), ::tolower);
    return string_val;
}

json evaluate_starts_with_function(const std::vector<json>& args)
{
    // Validate argument count
    if (args.size() != 2)
    {
        return nullptr; // DMN spec: return null for invalid argument count
    }

    const auto& str = args[0];
    const auto& prefix = args[1];

    // DMN null propagation
    if (str.is_null() || prefix.is_null())
    {
        return nullptr;
    }

    // Type validation - both must be strings
    if (!str.is_string() || !prefix.is_string())
    {
        return nullptr; // DMN spec: return null for invalid argument type
    }

    std::string string_val = str.get<std::string>();
    std::string prefix_val = prefix.get<std::string>();

    // Check if string starts with prefix
    return string_val.starts_with(prefix_val);
}

json evaluate_ends_with_function(const std::vector<json>& args)
{
    // Validate argument count
    if (args.size() != 2)
    {
        return nullptr; // DMN spec: return null for invalid argument count
    }

    const auto& str = args[0];
    const auto& suffix = args[1];

    // DMN null propagation
    if (str.is_null() || suffix.is_null())
    {
        return nullptr;
    }

    // Type validation - both must be strings
    if (!str.is_string() || !suffix.is_string())
    {
        return nullptr; // DMN spec: return null for invalid argument type
    }

    std::string string_val = str.get<std::string>();
    std::string suffix_val = suffix.get<std::string>();

    // Check if string ends with suffix
    return string_val.ends_with(suffix_val);
}

json evaluate_replace_function(const std::vector<json>& args)
{
    // Validate argument count (3 or 4 - flags is optional)
    if (args.size() < 3 || args.size() > 4)
    {
        return nullptr; // DMN spec: return null for invalid argument count
    }

    const auto& input = args[0];
    const auto& pattern = args[1];
    const auto& replacement = args[2];

    // DMN null propagation
    if (input.is_null() || pattern.is_null() || replacement.is_null())
    {
        return nullptr;
    }

    // Type validation
    if (!input.is_string() || !pattern.is_string() || !replacement.is_string())
    {
        return nullptr; // DMN spec: return null for invalid argument type
    }

    std::string input_val = input.get<std::string>();
    std::string pattern_val = pattern.get<std::string>();
    std::string replacement_val = replacement.get<std::string>();

    // For now, implement simple string replacement (not regex)
    // TODO: Add regex support when flags parameter is used
    std::string result = input_val;
    size_t pos = 0;
    
    // Replace all occurrences
    while ((pos = result.find(pattern_val, pos)) != std::string::npos)
    {
        result.replace(pos, pattern_val.length(), replacement_val);
        pos += replacement_val.length();
    }

    return result;
}

json evaluate_matches_function(const std::vector<json>& args, const EvaluationContext& eval_ctx)
{
    // Validate argument count (2 or 3 - flags is optional)
    if (args.size() < 2 || args.size() > 3)
    {
        return nullptr; // DMN spec: return null for invalid argument count
    }

    const auto& input = args[0];
    const auto& pattern = args[1];

    // DMN null propagation for input and pattern
    if (input.is_null() || pattern.is_null())
    {
        return nullptr;
    }

    // Type validation
    if (!input.is_string() || !pattern.is_string())
    {
        return nullptr; // DMN spec: return null for invalid argument type
    }

    std::string input_val = input.get<std::string>();
    std::string pattern_val = pattern.get<std::string>();

    // DMN spec: empty pattern matches only empty input
    if (pattern_val.empty()) {
        return input_val.empty();
    }

    // Handle optional flags parameter
    std::string flags_val;
    if (args.size() == 3)
    {
        const auto& flags = args[2];
        // DMN spec: null flags is treated as empty string (no flags)
        if (!flags.is_null())
        {
            if (!flags.is_string())
            {
                return nullptr; // Invalid flags type
            }
            flags_val = flags.get<std::string>();
        }
    }

    // FEEL strings use double-backslash for a literal backslash, but PCRE2 expects single
    // Unescape FEEL string escape sequences: double-backslash becomes single-backslash
    std::string unescaped_pattern;
    unescaped_pattern.reserve(pattern_val.size());
    for (size_t i = 0; i < pattern_val.size(); ++i) {
        if (pattern_val[i] == '\\' && i + 1 < pattern_val.size() && pattern_val[i + 1] == '\\') {
            unescaped_pattern += '\\';
            ++i; // Skip next backslash
        } else {
            unescaped_pattern += pattern_val[i];
        }
    }

    // Use engine-scoped cache (required - no fallback)
    auto compiled = eval_ctx.regex_cache.get_or_compile(unescaped_pattern, flags_val);
    
    if (!compiled) {
        // Invalid regex pattern or invalid flags - DMN spec says return null
        return nullptr;
    }
    
    bool match_result = compiled->matches(input_val);
    return match_result;
}

json evaluate_split_function(const std::vector<json>& args)
{
    // Validate argument count
    if (args.size() != 2)
    {
        return nullptr; // DMN spec: return null for invalid argument count
    }

    const auto& str = args[0];
    const auto& delimiter = args[1];

    // DMN null propagation
    if (str.is_null() || delimiter.is_null())
    {
        return nullptr;
    }

    // Type validation - both must be strings
    if (!str.is_string() || !delimiter.is_string())
    {
        return nullptr; // DMN spec: return null for invalid argument type
    }

    std::string string_val = str.get<std::string>();
    std::string delimiter_val = delimiter.get<std::string>();

    json result_array = json::array();

    // Handle empty delimiter - return array with each character
    if (delimiter_val.empty())
    {
        for (char ch : string_val)
        {
            result_array.push_back(std::string(1, ch));
        }
        return result_array;
    }

    // Split by delimiter
    size_t start = 0;
    size_t end = string_val.find(delimiter_val);

    while (end != std::string::npos)
    {
        result_array.push_back(string_val.substr(start, end - start));
        start = end + delimiter_val.length();
        end = string_val.find(delimiter_val, start);
    }

    // Add the last part
    result_array.push_back(string_val.substr(start));

    return result_array;
}

json evaluate_string_join_function(const std::vector<json>& args)
{
    // Validate argument count (1 or 2 - delimiter is optional)
    if (args.empty() || args.size() > 2)
    {
        return nullptr; // DMN spec: return null for invalid argument count
    }

    const auto& list = args[0];

    // DMN null propagation
    if (list.is_null())
    {
        return nullptr;
    }

    // Type validation - first argument must be array
    if (!list.is_array())
    {
        return nullptr; // DMN spec: return null for invalid argument type
    }

    // Get delimiter (default is empty string if not provided)
    std::string delimiter = "";
    if (args.size() == 2)
    {
        if (args[1].is_null())
        {
            // null delimiter means use empty string
            delimiter = "";
        }
        else if (args[1].is_string())
        {
            delimiter = args[1].get<std::string>();
        }
        else
        {
            return nullptr; // Invalid delimiter type
        }
    }

    // Join the list elements
    std::string result;
    bool first = true;

    for (const auto& element : list)
    {
        if (!first)
        {
            result += delimiter;
        }
        first = false;

        // Convert element to string
        if (element.is_string())
        {
            result += element.get<std::string>();
        }
        else if (element.is_number())
        {
            // Convert number to string
            double num = element.get<double>();
            // Check if it's an integer
            if (num == static_cast<int>(num))
            {
                result += std::to_string(static_cast<int>(num));
            }
            else
            {
                result += std::to_string(num);
            }
        }
        else if (element.is_boolean())
        {
            result += element.get<bool>() ? "true" : "false";
        }
        else if (element.is_null())
        {
            // Skip null elements or treat as empty string
            // DMN spec is unclear here, using empty string
        }
        else
        {
            // For complex types, use JSON representation
            result += element.dump();
        }
    }

    return result;
}

json evaluate_date_function(const std::vector<json>& args)
{
    // date() can be called with:
    // 1. One string argument: date("2017-01-01")
    // 2. Three number arguments: date(2017, 1, 1)
    
    if (args.size() == 1)
    {
        // Parse from string
        const auto& date_str = args[0];
        
        // DMN null propagation
        if (date_str.is_null())
        {
            return nullptr;
        }
        
        // Type validation
        if (!date_str.is_string())
        {
            return nullptr;
        }
        
        std::string date_string = date_str.get<std::string>();
        
        // Basic validation: ISO 8601 format YYYY-MM-DD
        // Simplified validation - full implementation would use regex or date library
        if (date_string.length() < 10)
        {
            return nullptr;
        }
        
        // Check basic format: YYYY-MM-DD
        if (date_string[4] != '-' || date_string[7] != '-')
        {
            return nullptr;
        }
        
        // For now, return the string as-is (representing a date)
        // In a full implementation, this would create a proper date object
        // The string can be compared lexicographically since it's ISO format
        return date_string;
    }

    if (args.size() == 3)
    {
        // Construct from year, month, day
        const auto& year = args[0];
        const auto& month = args[1];
        const auto& day = args[2];
        
        // DMN null propagation
        if (year.is_null() || month.is_null() || day.is_null())
        {
            return nullptr;
        }
        
        // Type validation
        if (!year.is_number() || !month.is_number() || !day.is_number())
        {
            return nullptr;
        }
        
        int year_num = year.get<int>();
        int month_num = month.get<int>();
        int day_num = day.get<int>();
        
        // Basic range validation
        if (year_num < 1 || year_num > 9999 || month_num < 1 || month_num > 12 || day_num < 1 || day_num > 31)
        {
            return nullptr;
        }
        
        // Format as ISO 8601 string: YYYY-MM-DD
        std::ostringstream oss;
        oss << std::setw(4) << std::setfill('0') << year_num << '-'
            << std::setw(2) << std::setfill('0') << month_num << '-'
            << std::setw(2) << std::setfill('0') << day_num;
        return oss.str();
    }

    // Invalid argument count
    return nullptr;
}

json evaluate_duration_function(const std::vector<json>& args)
{
    // duration() can be called with:
    // 1. One string argument: duration("P5DT10H")
    
    if (args.size() == 1)
    {
        // Parse from ISO 8601 duration string
        const auto& duration_str = args[0];
        
        // DMN null propagation
        if (duration_str.is_null())
        {
            return nullptr;
        }
        
        // Type validation
        if (!duration_str.is_string())
        {
            return nullptr;
        }
        
        std::string duration_string = duration_str.get<std::string>();
        
        // Validate using parse_duration() from types.cpp
        // This supports full ISO 8601: P[n]Y[n]M[n]DT[n]H[n]M[n]S
        auto parsed = parse_duration(duration_string);
        if (!parsed)
        {
            // Invalid duration format
            return nullptr;
        }
        
        // Return the validated duration string
        // Note: We return the string (not a Duration object) because:
        // 1. Duration comparisons in unary.cpp work with string_view
        // 2. Consistent with date() which returns ISO string
        // 3. Keeps JSON representation simple
        return duration_string;
    }

    // Invalid argument count
    return nullptr;
}

// ========== PHASE 1: TRIVIAL FUNCTIONS ==========

json evaluate_odd_function(const std::vector<json>& args)
{
    if (args.size() != 1)
    {
        return nullptr;
    }

    const auto& arg = args[0];

    if (arg.is_null())
    {
        return nullptr;
    }

    if (!arg.is_number())
    {
        return nullptr;
    }

    double value = arg.get<double>();

    // Must be an integer (no fractional part)
    if (value != std::floor(value))
    {
        return nullptr;
    }

    long long int_val = static_cast<long long>(value);
    return (int_val % 2 != 0);
}

json evaluate_even_function(const std::vector<json>& args)
{
    if (args.size() != 1)
    {
        return nullptr;
    }

    const auto& arg = args[0];

    if (arg.is_null())
    {
        return nullptr;
    }

    if (!arg.is_number())
    {
        return nullptr;
    }

    double value = arg.get<double>();

    // Must be an integer (no fractional part)
    if (value != std::floor(value))
    {
        return nullptr;
    }

    long long int_val = static_cast<long long>(value);
    return (int_val % 2 == 0);
}

json evaluate_number_function(const std::vector<json>& args)
{
    if (args.size() != 3)
    {
        return nullptr;
    }

    const auto& from = args[0];
    const auto& grouping_sep = args[1];
    const auto& decimal_sep = args[2];

    // Null propagation
    if (from.is_null())
    {
        return nullptr;
    }

    if (!from.is_string())
    {
        return nullptr;
    }

    std::string number_str = from.get<std::string>();

    // Get separator strings (null means no separator used)
    std::string group_sep_str;
    std::string dec_sep_str;

    if (!grouping_sep.is_null())
    {
        if (!grouping_sep.is_string())
        {
            return nullptr;
        }
        group_sep_str = grouping_sep.get<std::string>();
        // Grouping separator must be space, comma, or period
        if (group_sep_str != " " && group_sep_str != "," && group_sep_str != ".")
        {
            return nullptr;
        }
    }

    if (!decimal_sep.is_null())
    {
        if (!decimal_sep.is_string())
        {
            return nullptr;
        }
        dec_sep_str = decimal_sep.get<std::string>();
        // Decimal separator must be comma or period
        if (dec_sep_str != "," && dec_sep_str != ".")
        {
            return nullptr;
        }
    }

    // Grouping and decimal separators must be different
    if (!group_sep_str.empty() && !dec_sep_str.empty() && group_sep_str == dec_sep_str)
    {
        return nullptr;
    }

    // Remove grouping separators
    std::string cleaned;
    cleaned.reserve(number_str.size());
    for (size_t i = 0; i < number_str.size(); ++i)
    {
        std::string ch(1, number_str[i]);
        if (!group_sep_str.empty() && ch == group_sep_str)
        {
            continue; // Skip grouping separator
        }
        if (!dec_sep_str.empty() && ch == dec_sep_str)
        {
            cleaned += '.'; // Replace decimal separator with standard '.'
        }
        else
        {
            cleaned += number_str[i];
        }
    }

    // Parse the cleaned string as a number
    try
    {
        size_t pos = 0;
        double result = std::stod(cleaned, &pos);
        if (pos != cleaned.size())
        {
            return nullptr; // Not all characters consumed
        }
        return result;
    }
    catch (const std::exception&)
    {
        return nullptr;
    }
}

json evaluate_string_function(const std::vector<json>& args)
{
    if (args.size() != 1)
    {
        return nullptr;
    }

    const auto& arg = args[0];

    if (arg.is_null())
    {
        return nullptr;
    }

    if (arg.is_string())
    {
        return arg; // Already a string
    }

    if (arg.is_boolean())
    {
        return arg.get<bool>() ? "true" : "false";
    }

    if (arg.is_number())
    {
        double value = arg.get<double>();
        // Format integer values without decimal point
        if (value == std::floor(value) && std::abs(value) < 1e15)
        {
            long long int_val = static_cast<long long>(value);
            return std::to_string(int_val);
        }
        // Use ostringstream for proper formatting
        std::ostringstream oss;
        oss << value;
        return oss.str();
    }

    if (arg.is_array() || arg.is_object())
    {
        return arg.dump();
    }

    return nullptr;
}

json evaluate_is_function(const std::vector<json>& args)
{
    if (args.size() != 2)
    {
        return nullptr;
    }

    const auto& val1 = args[0];
    const auto& val2 = args[1];

    // Both null → true
    if (val1.is_null() && val2.is_null())
    {
        return true;
    }

    // One null, one not → false
    if (val1.is_null() || val2.is_null())
    {
        return false;
    }

    // Different types → false
    if (val1.type() != val2.type())
    {
        return false;
    }

    // Same type and value → compare
    return val1 == val2;
}

} // namespace orion::bre

