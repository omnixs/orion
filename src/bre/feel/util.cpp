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

#include "util_internal.hpp"
#include <orion/api/logger.hpp>
#include <regex>
#include <algorithm>
#include <cctype>
#include <limits>

namespace orion::bre::detail
{
    using json = nlohmann::json;
    using std::string;
    using std::vector;
    using orion::api::warn;

    // Forward declarations for recursive parsing functions
    double parse_expression(std::string_view expr, size_t& pos);
    double parse_term(std::string_view expr, size_t& pos);
    double parse_power(std::string_view expr, size_t& pos);
    double parse_factor(std::string_view expr, size_t& pos);
    double parse_parenthesized_expression_impl(std::string_view expr, size_t& pos);
    double parse_number_literal_impl(std::string_view expr, size_t& pos);
    double parse_identifier_or_variable(std::string_view expr, size_t& pos);
    std::string extract_variable_name(std::string_view expr, size_t start_pos, size_t& pos);
    std::string try_extend_variable_name(std::string_view expr, std::string_view var_name, size_t start_pos, size_t& pos);
    double resolve_feel_constant(std::string_view var_name);
    double resolve_variable_from_context(std::string_view var_name, bool& found);
    void skip_whitespace(std::string_view expr, size_t& pos);

    // Removed unused constant MONTHS_PER_YEAR

    std::vector<std::string> split_arguments(std::string_view args_str)
    {
        vector<string> args;
        string current;
        int paren_level = 0;

        for (char chr : args_str)
        {
            if (chr == ',' && paren_level == 0)
            {
                if (!current.empty())
                {
                    // Trim whitespace
                    size_t start = current.find_first_not_of(' ');
                    size_t end = current.find_last_not_of(' ');
                    if (start != string::npos)
                    {
                        args.push_back(current.substr(start, end - start + 1));
                    }
                }
                current.clear();
            }
            else
            {
                if (chr == '(') {
                    paren_level++;
                } else if (chr == ')') {
                    paren_level--;
                }
                current += chr;
            }
        }

        if (!current.empty())
        {
            // Trim whitespace
            size_t start = current.find_first_not_of(' ');
            size_t end = current.find_last_not_of(' ');
            if (start != string::npos)
            {
                args.push_back(current.substr(start, end - start + 1));
            }
        }

        return args;
    }

    nlohmann::json resolve_argument(std::string_view arg, const nlohmann::json& context)
    {
        // Handle dotted property access like "Loan.amount"
        if (arg.find('.') != std::string_view::npos)
        {
            size_t dot_pos = arg.find('.');
            string obj_name(arg.substr(0, dot_pos));
            string prop_name(arg.substr(dot_pos + 1));

            if (context.contains(obj_name) && context[obj_name].is_object())
            {
                const auto& obj = context[obj_name];
                if (obj.contains(prop_name))
                {
                    return obj[prop_name];
                }
            }
        }

        // Handle direct property access
        string arg_str(arg);
        if (context.contains(arg_str))
        {
            return context[arg_str];
        }

        // Try to parse as number
        try
        {
            if (arg.find('.') != std::string_view::npos)
            {
                return json(std::stod(arg_str));
            }
            return json(std::stoll(arg_str));
        }
        catch (...)
        {
            // Conversion failed - not a numeric literal
        }

        return json{};
    }

    double resolve_property_path(std::string_view path, const nlohmann::json& context)
    {
        size_t dot_pos = path.find('.');
        if (dot_pos == std::string_view::npos) {
            return std::numeric_limits<double>::quiet_NaN();
        }

        string obj_name(path.substr(0, dot_pos));
        string prop_name(path.substr(dot_pos + 1));

        // Try both cases - the original and lowercase
        const json* obj = nullptr;
        if (context.contains(obj_name))
        {
            obj = &context[obj_name];
        }
        else
        {
            // Try lowercase
            string lower_obj_name = obj_name;
            std::transform(lower_obj_name.begin(), lower_obj_name.end(), lower_obj_name.begin(), ::tolower);
            if (context.contains(lower_obj_name))
            {
                obj = &context[lower_obj_name];
            }
        }

        if (obj && obj->is_object() && obj->contains(prop_name) && (*obj)[prop_name].is_number())
        {
            return (*obj)[prop_name].get<double>();
        }

        return std::numeric_limits<double>::quiet_NaN();
    }

    nlohmann::json evaluate_complex_arithmetic_expression(std::string_view expression, const nlohmann::json& context)
    {
        // Handle expressions like "(loan.principal*loan.rate/MONTHS_PER_YEAR)/(1-(1+loan.rate/MONTHS_PER_YEAR)**-loan.termMonths)"
        try
        {
            string expr(expression);  // regex requires std::string

            // Replace property references with their values
            std::regex prop_regex(R"([a-zA-Z][a-zA-Z0-9_]*\.[a-zA-Z][a-zA-Z0-9_]*)");
            std::smatch match;

        auto replacer = [&context](const string& prop_path) -> string
        {
            size_t dot_pos = prop_path.find('.');
            if (dot_pos == string::npos)
            {
                return prop_path;
            }

            string obj_name = prop_path.substr(0, dot_pos);
            string prop_name = prop_path.substr(dot_pos + 1);               if (context.contains(obj_name) && context[obj_name].is_object())
                {
                    const auto& obj = context[obj_name];
                    if (obj.contains(prop_name) && obj[prop_name].is_number())
                    {
                        return std::to_string(obj[prop_name].get<double>());
                    }
                }
                return prop_path; // Return unchanged if not found
            };

            // Replace all property references
            string::const_iterator start = expr.cbegin();
            string result;
            while (std::regex_search(start, expr.cend(), match, prop_regex))
            {
                result.append(start, match[0].first);
                result.append(replacer(match.str()));
                start = match[0].second;
            }
            result.append(start, expr.cend());

            // Now evaluate the numerical expression
            return eval_math_expression(result);
        }
        catch (...)
        {
            // Expression evaluation failed
            return json{};
        }
    }

    nlohmann::json eval_math_expression(std::string_view expr)
    {
        warn("[LEGACY-MATH-PARSER] *** PARSING ARITHMETIC DURING EVALUATION *** Expression: '{}'", expr);
        
        try
        {
            // Simple expression evaluator using recursion
            size_t pos = 0;
            double result = parse_expression(expr, pos);

            // Check if we consumed the entire expression
            while (pos < expr.length() && std::isspace(static_cast<unsigned char>(expr[pos]))) {
                pos++;
            }
            if (pos >= expr.length())
            {
                // Check for NaN (division by zero) and return null
                if (std::isnan(result))
                {
                    return json{};
                }
                return json(result);
            }
        }
        catch (...)
        {
            // Parse error
        }

        return json{};
    }

    double parse_expression(std::string_view expr, size_t& pos)
    {
        double result = parse_term(expr, pos);

        while (pos < expr.length())
        {
            skip_whitespace(expr, pos);
            if (pos >= expr.length()) {
                break;
            }

            char oper = expr[pos];
            if (oper == '+' || oper == '-')
            {
                pos++;
                double right = parse_term(expr, pos);
                // DMN null propagation: if either operand is NaN (null), result is NaN
                if (std::isnan(result) || std::isnan(right))
                {
                    return std::numeric_limits<double>::quiet_NaN();
                }
                result = (oper == '+') ? result + right : result - right;
            }
            else {
                break;
            }
        }
        return result;
    }

    double parse_term(std::string_view expr, size_t& pos)
    {
        double result = parse_power(expr, pos);

        while (pos < expr.length())
        {
            skip_whitespace(expr, pos);
            if (pos >= expr.length()) {
                break;
            }

            if (expr[pos] == '*' || expr[pos] == '/')
            {
                char oper = expr[pos++];
                double right = parse_power(expr, pos);
                // DMN null propagation: if either operand is NaN (null), result is NaN
                if (std::isnan(result) || std::isnan(right))
                {
                    return std::numeric_limits<double>::quiet_NaN();
                }
                if (oper == '/')
                {
                    if (right == 0.0)
                    {
                        return std::numeric_limits<double>::quiet_NaN(); // Division by zero
                    }
                    result = result / right;
                }
                else
                {
                    result = result * right;
                }
            }
            else {
                break;
            }
        }
        return result;
    }   double parse_power(std::string_view expr, size_t& pos)
    {
        double result = parse_factor(expr, pos);

        while (pos < expr.length())
        {
            skip_whitespace(expr, pos);
            if (pos >= expr.length()) {
                break;
            }

            if (pos + 1 < expr.length() && expr.substr(pos, 2) == "**")
            {
                pos += 2;
                double right = parse_power(expr, pos); // Right associative
                // DMN null propagation: if either operand is NaN (null), result is NaN
                if (std::isnan(result) || std::isnan(right))
                {
                    return std::numeric_limits<double>::quiet_NaN();
                }
                result = std::pow(result, right);
            }
            else {
                break;
            }
        }
        return result;
    }

    double parse_factor(std::string_view expr, size_t& pos)
{
    skip_whitespace(expr, pos);
    if (pos >= expr.length()) {
        return 0.0;
    }
    
    // Handle unary + or -
    bool negative = false;
    if (expr[pos] == '-')
    {
        negative = true;
        pos++;
        skip_whitespace(expr, pos);
    }
    else if (expr[pos] == '+')
    {
        pos++;
        skip_whitespace(expr, pos);
    }
    
    // Dispatch to specialized parsing methods
    double result = 0.0;
    if (pos < expr.length() && expr[pos] == '(')
    {
        result = parse_parenthesized_expression_impl(expr, pos);
    }
    else if (pos < expr.length() && (std::isdigit(static_cast<unsigned char>(expr[pos])) || expr[pos] == '.'))
    {
        result = parse_number_literal_impl(expr, pos);
    }
    else if (pos < expr.length() && (std::isalpha(static_cast<unsigned char>(expr[pos])) || expr[pos] == '_'))
    {
        result = parse_identifier_or_variable(expr, pos);
    }
    
    return negative ? -result : result;
}

double parse_parenthesized_expression_impl(std::string_view expr, size_t& pos)
{
    pos++; // consume '('
    double result = parse_expression(expr, pos);
    skip_whitespace(expr, pos);
    if (pos < expr.length() && expr[pos] == ')') {
        pos++;
    }
    return result;
}

double parse_number_literal_impl(std::string_view expr, size_t& pos)
{
    size_t end_pos = pos;
    bool has_decimal = false;
    while (end_pos < expr.length() && (std::isdigit(static_cast<unsigned char>(expr[end_pos])) || (expr[end_pos] == '.' && !has_decimal)))
    {
        if (expr[end_pos] == '.') {
            has_decimal = true;
        }
        end_pos++;
    }
    double result = std::stod(string(expr.substr(pos, end_pos - pos)));
    pos = end_pos;
    return result;
}

double parse_identifier_or_variable(std::string_view expr, size_t& pos)
{
    size_t start_pos = pos;
    std::string var_name = extract_variable_name(expr, start_pos, pos);
    
    // Try to extend variable name with spaces if needed
    if (orion::bre::detail::current_eval_context)
    {
        var_name = try_extend_variable_name(expr, var_name, start_pos, pos);
    }
    
    // Handle FEEL literal constants
    double constant_value = resolve_feel_constant(var_name);
    if (!std::isnan(constant_value) || var_name == "null")
    {
        return constant_value;
    }
    
    // Resolve from context
    bool found = false;
    double var_value = resolve_variable_from_context(var_name, found);
    return found ? var_value : 0.0; // Unresolved: treat as 0.0
}

std::string extract_variable_name(std::string_view expr, size_t start_pos, size_t& pos)
{
    // Parse standard identifier (no spaces)
    while (pos < expr.length() && (std::isalnum(static_cast<unsigned char>(expr[pos])) || expr[pos] == '_' || expr[pos] == '-')) {
        pos++;
    }
    return string(expr.substr(start_pos, pos - start_pos));
}

std::string try_extend_variable_name(std::string_view expr, std::string_view var_name, size_t start_pos, size_t& pos)
{
    const auto& ctx = *orion::bre::detail::current_eval_context;
    
    // If standard name exists, use it
    if (ctx.contains(var_name))
    {
        return std::string(var_name);
    }
    
    // Try to extend with spaces for variable names like "Monthly Salary"
    size_t extended_pos = pos;
    while (extended_pos < expr.length())
    {
        // Skip whitespace
        while (extended_pos < expr.length() && std::isspace(static_cast<unsigned char>(expr[extended_pos]))) {
            extended_pos++;
        }
        
        // Check if next part looks like continuation
        if (extended_pos < expr.length() && std::isalpha(static_cast<unsigned char>(expr[extended_pos])))
        {
            // Found potential continuation
            while (extended_pos < expr.length() && (std::isalnum(static_cast<unsigned char>(expr[extended_pos])) || expr[extended_pos] == '_' || expr[extended_pos] == '-')) {
                extended_pos++;
            }
            
            std::string potential_name(expr.substr(start_pos, extended_pos - start_pos));
            if (ctx.contains(potential_name))
            {
                pos = extended_pos;
                return potential_name;
            }
        }
        else
        {
            break;
        }
    }
    
    return std::string(var_name); // Return original if extension didn't help
}

double resolve_feel_constant(std::string_view var_name)
{
    if (var_name == "null")
    {
        return std::numeric_limits<double>::quiet_NaN();
    }
    if (var_name == "true")
    {
        return 1.0;
    }
    if (var_name == "false")
    {
        return 0.0;
    }
    return std::numeric_limits<double>::quiet_NaN(); // Not a constant
}

double resolve_variable_from_context(std::string_view var_name, bool& found)
{
    found = false;
    if (!orion::bre::detail::current_eval_context)
    {
        return 0.0;
    }
    
    const auto& ctx = *orion::bre::detail::current_eval_context;
    if (!ctx.contains(var_name))
    {
        return 0.0;
    }
    
    const auto& value = ctx[var_name];
    
    if (value.is_number())
    {
        found = true;
        return value.get<double>();
    }
    
    if (value.is_string())
    {
        try
        {
            found = true;
            return std::stod(value.get<std::string>());
        }
        catch (...)
        {
            // String to number conversion failed
            return 0.0;
        }
    }
    
    if (value.is_boolean())
    {
        found = true;
        return value.get<bool>() ? 1.0 : 0.0;
    }
    
    if (value.is_null())
    {
        found = true;
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return 0.0;
}

    void skip_whitespace(std::string_view expr, size_t& pos)
    {
        while (pos < expr.length() && std::isspace(static_cast<unsigned char>(expr[pos]))) {
            pos++;
        }
    }
} // namespace orion::bre::detail
