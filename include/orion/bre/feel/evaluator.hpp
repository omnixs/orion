#pragma once

#include <string>
#include <nlohmann/json.hpp>

namespace orion::bre::feel {
    using json = nlohmann::json;

    /**
     * @brief FEEL Expression Evaluator
     * 
     * Evaluates FEEL expressions using AST-based parsing.
     * All FEEL features are now handled through the AST parser for optimal performance.
     */
    class Evaluator
    {
    public:
        /**
         * @brief Evaluate a FEEL expression
         * @param expression The FEEL expression to evaluate
         * @param context The evaluation context with variable bindings
         * @return The result of evaluation as JSON
         * @throws std::runtime_error if evaluation fails
         */
        [[nodiscard]] static json evaluate(const std::string& expression, const json& context = json::object());
    };
}
