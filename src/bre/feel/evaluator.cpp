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

    json Evaluator::evaluate(const std::string& expression, const json& context)
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
                throw std::runtime_error("FEEL expression evaluation failed: " + expression + " - " + std::string(e.what()));
            }
        }

        // If we reach here with AST parsing enabled, something went wrong
        throw std::runtime_error("FEEL expression evaluation failed: " + expression);
    }
}
