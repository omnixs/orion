#ifndef ORION_TST_BRE_FEEL_TEST_HELPERS_HPP
#define ORION_TST_BRE_FEEL_TEST_HELPERS_HPP

#include <orion/bre/feel/evaluator.hpp>
#include <orion/bre/feel/regex_cache.hpp>
#include <nlohmann/json.hpp>

namespace orion::bre::feel::test {

using json = nlohmann::json_abi_v3_12_0::json;

/// Helper to create RegexCache and EvaluationContext for tests
struct TestEvaluationContext {
    RegexCache regex_cache;
    EvaluationContext eval_ctx;
    
    TestEvaluationContext() {
        eval_ctx.regex_cache = &regex_cache;
    }
    
    // Convenience method for evaluation
    json evaluate(std::string_view expression, const json& context = json::object()) {
        return Evaluator::evaluate(expression, context, eval_ctx);
    }
};

/// Helper for AST evaluation in tests
struct TestASTEvaluator {
    RegexCache regex_cache;
    EvaluationContext eval_ctx;
    
    TestASTEvaluator() {
        eval_ctx.regex_cache = &regex_cache;
    }
    
    // Evaluate AST node
    json evaluate(const ASTNode* ast, const json& context) {
        return ast->evaluate(context, eval_ctx);
    }
};

} // namespace orion::bre::feel::test

#endif // ORION_TST_BRE_FEEL_TEST_HELPERS_HPP
