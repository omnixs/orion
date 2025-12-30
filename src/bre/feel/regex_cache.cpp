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

    std::optional<CompiledRegex> CompiledRegex::compile(std::string_view pattern)
    {
        int error_code = 0;
        PCRE2_SIZE error_offset = 0;

        pcre2_code* code = pcre2_compile(
            reinterpret_cast<PCRE2_SPTR>(pattern.data()),
            pattern.size(),
            0, // options
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
            PCRE2_ANCHORED, // full-string match (equivalent to std::regex_match)
            match_data_,
            nullptr // use default match context
        );

        // rc >= 0 means match found (rc is number of capturing groups)
        // We also need to verify it matched the entire string
        if (rc >= 0) {
            PCRE2_SIZE* ovector = pcre2_get_ovector_pointer(match_data_);
            // ovector[0] = start of match, ovector[1] = end of match
            return ovector[0] == 0 && ovector[1] == input.size();
        }

        return false;
    }

    // RegexCache implementation
    
    RegexCache::RegexCache(size_t max_size)
        : max_size_(max_size)
    {
    }

    std::optional<CompiledRegex> RegexCache::get_or_compile(std::string_view pattern)
    {
        std::string pattern_key(pattern);
        
        {
            std::lock_guard<std::mutex> lock(mutex_);
            
            // Check if pattern is in cache
            auto it = cache_.find(pattern_key);
            if (it != cache_.end()) {
                // Move to front of access_order_ (most recently used)
                access_order_.splice(access_order_.begin(), access_order_, it->second.second);
                
                // Return a compiled copy (PCRE2 code objects are thread-safe for matching)
                // However, our CompiledRegex is move-only, so we need to recompile
                // Alternative: make CompiledRegex copyable by sharing the pcre2_code via shared_ptr
                // For now, return by recompiling (we can optimize this later if needed)
                return CompiledRegex::compile(pattern);
            }
        }
        
        // Not in cache - compile it
        auto compiled = CompiledRegex::compile(pattern);
        if (!compiled) {
            // Invalid pattern
            return std::nullopt;
        }
        
        {
            std::lock_guard<std::mutex> lock(mutex_);
            
            // Evict if at capacity
            if (cache_.size() >= max_size_) {
                evict_lru();
            }
            
            // Add to cache
            access_order_.push_front(pattern_key);
            cache_.emplace(pattern_key, std::make_pair(CompiledRegex::compile(pattern).value(), access_order_.begin()));
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
        std::lock_guard<std::mutex> lock(mutex_);
        cache_.clear();
        access_order_.clear();
    }

    size_t RegexCache::size() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return cache_.size();
    }

    // Global regex cache singleton
    RegexCache& get_regex_cache()
    {
        // Default size of 100, can be configured via engine options
        static RegexCache cache(100);
        return cache;
    }

} // namespace orion::bre::feel
