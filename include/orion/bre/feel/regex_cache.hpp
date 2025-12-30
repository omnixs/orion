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

#pragma once

#include <string>
#include <string_view>
#include <memory>
#include <optional>
#include <map>
#include <list>
#include <shared_mutex>

// Forward declarations to avoid exposing PCRE2 in public headers
struct pcre2_real_code_8;
struct pcre2_real_match_data_8;

namespace orion::bre::feel {

    /**
     * @brief Internal wrapper for compiled PCRE2 regex pattern
     * 
     * Encapsulates PCRE2 compiled code and match data to avoid exposing
     * PCRE2 types in public API. Handles resource cleanup via RAII.
     */
    class CompiledRegex {
    public:
        /**
         * @brief Compile a PCRE2 regex pattern with optional flags
         * @param pattern The regex pattern string (PCRE2 syntax)
         * @param flags Optional PCRE2 flags string (i=case-insensitive, m=multiline, s=dotall, x=extended)
         * @return CompiledRegex if successful, nullopt if pattern or flags are invalid
         */
        [[nodiscard]] static std::optional<CompiledRegex> compile(std::string_view pattern, std::string_view flags = "");

        /**
         * @brief Test if input string matches the compiled pattern (full-string match)
         * @param input The string to match against
         * @return true if input matches the entire pattern, false otherwise
         */
        [[nodiscard]] bool matches(std::string_view input) const;

        // Move-only type (owns PCRE2 resources)
        CompiledRegex(CompiledRegex&&) noexcept;
        CompiledRegex& operator=(CompiledRegex&&) noexcept;
        CompiledRegex(const CompiledRegex&) = delete;
        CompiledRegex& operator=(const CompiledRegex&) = delete;
        ~CompiledRegex();

    private:
        CompiledRegex(pcre2_real_code_8* code);
        
        pcre2_real_code_8* code_ = nullptr;
        mutable pcre2_real_match_data_8* match_data_ = nullptr;
    };

    /**
     * @brief LRU cache for compiled PCRE2 regex patterns
     * 
     * Provides bounded caching to avoid repeated pattern compilation overhead
     * for dynamic FEEL matches() patterns.
     */
    class RegexCache {
    public:
        /**
         * @brief Construct cache with configurable maximum size
         * @param max_size Maximum number of cached patterns (default: 100)
         */
        explicit RegexCache(size_t max_size = 100);

        /**
         * @brief Get or compile a regex pattern with optional flags
         * @param pattern The regex pattern string
         * @param flags Optional PCRE2 flags string (i=case-insensitive, m=multiline, s=dotall, x=extended)
         * @return Compiled regex if valid, nullopt if pattern or flags are invalid
         */
        [[nodiscard]] std::optional<CompiledRegex> get_or_compile(std::string_view pattern, std::string_view flags = "");

        /**
         * @brief Clear all cached patterns
         */
        void clear();

        /**
         * @brief Get current cache size
         */
        [[nodiscard]] size_t size() const;

        /**
         * @brief Get maximum cache size
         */
        [[nodiscard]] size_t max_size() const { return max_size_; }

    private:
        void evict_lru();

        size_t max_size_;
        mutable std::shared_mutex mutex_;
        
        // LRU implementation: list maintains access order, map provides O(1) lookup
        std::list<std::string> access_order_;
        std::map<std::string, std::pair<CompiledRegex, std::list<std::string>::iterator>> cache_;
    };

    /**
     * @brief Get the global regex cache instance
     * 
     * Thread-safe singleton for caching dynamic FEEL matches() patterns.
     * Cache size can be configured via engine options.
     */
    RegexCache& get_regex_cache();

    /**
     * @brief Warm up the regex cache with realistic patterns
     * 
     * Compiles and tests several representative regex patterns to ensure
     * PCRE2 initialization and reduce first-use latency variability.
     * Exercises common features: character classes, quantifiers, anchors,
     * alternation, and escape sequences.
     * 
     * This function is automatically called once on first BusinessRulesEngine
     * construction (via std::call_once). Manual calls are optional but safe.
     * 
     * Performance impact: ~1-2 microseconds one-time cost at engine startup.
     * Benefit: Eliminates cold-start variability for first matches() call.
     * 
     * @note This function is idempotent and safe to call multiple times.
     * @note This function is thread-safe.
     * @note Automatic warmup occurs via BusinessRulesEngine constructor.
     */
    void warmup_regex_cache();

} // namespace orion::bre::feel
