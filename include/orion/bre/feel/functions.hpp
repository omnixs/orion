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

/**
 * @file feel_functions.hpp
 * @brief FEEL built-in function implementations
 * 
 * This file provides implementations of DMN FEEL built-in functions according to
 * the DMN 1.5 specification (Chapter 10.3.4).
 * 
 * All functions follow DMN null propagation semantics:
 * - If required arguments are null, the function returns null
 * - Functions validate argument types and counts
 * - Errors are reported via exceptions
 */

#pragma once

#include <vector>
#include <nlohmann/json.hpp>

namespace orion::bre::feel {
    using json = nlohmann::json;

    /**
     * @brief Boolean negation function
     * @param args Vector containing exactly 1 boolean argument
     * @return Negated boolean value, or null if argument is null
     * @throws std::runtime_error if argument count is wrong or argument is not boolean
     * 
     * DMN 1.5 Section 10.3.2.15: Logical negation with null propagation
     * - not(true) → false
     * - not(false) → true
     * - not(null) → null
     */
    [[nodiscard]] json evaluate_not_function(const std::vector<json>& args);

    /**
     * @brief Check if all elements in list are true
     * @param args Vector containing exactly 1 array of booleans
     * @return true if all elements are true, false otherwise, null if input is null
     * @throws std::runtime_error if argument count is wrong or elements are not boolean
     * 
     * DMN 1.5 Section 10.3.2.9: Universal quantification
     * - all([true, true]) → true
     * - all([true, false]) → false
     * - all([]) → true (vacuous truth)
     * - all(null) → null
     */
    [[nodiscard]] json evaluate_all_function(const std::vector<json>& args);

    /**
     * @brief Check if any element in list is true
     * @param args Vector containing exactly 1 array of booleans
     * @return true if any element is true, false otherwise, null if input is null
     * @throws std::runtime_error if argument count is wrong or elements are not boolean
     * 
     * DMN 1.5 Section 10.3.2.9: Existential quantification
     * - any([false, true]) → true
     * - any([false, false]) → false
     * - any([]) → false
     * - any(null) → null
     */
    [[nodiscard]] json evaluate_any_function(const std::vector<json>& args);

    /**
     * @brief Check if string contains substring
     * @param args Vector containing exactly 2 string arguments [string, substring]
     * @return true if string contains substring, false otherwise, null if either argument is null
     * @throws std::runtime_error if argument count is wrong or arguments are not strings
     * 
     * DMN 1.5 Section 10.3.4.3: String functions
     * - contains("hello world", "world") → true
     * - contains("hello", "goodbye") → false
     * - contains(null, "test") → null
     * - contains("test", null) → null
     */
    [[nodiscard]] json evaluate_contains_function(const std::vector<json>& args);

    // ========== MATH FUNCTIONS (DMN 1.5 Section 10.3.4.1) ==========

    /**
     * @brief Absolute value function
     * @param args Vector containing exactly 1 numeric argument
     * @return Absolute value of the number, or null if argument is null
     * @throws std::runtime_error if argument count is wrong or argument is not numeric
     * 
     * DMN 1.5 Section 10.3.4.1: Number functions
     * - abs(10) → 10
     * - abs(-10) → 10
     * - abs(0) → 0
     * - abs(null) → null
     */
    [[nodiscard]] json evaluate_abs_function(const std::vector<json>& args);

    /**
     * @brief Square root function
     * @param args Vector containing exactly 1 numeric argument
     * @return Square root of the number, or null if argument is null or negative
     * @throws std::runtime_error if argument count is wrong or argument is not numeric
     * 
     * DMN 1.5 Section 10.3.4.1: Number functions
     * - sqrt(16) → 4
     * - sqrt(0) → 0
     * - sqrt(-1) → null (undefined)
     * - sqrt(null) → null
     */
    [[nodiscard]] json evaluate_sqrt_function(const std::vector<json>& args);

    /**
     * @brief Floor function - round down to nearest integer
     * @param args Vector containing exactly 1 numeric argument
     * @return Number rounded down to nearest integer, or null if argument is null
     * @throws std::runtime_error if argument count is wrong or argument is not numeric
     * 
     * DMN 1.5 Section 10.3.4.1: Number functions
     * - floor(1.5) → 1
     * - floor(-1.5) → -2
     * - floor(5) → 5
     * - floor(null) → null
     */
    [[nodiscard]] json evaluate_floor_function(const std::vector<json>& args);

    /**
     * @brief Ceiling function - round up to nearest integer
     * @param args Vector containing exactly 1 numeric argument
     * @return Number rounded up to nearest integer, or null if argument is null
     * @throws std::runtime_error if argument count is wrong or argument is not numeric
     * 
     * DMN 1.5 Section 10.3.4.1: Number functions
     * - ceiling(1.5) → 2
     * - ceiling(-1.5) → -1
     * - ceiling(5) → 5
     * - ceiling(null) → null
     */
    [[nodiscard]] json evaluate_ceiling_function(const std::vector<json>& args);

    /**
     * @brief Exponential function
     * @param args Vector containing exactly 1 numeric argument
     * @return e raised to the power of the argument, or null if argument is null
     * @throws std::runtime_error if argument count is wrong or argument is not numeric
     * 
     * DMN 1.5 Section 10.3.4.1: Number functions
     * - exp(0) → 1
     * - exp(1) → 2.718281828...
     * - exp(null) → null
     */
    [[nodiscard]] json evaluate_exp_function(const std::vector<json>& args);

    /**
     * @brief Natural logarithm function
     * @param args Vector containing exactly 1 numeric argument
     * @return Natural logarithm of the argument, or null if argument is null or <= 0
     * @throws std::runtime_error if argument count is wrong or argument is not numeric
     * 
     * DMN 1.5 Section 10.3.4.1: Number functions
     * - log(1) → 0
     * - log(2.718281828) → 1
     * - log(0) → null
     * - log(-1) → null
     * - log(null) → null
     */
    [[nodiscard]] json evaluate_log_function(const std::vector<json>& args);

    /**
     * @brief Modulo function - remainder after division
     * @param args Vector containing exactly 2 numeric arguments [dividend, divisor]
     * @return Remainder of dividend / divisor, or null if either argument is null
     * @throws std::runtime_error if argument count is wrong or arguments are not numeric
     * 
     * DMN 1.5 Section 10.3.4.1: Number functions
     * - modulo(12, 5) → 2
     * - modulo(-12, 5) → 3
     * - modulo(10.1, 4.5) → 1.1
     * - modulo(10, null) → null
     */
    [[nodiscard]] json evaluate_modulo_function(const std::vector<json>& args);

    /**
     * @brief Decimal function - convert to decimal with scale
     * @param args Vector containing 2 numeric arguments [n, scale]
     * @return Number rounded to scale decimal places using half-even rounding
     * @throws std::runtime_error if argument count is wrong or arguments are not numeric
     * 
     * DMN 1.5 Section 10.3.4.1: Number functions
     * - decimal(1/3, 2) → 0.33
     * - decimal(1.5, 0) → 2
     * - decimal(2.5, 0) → 2 (half-even)
     */
    [[nodiscard]] json evaluate_decimal_function(const std::vector<json>& args);

    /**
     * @brief Round function - round to scale using half-even rounding (banker's rounding)
     * @param args Vector containing 2 numeric arguments [n, scale]
     * @return Number rounded to scale decimal places
     * @throws std::runtime_error if argument count is wrong or arguments are not numeric
     * 
     * DMN 1.5 Section 10.3.4.1: Number functions
     * - round(5.5, 0) → 6 (round to even)
     * - round(2.5, 0) → 2 (round to even)
     * - round(1.121, 2) → 1.12
     */
    [[nodiscard]] json evaluate_round_function(const std::vector<json>& args);

    /**
     * @brief Round up function - always round away from zero
     * @param args Vector containing 2 numeric arguments [n, scale]
     * @return Number rounded away from zero to scale decimal places
     * @throws std::runtime_error if argument count is wrong or arguments are not numeric
     * 
     * DMN 1.5 Section 10.3.4.1: Number functions
     * - round up(5.5, 0) → 6
     * - round up(-5.5, 0) → -6
     * - round up(1.121, 2) → 1.13
     */
    [[nodiscard]] json evaluate_round_up_function(const std::vector<json>& args);

    /**
     * @brief Round down function - always round toward zero
     * @param args Vector containing 2 numeric arguments [n, scale]
     * @return Number rounded toward zero to scale decimal places
     * @throws std::runtime_error if argument count is wrong or arguments are not numeric
     * 
     * DMN 1.5 Section 10.3.4.1: Number functions
     * - round down(5.5, 0) → 5
     * - round down(-5.5, 0) → -5
     * - round down(1.129, 2) → 1.12
     */
    [[nodiscard]] json evaluate_round_down_function(const std::vector<json>& args);

    /**
     * @brief Round half up function - standard rounding (0.5 rounds up)
     * @param args Vector containing 2 numeric arguments [n, scale]
     * @return Number rounded using standard half-up rounding
     * @throws std::runtime_error if argument count is wrong or arguments are not numeric
     * 
     * DMN 1.5 Section 10.3.4.1: Number functions
     * - round half up(5.5, 0) → 6
     * - round half up(-5.5, 0) → -6
     * - round half up(1.125, 2) → 1.13
     */
    [[nodiscard]] json evaluate_round_half_up_function(const std::vector<json>& args);

    /**
     * @brief Round half down function - 0.5 rounds down
     * @param args Vector containing 2 numeric arguments [n, scale]
     * @return Number rounded using half-down rounding
     * @throws std::runtime_error if argument count is wrong or arguments are not numeric
     * 
     * DMN 1.5 Section 10.3.4.1: Number functions
     * - round half down(5.5, 0) → 5
     * - round half down(-5.5, 0) → -5
     * - round half down(1.125, 2) → 1.12
     */
    [[nodiscard]] json evaluate_round_half_down_function(const std::vector<json>& args);

    // ========== STRING FUNCTIONS ==========

    /**
     * @brief Get substring before first occurrence of match
     * @param args Vector containing 2 string arguments [string, match]
     * @return Substring before match, or empty string if match not found
     * 
     * DMN 1.5 Section 10.3.4.3: String functions
     * - substring before("foobar", "bar") → "foo"
     * - substring before("foobar", "xyz") → ""
     */
    [[nodiscard]] json evaluate_substring_before_function(const std::vector<json>& args);

    /**
     * @brief Get substring after first occurrence of match
     * @param args Vector containing 2 string arguments [string, match]
     * @return Substring after match, or empty string if match not found
     * 
     * DMN 1.5 Section 10.3.4.3: String functions
     * - substring after("foobar", "foo") → "bar"
     * - substring after("foobar", "xyz") → ""
     */
    [[nodiscard]] json evaluate_substring_after_function(const std::vector<json>& args);

    /**
     * @brief Extract substring from string
     * @param args Vector containing 2-3 arguments [string, start position, length?]
     * @return Substring starting at position (1-based) with optional length
     * 
     * DMN 1.5 Section 10.3.4.3: String functions
     * - substring("foobar", 3) → "obar"
     * - substring("foobar", 3, 3) → "oba"
     * - substring("foobar", -2, 1) → "a"
     */
    [[nodiscard]] json evaluate_substring_function(const std::vector<json>& args);

    /**
     * @brief Get length of string
     * @param args Vector containing 1 string argument
     * @return Length of string as integer
     * 
     * DMN 1.5 Section 10.3.4.3: String functions
     * - string length("foo") → 3
     */
    [[nodiscard]] json evaluate_string_length_function(const std::vector<json>& args);

    /**
     * @brief Convert string to uppercase
     * @param args Vector containing 1 string argument
     * @return Uppercase version of string
     * 
     * DMN 1.5 Section 10.3.4.3: String functions
     * - upper case("aBc4") → "ABC4"
     */
    [[nodiscard]] json evaluate_upper_case_function(const std::vector<json>& args);

    /**
     * @brief Convert string to lowercase
     * @param args Vector containing 1 string argument
     * @return Lowercase version of string
     * 
     * DMN 1.5 Section 10.3.4.3: String functions
     * - lower case("aBc4") → "abc4"
     */
    [[nodiscard]] json evaluate_lower_case_function(const std::vector<json>& args);

    /**
     * @brief Check if string starts with prefix
     * @param args Vector containing 2 string arguments [string, prefix]
     * @return true if string starts with prefix, false otherwise
     * 
     * DMN 1.5 Section 10.3.4.3: String functions
     * - starts with("foobar", "foo") → true
     * - starts with("foobar", "bar") → false
     */
    [[nodiscard]] json evaluate_starts_with_function(const std::vector<json>& args);

    /**
     * @brief Check if string ends with suffix
     * @param args Vector containing 2 string arguments [string, suffix]
     * @return true if string ends with suffix, false otherwise
     * 
     * DMN 1.5 Section 10.3.4.3: String functions
     * - ends with("foobar", "bar") → true
     * - ends with("foobar", "foo") → false
     */
    [[nodiscard]] json evaluate_ends_with_function(const std::vector<json>& args);

    /**
     * @brief Replace all occurrences of pattern in string
     * @param args Vector containing 3-4 arguments [input, pattern, replacement, flags?]
     * @return String with replacements applied
     * 
     * DMN 1.5 Section 10.3.4.3: String functions
     * - replace("banana", "a", "o") → "bonono"
     * - replace("abcd", "x", "y") → "abcd"
     */
    [[nodiscard]] json evaluate_replace_function(const std::vector<json>& args);

    /**
     * @brief Test if string matches pattern
     * @param args Vector containing 2-3 arguments [input, pattern, flags?]
     * @return true if input matches pattern, false otherwise
     * 
     * DMN 1.5 Section 10.3.4.3: String functions
     * - matches("foobar", "foo") → true
     * - matches("foobar", "baz") → false
     */
    [[nodiscard]] json evaluate_matches_function(const std::vector<json>& args);

    /**
     * @brief Split string into list by delimiter
     * @param args Vector containing 2 arguments [string, delimiter]
     * @return Array of strings
     * 
     * DMN 1.5 Section 10.3.4.3: String functions
     * - split("a;b;c", ";") → ["a", "b", "c"]
     * - split("abc", "") → ["a", "b", "c"]
     */
    [[nodiscard]] json evaluate_split_function(const std::vector<json>& args);

    /**
     * @brief Join list into string with optional delimiter
     * @param args Vector containing 1-2 arguments [list, delimiter?]
     * @return Joined string
     * 
     * DMN 1.5 Section 10.3.4.3: String functions
     * - string join(["a", "b", "c"]) → "abc"
     * - string join(["a", "b", "c"], ", ") → "a, b, c"
     */
    [[nodiscard]] json evaluate_string_join_function(const std::vector<json>& args);

    /**
     * @brief Parse date from ISO 8601 string or construct from components
     * @param args Vector containing either 1 string or 3 numbers [year, month, day]
     * @return Date object as ISO string, or null on error
     * 
     * DMN 1.5 Section 10.3.4.1: Date/time functions
     * - date("2017-01-01") → date object for January 1, 2017
     * - date(2017, 1, 1) → date object for January 1, 2017
     * - date(null) → null
     * 
     * Note: This is a simplified implementation. Returns ISO string representation.
     * Full date type system to be implemented later.
     */
    [[nodiscard]] json evaluate_date_function(const std::vector<json>& args);

} // namespace orion::bre

