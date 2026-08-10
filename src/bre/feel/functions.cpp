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
#include <chrono>
#include <map>
#include <optional>
#include <string_view>
#include <unordered_map>
#include <limits>

namespace orion::bre::feel {

namespace {
    /// True if the byte is a UTF-8 continuation byte (10xxxxxx).
    inline bool is_utf8_continuation(unsigned char byte)
    {
        return (byte & 0xC0U) == 0x80U;
    }

    /**
     * @brief Count Unicode code points in a UTF-8 string.
     *
     * DMN defines the string functions in terms of characters, not bytes, so
     * string length("hello") and string length("h\u00e9llo") must both be 5.
     * Malformed input degrades gracefully: stray continuation bytes are simply
     * not counted as starts.
     */
    size_t utf8_length(std::string_view text)
    {
        size_t count = 0;
        for (const char ch : text)
        {
            if (!is_utf8_continuation(static_cast<unsigned char>(ch))) ++count;
        }
        return count;
    }

    /// Byte offset of the given code point index, or text.size() if past the end.
    size_t utf8_offset_of(std::string_view text, size_t char_index)
    {
        size_t chars_seen = 0;
        for (size_t offset = 0; offset < text.size(); ++offset)
        {
            if (is_utf8_continuation(static_cast<unsigned char>(text[offset]))) continue;
            if (chars_seen == char_index) return offset;
            ++chars_seen;
        }
        return text.size();
    }

    /**
     * @brief Substring by code point index and code point count.
     *
     * @param text Source string (UTF-8)
     * @param start_char 0-based code point index to start from
     * @param char_count Maximum number of code points to take
     */
    std::string utf8_substr(std::string_view text, size_t start_char, long long char_count)
    {
        if (char_count <= 0) return std::string();

        const size_t begin = utf8_offset_of(text, start_char);
        if (begin >= text.size()) return std::string();

        const size_t end = utf8_offset_of(text, start_char + static_cast<size_t>(char_count));
        return std::string(text.substr(begin, end - begin));
    }
} // namespace

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
    // DMN spec: all(list) or all(b1, b2, ..., bN)
    // Build flat list of items from args
    json items = json::array();
    if (args.size() == 1 && args[0].is_array())
    {
        items = args[0];
    }
    else if (args.size() == 1 && args[0].is_null())
    {
        return nullptr;
    }
    else if (args.size() == 1)
    {
        // Single non-list arg: treat as single boolean
        items.push_back(args[0]);
    }
    else
    {
        for (const auto& a : args) items.push_back(a);
    }

    if (items.empty()) return true; // vacuous truth

    bool has_null = false;
    for (const auto& elem : items)
    {
        if (elem.is_null()) { has_null = true; continue; }
        if (elem.is_boolean())
        {
            if (!elem.get<bool>()) return false;
        }
        else
        {
            return nullptr; // non-boolean element
        }
    }
    return has_null ? json(nullptr) : json(true);
}

json evaluate_any_function(const std::vector<json>& args)
{
    // DMN spec: any(list) or any(b1, b2, ..., bN)
    json items = json::array();
    if (args.size() == 1 && args[0].is_array())
    {
        items = args[0];
    }
    else if (args.size() == 1 && args[0].is_null())
    {
        return nullptr;
    }
    else if (args.size() == 1)
    {
        items.push_back(args[0]);
    }
    else
    {
        for (const auto& a : args) items.push_back(a);
    }

    if (items.empty()) return false;

    bool has_null = false;
    for (const auto& elem : items)
    {
        if (elem.is_null()) { has_null = true; continue; }
        if (elem.is_boolean())
        {
            if (elem.get<bool>()) return true;
        }
        else
        {
            return nullptr; // non-boolean element
        }
    }
    return has_null ? json(nullptr) : json(false);
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

    // Type validation - must be numeric or duration
    if (!arg.is_number())
    {
        // Check if it's a duration string
        if (arg.is_string()) {
            std::string s = arg.get<std::string>();
            auto parsed = parse_duration(s);
            if (parsed) {
                auto dur = parsed.value();
                // Determine if YM or DT
                bool has_time_part = s.find('T') != std::string::npos;
                bool has_day = s.find('D') != std::string::npos;
                bool is_ym = !has_time_part && !has_day;
                
                if (is_ym) {
                    int months = std::abs(dur.total_months);
                    int years = months / 12;
                    months = months % 12;
                    std::string result = "P";
                    if (years > 0) result += std::to_string(years) + "Y";
                    if (months > 0) result += std::to_string(months) + "M";
                    if (years == 0 && months == 0) result = "P0M";
                    return result;
                } else {
                    long long total = std::abs(dur.total_seconds);
                    long long days = total / 86400;
                    long long rem = total % 86400;
                    long long hours = rem / 3600;
                    rem %= 3600;
                    long long mins = rem / 60;
                    long long secs = rem % 60;
                    std::string result = "P";
                    if (days > 0) result += std::to_string(days) + "D";
                    if (hours > 0 || mins > 0 || secs > 0) {
                        result += "T";
                        if (hours > 0) result += std::to_string(hours) + "H";
                        if (mins > 0) result += std::to_string(mins) + "M";
                        if (secs > 0) result += std::to_string(secs) + "S";
                    }
                    if (result == "P") result = "P0D";
                    return result;
                }
            }
        }
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

    const std::string& string_val = str.get_ref<const std::string&>();

    // DMN counts characters (Unicode code points), not bytes
    const long long char_count = static_cast<long long>(utf8_length(string_val));

    // DMN uses 1-based indexing
    long long start_index = static_cast<long long>(start_pos.get<double>()) - 1;

    // Handle negative start position (from end)
    if (start_index < 0)
    {
        start_index = char_count + start_index + 1;
    }

    // Validate bounds
    if (start_index < 0 || start_index >= char_count)
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
            return utf8_substr(string_val, static_cast<size_t>(start_index), char_count);
        }

        if (!length.is_number())
        {
            return nullptr; // DMN spec: return null for invalid argument type
        }

        const long long len = static_cast<long long>(length.get<double>());
        
        // Negative length returns empty string
        if (len < 0)
        {
            return std::string("");
        }

        return utf8_substr(string_val, static_cast<size_t>(start_index), len);
    }

    // No length provided - return from start to end
    return utf8_substr(string_val, static_cast<size_t>(start_index), char_count);
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

    // DMN counts characters (Unicode code points), not bytes
    return static_cast<int>(utf8_length(str.get_ref<const std::string&>()));
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
    std::transform(string_val.begin(), string_val.end(), string_val.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::toupper(ch)); });
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
    std::transform(string_val.begin(), string_val.end(), string_val.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
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

json evaluate_replace_function(const std::vector<json>& args, const EvaluationContext& eval_ctx)
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

    const std::string& input_val = input.get_ref<const std::string&>();
    const std::string& pattern_val = pattern.get_ref<const std::string&>();
    const std::string& replacement_val = replacement.get_ref<const std::string&>();

    // Handle optional flags parameter
    std::string flags_val;
    if (args.size() == 4)
    {
        const auto& flags = args[3];
        if (!flags.is_null())
        {
            if (!flags.is_string()) return nullptr;
            flags_val = flags.get<std::string>();
        }
    }

    // Share the compiled-pattern cache, the JIT and the regex flavour with
    // matches(). std::regex would recompile on every call, use ECMAScript
    // semantics instead of the XPath flavour DMN mandates, and can exhaust the
    // stack on adversarial patterns.
    auto compiled = eval_ctx.regex_cache.get_or_compile(pattern_val, flags_val);
    if (!compiled)
    {
        return nullptr; // Invalid pattern or flags
    }

    auto result = compiled->replace_all(input_val, replacement_val);
    if (!result)
    {
        return nullptr;
    }
    return *result;
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

    // Handle empty delimiter - return array with each character (code point)
    if (delimiter_val.empty())
    {
        const size_t char_count = utf8_length(string_val);
        result_array.get_ref<json::array_t&>().reserve(char_count);
        for (size_t i = 0; i < char_count; ++i)
        {
            result_array.push_back(utf8_substr(string_val, i, 1));
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

    // Type validation - first argument must be array (coerce single value to list)
    json coerced_list;
    if (!list.is_array())
    {
        if (list.is_string())
        {
            coerced_list = json::array({list});
        }
        else
        {
            return nullptr; // DMN spec: return null for invalid argument type
        }
    }
    const auto& effective_list = list.is_array() ? list : coerced_list;

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

    // Join the list elements - DMN spec requires all elements to be strings or null
    // First validate all non-null elements are strings
    for (const auto& element : effective_list)
    {
        if (!element.is_null() && !element.is_string())
        {
            return nullptr; // Non-string, non-null element in list
        }
    }

    std::string result;
    bool first = true;

    for (const auto& element : effective_list)
    {
        if (element.is_null())
        {
            // Skip null elements per DMN spec
            continue;
        }

        if (!first)
        {
            result += delimiter;
        }
        first = false;

        result += element.get<std::string>();
    }

    return result;
}

// Date component parsing - needed by evaluate_date_function
struct DateComponents {
    int year = 0;
    int month = 0;
    int day = 0;
    bool valid = false;
};

static DateComponents parse_date_components(const std::string& s)
{
    DateComponents dc;
    size_t pos = 0;
    bool negative = false;

    if (!s.empty() && s[0] == '-') {
        negative = true;
        pos = 1;
    }

    // Find first dash after year
    auto dash1 = s.find('-', pos);
    if (dash1 == std::string::npos || dash1 == pos) return dc;

    try {
        dc.year = std::stoi(s.substr(pos, dash1 - pos));
        if (negative) dc.year = -dc.year;

        auto dash2 = s.find('-', dash1 + 1);
        if (dash2 == std::string::npos) return dc;

        dc.month = std::stoi(s.substr(dash1 + 1, dash2 - dash1 - 1));
        
        // Day ends at T, +, Z, @, or end of string
        size_t day_end = s.size();
        for (size_t i = dash2 + 1; i < s.size(); ++i) {
            char c = s[i];
            if (c == 'T' || c == '+' || c == 'Z' || c == '@') {
                day_end = i;
                break;
            }
        }
        dc.day = std::stoi(s.substr(dash2 + 1, day_end - dash2 - 1));
        dc.valid = (dc.month >= 1 && dc.month <= 12 && dc.day >= 1 && dc.day <= 31);
    } catch (...) {
        return dc;
    }
    return dc;
}

static std::string format_date_string(int year, int month, int day)
{
    std::ostringstream oss;
    if (year < 0) {
        oss << '-' << std::setw(4) << std::setfill('0') << (-year);
    } else if (year > 9999) {
        oss << year;
    } else {
        oss << std::setw(4) << std::setfill('0') << year;
    }
    oss << '-' << std::setw(2) << std::setfill('0') << month
        << '-' << std::setw(2) << std::setfill('0') << day;
    return oss.str();
}

json evaluate_date_function(const std::vector<json>& args)
{
    if (args.size() == 1)
    {
        if (args[0].is_null()) return nullptr;
        if (!args[0].is_string()) return nullptr;

        std::string s = args[0].get<std::string>();

        // If it's a datetime string, extract date part
        auto tpos = s.find('T');
        if (tpos != std::string::npos) {
            s = s.substr(0, tpos);
        }

        // Strict DMN date validation:
        // Must be [-]YYYY-MM-DD where YYYY is exactly 4 digits (or more without leading zeros)
        // Leading + is NOT allowed
        // 3-digit years NOT allowed
        // 5-digit years with leading zero NOT allowed
        size_t pos = 0;
        if (!s.empty() && s[0] == '-') pos = 1;
        if (!s.empty() && s[0] == '+') return nullptr; // Leading + not allowed

        auto dash1 = s.find('-', pos);
        if (dash1 == std::string::npos || dash1 == pos) return nullptr;
        
        std::string year_str = s.substr(pos, dash1 - pos);
        // Year must be at least 4 digits; if more than 4, no leading zeros
        if (year_str.size() < 4) return nullptr;
        if (year_str.size() > 4 && year_str[0] == '0') return nullptr;
        // All characters must be digits
        for (char c : year_str) {
            if (!std::isdigit(static_cast<unsigned char>(c))) return nullptr;
        }

        auto dc = parse_date_components(s);
        if (!dc.valid) return nullptr;

        return s;
    }

    if (args.size() == 3)
    {
        if (args[0].is_null() || args[1].is_null() || args[2].is_null()) return nullptr;
        if (!args[0].is_number() || !args[1].is_number() || !args[2].is_number()) return nullptr;

        int year_num = args[0].get<int>();
        int month_num = args[1].get<int>();
        int day_num = args[2].get<int>();

        if (year_num < -999999999 || year_num > 999999999) return nullptr;
        if (month_num < 1 || month_num > 12 || day_num < 1 || day_num > 31) return nullptr;

        return format_date_string(year_num, month_num, day_num);
    }

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

        // Enforce core lexical constraints before semantic parsing:
        // - bare P / -P is invalid
        // - H and S units require a T separator (e.g., P1H is invalid)
        if (duration_string == "P" || duration_string == "-P")
        {
            return nullptr;
        }

        const size_t value_start = (duration_string[0] == '-') ? 1 : 0;
        if (value_start >= duration_string.size() || duration_string[value_start] != 'P')
        {
            return nullptr;
        }

        const size_t t_pos = duration_string.find('T', value_start + 1);
        const size_t h_pos = duration_string.find('H', value_start + 1);
        const size_t s_pos = duration_string.find('S', value_start + 1);
        if ((h_pos != std::string::npos && (t_pos == std::string::npos || h_pos < t_pos)) ||
            (s_pos != std::string::npos && (t_pos == std::string::npos || s_pos < t_pos)))
        {
            return nullptr;
        }
        
        // Validate using parse_duration() from types.cpp
        // This supports full ISO 8601: P[n]Y[n]M[n]DT[n]H[n]M[n]S
        auto parsed = parse_duration(duration_string);
        if (!parsed)
        {
            // Invalid duration format
            return nullptr;
        }
        
        // Return the validated duration string in normalized form
        auto dur = parsed.value();
        // Determine if YM or DT based on string content
        // YM: only has Y/M components (no D, no T section)
        // DT: has D, H, or time components (T section)
        bool has_time_part = duration_string.find('T') != std::string::npos;
        bool has_day = duration_string.find('D') != std::string::npos;
        bool has_ym = (duration_string.find('Y') != std::string::npos || 
                       (duration_string.find('M') != std::string::npos && !has_time_part));
        bool is_ym = !has_time_part && !has_day;
        
        // Mixed durations (both YM and DT parts): return as-is
        if (has_ym && (has_time_part || has_day)) {
            return duration_string;
        }
        
        if (is_ym) {
            // YM duration: normalize to years and months
            int total_months = dur.total_months;
            bool negative = total_months < 0;
            if (negative) total_months = -total_months;
            int years = total_months / 12;
            int months = total_months % 12;
            std::string result = (negative && (years > 0 || months > 0)) ? "-P" : "P";
            if (years > 0) result += std::to_string(years) + "Y";
            if (months > 0) result += std::to_string(months) + "M";
            if (years == 0 && months == 0) result = "P0M";
            return result;
        } else {
            // DT duration: normalize to days/hours/minutes/seconds
            long long total = dur.total_seconds;
            bool negative = total < 0;
            if (negative) total = -total;
            long long days = total / 86400;
            long long rem = total % 86400;
            long long hours = rem / 3600;
            rem %= 3600;
            long long mins = rem / 60;
            long long secs = rem % 60;
            std::string result = (negative && (days > 0 || hours > 0 || mins > 0 || secs > 0)) ? "-P" : "P";
            if (days > 0) result += std::to_string(days) + "D";
            if (hours > 0 || mins > 0 || secs > 0) {
                result += "T";
                if (hours > 0) result += std::to_string(hours) + "H";
                if (mins > 0) result += std::to_string(mins) + "M";
                if (secs > 0) result += std::to_string(secs) + "S";
            }
            if (result == "P" || result == "-P") result = has_time_part ? "PT0S" : "P0D";
            return result;
        }
    }

    // Invalid argument count
    return nullptr;
}

// ========== TEMPORAL HELPER FUNCTIONS ==========

// Detect if a string looks like a date: YYYY-MM-DD or -YYYY-MM-DD
static bool is_date_string(const std::string& s)
{
    size_t start = 0;
    if (!s.empty() && s[0] == '-') start = 1;
    if (s.size() < start + 10) return false;
    // Check YYYY-MM-DD pattern
    for (size_t i = start; i < start + 4; ++i) if (!std::isdigit(static_cast<unsigned char>(s[i]))) return false;
    if (s[start + 4] != '-') return false;
    for (size_t i = start + 5; i < start + 7; ++i) if (!std::isdigit(static_cast<unsigned char>(s[i]))) return false;
    if (s[start + 7] != '-') return false;
    for (size_t i = start + 8; i < start + 10; ++i) if (!std::isdigit(static_cast<unsigned char>(s[i]))) return false;
    // Must be exactly 10 chars (no T following = pure date)
    return s.size() == start + 10;
}

// Detect if a string is a datetime: contains T or is date + T + time
static bool is_datetime_string(const std::string& s)
{
    // Look for T separator between date and time parts
    auto tpos = s.find('T');
    if (tpos == std::string::npos) return false;
    if (tpos < 8) return false; // Need at least YYYY-MM-DD before T
    return true;
}

// Detect if a string looks like a time: HH:MM:SS or HH:MM:SS.fff or with timezone
static bool is_time_string(const std::string& s)
{
    if (s.size() < 5) return false;
    if (!std::isdigit(static_cast<unsigned char>(s[0])) || !std::isdigit(static_cast<unsigned char>(s[1]))) return false;
    if (s[2] != ':') return false;
    if (!std::isdigit(static_cast<unsigned char>(s[3])) || !std::isdigit(static_cast<unsigned char>(s[4]))) return false;
    return true;
}

// Detect if a string is a duration: starts with P or -P
static bool is_duration_string(const std::string& s)
{
    if (s.empty()) return false;
    if (s[0] == 'P') return true;
    if (s.size() > 1 && s[0] == '-' && s[1] == 'P') return true;
    return false;
}

// Parse time components from a string
struct TimeComponents {
    int hour = 0;
    int minute = 0;
    int second = 0;
    int nanosecond = 0;
    std::string offset; // "+05:00", "Z", "@Europe/Paris", or ""
    bool valid = false;
};

static TimeComponents parse_time_components(const std::string& s)
{
    TimeComponents tc;
    if (s.size() < 5) return tc;

    // Helper to check 2 digits at position
    auto is_two_digits = [&](size_t pos) -> bool {
        return pos + 1 < s.size() &&
               std::isdigit(static_cast<unsigned char>(s[pos])) &&
               std::isdigit(static_cast<unsigned char>(s[pos + 1]));
    };

    try {
        if (!is_two_digits(0)) return tc;
        tc.hour = std::stoi(s.substr(0, 2));
        if (s[2] != ':') return tc;
        if (!is_two_digits(3)) return tc;
        tc.minute = std::stoi(s.substr(3, 2));

        size_t pos = 5;
        if (pos < s.size() && s[pos] == ':') {
            if (!is_two_digits(pos + 1)) return tc;
            tc.second = std::stoi(s.substr(pos + 1, 2));
            pos += 3;
            // Fractional seconds
            if (pos < s.size() && s[pos] == '.') {
                size_t frac_start = pos + 1;
                size_t frac_end = frac_start;
                while (frac_end < s.size() && std::isdigit(static_cast<unsigned char>(s[frac_end]))) ++frac_end;
                if (frac_end == frac_start) return tc; // No digits after dot
                std::string frac = s.substr(frac_start, frac_end - frac_start);
                while (frac.size() < 9) frac += '0';
                tc.nanosecond = std::stoi(frac.substr(0, 9));
                pos = frac_end;
            }
        }

        // Timezone/offset
        if (pos < s.size()) {
            std::string tz_part = s.substr(pos);
            if (tz_part == "Z" || tz_part == "z") {
                tc.offset = "Z";
            } else if (tz_part[0] == '+' || tz_part[0] == '-') {
                // Must not also have @timezone
                if (tz_part.find('@') != std::string::npos) return tc;
                // Validate offset format: ±HH:MM (exactly)
                if (tz_part.size() != 6) return tc;
                if (!std::isdigit(static_cast<unsigned char>(tz_part[1])) ||
                    !std::isdigit(static_cast<unsigned char>(tz_part[2]))) return tc;
                if (tz_part[3] != ':') return tc;
                if (!std::isdigit(static_cast<unsigned char>(tz_part[4])) ||
                    !std::isdigit(static_cast<unsigned char>(tz_part[5]))) return tc;
                int off_h = std::stoi(tz_part.substr(1, 2));
                int off_m = std::stoi(tz_part.substr(4, 2));
                if (off_h > 18 || (off_h == 18 && off_m > 0)) return tc;
                if (off_m > 59) return tc;
                tc.offset = tz_part;
            } else if (tz_part[0] == '@') {
                // IANA timezone name - validate region prefix
                std::string tz_name = tz_part.substr(1);
                if (tz_name.find('/') == std::string::npos) return tc;
                std::string region = tz_name.substr(0, tz_name.find('/'));
                // Known IANA top-level regions
                static const std::vector<std::string> valid_regions = {
                    "Africa", "America", "Antarctica", "Arctic", "Asia", "Atlantic",
                    "Australia", "Europe", "Indian", "Pacific", "Etc", "US"
                };
                bool valid_region = false;
                for (const auto& r : valid_regions) {
                    if (region == r) { valid_region = true; break; }
                }
                if (!valid_region) return tc;
                tc.offset = tz_part;
            } else {
                return tc; // Unknown suffix
            }
        }

        tc.valid = (tc.hour >= 0 && tc.hour <= 23 && tc.minute >= 0 && tc.minute <= 59 && tc.second >= 0 && tc.second <= 59);
    } catch (...) {
        return tc;
    }
    return tc;
}

// Normalize a time string to canonical form: strip fractional zeros, normalize Z to +00:00
static std::string normalize_time_string(const std::string& s)
{
    auto tc = parse_time_components(s);
    if (!tc.valid) return s;

    std::ostringstream oss;
    oss << std::setw(2) << std::setfill('0') << tc.hour << ':'
        << std::setw(2) << std::setfill('0') << tc.minute << ':'
        << std::setw(2) << std::setfill('0') << tc.second;

    // Only add fractional part if non-zero
    if (tc.nanosecond > 0) {
        // Format nanoseconds, trimming trailing zeros
        std::string frac = std::to_string(tc.nanosecond);
        while (frac.size() < 9) frac = "0" + frac;
        // Remove trailing zeros
        size_t last_nonzero = frac.find_last_not_of('0');
        if (last_nonzero != std::string::npos) {
            frac = frac.substr(0, last_nonzero + 1);
        }
        oss << '.' << frac;
    }

    // Normalize timezone: +00:00 and -00:00 → Z
    if (!tc.offset.empty()) {
        if (tc.offset == "+00:00" || tc.offset == "-00:00") {
            oss << "Z";
        } else {
            oss << tc.offset;
        }
    }

    return oss.str();
}

// Parse duration components
struct DurationComponents {
    bool negative = false;
    int years = 0;
    int months = 0;
    int days = 0;
    int hours = 0;
    int minutes = 0;
    int seconds = 0;
    bool valid = false;
};

static DurationComponents parse_duration_components(const std::string& s)
{
    DurationComponents dc;
    auto parsed = parse_duration(s);
    if (!parsed) return dc;

    dc.negative = (!s.empty() && s[0] == '-');
    int total_months = parsed->total_months;
    long long total_seconds = parsed->total_seconds;

    if (dc.negative) {
        total_months = -total_months;
        total_seconds = -total_seconds;
    }

    dc.years = total_months / 12;
    dc.months = total_months % 12;
    dc.days = static_cast<int>(total_seconds / 86400);
    total_seconds %= 86400;
    dc.hours = static_cast<int>(total_seconds / 3600);
    total_seconds %= 3600;
    dc.minutes = static_cast<int>(total_seconds / 60);
    dc.seconds = static_cast<int>(total_seconds % 60);
    dc.valid = true;
    return dc;
}

// Get temporal property from a string value
json get_temporal_property(const std::string& val, const std::string& prop)
{
    // Check if duration first (starts with P or -P)
    if (is_duration_string(val)) {
        auto dc = parse_duration_components(val);
        if (!dc.valid) return nullptr;
        // Determine type from string: contains Y or M before T → YM, contains D/H/S or T → DT
        // More reliable: check if string has Y or non-time M
        bool has_ym_component = false;
        bool has_dt_component = false;
        {
            bool in_time = false;
            for (size_t i = (val[0] == '-' ? 2 : 1); i < val.size(); i++) {
                if (val[i] == 'T') { in_time = true; continue; }
                if (val[i] == 'Y') has_ym_component = true;
                if (val[i] == 'M' && !in_time) has_ym_component = true;
                if (val[i] == 'D' || val[i] == 'H' || val[i] == 'S') has_dt_component = true;
                if (val[i] == 'M' && in_time) has_dt_component = true;
            }
        }
        if (has_ym_component && !has_dt_component) {
            // YM duration: only years and months
            if (prop == "years") return dc.years;
            if (prop == "months") return dc.months;
            return nullptr;
        }
        if (has_dt_component && !has_ym_component) {
            // DT duration: only days, hours, minutes, seconds
            if (prop == "days") return dc.days;
            if (prop == "hours") return dc.hours;
            if (prop == "minutes") return dc.minutes;
            if (prop == "seconds") return dc.seconds;
            return nullptr;
        }
        // Mixed or zero — return whatever is asked
        if (prop == "years") return dc.years;
        if (prop == "months") return dc.months;
        if (prop == "days") return dc.days;
        if (prop == "hours") return dc.hours;
        if (prop == "minutes") return dc.minutes;
        if (prop == "seconds") return dc.seconds;
        return nullptr;
    }

    // Check datetime (has T separator)
    if (is_datetime_string(val)) {
        auto tpos = val.find('T');
        auto date_part = val.substr(0, tpos);
        auto time_part = val.substr(tpos + 1);

        auto dcomp = parse_date_components(date_part);
        auto tcomp = parse_time_components(time_part);

        if (prop == "year" && dcomp.valid) return dcomp.year;
        if (prop == "month" && dcomp.valid) return dcomp.month;
        if (prop == "day" && dcomp.valid) return dcomp.day;
        if (prop == "hour" && tcomp.valid) return tcomp.hour;
        if (prop == "minute" && tcomp.valid) return tcomp.minute;
        if (prop == "second" && tcomp.valid) return tcomp.second;
        if (prop == "time offset" || prop == "timezone") {
            if (!tcomp.offset.empty()) return tcomp.offset;
            return nullptr;
        }
        if (prop == "date") {
            if (dcomp.valid) return date_part;
            return nullptr;
        }
        if (prop == "time") {
            if (tcomp.valid) return time_part;
            return nullptr;
        }
        if (prop == "weekday" && dcomp.valid) {
            // Zeller-like day of week calculation
            int y = dcomp.year, m = dcomp.month, d = dcomp.day;
            if (m <= 2) { y--; m += 12; }
            int dow = (d + 13*(m+1)/5 + y + y/4 - y/100 + y/400) % 7;
            // Convert Zeller (0=Sat) to ISO 8601 (1=Mon..7=Sun)
            int iso = ((dow + 5) % 7) + 1;
            return iso;
        }
        return nullptr;
    }

    // Check date (YYYY-MM-DD, no T)
    if (is_date_string(val)) {
        auto dcomp = parse_date_components(val);
        if (!dcomp.valid) return nullptr;
        if (prop == "year") return dcomp.year;
        if (prop == "month") return dcomp.month;
        if (prop == "day") return dcomp.day;
        if (prop == "weekday") {
            int y = dcomp.year, m = dcomp.month, d = dcomp.day;
            if (m <= 2) { y--; m += 12; }
            int dow = (d + 13*(m+1)/5 + y + y/4 - y/100 + y/400) % 7;
            int iso = ((dow + 5) % 7) + 1;
            return iso;
        }
        return nullptr;
    }

    // Check time
    if (is_time_string(val)) {
        auto tcomp = parse_time_components(val);
        if (!tcomp.valid) return nullptr;
        if (prop == "hour") return tcomp.hour;
        if (prop == "minute") return tcomp.minute;
        if (prop == "second") return tcomp.second;
        if (prop == "time offset" || prop == "timezone") {
            if (!tcomp.offset.empty()) return tcomp.offset;
            return nullptr;
        }
        return nullptr;
    }

    return nullptr;
}

// ========== TEMPORAL FUNCTIONS ==========

json evaluate_time_function(const std::vector<json>& args)
{
    if (args.empty() || args.size() > 4) return nullptr;

    if (args.size() == 1) {
        if (args[0].is_null()) return nullptr;
        if (args[0].is_string()) {
            std::string s = args[0].get<std::string>();
            // Could be a time string or a datetime string
            // If datetime, extract time part
            auto tpos = s.find('T');
            if (tpos != std::string::npos) {
                s = s.substr(tpos + 1);
            }
            // Validate it parses as a time
            auto tc = parse_time_components(s);
            if (!tc.valid) return nullptr;
            return normalize_time_string(s);
        }
        return nullptr;
    }

    // time(hour, minute, second) or time(hour, minute, second, offset)
    if (args.size() >= 3) {
        if (args[0].is_null() || args[1].is_null() || args[2].is_null()) return nullptr;
        if (!args[0].is_number() || !args[1].is_number() || !args[2].is_number()) return nullptr;

        int h = args[0].get<int>();
        int m = args[1].get<int>();
        int sec = args[2].get<int>();

        if (h < 0 || h > 23 || m < 0 || m > 59 || sec < 0 || sec > 59) return nullptr;

        std::ostringstream oss;
        oss << std::setw(2) << std::setfill('0') << h << ':'
            << std::setw(2) << std::setfill('0') << m << ':'
            << std::setw(2) << std::setfill('0') << sec;

        if (args.size() >= 4 && !args[3].is_null()) {
            if (args[3].is_string()) {
                std::string offset_str = args[3].get<std::string>();
                // Check if it's a duration string (timezone offset as duration)
                if (!offset_str.empty() && (offset_str[0] == 'P' || (offset_str[0] == '-' && offset_str.size() > 1 && offset_str[1] == 'P'))) {
                    // Parse duration to get hours and minutes for timezone offset
                    bool negative = (offset_str[0] == '-');
                    std::string dur = negative ? offset_str.substr(1) : offset_str;
                    // Must be a day-time duration (PT...)
                    if (dur.size() < 2 || dur[0] != 'P') return nullptr;
                    dur = dur.substr(1); // remove P
                    if (dur.empty() || dur[0] != 'T') return nullptr;
                    dur = dur.substr(1); // remove T
                    int off_h = 0, off_m = 0, off_s = 0;
                    std::string num_buf;
                    for (char c : dur) {
                        if (std::isdigit(static_cast<unsigned char>(c))) {
                            num_buf += c;
                        } else if (c == 'H') {
                            off_h = num_buf.empty() ? 0 : std::stoi(num_buf);
                            num_buf.clear();
                        } else if (c == 'M') {
                            off_m = num_buf.empty() ? 0 : std::stoi(num_buf);
                            num_buf.clear();
                        } else if (c == 'S') {
                            off_s = num_buf.empty() ? 0 : std::stoi(num_buf);
                            num_buf.clear();
                        }
                    }
                    // Format timezone offset
                    if (off_h == 0 && off_m == 0 && off_s == 0) {
                        oss << "Z";
                    } else if (off_s == 0) {
                        oss << (negative ? "-" : "+")
                            << std::setw(2) << std::setfill('0') << off_h << ':'
                            << std::setw(2) << std::setfill('0') << off_m;
                    } else {
                        // Offset with seconds (rare but valid per spec)
                        oss << (negative ? "-" : "+")
                            << std::setw(2) << std::setfill('0') << off_h << ':'
                            << std::setw(2) << std::setfill('0') << off_m << ':'
                            << std::setw(2) << std::setfill('0') << off_s;
                    }
                } else {
                    // Assume it's already a timezone string (e.g., "@Europe/Paris" or "+02:00")
                    oss << offset_str;
                }
            } else {
                return nullptr; // 4th arg must be a string (duration) or null
            }
        }
        return oss.str();
    }
    return nullptr;
}

json evaluate_date_and_time_function(const std::vector<json>& args)
{
    if (args.empty() || args.size() > 2) return nullptr;

    // Helper to validate strict date format: [-]YYYY-MM-DD, no leading +, min 4-digit year
    auto validate_date_strict = [](const std::string& ds) -> bool {
        size_t pos = 0;
        if (!ds.empty() && ds[0] == '-') pos = 1;
        if (!ds.empty() && ds[0] == '+') return false;
        auto dash1 = ds.find('-', pos);
        if (dash1 == std::string::npos || dash1 == pos) return false;
        std::string year_str = ds.substr(pos, dash1 - pos);
        if (year_str.size() < 4) return false;
        if (year_str.size() > 4 && year_str[0] == '0') return false;
        for (char c : year_str) if (!std::isdigit(static_cast<unsigned char>(c))) return false;
        return true;
    };

    if (args.size() == 1) {
        // date and time(string)
        if (args[0].is_null()) return nullptr;
        if (!args[0].is_string()) return nullptr;
        std::string s = args[0].get<std::string>();
        // Validate it has both date and time parts
        auto tpos = s.find('T');
        if (tpos == std::string::npos) {
            // Date-only string: treat as midnight
            if (!validate_date_strict(s)) return nullptr;
            auto dcomp = parse_date_components(s);
            if (!dcomp.valid) return nullptr;
            return s + "T00:00:00";
        }
        std::string date_part = s.substr(0, tpos);
        if (!validate_date_strict(date_part)) return nullptr;
        auto dcomp = parse_date_components(date_part);
        auto tcomp = parse_time_components(s.substr(tpos + 1));
        if (!dcomp.valid || !tcomp.valid) return nullptr;
        return date_part + "T" + normalize_time_string(s.substr(tpos + 1));
    }

    // date and time(date, time)
    if (args[0].is_null() || args[1].is_null()) return nullptr;
    if (!args[0].is_string() || !args[1].is_string()) return nullptr;

    std::string date_str = args[0].get<std::string>();
    std::string time_str = args[1].get<std::string>();

    // If date_str is a datetime, extract just the date part
    auto tpos = date_str.find('T');
    if (tpos != std::string::npos) {
        date_str = date_str.substr(0, tpos);
    }

    return date_str + "T" + normalize_time_string(time_str);
}

json evaluate_years_and_months_duration_function(const std::vector<json>& args)
{
    if (args.size() != 2) return nullptr;
    if (args[0].is_null() || args[1].is_null()) return nullptr;
    if (!args[0].is_string() || !args[1].is_string()) return nullptr;

    std::string from_str = args[0].get<std::string>();
    std::string to_str = args[1].get<std::string>();

    // Extract date parts (handle both date and datetime)
    auto t1 = from_str.find('T');
    if (t1 != std::string::npos) from_str = from_str.substr(0, t1);
    auto t2 = to_str.find('T');
    if (t2 != std::string::npos) to_str = to_str.substr(0, t2);

    auto from = parse_date_components(from_str);
    auto to = parse_date_components(to_str);
    if (!from.valid || !to.valid) return nullptr;

    int total_months = (to.year * 12 + to.month) - (from.year * 12 + from.month);
    // Adjust for incomplete months based on day difference
    if (total_months > 0 && to.day < from.day) {
        total_months--;
    } else if (total_months < 0 && to.day > from.day) {
        total_months++;
    }

    bool negative = total_months < 0;
    if (negative) total_months = -total_months;

    int years = total_months / 12;
    int months = total_months % 12;

    std::ostringstream oss;
    if (negative) oss << '-';
    oss << 'P';
    if (years > 0) oss << years << 'Y';
    if (months > 0) oss << months << 'M';
    if (years == 0 && months == 0) oss << "0M";
    return oss.str();
}

// Day of year (1-366)
json evaluate_day_of_year_function(const std::vector<json>& args)
{
    if (args.size() != 1) return nullptr;
    if (args[0].is_null()) return nullptr;
    if (!args[0].is_string()) return nullptr;

    std::string s = args[0].get<std::string>();
    // Extract date part if datetime
    auto tpos = s.find('T');
    if (tpos != std::string::npos) s = s.substr(0, tpos);

    auto dc = parse_date_components(s);
    if (!dc.valid) return nullptr;

    static const int days_before_month[] = {0, 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
    int doy = days_before_month[dc.month] + dc.day;
    // Leap year adjustment
    bool leap = (dc.year % 4 == 0 && dc.year % 100 != 0) || (dc.year % 400 == 0);
    if (leap && dc.month > 2) doy++;
    return doy;
}

// Day of week: "Monday"..."Sunday"
json evaluate_day_of_week_function(const std::vector<json>& args)
{
    if (args.size() != 1) return nullptr;
    if (args[0].is_null()) return nullptr;
    if (!args[0].is_string()) return nullptr;

    std::string s = args[0].get<std::string>();
    auto tpos = s.find('T');
    if (tpos != std::string::npos) s = s.substr(0, tpos);

    auto dc = parse_date_components(s);
    if (!dc.valid) return nullptr;

    // Zeller's formula
    int y = dc.year, m = dc.month, d = dc.day;
    if (m <= 2) { y--; m += 12; }
    int dow = (d + 13*(m+1)/5 + y + y/4 - y/100 + y/400) % 7;
    // Zeller: 0=Sat, 1=Sun, ..., 6=Fri → ISO: Mon=1..Sun=7
    int iso = ((dow + 5) % 7) + 1;
    static const char* names[] = {"", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"};
    return std::string(names[iso]);
}

// Month of year: "January"..."December"
json evaluate_month_of_year_function(const std::vector<json>& args)
{
    if (args.size() != 1) return nullptr;
    if (args[0].is_null()) return nullptr;
    if (!args[0].is_string()) return nullptr;

    std::string s = args[0].get<std::string>();
    auto tpos = s.find('T');
    if (tpos != std::string::npos) s = s.substr(0, tpos);

    auto dc = parse_date_components(s);
    if (!dc.valid) return nullptr;

    static const char* names[] = {"", "January", "February", "March", "April", "May", "June",
        "July", "August", "September", "October", "November", "December"};
    if (dc.month < 1 || dc.month > 12) return nullptr;
    return std::string(names[dc.month]);
}

// Week of year (ISO 8601)
json evaluate_week_of_year_function(const std::vector<json>& args)
{
    if (args.size() != 1) return nullptr;
    if (args[0].is_null()) return nullptr;
    if (!args[0].is_string()) return nullptr;

    std::string s = args[0].get<std::string>();
    auto tpos = s.find('T');
    if (tpos != std::string::npos) s = s.substr(0, tpos);

    auto dc = parse_date_components(s);
    if (!dc.valid) return nullptr;

    // ISO 8601 week number calculation
    // First compute day-of-year
    static const int days_before_month[] = {0, 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
    int doy = days_before_month[dc.month] + dc.day;
    bool leap = (dc.year % 4 == 0 && dc.year % 100 != 0) || (dc.year % 400 == 0);
    if (leap && dc.month > 2) doy++;

    // Day of week for this date (ISO: Mon=1..Sun=7)
    int y = dc.year, m = dc.month, d = dc.day;
    if (m <= 2) { y--; m += 12; }
    int dow_z = (d + 13*(m+1)/5 + y + y/4 - y/100 + y/400) % 7;
    int dow = ((dow_z + 5) % 7) + 1; // ISO dow

    // ISO week number
    int woy = (doy - dow + 10) / 7;

    // Handle edge cases: week 0 → last week of previous year, week 53 validity
    if (woy < 1) {
        // Last week of previous year
        int prev_year = dc.year - 1;
        bool prev_leap = (prev_year % 4 == 0 && prev_year % 100 != 0) || (prev_year % 400 == 0);
        int prev_days = prev_leap ? 366 : 365;
        // Calculate dow of Dec 31 of previous year
        int py = prev_year, pm = 12, pd = 31;
        if (pm <= 2) { py--; pm += 12; }
        int pdow_z = (pd + 13*(pm+1)/5 + py + py/4 - py/100 + py/400) % 7;
        int pdow = ((pdow_z + 5) % 7) + 1;
        woy = (prev_days - pdow + 10) / 7;
    } else if (woy == 53) {
        // Check if this year actually has 53 weeks
        int jan1_y = dc.year, jan1_m = 1, jan1_d = 1;
        if (jan1_m <= 2) { jan1_y--; jan1_m += 12; }
        int jan1_z = (jan1_d + 13*(jan1_m+1)/5 + jan1_y + jan1_y/4 - jan1_y/100 + jan1_y/400) % 7;
        int jan1_dow = ((jan1_z + 5) % 7) + 1;
        // Year has 53 weeks if Jan 1 is Thursday, or Dec 31 is Thursday
        if (jan1_dow != 4) {
            int dec31_y = dc.year, dec31_m = 12, dec31_d = 31;
            if (dec31_m <= 2) { dec31_y--; dec31_m += 12; }
            int dec31_z = (dec31_d + 13*(dec31_m+1)/5 + dec31_y + dec31_y/4 - dec31_y/100 + dec31_y/400) % 7;
            int dec31_dow = ((dec31_z + 5) % 7) + 1;
            if (dec31_dow != 4) {
                woy = 1; // First week of next year
            }
        }
    }

    return woy;
}

// now() - returns current date and time
json evaluate_now_function(const std::vector<json>& args)
{
    if (!args.empty()) return nullptr;

    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    struct tm tm_now;
#ifdef _WIN32
    localtime_s(&tm_now, &time_t_now);
#else
    localtime_r(&time_t_now, &tm_now);
#endif

    std::ostringstream oss;
    oss << std::setw(4) << std::setfill('0') << (tm_now.tm_year + 1900) << '-'
        << std::setw(2) << std::setfill('0') << (tm_now.tm_mon + 1) << '-'
        << std::setw(2) << std::setfill('0') << tm_now.tm_mday << 'T'
        << std::setw(2) << std::setfill('0') << tm_now.tm_hour << ':'
        << std::setw(2) << std::setfill('0') << tm_now.tm_min << ':'
        << std::setw(2) << std::setfill('0') << tm_now.tm_sec;
    return oss.str();
}

// today() - returns current date
json evaluate_today_function(const std::vector<json>& args)
{
    if (!args.empty()) return nullptr;

    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    struct tm tm_now;
#ifdef _WIN32
    localtime_s(&tm_now, &time_t_now);
#else
    localtime_r(&time_t_now, &tm_now);
#endif

    std::ostringstream oss;
    oss << std::setw(4) << std::setfill('0') << (tm_now.tm_year + 1900) << '-'
        << std::setw(2) << std::setfill('0') << (tm_now.tm_mon + 1) << '-'
        << std::setw(2) << std::setfill('0') << tm_now.tm_mday;
    return oss.str();
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
        // FEEL-style formatting: spaces after separators, unquoted keys
        if (arg.is_array())
        {
            std::string result = "[";
            bool first = true;
            for (const auto& elem : arg)
            {
                if (!first) result += ", ";
                first = false;
                if (elem.is_string())
                {
                    // Re-escape embedded quotes/backslashes so the rendered
                    // literal round-trips (e.g. `"foo"` -> "\"foo\"").
                    result += elem.dump(-1, ' ', false, json::error_handler_t::replace);
                }
                else if (elem.is_null())
                {
                    result += "null";
                }
                else
                {
                    // Recurse for nested structures
                    auto nested = evaluate_string_function({elem});
                    if (nested.is_string())
                        result += nested.get<std::string>();
                    else
                        result += elem.dump();
                }
            }
            result += "]";
            return result;
        }
        else
        {
            // Context: {key: value, ...} with unquoted keys when valid identifiers
            std::string result = "{";
            bool first = true;
            for (auto it = arg.begin(); it != arg.end(); ++it)
            {
                if (!first) result += ", ";
                first = false;
                // Check if key needs quoting (contains special chars)
                const std::string& key = it.key();
                bool needs_quotes = false;
                for (char c : key)
                {
                    if (c == '{' || c == '}' || c == ':' || c == ',' || c == '"' || c == ' ')
                    {
                        needs_quotes = true;
                        break;
                    }
                }
                if (needs_quotes)
                    result += json(key).dump(-1, ' ', false, json::error_handler_t::replace) + ": ";
                else
                    result += key + ": ";
                
                const auto& val = it.value();
                if (val.is_string())
                {
                    result += "\"" + val.get<std::string>() + "\"";
                }
                else if (val.is_null())
                {
                    result += "null";
                }
                else
                {
                    auto nested = evaluate_string_function({val});
                    if (nested.is_string())
                        result += nested.get<std::string>();
                    else
                        result += val.dump();
                }
            }
            result += "}";
            return result;
        }
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

// ========== PHASE 2A: AGGREGATION FUNCTIONS ==========

// Helper: FEEL value equality over a JSON array.
// Uses json::operator==, which compares numbers numerically (1 == 1.0) as
// required by DMN. Do not substitute a serialization-based key: dump() would
// distinguish 1 from 1.0, map NaN/Infinity onto "null", and throw on invalid
// UTF-8. Linear scan is also faster than hashing for the small lists typical
// of decision tables.
static bool array_contains_value(const json& array_value, const json& element)
{
    for (const auto& item : array_value)
    {
        if (item == element) return true;
    }
    return false;
}

// Helper: Convert a FEEL number to a list index safely.
// Fractional positions are truncated toward zero, as required by DMN
// (e.g. list replace([1,2,3], 2.5, 4) addresses position 2). Non-numbers,
// non-finite values and magnitudes that would make the float->int conversion
// undefined behaviour are rejected.
static std::optional<long long> to_list_index(const json& value)
{
    if (!value.is_number()) return std::nullopt;
    const double raw = value.get<double>();
    if (!std::isfinite(raw)) return std::nullopt;
    if (raw < -9.0e15 || raw > 9.0e15) return std::nullopt;
    return static_cast<long long>(raw);
}

// Helper: Normalize variadic args to a single flat list of numbers
// min(1,2,3) or min([1,2,3]) → [1,2,3]
//
// Postcondition: on success the result is always an array, so callers must not
// re-test for that. Returns nullopt only for a single null argument.
static std::optional<json> normalize_to_list(const std::vector<json>& args)
{
    if (args.size() == 1)
    {
        const auto& arg = args[0];
        if (arg.is_null()) return std::nullopt;
        if (arg.is_array()) return std::optional<json>(arg);
        // Single scalar → wrap in array
        return std::optional<json>(json::array({arg}));
    }
    // Multiple args → treat as list
    json result = json::array();
    result.get_ref<json::array_t&>().reserve(args.size());
    for (const auto& a : args)
    {
        result.push_back(a);
    }
    return std::optional<json>(std::move(result));
}

json evaluate_count_function(const std::vector<json>& args)
{
    if (args.empty()) return nullptr;
    const auto list_opt = normalize_to_list(args);
    if (!list_opt) return nullptr;
    return json(static_cast<double>(list_opt->size()));
}

json evaluate_sum_function(const std::vector<json>& args)
{
    if (args.empty()) return nullptr;
    const auto list_opt = normalize_to_list(args);
    if (!list_opt) return nullptr;
    const json& list = *list_opt;
    if (list.empty()) return json(0);

    double total = 0.0;
    for (const auto& item : list)
    {
        if (item.is_null()) return nullptr;
        if (!item.is_number()) return nullptr;
        total += item.get<double>();
    }
    return total;
}

json evaluate_min_function(const std::vector<json>& args)
{
    if (args.empty()) return nullptr;
    const auto list_opt = normalize_to_list(args);
    if (!list_opt) return nullptr;
    const json& list = *list_opt;
    if (list.empty()) return nullptr;

    double result = std::numeric_limits<double>::infinity();
    for (const auto& item : list)
    {
        if (item.is_null()) return nullptr;
        if (!item.is_number()) return nullptr;
        double val = item.get<double>();
        if (val < result) result = val;
    }
    return result;
}

json evaluate_max_function(const std::vector<json>& args)
{
    if (args.empty()) return nullptr;
    const auto list_opt = normalize_to_list(args);
    if (!list_opt) return nullptr;
    const json& list = *list_opt;
    if (list.empty()) return nullptr;

    double result = -std::numeric_limits<double>::infinity();
    for (const auto& item : list)
    {
        if (item.is_null()) return nullptr;
        if (!item.is_number()) return nullptr;
        double val = item.get<double>();
        if (val > result) result = val;
    }
    return result;
}

json evaluate_mean_function(const std::vector<json>& args)
{
    if (args.empty()) return nullptr;
    const auto list_opt = normalize_to_list(args);
    if (!list_opt) return nullptr;
    const json& list = *list_opt;
    if (list.empty()) return nullptr;

    double total = 0.0;
    for (const auto& item : list)
    {
        if (item.is_null()) return nullptr;
        if (!item.is_number()) return nullptr;
        total += item.get<double>();
    }
    return total / static_cast<double>(list.size());
}

json evaluate_product_function(const std::vector<json>& args)
{
    if (args.empty()) return nullptr;
    const auto list_opt = normalize_to_list(args);
    if (!list_opt) return nullptr;
    const json& list = *list_opt;
    if (list.empty()) return nullptr;

    double result = 1.0;
    for (const auto& item : list)
    {
        if (item.is_null()) return nullptr;
        if (!item.is_number()) return nullptr;
        result *= item.get<double>();
    }
    return result;
}

json evaluate_median_function(const std::vector<json>& args)
{
    if (args.empty()) return nullptr;
    const auto list_opt = normalize_to_list(args);
    if (!list_opt) return nullptr;
    const json& list = *list_opt;
    if (list.empty()) return nullptr;

    std::vector<double> values;
    values.reserve(list.size());  // Reserve capacity upfront
    for (const auto& item : list)
    {
        if (item.is_null()) return nullptr;
        if (!item.is_number()) return nullptr;
        values.push_back(item.get<double>());
    }

    std::sort(values.begin(), values.end());
    size_t n = values.size();
    if (n % 2 == 1)
    {
        return values[n / 2];
    }
    return (values[n / 2 - 1] + values[n / 2]) / 2.0;
}

json evaluate_stddev_function(const std::vector<json>& args)
{
    if (args.empty()) return nullptr;
    const auto list_opt = normalize_to_list(args);
    if (!list_opt) return nullptr;
    const json& list = *list_opt;
    if (list.size() < 2) return nullptr;

    // Single pass: validate, calculate sum, and build values vector
    double total = 0.0;
    std::vector<double> values;
    values.reserve(list.size());  // Reserve capacity upfront
    for (const auto& item : list)
    {
        if (item.is_null()) return nullptr;
        if (!item.is_number()) return nullptr;
        double val = item.get<double>();
        values.push_back(val);
        total += val;
    }
    double mean = total / static_cast<double>(values.size());

    // Calculate sample standard deviation (single pass with cached mean)
    double sum_sq_diff = 0.0;
    for (double v : values)
    {
        double diff = v - mean;
        sum_sq_diff += diff * diff;
    }
    // Sample stddev: divide by (N-1)
    return std::sqrt(sum_sq_diff / static_cast<double>(values.size() - 1));
}

json evaluate_mode_function(const std::vector<json>& args)
{
    if (args.empty()) return nullptr;
    const auto list_opt = normalize_to_list(args);
    if (!list_opt) return nullptr;
    const json& list = *list_opt;
    if (list.empty()) return json::array();

    // Count frequencies keyed by numeric value. std::map keeps keys ordered, so
    // the result comes out ascending without sorting the input separately. The
    // index of the first occurrence is retained so the original JSON numeric
    // representation (integer vs float) is preserved in the output.
    std::map<double, std::pair<int, size_t>> freq;
    for (size_t i = 0; i < list.size(); ++i)
    {
        const auto& item = list[i];
        if (item.is_null()) return nullptr;
        if (!item.is_number()) return nullptr;
        auto [entry, inserted] = freq.try_emplace(item.get<double>(), 0, i);
        entry->second.first++;
    }

    // Find max frequency
    int max_freq = 0;
    for (const auto& [val, info] : freq)
    {
        if (info.first > max_freq) max_freq = info.first;
    }

    // Return values with max frequency from the ordered map
    json result = json::array();
    for (const auto& [val, info] : freq)
    {
        if (info.first == max_freq)
        {
            result.push_back(list[info.second]);
        }
    }
    return result;
}

// ========== PHASE 2B: LIST MANIPULATION FUNCTIONS ==========

json evaluate_list_contains_function(const std::vector<json>& args)
{
    if (args.size() != 2) return nullptr;
    const auto& list = args[0];
    const auto& element = args[1];

    if (list.is_null()) return nullptr;
    if (!list.is_array()) return nullptr;

    return array_contains_value(list, element);
}

json evaluate_append_function(const std::vector<json>& args)
{
    if (args.empty()) return nullptr;
    const auto& list = args[0];

    json result;
    if (list.is_null())
    {
        result = json::array();
    }
    else if (list.is_array())
    {
        result = list;
    }
    else
    {
        result = json::array({list});
    }

    for (size_t i = 1; i < args.size(); ++i)
    {
        result.push_back(args[i]);
    }
    return result;
}

json evaluate_concatenate_function(const std::vector<json>& args)
{
    if (args.empty()) return nullptr;

    json result = json::array();
    auto& elements = result.get_ref<json::array_t&>();
    size_t expected = 0;
    for (const auto& arg : args)
    {
        if (arg.is_null()) continue;
        expected += arg.is_array() ? arg.size() : 1;
    }
    elements.reserve(expected);

    for (const auto& arg : args)
    {
        if (arg.is_null()) continue;
        if (arg.is_array())
        {
            for (const auto& item : arg)
            {
                result.push_back(item);
            }
        }
        else
        {
            result.push_back(arg);
        }
    }
    return result;
}

json evaluate_insert_before_function(const std::vector<json>& args)
{
    if (args.size() != 3) return nullptr;
    const auto& list = args[0];
    const auto& position = args[1];
    const auto& new_item = args[2];

    if (list.is_null() || !list.is_array()) return nullptr;
    if (position.is_null()) return nullptr;

    const auto pos_opt = to_list_index(position);
    if (!pos_opt) return nullptr;
    const long long pos = *pos_opt;
    const long long size = static_cast<long long>(list.size());

    // FEEL uses 1-based indexing, negative from end
    long long idx;
    if (pos > 0)
    {
        idx = pos - 1;
    }
    else if (pos < 0)
    {
        idx = size + pos;
    }
    else
    {
        return nullptr; // 0 is invalid
    }

    if (idx < 0 || idx > size) return nullptr;

    json result = list;
    result.insert(result.begin() + static_cast<json::difference_type>(idx), new_item);
    return result;
}

json evaluate_remove_function(const std::vector<json>& args)
{
    if (args.size() != 2) return nullptr;
    const auto& list = args[0];
    const auto& position = args[1];

    if (list.is_null() || !list.is_array()) return nullptr;
    if (position.is_null()) return nullptr;

    const auto pos_opt = to_list_index(position);
    if (!pos_opt) return nullptr;
    const long long pos = *pos_opt;
    const long long size = static_cast<long long>(list.size());

    long long idx;
    if (pos > 0)
    {
        idx = pos - 1;
    }
    else if (pos < 0)
    {
        idx = size + pos;
    }
    else
    {
        return nullptr;
    }

    if (idx < 0 || idx >= size) return nullptr;

    json result = list;
    result.erase(result.begin() + static_cast<json::difference_type>(idx));
    return result;
}

json evaluate_reverse_function(const std::vector<json>& args)
{
    if (args.size() != 1) return nullptr;
    const auto& list = args[0];

    if (list.is_null()) return nullptr;
    if (!list.is_array()) return json::array({list});

    json result = json::array();
    result.get_ref<json::array_t&>().reserve(list.size());  // Reserve capacity
    for (auto it = list.rbegin(); it != list.rend(); ++it)
    {
        result.push_back(*it);
    }
    return result;
}

json evaluate_index_of_function(const std::vector<json>& args)
{
    if (args.size() != 2) return nullptr;
    const auto& list = args[0];
    const auto& match = args[1];

    if (list.is_null() || !list.is_array()) return nullptr;

    json result = json::array();
    for (size_t i = 0; i < list.size(); ++i)
    {
        if (list[i] == match)
        {
            result.push_back(static_cast<double>(i + 1)); // 1-based
        }
    }
    return result;
}

json evaluate_sublist_function(const std::vector<json>& args)
{
    if (args.size() < 2 || args.size() > 3) return nullptr;
    const auto& list = args[0];
    const auto& start_pos = args[1];

    if (list.is_null() || !list.is_array()) return nullptr;
    if (start_pos.is_null()) return nullptr;

    const auto pos_opt = to_list_index(start_pos);
    if (!pos_opt) return nullptr;
    const long long pos = *pos_opt;
    const long long size = static_cast<long long>(list.size());

    long long start;
    if (pos > 0)
    {
        start = pos - 1;
    }
    else if (pos < 0)
    {
        start = size + pos;
    }
    else
    {
        return nullptr;
    }

    if (start < 0 || start >= size) return nullptr;

    long long length = size - start; // default: rest of list
    if (args.size() == 3 && !args[2].is_null())
    {
        const auto len_opt = to_list_index(args[2]);
        if (!len_opt) return nullptr;
        length = *len_opt;
        if (length < 0) return nullptr;
    }

    // Clamp before adding so the bound cannot overflow
    const long long end = (length > size - start) ? size : start + length;

    json result = json::array();
    result.get_ref<json::array_t&>().reserve(static_cast<size_t>(end - start));
    for (long long i = start; i < end; ++i)
    {
        result.push_back(list[static_cast<size_t>(i)]);
    }
    return result;
}

json evaluate_union_function(const std::vector<json>& args)
{
    if (args.empty()) return nullptr;

    json result = json::array();
    for (const auto& arg : args)
    {
        if (arg.is_null()) continue;
        if (arg.is_array())
        {
            for (const auto& item : arg)
            {
                if (!array_contains_value(result, item)) result.push_back(item);
            }
        }
        else
        {
            if (!array_contains_value(result, arg)) result.push_back(arg);
        }
    }
    return result;
}

json evaluate_distinct_values_function(const std::vector<json>& args)
{
    if (args.size() != 1) return nullptr;
    const auto& list = args[0];

    if (list.is_null()) return nullptr;
    if (!list.is_array()) return json::array({list});

    json result = json::array();
    result.get_ref<json::array_t&>().reserve(list.size());
    for (const auto& item : list)
    {
        if (!array_contains_value(result, item)) result.push_back(item);
    }
    return result;
}

static void flatten_recursive(const json& value, json& result)
{
    if (value.is_array())
    {
        for (const auto& item : value)
        {
            flatten_recursive(item, result);
        }
    }
    else
    {
        result.push_back(value);
    }
}

json evaluate_flatten_function(const std::vector<json>& args)
{
    if (args.size() != 1) return nullptr;
    const auto& list = args[0];

    if (list.is_null()) return nullptr;
    if (!list.is_array()) return json::array({list});

    json result = json::array();
    result.get_ref<json::array_t&>().reserve(list.size());  // Lower bound for nested lists
    flatten_recursive(list, result);
    return result;
}

json evaluate_sort_function(const std::vector<json>& args)
{
    // The sort(list, precedes) overload needs first-class functions (Phase 7B).
    // Reject it explicitly rather than silently ignoring the comparator.
    if (args.size() != 1) return nullptr;
    const auto& list = args[0];

    if (list.is_null()) return nullptr;
    if (!list.is_array()) return nullptr;
    if (list.empty()) return json::array();

    // Single pass to determine element type
    bool all_numbers = true;
    bool all_strings = true;
    for (const auto& item : list)
    {
        if (!item.is_number()) all_numbers = false;
        if (!item.is_string()) all_strings = false;
        if (!all_numbers && !all_strings) break;  // Early exit if mixed
    }

    // Sort the JSON values in place so the original numeric representation
    // (integer vs float) and full integer precision are preserved.
    if (all_numbers)
    {
        json result = list;
        auto& elements = result.get_ref<json::array_t&>();
        std::sort(elements.begin(), elements.end(), [](const json& a, const json& b) {
            return a.get<double>() < b.get<double>();
        });
        return result;
    }
    if (all_strings)
    {
        json result = list;
        auto& elements = result.get_ref<json::array_t&>();
        std::sort(elements.begin(), elements.end(), [](const json& a, const json& b) {
            return a.get_ref<const std::string&>() < b.get_ref<const std::string&>();
        });
        return result;
    }

    // Mixed types or unsupported element types: return null
    return nullptr;
}

json evaluate_list_replace_function(const std::vector<json>& args)
{
    if (args.size() != 3) return nullptr;
    const auto& list_arg = args[0];
    const auto& position_or_match = args[1];
    const auto& new_item = args[2];

    if (list_arg.is_null()) return nullptr;

    // Single copy: either the list itself or the singleton-coerced wrapper.
    json list = list_arg.is_array() ? list_arg : json::array({list_arg});

    // FEEL overload: list replace(list, match(item, newItem), newItem)
    // Current parser/evaluator does not represent first-class functions yet.
    // For constant predicates (true/false), apply the match semantics directly.
    if (position_or_match.is_boolean())
    {
        if (!position_or_match.get<bool>())
        {
            return list; // Predicate never matches
        }
        for (auto& item : list)
        {
            item = new_item; // Predicate always matches
        }
        return list;
    }

    if (position_or_match.is_null()) return nullptr;

    const auto pos_opt = to_list_index(position_or_match);
    if (!pos_opt) return nullptr;
    const long long pos = *pos_opt;
    const long long size = static_cast<long long>(list.size());

    long long idx;
    if (pos > 0)
    {
        idx = pos - 1;
    }
    else if (pos < 0)
    {
        idx = size + pos;
    }
    else
    {
        return nullptr;
    }

    if (idx < 0 || idx >= size) return nullptr;

    list[static_cast<size_t>(idx)] = new_item;
    return list;
}

// ========== PHASE 3: CONTEXT FUNCTIONS ==========

json evaluate_get_value_function(const std::vector<json>& args)
{
    if (args.size() != 2) return nullptr;
    const auto& context = args[0];
    const auto& key = args[1];

    if (context.is_null() || key.is_null()) return nullptr;
    if (!context.is_object()) return nullptr;
    if (!key.is_string()) return nullptr;

    std::string key_str = key.get<std::string>();
    auto it = context.find(key_str);
    if (it != context.end())
    {
        return *it;
    }
    return nullptr;
}

json evaluate_get_entries_function(const std::vector<json>& args)
{
    if (args.size() != 1) return nullptr;
    const auto& context = args[0];

    if (context.is_null()) return nullptr;
    if (!context.is_object()) return nullptr;

    json result = json::array();
    for (auto it = context.begin(); it != context.end(); ++it)
    {
        json entry = json::object();
        entry["key"] = it.key();
        entry["value"] = it.value();
        result.push_back(entry);
    }
    return result;
}

json evaluate_context_function(const std::vector<json>& args)
{
    if (args.size() != 1) return nullptr;
    const auto& entries = args[0];

    if (entries.is_null()) return nullptr;

    // Single entry coercion: if given a single context instead of a list, wrap it
    json entry_list;
    if (entries.is_object())
    {
        entry_list = json::array();
        entry_list.push_back(entries);
    }
    else if (entries.is_array())
    {
        entry_list = entries;
    }
    else
    {
        return nullptr;
    }

    json result = json::object();
    for (const auto& entry : entry_list)
    {
        if (!entry.is_object()) return nullptr;

        auto key_it = entry.find("key");
        auto value_it = entry.find("value");
        if (key_it == entry.end() || value_it == entry.end()) return nullptr;
        if (!key_it->is_string()) return nullptr;

        std::string key_str = key_it->get<std::string>();

        // Duplicate keys are not allowed per DMN spec
        if (result.contains(key_str)) return nullptr;

        result[key_str] = *value_it;
    }
    return result;
}

json evaluate_context_put_function(const std::vector<json>& args)
{
    if (args.size() != 3) return nullptr;
    const auto& context = args[0];
    const auto& key = args[1];
    const auto& value = args[2];

    if (context.is_null()) return nullptr;
    if (!context.is_object()) return nullptr;

    // Handle list-of-keys (nested path) variant
    if (key.is_array())
    {
        if (key.empty()) return nullptr;
        
        // Validate all keys are strings
        for (const auto& k : key)
        {
            if (k.is_null() || !k.is_string()) return nullptr;
        }
        
        if (key.size() == 1)
        {
            json result = context;
            result[key[0].get<std::string>()] = value;
            return result;
        }
        
        // Nested path: recursively set value
        std::string first_key = key[0].get<std::string>();
        json sub_keys = json::array();
        for (size_t i = 1; i < key.size(); ++i)
            sub_keys.push_back(key[i]);
        
        // Get sub-context — must exist and be an object
        json sub_context;
        if (context.contains(first_key))
        {
            if (!context[first_key].is_object())
                return nullptr; // Can't traverse into non-object
            sub_context = context[first_key];
        }
        else
        {
            sub_context = json::object();
        }
        
        json new_sub = evaluate_context_put_function({sub_context, sub_keys, value});
        if (new_sub.is_null()) return nullptr;
        
        json result = context;
        result[first_key] = new_sub;
        return result;
    }

    // DMN spec: key must be a string
    if (!key.is_string())
    {
        return nullptr;
    }

    json result = context;
    result[key.get<std::string>()] = value;
    return result;
}

json evaluate_context_merge_function(const std::vector<json>& args)
{
    if (args.size() != 1) return nullptr;
    const auto& contexts = args[0];

    if (contexts.is_null()) return nullptr;

    // Can accept a single context or a list of contexts
    if (contexts.is_object())
    {
        return contexts; // Single context, return as-is
    }

    if (!contexts.is_array()) return nullptr;

    json result = json::object();
    for (const auto& ctx : contexts)
    {
        if (ctx.is_null()) continue;
        if (!ctx.is_object()) return nullptr;

        for (auto it = ctx.begin(); it != ctx.end(); ++it)
        {
            result[it.key()] = it.value(); // Later context wins
        }
    }
    return result;
}

// ========== BUILT-IN FUNCTION DISPATCH ==========

namespace {
    /// Adapt a plain `json(const std::vector<json>&)` builtin to BuiltinHandler.
    template <json (*Fn)(const std::vector<json>&)>
    json adapt_args_only(const std::vector<json>& args, const json&, const EvaluationContext&)
    {
        return Fn(args);
    }

    /// `matches` additionally needs the evaluation context for the regex cache.
    json adapt_matches(const std::vector<json>& args, const json&, const EvaluationContext& eval_ctx)
    {
        return evaluate_matches_function(args, eval_ctx);
    }

    /// `replace` also needs the evaluation context for the regex cache.
    json adapt_replace(const std::vector<json>& args, const json&, const EvaluationContext& eval_ctx)
    {
        return evaluate_replace_function(args, eval_ctx);
    }

    const std::unordered_map<std::string_view, BuiltinHandler>& builtin_table()
    {
        static const std::unordered_map<std::string_view, BuiltinHandler> table = {
            // Boolean / list predicates
            {"not",                       adapt_args_only<&evaluate_not_function>},
            {"all",                       adapt_args_only<&evaluate_all_function>},
            {"any",                       adapt_args_only<&evaluate_any_function>},
            {"contains",                  adapt_args_only<&evaluate_contains_function>},

            // Math functions
            {"abs",                       adapt_args_only<&evaluate_abs_function>},
            {"sqrt",                      adapt_args_only<&evaluate_sqrt_function>},
            {"floor",                     adapt_args_only<&evaluate_floor_function>},
            {"ceiling",                   adapt_args_only<&evaluate_ceiling_function>},
            {"exp",                       adapt_args_only<&evaluate_exp_function>},
            {"log",                       adapt_args_only<&evaluate_log_function>},
            {"modulo",                    adapt_args_only<&evaluate_modulo_function>},
            {"decimal",                   adapt_args_only<&evaluate_decimal_function>},
            {"round",                     adapt_args_only<&evaluate_round_function>},
            {"round up",                  adapt_args_only<&evaluate_round_up_function>},
            {"round down",                adapt_args_only<&evaluate_round_down_function>},
            {"round half up",             adapt_args_only<&evaluate_round_half_up_function>},
            {"round half down",           adapt_args_only<&evaluate_round_half_down_function>},

            // String functions
            {"substring before",          adapt_args_only<&evaluate_substring_before_function>},
            {"substring after",           adapt_args_only<&evaluate_substring_after_function>},
            {"substring",                 adapt_args_only<&evaluate_substring_function>},
            {"string length",             adapt_args_only<&evaluate_string_length_function>},
            {"upper case",                adapt_args_only<&evaluate_upper_case_function>},
            {"lower case",                adapt_args_only<&evaluate_lower_case_function>},
            {"starts with",               adapt_args_only<&evaluate_starts_with_function>},
            {"ends with",                 adapt_args_only<&evaluate_ends_with_function>},
            {"replace",                   adapt_replace},
            {"matches",                   adapt_matches},
            {"split",                     adapt_args_only<&evaluate_split_function>},
            {"string join",               adapt_args_only<&evaluate_string_join_function>},

            // Date / time functions
            {"date",                      adapt_args_only<&evaluate_date_function>},
            {"duration",                  adapt_args_only<&evaluate_duration_function>},
            {"time",                      adapt_args_only<&evaluate_time_function>},
            {"date and time",             adapt_args_only<&evaluate_date_and_time_function>},
            {"years and months duration", adapt_args_only<&evaluate_years_and_months_duration_function>},
            {"day of year",               adapt_args_only<&evaluate_day_of_year_function>},
            {"day of week",               adapt_args_only<&evaluate_day_of_week_function>},
            {"month of year",             adapt_args_only<&evaluate_month_of_year_function>},
            {"week of year",              adapt_args_only<&evaluate_week_of_year_function>},
            {"now",                       adapt_args_only<&evaluate_now_function>},
            {"today",                     adapt_args_only<&evaluate_today_function>},

            // Phase 1: Trivial functions
            {"odd",                       adapt_args_only<&evaluate_odd_function>},
            {"even",                      adapt_args_only<&evaluate_even_function>},
            {"number",                    adapt_args_only<&evaluate_number_function>},
            {"string",                    adapt_args_only<&evaluate_string_function>},
            {"is",                        adapt_args_only<&evaluate_is_function>},

            // Phase 2A: Aggregation functions
            {"count",                     adapt_args_only<&evaluate_count_function>},
            {"sum",                       adapt_args_only<&evaluate_sum_function>},
            {"min",                       adapt_args_only<&evaluate_min_function>},
            {"max",                       adapt_args_only<&evaluate_max_function>},
            {"mean",                      adapt_args_only<&evaluate_mean_function>},
            {"product",                   adapt_args_only<&evaluate_product_function>},
            {"median",                    adapt_args_only<&evaluate_median_function>},
            {"stddev",                    adapt_args_only<&evaluate_stddev_function>},
            {"mode",                      adapt_args_only<&evaluate_mode_function>},

            // Phase 2B: List manipulation functions
            {"list contains",             adapt_args_only<&evaluate_list_contains_function>},
            {"append",                    adapt_args_only<&evaluate_append_function>},
            {"concatenate",               adapt_args_only<&evaluate_concatenate_function>},
            {"insert before",             adapt_args_only<&evaluate_insert_before_function>},
            {"remove",                    adapt_args_only<&evaluate_remove_function>},
            {"reverse",                   adapt_args_only<&evaluate_reverse_function>},
            {"index of",                  adapt_args_only<&evaluate_index_of_function>},
            {"sublist",                   adapt_args_only<&evaluate_sublist_function>},
            {"union",                     adapt_args_only<&evaluate_union_function>},
            {"distinct values",           adapt_args_only<&evaluate_distinct_values_function>},
            {"flatten",                   adapt_args_only<&evaluate_flatten_function>},
            {"sort",                      adapt_args_only<&evaluate_sort_function>},
            {"list replace",              adapt_args_only<&evaluate_list_replace_function>},

            // Phase 3: Context functions
            {"get value",                 adapt_args_only<&evaluate_get_value_function>},
            {"get entries",               adapt_args_only<&evaluate_get_entries_function>},
            {"context",                   adapt_args_only<&evaluate_context_function>},
            {"context put",               adapt_args_only<&evaluate_context_put_function>},
            {"context merge",             adapt_args_only<&evaluate_context_merge_function>},
        };
        return table;
    }
} // namespace

const BuiltinHandler* find_builtin_handler(std::string_view name)
{
    const auto& table = builtin_table();
    const auto it = table.find(name);
    return it != table.end() ? &it->second : nullptr;
}

} // namespace orion::bre::feel

