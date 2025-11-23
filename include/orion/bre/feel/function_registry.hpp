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
#include <vector>
#include <map>
#include <optional>

namespace orion::bre::feel {

/**
 * @brief Represents a formal parameter in a function signature
 */
struct FormalParameter {
    std::string name;
    bool optional;  // For functions with optional parameters
    
    FormalParameter(std::string param_name, bool opt = false)
        : name(std::move(param_name)), optional(opt) {}
};

/**
 * @brief Represents a complete function signature with parameter metadata
 */
struct FunctionSignature {
    std::string name;
    std::vector<FormalParameter> parameters;
    bool variadic;  // For functions accepting variable number of arguments
    
    FunctionSignature() : variadic(false) {}
    
    FunctionSignature(std::string func_name, 
                     std::vector<FormalParameter> params,
                     bool var = false)
        : name(std::move(func_name)), parameters(std::move(params)), variadic(var) {}
};

/**
 * @brief Registry of all built-in FEEL functions with their formal parameter names
 * 
 * This singleton class maintains metadata about all built-in functions to enable
 * named parameter support as required by DMN 1.5 Section 10.3.2.13.5.
 */
class FunctionRegistry {
public:
    /**
     * @brief Get the singleton instance
     */
    static FunctionRegistry& instance();
    
    /**
     * @brief Register a function signature
     * @param sig The function signature to register
     */
    void register_function(const FunctionSignature& sig);
    
    /**
     * @brief Get the signature for a function by name
     * @param name The function name (case-sensitive)
     * @return The function signature if found, otherwise nullopt
     */
    [[nodiscard]] std::optional<FunctionSignature> get_signature(std::string_view name) const;
    
    // Prevent copying and moving (singleton pattern)
    FunctionRegistry(const FunctionRegistry&) = delete;
    FunctionRegistry& operator=(const FunctionRegistry&) = delete;
    FunctionRegistry(FunctionRegistry&&) = delete;
    FunctionRegistry& operator=(FunctionRegistry&&) = delete;
    
private:
    FunctionRegistry();
    ~FunctionRegistry() = default;
    
    std::map<std::string, FunctionSignature> functions_;
};

} // namespace orion::bre::feel
