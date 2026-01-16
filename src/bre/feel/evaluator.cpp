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
#include <orion/bre/feel/regex_cache.hpp>
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

    json Evaluator::evaluate(std::string_view expression, const json& input, const EvaluationContext& eval_ctx)
    {
        // AST-based evaluation path (all FEEL features supported)
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
                Lexer lexer;
                auto tokens = lexer.tokenize(expression);
                
                Parser parser;
                auto ast = parser.parse(tokens);
                
                return ast->evaluate(input, eval_ctx);
            }
            catch (const std::exception& e)
            {
                // AST evaluation failed - unsupported FEEL features
                throw std::runtime_error(std::string("FEEL expression evaluation failed: ").append(expression) + " - " + e.what());
            }
        }

        // If we reach here with AST parsing enabled, something went wrong
        throw std::runtime_error(std::string("FEEL expression evaluation failed: ").append(expression));
    }
}
