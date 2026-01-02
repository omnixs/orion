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
     * 
     * @note Architectural Decision: PCRE2 vs CTRE for Dynamic Patterns
     * 
     * ORION uses two regex libraries with distinct purposes:
     * - **CTRE**: Compile-time patterns (zero runtime overhead)
     *   - Used for: Fixed patterns in parsers (unary.cpp, types.cpp, bkm_manager.cpp)
     *   - Benefit: Pattern validation at compile time, zero runtime cost
     * 
     * - **PCRE2**: Runtime/dynamic patterns (this file)
     *   - Used for: FEEL matches() function where patterns come from user data
     *   - Benefits over CTRE dynamic mode:
     *     1. **JIT Compilation**: ~20% faster execution via native machine code
     *     2. **Caching**: LRU cache avoids recompilation of repeated patterns
     *     3. **Resource Management**: Better RAII integration for compiled code
     *     4. **Feature Completeness**: Full Perl-compatible syntax, robust Unicode support
     * 
     * While CTRE supports dynamic patterns, it lacks JIT optimization and is
     * essentially interpreted at runtime. PCRE2's JIT compiler provides significant
     * performance advantages for the FEEL matches() use case where patterns are
     * user-provided at runtime and may be reused across multiple evaluations.
     * 
     * See: .github/tasks/17_replace_std_regex.md for complete rationale
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

        /**
         * @brief Warm up cache with common regex patterns
         * 
         * Compiles representative patterns to initialize PCRE2 and reduce
         * first-use latency variability. Safe to call multiple times.
         */
        void warmup();

    private:
        void evict_lru();

        size_t max_size_;
        mutable std::shared_mutex mutex_;
        
        // LRU implementation: list maintains access order, map provides O(1) lookup
        std::list<std::string> access_order_;
        std::map<std::string, std::pair<CompiledRegex, std::list<std::string>::iterator>> cache_;
    };

} // namespace orion::bre::feel
