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
#pragma once
#include <orion/bre/feel/types.hpp>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace orion::bre::feel {
    bool unary_test_matches(std::string_view test, std::string_view candidate);

    /**
     * @brief Candidate value of a unary test, classified once so it can be matched against many tests.
     *
     * Holds the same interpretations unary_test_matches() derives from the candidate text on every call.
     */
    struct UnaryCandidate
    {
        std::string raw;   ///< Candidate as passed to unary_test_matches()
        std::string text;  ///< Trimmed candidate
        std::optional<double> number;
        std::optional<bool> boolean;
        std::optional<Date> date;
        std::optional<Time> time;
        std::optional<DateTime> datetime;
        std::optional<Duration> duration;

        explicit UnaryCandidate(std::string candidate);
    };

    /**
     * @brief Comma-separated unary test list (e.g. `"AAA","BBB",>5`) pre-parsed at model load.
     *
     * Matches exactly like unary_test_matches(), but splits the list and classifies literal
     * items once instead of on every evaluation. Immutable after construction; safe to share
     * between threads.
     */
    class CompiledUnaryTests
    {
    public:
        /// Returns std::nullopt when @p test is not a comma-separated list of unary tests.
        [[nodiscard]] static std::optional<CompiledUnaryTests> compile(std::string_view test);

        [[nodiscard]] bool matches(const UnaryCandidate& candidate) const;

    private:
        struct Literal
        {
            std::string text;  ///< Trimmed and unquoted
            std::optional<double> number;
            std::optional<bool> boolean;
            std::optional<Date> date;
            std::optional<Time> time;
            std::optional<DateTime> datetime;
            std::optional<Duration> duration;
        };

        /// Literal items are pre-classified; any other item keeps its text for unary_test_matches().
        std::vector<std::variant<Literal, std::string>> items_;
    };
}
