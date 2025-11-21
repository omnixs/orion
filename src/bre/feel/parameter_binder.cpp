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

#include <orion/bre/feel/parameter_binder.hpp>
#include <sstream>
#include <stdexcept>

namespace orion::bre::feel {

namespace {

// Helper: Bind parameters positionally
std::vector<json> bind_positional_parameters(
    const std::string& functionName,
    const FunctionSignature& sig,
    const std::vector<FunctionParameter>& parameters,
    const json& context)
{
    const auto& formal_params = sig.parameters;
    std::vector<json> args;
    
    // Validate parameter count for non-variadic functions
    if (!sig.variadic && parameters.size() > formal_params.size())
    {
        std::ostringstream oss;
        oss << "Function '" << functionName << "' expects " 
            << formal_params.size() << " parameter(s), but " 
            << parameters.size() << " were provided";
        throw std::runtime_error(oss.str());
    }
    
    // Bind positional parameters
    for (size_t i = 0; i < formal_params.size(); ++i)
    {
        if (i < parameters.size())
        {
            // Evaluate provided parameter
            args.push_back(parameters[i].valueExpr->evaluate(context));
        }
        else if (formal_params[i].optional)
        {
            // Optional parameter not provided - use null
            args.emplace_back(nullptr);
        }
        else
        {
            // Required parameter not provided
            std::ostringstream oss;
            oss << "Required parameter '" << formal_params[i].name 
                << "' not provided for function '" << functionName << "'";
            throw std::runtime_error(oss.str());
        }
    }
    
    // Handle variadic parameters (extra args beyond formal params)
    if (sig.variadic)
    {
        for (size_t i = formal_params.size(); i < parameters.size(); ++i)
        {
            args.push_back(parameters[i].valueExpr->evaluate(context));
        }
    }
    
    return args;
}

// Helper: Bind parameters by name
std::vector<json> bind_named_parameters(
    const std::string& functionName,
    const FunctionSignature& sig,
    const std::vector<FunctionParameter>& parameters,
    const json& context)
{
    const auto& formal_params = sig.parameters;
    std::vector<json> args;
    args.resize(formal_params.size(), nullptr);
    
    // Track which parameters were actually provided
    std::vector<bool> provided(formal_params.size(), false);
    
    // Map named parameters to formal parameter positions
    for (const auto& actual_param : parameters)
    {
        // Find matching formal parameter
        bool found = false;
        for (size_t i = 0; i < formal_params.size(); ++i)
        {
            if (formal_params[i].name == actual_param.name)
            {
                // Evaluate and bind parameter
                args[i] = actual_param.valueExpr->evaluate(context);
                provided[i] = true;
                found = true;
                break;
            }
        }
        
        if (!found)
        {
            // Named parameter doesn't match any formal parameter
            // For variadic functions, this might be OK - collect extra args
            if (sig.variadic)
            {
                // Add to end of args vector
                args.push_back(actual_param.valueExpr->evaluate(context));
            }
            else
            {
                std::ostringstream oss;
                oss << "Unknown parameter '" << actual_param.name 
                    << "' for function '" << functionName << "'";
                throw std::runtime_error(oss.str());
            }
        }
    }
    
    // Validate that all required parameters were provided
    for (size_t i = 0; i < formal_params.size(); ++i)
    {
        if (!provided[i] && !formal_params[i].optional)
        {
            std::ostringstream oss;
            oss << "Required parameter '" << formal_params[i].name 
                << "' not provided for function '" << functionName << "'";
            throw std::runtime_error(oss.str());
        }
    }
    
    return args;
}

// Helper: Detect if using named parameters
bool has_named_parameters(const std::vector<FunctionParameter>& parameters)
{
    return std::ranges::any_of(parameters, [](const auto& param) {
        return !param.name.empty();
    });
}

} // anonymous namespace
    std::vector<json> bind_parameters(
        const std::string& functionName,
        const std::vector<FunctionParameter>& parameters,
        const json& context
    )
    {
    // Get function signature from registry
    auto& registry = FunctionRegistry::instance();
    auto signature = registry.get_signature(functionName);      if (!signature.has_value())
        {
            // Function not in registry - fall back to positional binding
            // This allows calling functions that haven't been registered yet
            std::vector<json> args;
            args.reserve(parameters.size());
            for (const auto& param : parameters)
            {
                args.push_back(param.valueExpr->evaluate(context));
            }
            return args;
        }
        
        const auto& sig = signature.value();
        
        // Detect if using named or positional parameters
        bool using_named_params = has_named_parameters(parameters);
        
    if (using_named_params)
    {
        return bind_named_parameters(functionName, sig, parameters, context);
    }
    return bind_positional_parameters(functionName, sig, parameters, context);
    }
} // namespace orion::bre::feel