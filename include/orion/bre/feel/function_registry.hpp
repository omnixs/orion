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
#include <span>
#include <string_view>

namespace orion::bre::feel {

/**
 * @brief Represents a formal parameter in a function signature
 */
struct FormalParameter {
    std::string_view name;
    bool optional = false;  // For functions with optional parameters
};

/**
 * @brief Represents a complete function signature with parameter metadata
 *
 * All members reference static, compile-time data.
 */
struct FunctionSignature {
    std::string_view name;
    std::span<const FormalParameter> parameters;
    bool variadic = false;  // For functions accepting variable number of arguments
};

/**
 * @brief Registry of all built-in FEEL functions with their formal parameter names
 * 
 * Maintains metadata about all built-in functions to enable named parameter
 * support as required by DMN 1.5 Section 10.3.2.13.5. The metadata is a
 * compile-time constant table: it never allocates and needs no runtime
 * initialization, so it is safe to use concurrently from any thread.
 */
class FunctionRegistry {
public:
    /**
     * @brief Get the singleton instance
     */
    static const FunctionRegistry& instance() noexcept;
    
    /**
     * @brief Get the signature for a function by name
     *
     * @param name The function name (case-sensitive)
     * @return Pointer to the signature if found, otherwise nullptr. The pointee
     *         has static storage duration.
     */
    [[nodiscard]] const FunctionSignature* get_signature(std::string_view name) const noexcept;
    
    // Prevent copying and moving (singleton pattern)
    FunctionRegistry(const FunctionRegistry&) = delete;
    FunctionRegistry& operator=(const FunctionRegistry&) = delete;
    FunctionRegistry(FunctionRegistry&&) = delete;
    FunctionRegistry& operator=(FunctionRegistry&&) = delete;
    
private:
    constexpr FunctionRegistry() = default;
    ~FunctionRegistry() = default;
};

} // namespace orion::bre::feel
