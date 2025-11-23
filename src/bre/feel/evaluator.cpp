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

#include <orion/bre/feel/evaluator.hpp>
#include <orion/api/logger.hpp>
#include <orion/bre/feel/lexer.hpp>
#include <orion/bre/feel/parser.hpp>
#include "orion/bre/ast_node.hpp"
#include "util_internal.hpp"
#include <algorithm>
#include <regex>
#include <limits>
#include <iomanip>
#include <iostream>
#include <stdexcept>

// Feature flag: Enable AST-based FEEL evaluation
namespace orion::bre::feel {
    // Import logger functions
    using orion::api::debug;
    using orion::api::warn;
    using orion::api::error;

    json Evaluator::evaluate(std::string_view expression, const json& context)
    {
        // AST-based evaluation path (all FEEL features supported)
        
        debug("[LEGACY-TRACE] Evaluator::evaluate() called: '{}'", expression);
        
        // All FEEL features now supported via AST!
        // Phase 1: Function calls (not, all, any, contains)
        // Phase 2: List operations ([...])
        bool has_unsupported_features = false;
        
        if (has_unsupported_features)
        {
            warn("[LEGACY-USED] Expression has unsupported features, using LEGACY path: '{}'", expression);
        }
        
        if (!has_unsupported_features)
        {
            try
            {
                debug("[AST-PATH] Trying AST evaluation for: '{}'", expression);
                Lexer lexer;
                auto tokens = lexer.tokenize(expression);
                
                Parser parser;
                auto ast = parser.parse(tokens);
                
                auto result = ast->evaluate(context);
                debug("[AST-SUCCESS] AST evaluation succeeded: '{}'", expression);
                return result;
            }
            catch (const std::exception& e)
            {
                // AST evaluation failed - log at debug level to avoid spam during TCK testing
                // When testing compliance suites, many unimplemented features are expected
                debug("[AST-FAILED] AST evaluation failed: '{}' - Error: {}", expression, e.what());
                throw std::runtime_error(std::string("FEEL expression evaluation failed: ") + std::string(expression) + " - " + std::string(e.what()));
            }
        }

        // If we reach here with AST parsing enabled, something went wrong
        throw std::runtime_error(std::string("FEEL expression evaluation failed: ") + std::string(expression));
    }
}
