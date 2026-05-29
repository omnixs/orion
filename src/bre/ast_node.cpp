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

#include "orion/bre/ast_node.hpp"
#include <orion/bre/feel/functions.hpp>
#include <orion/bre/feel/parameter_binder.hpp>
#include <stdexcept>
#include <sstream>
#include <cmath>
#include <algorithm>

namespace orion::bre
{
    namespace
    {
        /**
         * @brief Resolve a variable from context with multiple naming variants
         * 
         * Tries multiple variations of the variable name to handle DMN naming flexibility:
         * - Exact match
         * - With underscores instead of spaces
         * - Lowercase
         * - Lowercase with underscores
         * - No spaces
         */
        json resolveVariable(std::string_view name, const json& context)
        {
            // Try exact match first (hot path — avoids all string allocations)
            if (auto it = context.find(name); it != context.end())
            {
                return *it;
            }
            
            // Try with underscores instead of spaces (only if name contains spaces)
            if (name.find(' ') != std::string_view::npos)
            {
                std::string underscored(name);
                std::replace(underscored.begin(), underscored.end(), ' ', '_');
                if (auto it = context.find(underscored); it != context.end())
                {
                    return *it;
                }
                
                // Try lowercase with underscores
                std::string lower_us = underscored;
                std::transform(lower_us.begin(), lower_us.end(), lower_us.begin(), ::tolower);
                if (lower_us != underscored)
                {
                    if (auto it = context.find(lower_us); it != context.end())
                    {
                        return *it;
                    }
                }
                
                // Try without spaces
                std::string nospace(name);
                nospace.erase(std::remove(nospace.begin(), nospace.end(), ' '), nospace.end());
                if (auto it = context.find(nospace); it != context.end())
                {
                    return *it;
                }
            }
            
            // Try lowercase (only if name has uppercase characters)
            {
                bool has_upper = false;
                for (char c : name)
                {
                    if (std::isupper(static_cast<unsigned char>(c)))
                    {
                        has_upper = true;
                        break;
                    }
                }
                if (has_upper)
                {
                    std::string lower(name);
                    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
                    if (auto it = context.find(lower); it != context.end())
                    {
                        return *it;
                    }
                }
            }
            
            // Variable not found
            std::ostringstream oss;
            oss << "Undefined variable: '" << name << "'";
            throw std::runtime_error(oss.str());
        }
        
        /**
         * @brief Convert JSON value to number for arithmetic operations
         */
        double toNumber(const json& value, std::string_view operation)
        {
            if (value.is_number())
            {
                return value.get<double>();
            }
            if (value.is_null())
            {
                return 0.0; // null treated as 0 in arithmetic
            }
            if (value.is_boolean())
            {
                return value.get<bool>() ? 1.0 : 0.0;
            }
            if (value.is_string())
            {
                try
                {
                    return std::stod(value.get<std::string>());
                }
                catch (const std::invalid_argument&)
                {
                    std::ostringstream oss;
                    oss << "Cannot convert string to number in " << operation;
                    throw std::runtime_error(oss.str());
                }
                catch (const std::out_of_range&)
                {
                    std::ostringstream oss;
                    oss << "Number out of range in " << operation;
                    throw std::runtime_error(oss.str());
                }
            }
            
            std::ostringstream oss;
            oss << "Type error in " << operation << ": expected number";
            throw std::runtime_error(oss.str());
        }
        
        /**
         * @brief Convert JSON value to string for concatenation
         */
        std::string toString(const json& value)
        {
            if (value.is_string())
            {
                return value.get<std::string>();
            }
            if (value.is_number())
            {
                double d = value.get<double>();
                // Check if it's an integer
                if (d == std::floor(d))
                {
                    return std::to_string(static_cast<long long>(d));
                }
                return std::to_string(d);
            }
            if (value.is_boolean())
            {
                return value.get<bool>() ? "true" : "false";
            }
            if (value.is_null())
            {
                return "null";
            }
            return value.dump(); // Fallback: JSON representation
        }
        
        /**
         * @brief Convert JSON value to boolean for logical operations
         */
        bool toBoolean(const json& value)
        {
            if (value.is_boolean())
            {
                return value.get<bool>();
            }
            if (value.is_number())
            {
                return value.get<double>() != 0.0;
            }
            if (value.is_string())
            {
                std::string s = value.get<std::string>();
                return !s.empty() && s != "false" && s != "0";
            }
            if (value.is_null())
            {
                return false;
            }
            return true; // Non-empty objects/arrays are truthy
        }
    }
    
    json ASTNode::evaluate(const json& input, const EvaluationContext& eval_ctx) const
    {
        switch (type)
        {
            case ASTNodeType::LITERAL_NUMBER:
            {
                // Handle special keywords
                if (value == "true") return true;
                if (value == "false") return false;
                if (value == "null") return nullptr;
                
                // Parse numeric literal
                try
                {
                    // Try integer first
                    if (value.find('.') == std::string::npos && 
                        value.find('e') == std::string::npos && 
                        value.find('E') == std::string::npos)
                    {
                        return std::stoll(value);
                    }
                    // Parse as double
                    return std::stod(value);
                }
                catch (const std::invalid_argument&)
                {
                    std::ostringstream oss;
                    oss << "Invalid number literal: '" << value << "'";
                    throw std::runtime_error(oss.str());
                }
                catch (const std::out_of_range&)
                {
                    std::ostringstream oss;
                    oss << "Number literal out of range: '" << value << "'";
                    throw std::runtime_error(oss.str());
            }
        }
        
        case ASTNodeType::LITERAL_STRING:
        {
            return value; // Already unquoted by parser
        }
        
        case ASTNodeType::LITERAL_LIST:
        {
            json listArray = json::array();
            for (const auto& child : children)
            {
                listArray.push_back(child->evaluate(input, eval_ctx));
            }
            return listArray;
        }
        
        case ASTNodeType::LITERAL_CONTEXT:
        {
            json contextObject = json::object();
            // Children are stored as pairs: [key_node, value_node, key_node, value_node, ...]
            for (size_t i = 0; i + 1 < children.size(); i += 2)
            {
                // Key is stored in a LITERAL_STRING node
                const std::string& key = children[i]->value;
                // Value is any expression
                json val = children[i + 1]->evaluate(input, eval_ctx);
                contextObject[key] = val;
            }
            return contextObject;
        }
        
        case ASTNodeType::VARIABLE:
        {
            return resolveVariable(value, input);
        }           case ASTNodeType::UNARY_OP:
            {
                if (children.size() != 1)
                {
                    throw std::runtime_error("Unary operator requires exactly one operand");
                }
                
                json operand = children[0]->evaluate(input, eval_ctx);
                
                if (value == "-")
                {
                    return -toNumber(operand, "unary minus");
                }
                else if (value == "not")
                {
                    return !toBoolean(operand);
                }
                
                std::ostringstream oss;
                oss << "Unknown unary operator: '" << value << "'";
                throw std::runtime_error(oss.str());
            }
            
            case ASTNodeType::BINARY_OP:
            {
                if (children.size() != 2)
                {
                    throw std::runtime_error("Binary operator requires exactly two operands");
                }
                
                json left = children[0]->evaluate(input, eval_ctx);
                json right = children[1]->evaluate(input, eval_ctx);
                
                // Arithmetic operators - DMN null propagation
                if (value == "+")
                {
                    // Handle string concatenation
                    if (left.is_string() || right.is_string())
                    {
                        return toString(left) + toString(right);
                    }
                    // DMN: null in arithmetic returns null
                    if (left.is_null() || right.is_null())
                    {
                        return nullptr;
                    }
                    return toNumber(left, "addition") + toNumber(right, "addition");
                }
                else if (value == "-")
                {
                    // DMN: null in arithmetic returns null
                    if (left.is_null() || right.is_null())
                    {
                        return nullptr;
                    }
                    return toNumber(left, "subtraction") - toNumber(right, "subtraction");
                }
                else if (value == "*")
                {
                    // DMN: null in arithmetic returns null
                    if (left.is_null() || right.is_null())
                    {
                        return nullptr;
                    }
                    return toNumber(left, "multiplication") * toNumber(right, "multiplication");
                }
                else if (value == "/")
                {
                    // DMN: null in arithmetic returns null
                    if (left.is_null() || right.is_null())
                    {
                        return nullptr;
                    }
                    double divisor = toNumber(right, "division");
                    // DMN: Division by zero returns null
                    if (divisor == 0.0)
                    {
                        return nullptr;
                    }
                    return toNumber(left, "division") / divisor;
                }
                else if (value == "**")
                {
                    // DMN: null in arithmetic returns null
                    if (left.is_null() || right.is_null())
                    {
                        return nullptr;
                    }
                    return std::pow(toNumber(left, "exponentiation"), toNumber(right, "exponentiation"));
                }
                
                // Comparison operators
                else if (value == "<")
                {
                    // Support string comparison for dates and other strings
                    if (left.is_string() && right.is_string())
                    {
                        return left.get<std::string>() < right.get<std::string>();
                    }
                    return toNumber(left, "less than") < toNumber(right, "less than");
                }
                else if (value == ">")
                {
                    // Support string comparison for dates and other strings
                    if (left.is_string() && right.is_string())
                    {
                        return left.get<std::string>() > right.get<std::string>();
                    }
                    return toNumber(left, "greater than") > toNumber(right, "greater than");
                }
                else if (value == "<=")
                {
                    // Support string comparison for dates and other strings
                    if (left.is_string() && right.is_string())
                    {
                        return left.get<std::string>() <= right.get<std::string>();
                    }
                    return toNumber(left, "less or equal") <= toNumber(right, "less or equal");
                }
                else if (value == ">=")
                {
                    // Support string comparison for dates and other strings
                    if (left.is_string() && right.is_string())
                    {
                        return left.get<std::string>() >= right.get<std::string>();
                    }
                    return toNumber(left, "greater or equal") >= toNumber(right, "greater or equal");
                }
                else if (value == "=" || value == "==")
                {
                    // Handle different types
                    if (left.type() != right.type())
                    {
                        return false; // Different types are not equal
                    }
                    return left == right;
                }
                else if (value == "!=")
                {
                    return left != right;
                }
                
                // Logical operators - DMN ternary logic with null propagation
                else if (value == "and")
                {
                    // DMN ternary AND logic:
                    // false and X = false (short-circuit)
                    // null and false = false
                    // null and true = null
                    // true and X = X
                    if (left.is_null())
                    {
                        // null and false = false
                        if (right.is_boolean() && !right.get<bool>())
                            return false;
                        // null and true = null, null and null = null
                        return nullptr;
                    }
                    if (right.is_null())
                    {
                        // false and null = false
                        if (left.is_boolean() && !left.get<bool>())
                            return false;
                        // true and null = null
                        return nullptr;
                    }
                    // Regular boolean AND for non-null values
                    return toBoolean(left) && toBoolean(right);
                }
                else if (value == "or")
                {
                    // DMN ternary OR logic:
                    // true or X = true (short-circuit)
                    // null or true = true
                    // null or false = null
                    // false or X = X
                    if (left.is_null())
                    {
                        // null or true = true
                        if (right.is_boolean() && right.get<bool>())
                            return true;
                        // null or false = null, null or null = null
                        return nullptr;
                    }
                    if (right.is_null())
                    {
                        // true or null = true
                        if (left.is_boolean() && left.get<bool>())
                            return true;
                        // false or null = null
                        return nullptr;
                    }
                    // Regular boolean OR for non-null values
                    return toBoolean(left) || toBoolean(right);
                }
                
                std::ostringstream oss;
                oss << "Unknown binary operator: '" << value << "'";
                throw std::runtime_error(oss.str());
            }
            
            case ASTNodeType::PROPERTY_ACCESS:
            {
                // Property access: object.property
                // The child is the object expression, value is the property name
                if (children.size() != 1)
                {
                    throw std::runtime_error("Property access requires exactly one child (object expression)");
                }
                
                // Evaluate the object expression
                json obj = children[0]->evaluate(input, eval_ctx);
                
                // Property name is stored in value
                const std::string& propertyName = value;
                
                // Handle null object - return null per DMN semantics
                if (obj.is_null())
                {
                    return nullptr;
                }
                
                // Obj must be an object/dict to have properties
                if (!obj.is_object())
                {
                    std::ostringstream oss;
                    oss << "Cannot access property '" << propertyName 
                        << "' on non-object value (type: " << obj.type_name() << ")";
                    throw std::runtime_error(oss.str());
                }
                
                // Try exact property name first (hot path — avoids all string allocations)
                if (auto it = obj.find(propertyName); it != obj.end())
                {
                    return *it;
                }
                
                // Try with spaces replaced by underscores (only if name contains spaces)
                if (propertyName.find(' ') != std::string::npos)
                {
                    std::string underscored = propertyName;
                    std::replace(underscored.begin(), underscored.end(), ' ', '_');
                    if (auto it = obj.find(underscored); it != obj.end())
                    {
                        return *it;
                    }
                }
                
                // Convert camelCase to snake_case (only if name has uppercase)
                bool has_upper = false;
                for (char c : propertyName)
                {
                    if (std::isupper(static_cast<unsigned char>(c)))
                    {
                        has_upper = true;
                        break;
                    }
                }
                
                if (has_upper)
                {
                    std::string snake_case;
                    snake_case.reserve(propertyName.length() + 4);
                    for (size_t i = 0; i < propertyName.length(); ++i)
                    {
                        char c = propertyName[i];
                        if (std::isupper(c) && i > 0)
                        {
                            snake_case += '_';
                            snake_case += static_cast<char>(std::tolower(c));
                        }
                        else
                        {
                            snake_case += static_cast<char>(std::tolower(c));
                        }
                    }
                    if (auto it = obj.find(snake_case); it != obj.end())
                    {
                        return *it;
                    }
                    
                    // Try lowercase (reuse first part of snake_case logic)
                    std::string lower = propertyName;
                    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
                    if (lower != snake_case)
                    {
                        if (auto it = obj.find(lower); it != obj.end())
                        {
                            return *it;
                        }
                    }
                }
                
                // Property not found - throw error
                std::ostringstream oss;
                oss << "Property '" << propertyName << "' not found on object";
                throw std::runtime_error(oss.str());
            }
            
        case ASTNodeType::CONDITIONAL:
        {
            // If-then-else conditional expression
            // children[0] = condition
            // children[1] = then expression
            // children[2] = else expression
            
            if (children.size() != 3)
            {
                throw std::runtime_error("Conditional expression requires exactly 3 children (condition, then, else)");
            }
            
            // Evaluate condition
            json condition = children[0]->evaluate(input, eval_ctx);
            
            // DMN spec: null condition → else branch
            if (condition.is_null())
            {
                return children[2]->evaluate(input, eval_ctx);
            }
            
            // Type validation: condition should be boolean
            if (!condition.is_boolean())
            {
                // Per DMN spec, return null for type errors
                return json(nullptr);
            }
            
            // Short-circuit evaluation: only evaluate selected branch
            if (condition.get<bool>())
            {
                // Evaluate then branch
                return children[1]->evaluate(input, eval_ctx);
            }
            else
            {
                // Evaluate else branch
                return children[2]->evaluate(input, eval_ctx);
            }
        }
            
        case ASTNodeType::FUNCTION_CALL:
        {
            const std::string& funcName = value;
            
            // Bind parameters using parameter binder
            // This handles both positional and named parameters
            // Parameter validation errors should return null per DMN spec
            std::vector<json> args;
            try {
                args = feel::bind_parameters(funcName, parameters, input, eval_ctx);
            } catch (const std::runtime_error&) {
                // Parameter validation failed (wrong param names, wrong count, etc.)
                // Return null as per DMN 1.5 spec
                return json(nullptr);
            }
            
            // Dispatch to appropriate function
            if (funcName == "not")
            {
                return feel::evaluate_not_function(args);
            }
            else if (funcName == "all")
            {
                return feel::evaluate_all_function(args);
            }
            else if (funcName == "any")
            {
                return feel::evaluate_any_function(args);
            }
            else if (funcName == "contains")
            {
                return feel::evaluate_contains_function(args);
            }
            // Math functions
            else if (funcName == "abs")
            {
                return feel::evaluate_abs_function(args);
            }
            else if (funcName == "sqrt")
            {
                return feel::evaluate_sqrt_function(args);
            }
            else if (funcName == "floor")
            {
                return feel::evaluate_floor_function(args);
            }
            else if (funcName == "ceiling")
            {
                return feel::evaluate_ceiling_function(args);
            }
            else if (funcName == "exp")
            {
                return feel::evaluate_exp_function(args);
            }
            else if (funcName == "log")
            {
                return feel::evaluate_log_function(args);
            }
            else if (funcName == "modulo")
            {
                return feel::evaluate_modulo_function(args);
            }
            else if (funcName == "decimal")
            {
                return feel::evaluate_decimal_function(args);
            }
            else if (funcName == "round")
            {
                return feel::evaluate_round_function(args);
            }
            else if (funcName == "round up")
            {
                return feel::evaluate_round_up_function(args);
            }
            else if (funcName == "round down")
            {
                return feel::evaluate_round_down_function(args);
            }
            else if (funcName == "round half up")
            {
                return feel::evaluate_round_half_up_function(args);
            }
            else if (funcName == "round half down")
            {
                return feel::evaluate_round_half_down_function(args);
            }
            // String functions
            else if (funcName == "substring before")
            {
                return feel::evaluate_substring_before_function(args);
            }
            else if (funcName == "substring after")
            {
                return feel::evaluate_substring_after_function(args);
            }
            else if (funcName == "substring")
            {
                return feel::evaluate_substring_function(args);
            }
            else if (funcName == "string length")
            {
                return feel::evaluate_string_length_function(args);
            }
            else if (funcName == "upper case")
            {
                return feel::evaluate_upper_case_function(args);
            }
            else if (funcName == "lower case")
            {
                return feel::evaluate_lower_case_function(args);
            }
            else if (funcName == "starts with")
            {
                return feel::evaluate_starts_with_function(args);
            }
            else if (funcName == "ends with")
            {
                return feel::evaluate_ends_with_function(args);
            }
            else if (funcName == "replace")
            {
                return feel::evaluate_replace_function(args);
            }
            else if (funcName == "matches")
            {
                return feel::evaluate_matches_function(args, eval_ctx);
            }
            else if (funcName == "split")
            {
                return feel::evaluate_split_function(args);
            }
            else if (funcName == "string join")
            {
                return feel::evaluate_string_join_function(args);
            }
            else if (funcName == "date")
            {
                return feel::evaluate_date_function(args);
            }
            else if (funcName == "duration")
            {
                return feel::evaluate_duration_function(args);
            }
            // Phase 1: Trivial functions
            else if (funcName == "odd")
            {
                return feel::evaluate_odd_function(args);
            }
            else if (funcName == "even")
            {
                return feel::evaluate_even_function(args);
            }
            else if (funcName == "number")
            {
                return feel::evaluate_number_function(args);
            }
            else if (funcName == "string")
            {
                return feel::evaluate_string_function(args);
            }
            else if (funcName == "is")
            {
                return feel::evaluate_is_function(args);
            }
            // Phase 2A: Aggregation functions
            else if (funcName == "count")
            {
                return feel::evaluate_count_function(args);
            }
            else if (funcName == "sum")
            {
                return feel::evaluate_sum_function(args);
            }
            else if (funcName == "min")
            {
                return feel::evaluate_min_function(args);
            }
            else if (funcName == "max")
            {
                return feel::evaluate_max_function(args);
            }
            else if (funcName == "mean")
            {
                return feel::evaluate_mean_function(args);
            }
            else if (funcName == "product")
            {
                return feel::evaluate_product_function(args);
            }
            else if (funcName == "median")
            {
                return feel::evaluate_median_function(args);
            }
            else if (funcName == "stddev")
            {
                return feel::evaluate_stddev_function(args);
            }
            else if (funcName == "mode")
            {
                return feel::evaluate_mode_function(args);
            }
            // Phase 2B: List manipulation functions
            else if (funcName == "list contains")
            {
                return feel::evaluate_list_contains_function(args);
            }
            else if (funcName == "append")
            {
                return feel::evaluate_append_function(args);
            }
            else if (funcName == "concatenate")
            {
                return feel::evaluate_concatenate_function(args);
            }
            else if (funcName == "insert before")
            {
                return feel::evaluate_insert_before_function(args);
            }
            else if (funcName == "remove")
            {
                return feel::evaluate_remove_function(args);
            }
            else if (funcName == "reverse")
            {
                return feel::evaluate_reverse_function(args);
            }
            else if (funcName == "index of")
            {
                return feel::evaluate_index_of_function(args);
            }
            else if (funcName == "sublist")
            {
                return feel::evaluate_sublist_function(args);
            }
            else if (funcName == "union")
            {
                return feel::evaluate_union_function(args);
            }
            else if (funcName == "distinct values")
            {
                return feel::evaluate_distinct_values_function(args);
            }
            else if (funcName == "flatten")
            {
                return feel::evaluate_flatten_function(args);
            }
            else if (funcName == "sort")
            {
                return feel::evaluate_sort_function(args);
            }
            else if (funcName == "list replace")
            {
                return feel::evaluate_list_replace_function(args);
            }
            // ========== PHASE 3: CONTEXT FUNCTIONS ==========
            else if (funcName == "get value")
            {
                return feel::evaluate_get_value_function(args);
            }
            else if (funcName == "get entries")
            {
                return feel::evaluate_get_entries_function(args);
            }
            else if (funcName == "context")
            {
                return feel::evaluate_context_function(args);
            }
            else if (funcName == "context put")
            {
                return feel::evaluate_context_put_function(args);
            }
            else if (funcName == "context merge")
            {
                return feel::evaluate_context_merge_function(args);
            }
            else
            {
                std::ostringstream oss;
                oss << "Unknown function: " << funcName;
                throw std::runtime_error(oss.str());
            }
        }           default:
            {
                std::ostringstream oss;
                oss << "Unknown AST node type: " << static_cast<int>(type);
                throw std::runtime_error(oss.str());
            }
        }
    }
} // namespace orion::bre
