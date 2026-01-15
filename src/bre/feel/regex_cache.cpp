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

#include <orion/bre/feel/regex_cache.hpp>

#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>
#include <mutex>

namespace orion::bre::feel {

    // CompiledRegex implementation
    
    CompiledRegex::CompiledRegex(pcre2_real_code_8* code)
        : code_(code)
        , match_data_(nullptr)
    {
    }

    CompiledRegex::CompiledRegex(CompiledRegex&& other) noexcept
        : code_(other.code_)
        , match_data_(other.match_data_)
    {
        other.code_ = nullptr;
        other.match_data_ = nullptr;
    }

    CompiledRegex& CompiledRegex::operator=(CompiledRegex&& other) noexcept
    {
        if (this != &other) {
            if (code_) {
                pcre2_code_free(code_);
            }
            if (match_data_) {
                pcre2_match_data_free(match_data_);
            }
            
            code_ = other.code_;
            match_data_ = other.match_data_;
            
            other.code_ = nullptr;
            other.match_data_ = nullptr;
        }
        return *this;
    }

    CompiledRegex::~CompiledRegex()
    {
        if (code_) {
            pcre2_code_free(code_);
        }
        if (match_data_) {
            pcre2_match_data_free(match_data_);
        }
    }

    std::optional<CompiledRegex> CompiledRegex::compile(std::string_view pattern, std::string_view flags)
    {
        int error_code = 0;
        PCRE2_SIZE error_offset = 0;

        // Parse PCRE2 flags from DMN flags string
        uint32_t pcre2_options = 0;
        for (char flag : flags) {
            switch (flag) {
                case 'i': pcre2_options |= PCRE2_CASELESS; break;
                case 'm': pcre2_options |= PCRE2_MULTILINE; break;
                case 's': pcre2_options |= PCRE2_DOTALL; break;
                case 'x': pcre2_options |= PCRE2_EXTENDED; break;
                default:
                    // Invalid flag - return null per DMN spec
                    return std::nullopt;
            }
        }

        pcre2_code* code = pcre2_compile(
            reinterpret_cast<PCRE2_SPTR>(pattern.data()),
            pattern.size(),
            pcre2_options,
            &error_code,
            &error_offset,
            nullptr // use default compile context
        );

        if (!code) {
            // Compilation failed - invalid pattern
            return std::nullopt;
        }

        return CompiledRegex(code);
    }

    bool CompiledRegex::matches(std::string_view input) const
    {
        if (!code_) {
            return false;
        }

        // Lazy initialize match_data
        if (!match_data_) {
            match_data_ = pcre2_match_data_create_from_pattern(code_, nullptr);
            if (!match_data_) {
                return false;
            }
        }

        int rc = pcre2_match(
            code_,
            reinterpret_cast<PCRE2_SPTR>(input.data()),
            input.size(),
            0, // start offset
            0, // XPath/DMN spec: partial match (substring search) - see W3C XPath fn:matches
            match_data_,
            nullptr // use default match context
        );

        // rc >= 0 means match found (rc is number of capturing groups + 1)
        // Per XPath spec: "returns true if input or SOME SUBSTRING matches"
        // DMN 1.5 Section 10.3.4.3 delegates to XQuery/XPath semantics
        return rc >= 0;
    }

    // RegexCache implementation
    
    RegexCache::RegexCache(size_t max_size)
        : max_size_(max_size)
    {
    }

    std::optional<CompiledRegex> RegexCache::get_or_compile(std::string_view pattern, std::string_view flags)
    {
        // Create cache key from pattern + flags (flags affect compilation)
        std::string pattern_key(pattern);
        pattern_key += "\0"; // Null separator
        pattern_key += flags;
        
        {
            // Use shared_lock for read-only cache lookup (allows concurrent reads)
            std::shared_lock<std::shared_mutex> lock(mutex_);
            
            // Check if pattern is in cache
            auto it = cache_.find(pattern_key);
            if (it != cache_.end()) {
                // Move to front of access_order_ (most recently used)
                access_order_.splice(access_order_.begin(), access_order_, it->second.second);
                
                // Return a compiled copy (PCRE2 code objects are thread-safe for matching)
                // However, our CompiledRegex is move-only, so we need to recompile
                // Alternative: make CompiledRegex copyable by sharing the pcre2_code via shared_ptr
                // For now, return by recompiling (we can optimize this later if needed)
                return CompiledRegex::compile(pattern, flags);
            }
        }
        
        // Not in cache - compile it
        auto compiled = CompiledRegex::compile(pattern, flags);
        if (!compiled) {
            // Invalid pattern or invalid flags
            return std::nullopt;
        }
        
        {
            // Use unique_lock for cache modification (exclusive access)
            std::unique_lock<std::shared_mutex> lock(mutex_);
            
            // Evict if at capacity
            if (cache_.size() >= max_size_) {
                evict_lru();
            }
            
            // Add to cache
            access_order_.push_front(pattern_key);
            cache_.emplace(pattern_key, std::make_pair(CompiledRegex::compile(pattern, flags).value(), access_order_.begin()));
        }
        
        return compiled;
    }

    void RegexCache::evict_lru()
    {
        // Assumes mutex is already held
        if (access_order_.empty()) {
            return;
        }
        
        // Remove least recently used (back of list)
        std::string lru_key = access_order_.back();
        access_order_.pop_back();
        cache_.erase(lru_key);
    }

    void RegexCache::clear()
    {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        cache_.clear();
        access_order_.clear();
    }

    size_t RegexCache::size() const
    {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        return cache_.size();
    }



    // Instance method for warming up regex cache
    void RegexCache::warmup()
    {
        // Warm up PCRE2 with realistic patterns that exercise common features
        // This reduces first-use latency and initializes PCRE2 internal structures
        
        // Pattern exercises:
        // - Character classes: [0-9], [A-Za-z]
        // - Quantifiers: {2,4}, +, *
        // - Anchors: ^, $, \b
        // - Alternation: |
        // - Escape sequences: \d, \w, \s
        const char* warmup_patterns[] = {
            "[0-9]{2,4}",           // Digits with quantifier
            "^[A-Za-z]+$",          // Letters with anchors
            "\\d+\\.\\d+",            // Floating point numbers
            "\\b\\w+\\b",            // Word boundaries
            "(true|false|null)",   // Alternation with groups
        };
        
        const char* warmup_inputs[] = {
            "123",
            "abc",
            "3.14",
            "word",
            "true",
        };
        
        // Compile and match each pattern to initialize PCRE2
        for (size_t i = 0; i < 5; ++i) {
            auto pattern = CompiledRegex::compile(warmup_patterns[i]);
            if (pattern) {
                [[maybe_unused]] bool result = pattern->matches(warmup_inputs[i]);
            }
        }
        
        // Also add one pattern to this cache instance
        [[maybe_unused]] auto cached = get_or_compile("[0-9]+");
    }

    // Global thread-local cache for backward compatibility with test code
    RegexCache& get_regex_cache() {
        // Same thread-local cache used by Evaluator::evaluate(expr, context) overload
        // This allows test code to inspect the cache used by the convenience API
        static thread_local RegexCache cache(100);
        return cache;
    }

} // namespace orion::bre::feel
