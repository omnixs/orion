#ifndef ORION_TST_BRE_FEEL_TEST_HELPERS_HPP
#define ORION_TST_BRE_FEEL_TEST_HELPERS_HPP

#include <orion/bre/feel/evaluator.hpp>
#include <orion/bre/feel/regex_cache.hpp>
#include <orion/bre/ast_node.hpp>
#include <nlohmann/json.hpp>

namespace orion::bre::feel::test {

using json = nlohmann::json_abi_v3_12_0::json;

/// Get a static test evaluation context
inline EvaluationContext& get_test_eval_ctx() {
    static RegexCache regex_cache;
    static EvaluationContext eval_ctx(regex_cache);
    return eval_ctx;
}

} // namespace orion::bre::feel::test

#endif // ORION_TST_BRE_FEEL_TEST_HELPERS_HPP
