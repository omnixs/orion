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
 * @file parameter_binder.hpp
 * @brief Parameter binding for FEEL function calls with named parameter support
 * 
 * Implements DMN 1.5 Section 10.3.2.13 - Named Parameters specification.
 * 
 * Supports both positional and named parameter binding:
 * - **Positional**: `abs(-42)` - parameters matched by position
 * - **Named**: `abs(n: -42)` - parameters matched by name
 * 
 * Key features:
 * - Maps actual parameters to formal parameters using function registry
 * - Validates parameter names against function signature
 * - Handles optional parameters with null defaults
 * - Detects and rejects mixed positional/named parameters
 * - Supports variadic functions (e.g., append, concatenate)
 * 
 * @example Positional binding
 * ```cpp
 * // abs(42) → binds to formal parameter "n"
 * std::vector<json> args = bindParameters("abs", params, context);
 * // args[0] = 42
 * ```
 * 
 * @example Named binding
 * ```cpp
 * // decimal(scale: 2, n: 3.14159) → binds out of order
 * std::vector<json> args = bindParameters("decimal", params, context);
 * // args[0] = 3.14159 (n)
 * // args[1] = 2 (scale)
 * ```
 * 
 * @example Optional parameters
 * ```cpp
 * // substring("hello", 2) → length is optional
 * std::vector<json> args = bindParameters("substring", params, context);
 * // args[0] = "hello"
 * // args[1] = 2
 * // args[2] = null (length not provided)
 * ```
 */

#pragma once
#include <orion/bre/ast_node.hpp>
#include <orion/bre/feel/function_registry.hpp>
#include <nlohmann/json.hpp>
#include <vector>
#include <string>

namespace orion::bre::feel {
    using json = nlohmann::json;

    /**
     * @brief Bind actual parameters to formal parameters for function call
     * 
     * Performs parameter binding according to DMN 1.5 specification:
     * 
     * 1. **Positional Parameters**: Match by position in signature
     * 2. **Named Parameters**: Match by name, validate against signature
     * 3. **Optional Parameters**: Fill with null if not provided
     * 4. **Variadic Functions**: Accept extra arguments after required params
     * 
     * @param functionName Name of the function being called
     * @param parameters Actual parameters from AST (may be positional or named)
     * @param context Evaluation context for evaluating parameter expressions
     * @return Vector of evaluated argument values in signature order
     * @throws std::runtime_error if:
     *   - Function not found in registry
     *   - Named parameter doesn't match any formal parameter
     *   - Required parameter not provided
     *   - Too many positional parameters for non-variadic function
     * 
     * @example
     * ```cpp
     * // Function call: abs(n: -42)
     * std::vector<FunctionParameter> params;
     * FunctionParameter p;
     * p.name = "n";
     * p.valueExpr = std::make_unique<ASTNode>(ASTNodeType::LITERAL_NUMBER, "-42");
     * params.push_back(std::move(p));
     * 
     * json context = {};
     * std::vector<json> args = bind_parameters("abs", params, context);
     * // args[0] = -42
     * ```
     */
    std::vector<json> bind_parameters(
        const std::string& functionName,
        const std::vector<FunctionParameter>& parameters,
        const json& context
    );
} // namespace orion::bre::feel
