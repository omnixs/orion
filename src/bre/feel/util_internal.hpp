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

/**
 * @file feel_util.hpp
 * @brief Internal utility functions for FEEL expression evaluation
 * 
 * Contains shared arithmetic parsing and evaluation functions used by
 * both feel_expr.cpp and bkm_manager.cpp for complex mathematical expressions.
 * 
 * @internal This is an internal implementation detail and should not be
 * included by external code.
 */

#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace orion::bre::detail
{
    // Thread-local context pointer for variable resolution in math expressions
    inline thread_local const nlohmann::json* current_eval_context = nullptr;

    /**
     * @brief Split function arguments respecting parentheses nesting
     * @param args_str The argument string (e.g., "a, b, c(x, y), d")
     * @return Vector of individual arguments
     */
    std::vector<std::string> split_arguments(std::string_view args_str);

    /**
     * @brief Resolve argument value from context or parse as literal
     * @param arg The argument string (may be dotted path like "Loan.amount")
     * @param context JSON context for variable resolution
     * @return Resolved JSON value
     */
    nlohmann::json resolve_argument(std::string_view arg, const nlohmann::json& context);

    /**
     * @brief Resolve property path to numeric value
     * @param path Property path like "loan.principal" or "Loan.amount"
     * @param context JSON context
     * @return Numeric value or NaN if not found
     */
    double resolve_property_path(std::string_view path, const nlohmann::json& context);

    /**
     * @brief Evaluate complex arithmetic expression with property substitution
     * @param expression Expression like "(loan.principal*loan.rate/MONTHS_PER_YEAR)/(1-(1+loan.rate/MONTHS_PER_YEAR)**-loan.termMonths)"
     * @param context JSON context for property resolution
     * @return Result as JSON or null on error
     */
    nlohmann::json evaluate_complex_arithmetic_expression(std::string_view expression, const nlohmann::json& context);

    /**
     * @brief Evaluate mathematical expression (numbers only, no properties)
     * @param expr Pure mathematical expression like "(P*r/MONTHS_PER_YEAR)/(1-(1+r/MONTHS_PER_YEAR)**-n)" where P, r, n are numeric values
     * @return Result as JSON or null on error
     */
    nlohmann::json eval_math_expression(std::string_view expr);

    /**
     * @brief Parse arithmetic expression (recursive descent parser)
     * @param expr Expression string
     * @param pos Current parsing position (modified during parsing)
     * @return Parsed numeric value
     */
    double parse_expression(std::string_view expr, size_t& pos);

    /**
     * @brief Parse arithmetic term (* / **)
     * @param expr Expression string
     * @param pos Current parsing position (modified during parsing)
     * @return Parsed numeric value
     */
    double parse_term(std::string_view expr, size_t& pos);

    /**
     * @brief Parse arithmetic power (**) 
     * @param expr Expression string
     * @param pos Current parsing position (modified during parsing)
     * @return Parsed numeric value
     */
    double parse_power(std::string_view expr, size_t& pos);

    /**
     * @brief Parse arithmetic factor (numbers, parentheses, unary operators)
     * @param expr Expression string
     * @param pos Current parsing position (modified during parsing)
     * @return Parsed numeric value
     */
    double parse_factor(std::string_view expr, size_t& pos);

    // Helper functions for parse_factor to reduce cognitive complexity
    double parse_parenthesized_expression_impl(std::string_view expr, size_t& pos);
    double parse_number_literal_impl(std::string_view expr, size_t& pos);
    double parse_identifier_or_variable(std::string_view expr, size_t& pos);
    std::string extract_variable_name(std::string_view expr, size_t start_pos, size_t& pos);
    std::string try_extend_variable_name(std::string_view expr, std::string_view var_name, size_t start_pos, size_t& pos);
    double resolve_feel_constant(std::string_view var_name);
    double resolve_variable_from_context(std::string_view var_name, bool& found);

    /**
     * @brief Skip whitespace characters
     * @param expr Expression string
     * @param pos Current position (modified to skip whitespace)
     */
    void skip_whitespace(std::string_view expr, size_t& pos);
} // namespace orion::bre::detail
