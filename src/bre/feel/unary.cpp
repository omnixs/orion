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
#include <regex>
#include <charconv>
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

    // Helper: Three-way comparison for dates
    static std::optional<int> try_compare_dates(std::string_view lhs, std::string_view rhs)
    {
        auto date_lhs = parse_date(lhs);
        if (!date_lhs) { return std::nullopt;
}
        
        auto date_rhs = parse_date(rhs);
        if (!date_rhs) { return std::nullopt;
}
        
        if (*date_lhs < *date_rhs) { return -1;
}
        if (*date_lhs > *date_rhs) { return 1;
}
        return 0;
    }

    // Helper: Three-way comparison for times
    static std::optional<int> try_compare_times(std::string_view lhs, std::string_view rhs)
    {
        auto time_lhs = parse_time(lhs);
        if (!time_lhs) { return std::nullopt;
}
        
        auto time_rhs = parse_time(rhs);
        if (!time_rhs) { return std::nullopt;
}
        
        if (*time_lhs < *time_rhs) { return -1;
}
        if (*time_lhs > *time_rhs) { return 1;
}
        return 0;
    }

    // Helper: Three-way comparison for datetimes
    static std::optional<int> try_compare_datetimes(std::string_view lhs, std::string_view rhs)
    {
        auto datetime_lhs = parse_datetime(lhs);
        if (!datetime_lhs) { return std::nullopt;
}
        
        auto datetime_rhs = parse_datetime(rhs);
        if (!datetime_rhs) { return std::nullopt;
}
        
        if (*datetime_lhs < *datetime_rhs) { return -1;
}
        if (*datetime_lhs > *datetime_rhs) { return 1;
}
        return 0;
    }

    // Helper: Three-way comparison for durations
    static std::optional<int> try_compare_durations(std::string_view lhs, std::string_view rhs)
    {
        auto duration_lhs = parse_duration(lhs);
        if (!duration_lhs) { return std::nullopt;
}
        
        auto duration_rhs = parse_duration(rhs);
        if (!duration_rhs) { return std::nullopt;
}
        
        if (duration_lhs->total_months != duration_rhs->total_months)
        {
            return (duration_lhs->total_months < duration_rhs->total_months) ? -1 : 1;
        }
        if (duration_lhs->total_seconds != duration_rhs->total_seconds)
        {
            return (duration_lhs->total_seconds < duration_rhs->total_seconds) ? -1 : 1;
        }
        return 0;
    }

    // Main dispatcher: Try each type comparison in order
    static int cmp_values(std::string_view lhs, std::string_view rhs)
    {
        // Try numeric comparison
        double num_lhs = 0.0;
        double num_rhs = 0.0;
        if (parse_number(lhs, num_lhs) && parse_number(rhs, num_rhs))
        {
            return compare_numbers(num_lhs, num_rhs);
        }
        
        // Try date comparison
        if (auto result = try_compare_dates(lhs, rhs))
        {
            return *result;
        }
        
        // Try time comparison
        if (auto result = try_compare_times(lhs, rhs))
        {
            return *result;
        }
        
        // Try datetime comparison
        if (auto result = try_compare_datetimes(lhs, rhs))
        {
            return *result;
        }
        
        // Try duration comparison
        if (auto result = try_compare_durations(lhs, rhs))
        {
            return *result;
        }
        
        // Fallback: String comparison
        if (lhs < rhs) { return -1;
}
        if (lhs > rhs) { return 1;
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
        static const std::regex cmp_re(R"(^\s*([<>]=?|==)\s*(.+)\s*$)");
        std::smatch match;
        std::string test_str(test);  // regex requires std::string
        if (!std::regex_match(test_str, match, cmp_re))
        {
            return false;
        }

        std::string oper = match[1];
        std::string rhs = orion::common::trim(std::string(match[2]));
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
        return false;
    }

    // Helper: Handle range test [a..b], (a..b), [a..b), (a..b]
    static bool match_range(std::string_view test, std::string_view candidate)
    {
        static const std::regex range_re(R"(^\s*([\[(])\s*(.+)\s*\.\.\s*(.+)\s*([\])])\s*$)");
        std::smatch match;
        std::string test_str(test);
        if (!std::regex_match(test_str, match, range_re))
        {
            return false;
        }

        bool inc_l = match[1] == "[";
        bool inc_r = match[4] == "]";
        std::string lower = orion::common::trim(std::string(match[2]));
        std::string upper = orion::common::trim(std::string(match[3]));
        
        int cmp_lower = cmp_values(candidate, lower);
        int cmp_upper = cmp_values(candidate, upper);
        
        bool left_ok = inc_l ? (cmp_lower >= 0) : (cmp_lower > 0);
        bool right_ok = inc_r ? (cmp_upper <= 0) : (cmp_upper < 0);
        return left_ok && right_ok;
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
