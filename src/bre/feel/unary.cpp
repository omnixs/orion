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

#include <orion/bre/feel/unary.hpp>
#include <orion/bre/feel/types.hpp>
#include <orion/bre/feel/parser.hpp>
#include <orion/bre/feel/lexer.hpp>
#include <orion/bre/feel/regex_cache.hpp>
#include <orion/bre/evaluation_context.hpp>
#include <ctre.hpp>
#include <charconv>
#include <iostream>
#include "common/util.hpp" // added for orion::common::trim, split

namespace orion::bre::feel {
    static std::string unquote(std::string str)
    {
        if (str.size() >= 2 && ((str.front() == '"' && str.back() == '"') || (str.front() == '\'' && str.back() == '\'')))
        {
            return str.substr(1, str.size() - 2);
        }
        return str;
    }

    static bool parse_bool(std::string_view str, bool& value)
    {
        if (str == "true" || str == "True" || str == "TRUE")
        {
            value = true;
            return true;
        }
        if (str == "false" || str == "False" || str == "FALSE")
        {
            value = false;
            return true;
        }
        return false;
    }

    // Helper: Evaluate FEEL expressions (typically function calls)
    // E.g., "duration(\"PT31H\")" -> "PT31H"
    // Returns original expression if it's not a FEEL function call
    static std::string evaluate_feel_functions(std::string_view expression)
    {
        // Check if expression looks like a FEEL function call
        // FEEL functions have the pattern: function_name(args)
        // If no parenthesis, it's a plain literal - return as-is
        if (expression.find('(') == std::string_view::npos)
        {
            return std::string(expression);
        }
        
        feel::Lexer lexer;
        std::string expression_str(expression);  // Store expression to keep string_view tokens valid
        auto tokens = lexer.tokenize(expression_str);
        
        feel::Parser parser;
        auto ast = parser.parse(tokens);
        
        feel::RegexCache regex_cache;
        orion::bre::EvaluationContext eval_ctx(regex_cache);
        nlohmann::json result = ast->evaluate(nlohmann::json::object(), eval_ctx);
        
        // Convert result to string representation
        if (result.is_string()) {
            return result.get<std::string>();  // Avoid quotes around strings
        }
        // For all other types (int, float, bool, null, objects, arrays), dump() handles formatting correctly
        return result.dump();
    }

    static bool parse_number(std::string_view str, double& out)
    {
        auto str_view = std::string_view(str);
        double value{};
        auto [ptr, error_code] = std::from_chars(str_view.data(), str_view.data() + str_view.size(), value);
        if (error_code == std::errc() && ptr == str_view.data() + str_view.size())
        {
            out = value;
            return true;
        }
        try
        {
            out = std::stod(std::string(str));
            return true;
        }
        catch (const std::invalid_argument&) {
            return false; // Not a valid number
        }
        catch (const std::out_of_range&) {
            return false; // Number out of range
        }
    }

    // Helper: Three-way comparison for numeric types
    static int compare_numbers(double lhs, double rhs)
    {
        if (lhs < rhs) { return -1;
}
        if (lhs > rhs) { return 1;
}
        return 0;
    }

    // Main dispatcher for numeric and temporal values, with lexical fallback.
    static int cmp_values(std::string_view lhs, std::string_view rhs)
    {
        // Evaluate any FEEL function calls first (e.g., duration("PT31H"))
        std::string lhs_evaluated = evaluate_feel_functions(lhs);
        std::string rhs_evaluated = evaluate_feel_functions(rhs);
        
        // Unquote string literals for comparison
        std::string lhs_unquoted = unquote(lhs_evaluated);
        std::string rhs_unquoted = unquote(rhs_evaluated);
        
        // Try numeric comparison
        double num_lhs = 0.0;
        double num_rhs = 0.0;
        if (parse_number(lhs_unquoted, num_lhs) && parse_number(rhs_unquoted, num_rhs))
        {
            return compare_numbers(num_lhs, num_rhs);
        }
        
        if (auto result = compare_temporal_values(lhs_unquoted, rhs_unquoted))
        {
            return *result;
        }
        
        // Fallback: String comparison (use unquoted for consistent behavior)
        if (lhs_unquoted < rhs_unquoted) { return -1;
}
        if (lhs_unquoted > rhs_unquoted) { return 1;
}
        return 0;
    }

    static bool match_single_literal(std::string_view test, std::string_view cand)
    {
        std::string test_val = unquote(orion::common::trim(test));
        std::string cand_val = orion::common::trim(cand);
        double num_test = 0.0;
        double num_cand = 0.0;
        if (parse_number(test_val, num_test) && parse_number(cand_val, num_cand))
        {
            return num_test == num_cand;
        }
        bool bool_test = false;
        bool bool_cand = false;
        if (parse_bool(test_val, bool_test) && parse_bool(cand_val, bool_cand))
        {
            return bool_test == bool_cand;
        }
        if (auto date_test = parse_date(test_val))
        {
            if (auto date_cand = parse_date(cand_val))
            {
                return *date_test == *date_cand;
            }
            else
            {
                return false;
            }
        }
        if (auto time_test = parse_time(test_val))
        {
            if (auto time_cand = parse_time(cand_val))
            {
                return *time_test == *time_cand;
            }
            else
            {
                return false;
            }
        }
        if (auto datetime_test = parse_datetime(test_val))
        {
            if (auto datetime_cand = parse_datetime(cand_val))
            {
                return *datetime_test == *datetime_cand;
            }
            else
            {
                return false;
            }
        }
        if (auto duration_test = parse_duration(test_val))
        {
            if (auto duration_cand = parse_duration(cand_val))
            {
                return *duration_test == *duration_cand;
            }
            else
            {
                return false;
            }
        }
        return test_val == cand_val;
    }

    // Helper: Handle not() function - returns true if NONE of the inner tests match
    static bool match_not_function(std::string_view test, std::string_view candidate)
    {
        if (!test.ends_with(")"))
        {
            return false;
        }
        auto inner = test.substr(4, test.size() - 5);
        for (auto& part : orion::common::split(inner, ','))
        {
            if (unary_test_matches(orion::common::trim(part), candidate))
            {
                return false;
            }
        }
        return true;
    }

    // Helper: Handle comma-separated list - returns true if ANY test matches
    static bool match_list(std::string_view test, std::string_view candidate)
    {
        for (auto& part : orion::common::split(test, ','))
        {
            if (unary_test_matches(orion::common::trim(part), candidate))
            {
                return true;
            }
        }
        return false;
    }

    // Helper: Handle comparison operators (<, <=, >, >=, ==)
    static bool match_comparison(std::string_view test, std::string_view candidate)
    {
        // CTRE compile-time regex for comparison pattern
        if (auto match = ctre::match<R"(^\s*([<>]=?|==)\s*(.+)\s*$)">(test))
        {
            std::string oper = match.get<1>().to_string();
            std::string rhs = orion::common::trim(match.get<2>().to_string());
            
            // FEEL evaluation happens inside cmp_values()
            int cmp_result = cmp_values(candidate, rhs);

            if (oper == "<") { return cmp_result < 0;
}
            if (oper == "<=") { return cmp_result <= 0;
}
            if (oper == ">") { return cmp_result > 0;
}
            if (oper == ">=") { return cmp_result >= 0;
}
            if (oper == "==") { return cmp_result == 0;
}
        }
        return false;
    }

    // Helper: Handle range test [a..b], (a..b), [a..b), (a..b]
    static bool match_range(std::string_view test, std::string_view candidate)
    {
        // CTRE compile-time regex for range pattern
        if (auto match = ctre::match<R"(^\s*([\[(])\s*(.+)\s*\.\.\s*(.+)\s*([\])])\s*$)">(test))
        {
            bool inc_l = match.get<1>().to_view() == "[";
            bool inc_r = match.get<4>().to_view() == "]";
            std::string lower = orion::common::trim(match.get<2>().to_string());
            std::string upper = orion::common::trim(match.get<3>().to_string());
            
            int cmp_lower = cmp_values(candidate, lower);
            int cmp_upper = cmp_values(candidate, upper);
            
            bool left_ok = inc_l ? (cmp_lower >= 0) : (cmp_lower > 0);
            bool right_ok = inc_r ? (cmp_upper <= 0) : (cmp_upper < 0);
            return left_ok && right_ok;
        }
        return false;
    }

    // Main dispatcher: Try each test type in order
    bool unary_test_matches(std::string_view test_raw, std::string_view candidate)
    {
        std::string test = orion::common::trim(test_raw);
        
        // Special case: "-" or empty means always match
        if (test == "-" || test.empty())
        {
            return true;
        }

        // Try not() function
        if (test.starts_with("not("))
        {
            return match_not_function(test, candidate);
        }

        // Try comma-separated list
        if (test.find(',') != std::string::npos)
        {
            return match_list(test, candidate);
        }

        // Try comparison operators
        if (match_comparison(test, candidate))
        {
            return true;
        }

        // Try range test
        if (match_range(test, candidate))
        {
            return true;
        }

        // Fallback: literal match
        return match_single_literal(test, candidate);
    }
}
