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

#include <orion/bre/feel/function_registry.hpp>

namespace orion::bre::feel {

FunctionRegistry& FunctionRegistry::instance() {
    static FunctionRegistry registry;
    return registry;
}

// Helper: Register numeric functions
void register_numeric_functions(FunctionRegistry& reg) {
    // Single parameter 'n'
    reg.register_function({"abs", {{"n"}}});
    reg.register_function({"floor", {{"n"}}});
    reg.register_function({"ceiling", {{"n"}}});
    
    // Single parameter 'number'
    reg.register_function({"sqrt", {{"number"}}});
    reg.register_function({"exp", {{"number"}}});
    reg.register_function({"log", {{"number"}}});
    reg.register_function({"odd", {{"number"}}});
    reg.register_function({"even", {{"number"}}});
    
    // Two parameters
    reg.register_function({"modulo", {{"dividend"}, {"divisor"}}});
    reg.register_function({"decimal", {{"n"}, {"scale"}}});
    
    // Rounding functions with two parameters
    reg.register_function({"round", {{"n"}, {"scale"}}});
    reg.register_function({"round up", {{"n"}, {"scale"}}});
    reg.register_function({"round down", {{"n"}, {"scale"}}});
    reg.register_function({"round half up", {{"n"}, {"scale"}}});
    reg.register_function({"round half down", {{"n"}, {"scale"}}});
}

// Helper: Register string functions
void register_string_functions(FunctionRegistry& reg) {
    reg.register_function({"substring", {
        {"string"},
        {"start position"},
        {"length", true}  // optional parameter
    }});
    reg.register_function({"string length", {{"string"}}});
    reg.register_function({"upper case", {{"string"}}});
    reg.register_function({"lower case", {{"string"}}});
    reg.register_function({"substring before", {{"string"}, {"match"}}});
    reg.register_function({"substring after", {{"string"}, {"match"}}});
    reg.register_function({"contains", {{"string"}, {"match"}}});
    reg.register_function({"starts with", {{"string"}, {"match"}}});
    reg.register_function({"ends with", {{"string"}, {"match"}}});
    reg.register_function({"replace", {
        {"input"},
        {"pattern"},
        {"replacement"},
        {"flags", true}  // optional parameter
    }});
    reg.register_function({"matches", {
        {"input"},
        {"pattern"},
        {"flags", true}  // optional parameter
    }});
    reg.register_function({"split", {{"string"}, {"delimiter"}}});
    reg.register_function({"string join", {
        {"list"},
        {"delimiter", true}  // optional parameter
    }});
}

// Helper: Register list functions
void register_list_functions(FunctionRegistry& reg) {
    reg.register_function({"list contains", {{"list"}, {"element"}}});
    reg.register_function({"count", {{"list"}}});
    reg.register_function({"min", {{"list"}}});
    reg.register_function({"max", {{"list"}}});
    reg.register_function({"sum", {{"list"}}});
    reg.register_function({"mean", {{"list"}}});
    reg.register_function({"all", {{"list"}}});
    reg.register_function({"any", {{"list"}}});
    
    reg.register_function({"sublist", {
        {"list"},
        {"start position"},
        {"length", true}  // optional parameter
    }});
    
    // Variadic functions
    reg.register_function({"append", {{"list"}}, true});
    reg.register_function({"concatenate", {{"list"}}, true});
    
    reg.register_function({"insert before", {{"list"}, {"position"}, {"newItem"}}});
    reg.register_function({"remove", {{"list"}, {"position"}}});
    reg.register_function({"reverse", {{"list"}}});
    reg.register_function({"index of", {{"list"}, {"match"}}});
    reg.register_function({"union", {{"list"}}, true});
    reg.register_function({"distinct values", {{"list"}}});
    reg.register_function({"flatten", {{"list"}}});
    reg.register_function({"product", {{"list"}}});
    reg.register_function({"median", {{"list"}}});
    reg.register_function({"stddev", {{"list"}}});
    reg.register_function({"mode", {{"list"}}});
    reg.register_function({"list replace", {{"list"}, {"position"}, {"newItem"}}});
}

// Helper: Register date/time and temporal functions
void register_date_time_functions(FunctionRegistry& reg) {
    // Date conversion functions
    reg.register_function({"date", {{"from"}}});
    reg.register_function({"time", {{"from"}}});
    reg.register_function({"date and time", {{"from"}}});
    reg.register_function({"duration", {{"from"}}});
    
    reg.register_function({"number", {
        {"from"},
        {"grouping separator"},
        {"decimal separator"}
    }});
    
    reg.register_function({"string", {{"from"}}});
    reg.register_function({"years and months duration", {{"from"}, {"to"}}});
    
    // Temporal functions
    reg.register_function({"day of year", {{"date"}}});
    reg.register_function({"day of week", {{"date"}}});
    reg.register_function({"month of year", {{"date"}}});
    reg.register_function({"week of year", {{"date"}}});
}

// Helper: Register context and miscellaneous functions
void register_context_and_misc_functions(FunctionRegistry& reg) {
    // Boolean functions
    reg.register_function({"not", {{"negand"}}});
    
    // Context functions
    reg.register_function({"get value", {{"m"}, {"key"}}});
    reg.register_function({"get entries", {{"m"}}});
    reg.register_function({"context", {{"entries"}}});
    reg.register_function({"context put", {{"context"}, {"key"}, {"value"}}});
    reg.register_function({"context merge", {{"contexts"}}});
    
    // Miscellaneous functions
    reg.register_function({"sort", {{"list"}, {"precedes"}}});
    reg.register_function({"is", {{"value1"}, {"value2"}}});
    reg.register_function({"now", {}});
    reg.register_function({"today", {}});
}

// Helper: Register range functions
void register_range_functions(FunctionRegistry& reg) {
    reg.register_function({"before", {{"point1"}, {"point2"}}});
    reg.register_function({"after", {{"point1"}, {"point2"}}});
    reg.register_function({"meets", {{"range1"}, {"range2"}}});
    reg.register_function({"met by", {{"range1"}, {"range2"}}});
    reg.register_function({"overlaps", {{"range1"}, {"range2"}}});
    reg.register_function({"overlaps before", {{"range1"}, {"range2"}}});
    reg.register_function({"overlaps after", {{"range1"}, {"range2"}}});
    reg.register_function({"finishes", {{"point"}, {"range"}}});
    reg.register_function({"finished by", {{"range"}, {"point"}}});
    reg.register_function({"includes", {{"range"}, {"point"}}});
    reg.register_function({"during", {{"point"}, {"range"}}});
    reg.register_function({"starts", {{"point"}, {"range"}}});
    reg.register_function({"started by", {{"range"}, {"point"}}});
    reg.register_function({"coincides", {{"point1"}, {"point2"}}});
}

FunctionRegistry::FunctionRegistry() {
    // Register all built-in functions with their formal parameter names
    // Based on DMN 1.5 specification (formal-24-01-01.txt)
    
    register_numeric_functions(*this);
    register_string_functions(*this);
    register_list_functions(*this);
    register_date_time_functions(*this);
    register_context_and_misc_functions(*this);
    register_range_functions(*this);
}

void FunctionRegistry::register_function(const FunctionSignature& sig) {
    functions_[sig.name] = sig;
}

std::optional<FunctionSignature> FunctionRegistry::get_signature(
    std::string_view name) const 
{
    auto iter = functions_.find(std::string(name));
    if (iter != functions_.end()) {
        return iter->second;
    }
    return std::nullopt;
}

} // namespace orion::bre::feel
