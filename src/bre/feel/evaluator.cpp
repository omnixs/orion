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
#include <cctype>
#include <regex>
#include <limits>
#include <iomanip>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>

// Feature flag: Enable AST-based FEEL evaluation
namespace orion::bre::feel {
    // Import logger functions
    using orion::api::debug;
    using orion::api::warn;
    using orion::api::error;

    namespace {

    std::string trim_copy(std::string_view sv)
    {
        size_t start = 0;
        while (start < sv.size() && std::isspace(static_cast<unsigned char>(sv[start])))
        {
            ++start;
        }
        size_t end = sv.size();
        while (end > start && std::isspace(static_cast<unsigned char>(sv[end - 1])))
        {
            --end;
        }
        return std::string(sv.substr(start, end - start));
    }

    bool starts_with_ci(std::string_view text, std::string_view prefix)
    {
        if (text.size() < prefix.size()) return false;
        for (size_t i = 0; i < prefix.size(); ++i)
        {
            char a = static_cast<char>(std::tolower(static_cast<unsigned char>(text[i])));
            char b = static_cast<char>(std::tolower(static_cast<unsigned char>(prefix[i])));
            if (a != b) return false;
        }
        return true;
    }

    std::vector<std::string> split_top_level_arguments(std::string_view args)
    {
        std::vector<std::string> out;
        std::string current;
        int paren = 0;
        int bracket = 0;
        int brace = 0;
        bool in_string = false;

        for (size_t i = 0; i < args.size(); ++i)
        {
            char c = args[i];

            if (c == '"' && (i == 0 || args[i - 1] != '\\'))
            {
                in_string = !in_string;
                current.push_back(c);
                continue;
            }

            if (!in_string)
            {
                if (c == '(') ++paren;
                else if (c == ')') --paren;
                else if (c == '[') ++bracket;
                else if (c == ']') --bracket;
                else if (c == '{') ++brace;
                else if (c == '}') --brace;

                if (c == ',' && paren == 0 && bracket == 0 && brace == 0)
                {
                    out.push_back(trim_copy(current));
                    current.clear();
                    continue;
                }
            }

            current.push_back(c);
        }

        if (!current.empty())
        {
            out.push_back(trim_copy(current));
        }

        return out;
    }

    std::optional<size_t> find_last_top_level_char(std::string_view text, char target)
    {
        int paren = 0;
        int bracket = 0;
        int brace = 0;
        bool in_string = false;
        std::optional<size_t> last_pos;

        for (size_t i = 0; i < text.size(); ++i)
        {
            char c = text[i];

            if (c == '"' && (i == 0 || text[i - 1] != '\\'))
            {
                in_string = !in_string;
                continue;
            }

            if (in_string)
            {
                continue;
            }

            if (c == target && paren == 0 && bracket == 0 && brace == 0)
            {
                last_pos = i;
            }

            if (c == '(') ++paren;
            else if (c == ')') --paren;
            else if (c == '[') ++bracket;
            else if (c == ']') --bracket;
            else if (c == '{') ++brace;
            else if (c == '}') --brace;
        }

        return last_pos;
    }

    std::optional<json> try_evaluate_sort_with_lambda_precedes(
        std::string_view expression,
        const json& input,
        const EvaluationContext& eval_ctx)
    {
        std::string expr = trim_copy(expression);
        if (!starts_with_ci(expr, "sort"))
        {
            return std::nullopt;
        }

        size_t lparen = expr.find('(');
        size_t rparen = expr.rfind(')');
        if (lparen == std::string::npos || rparen == std::string::npos || rparen <= lparen)
        {
            return std::nullopt;
        }

        std::string args_str = expr.substr(lparen + 1, rparen - lparen - 1);
        auto args = split_top_level_arguments(args_str);
        if (args.size() != 2)
        {
            return std::nullopt;
        }

        std::string list_expr = trim_copy(args[0]);
        std::string precedes_expr = trim_copy(args[1]);

        if (!starts_with_ci(precedes_expr, "function"))
        {
            return std::nullopt;
        }

        size_t fn_lparen = precedes_expr.find('(');
        size_t fn_rparen = precedes_expr.find(')', fn_lparen == std::string::npos ? 0 : fn_lparen + 1);
        if (fn_lparen == std::string::npos || fn_rparen == std::string::npos || fn_rparen <= fn_lparen)
        {
            return json(nullptr);
        }

        std::string params_str = precedes_expr.substr(fn_lparen + 1, fn_rparen - fn_lparen - 1);
        auto params = split_top_level_arguments(params_str);
        if (params.size() != 2)
        {
            return json(nullptr);
        }

        std::string p1 = trim_copy(params[0]);
        std::string p2 = trim_copy(params[1]);
        std::string predicate_expr = trim_copy(precedes_expr.substr(fn_rparen + 1));
        if (p1.empty() || p2.empty() || predicate_expr.empty())
        {
            return json(nullptr);
        }

        json list_val = Evaluator::evaluate(list_expr, input, eval_ctx);
        if (list_val.is_null()) return json(nullptr);
        if (!list_val.is_array()) return json(nullptr);

        std::vector<json> items;
        items.reserve(list_val.size());
        for (const auto& v : list_val)
        {
            items.push_back(v);
        }

        bool invalid_comparator = false;
        std::stable_sort(items.begin(), items.end(), [&](const json& a, const json& b) {
            if (invalid_comparator)
            {
                return false;
            }

            json local_ctx = input;
            if (!local_ctx.is_object())
            {
                local_ctx = json::object();
            }
            local_ctx[p1] = a;
            local_ctx[p2] = b;

            try
            {
                json pred = Evaluator::evaluate(predicate_expr, local_ctx, eval_ctx);
                if (!pred.is_boolean())
                {
                    invalid_comparator = true;
                    return false;
                }
                return pred.get<bool>();
            }
            catch (const std::exception&)
            {
                invalid_comparator = true;
                return false;
            }
        });

        if (invalid_comparator)
        {
            return json(nullptr);
        }

        json result = json::array();
        for (const auto& v : items)
        {
            result.push_back(v);
        }
        return result;
    }

    std::optional<json> try_evaluate_list_filter_or_projection(
        std::string_view expression,
        const json& input,
        const EvaluationContext& eval_ctx)
    {
        std::string expr = trim_copy(expression);

        // Projection: <listExpr>.<property>
        if (auto dot_pos = find_last_top_level_char(expr, '.'); dot_pos.has_value())
        {
            const size_t pos = dot_pos.value();
            if (pos > 0 && pos + 1 < expr.size())
            {
                std::string base_expr = trim_copy(std::string_view(expr).substr(0, pos));
                std::string prop_name = trim_copy(std::string_view(expr).substr(pos + 1));

                if (!base_expr.empty() && !prop_name.empty())
                {
                    // Restrict projection interception to list-shaped bases (e.g. [..].y)
                    // so numeric literals like -3.14 are not treated as property access.
                    if (base_expr.back() != ']')
                    {
                        return std::nullopt;
                    }

                    json base_val = Evaluator::evaluate(base_expr, input, eval_ctx);
                    if (base_val.is_array())
                    {
                        json out = json::array();
                        for (const auto& item : base_val)
                        {
                            if (item.is_object())
                            {
                                if (auto it = item.find(prop_name); it != item.end())
                                {
                                    out.push_back(*it);
                                }
                                else
                                {
                                    out.push_back(nullptr);
                                }
                            }
                            else
                            {
                                out.push_back(nullptr);
                            }
                        }
                        return out;
                    }
                }
            }
        }

        // Filter: <listExpr>[<predicate>]
        size_t rbracket = expr.rfind(']');
        if (rbracket == std::string::npos || rbracket != expr.size() - 1)
        {
            return std::nullopt;
        }

        auto lbracket_opt = find_last_top_level_char(expr, '[');
        if (!lbracket_opt.has_value())
        {
            return std::nullopt;
        }

        size_t lbracket = lbracket_opt.value();
        if (lbracket == 0 || lbracket + 1 >= rbracket)
        {
            return std::nullopt;
        }

        std::string base_expr = trim_copy(std::string_view(expr).substr(0, lbracket));
        std::string selector_expr = trim_copy(std::string_view(expr).substr(lbracket + 1, rbracket - lbracket - 1));
        if (base_expr.empty() || selector_expr.empty())
        {
            return std::nullopt;
        }

        json base_val = Evaluator::evaluate(base_expr, input, eval_ctx);
        if (base_val.is_null() || !base_val.is_array())
        {
            return json(nullptr);
        }

        // Parse the selector once and reuse the AST for every element. Calling
        // Evaluator::evaluate per item re-ran the lexer, the parser and the
        // string pre-passes for each element of the list.
        Lexer selector_lexer;
        Parser selector_parser;
        std::unique_ptr<ASTNode> selector_ast;
        try
        {
            const auto selector_tokens = selector_lexer.tokenize(selector_expr);
            selector_ast = selector_parser.parse(selector_tokens);
        }
        catch (const std::exception&)
        {
            return std::nullopt; // Not something we can handle here
        }

        json out = json::array();
        json local_ctx = input.is_object() ? input : json::object();
        std::vector<std::string> injected_keys;

        for (const auto& item : base_val)
        {
            // Undo the previous item's injections so its properties do not leak
            // into the predicate evaluation of subsequent items.
            for (const auto& key : injected_keys)
            {
                if (auto original = input.find(key); original != input.end())
                {
                    local_ctx[key] = *original;
                }
                else
                {
                    local_ctx.erase(key);
                }
            }
            injected_keys.clear();

            local_ctx["item"] = item;
            if (item.is_object())
            {
                for (auto it = item.begin(); it != item.end(); ++it)
                {
                    local_ctx[it.key()] = it.value();
                    injected_keys.push_back(it.key());
                }
            }

            try
            {
                json pred = selector_ast->evaluate(local_ctx, eval_ctx);
                if (pred.is_boolean() && pred.get<bool>())
                {
                    out.push_back(item);
                }
            }
            catch (const std::exception&)
            {
                // Missing properties or type errors in selector evaluate as non-match for filtering.
            }
        }

        return out;
    }

    std::optional<json> try_evaluate_list_replace_match_overload(
        std::string_view expression,
        const json& input,
        const EvaluationContext& eval_ctx)
    {
        std::string expr = trim_copy(expression);
        if (!starts_with_ci(expr, "list replace") && !starts_with_ci(expr, "replace"))
        {
            return std::nullopt;
        }

        size_t lparen = expr.find('(');
        size_t rparen = expr.rfind(')');
        if (lparen == std::string::npos || rparen == std::string::npos || rparen <= lparen)
        {
            return std::nullopt;
        }

        std::string args_str = expr.substr(lparen + 1, rparen - lparen - 1);
        auto args = split_top_level_arguments(args_str);
        if (args.size() != 3)
        {
            return std::nullopt;
        }

        std::string list_expr;
        std::string match_expr;
        std::string new_item_expr;

        bool has_named = (args[0].find(':') != std::string::npos) ||
                         (args[1].find(':') != std::string::npos) ||
                         (args[2].find(':') != std::string::npos);

        if (has_named)
        {
            for (const auto& arg : args)
            {
                size_t colon = arg.find(':');
                if (colon == std::string::npos) return json(nullptr);

                std::string key = trim_copy(arg.substr(0, colon));
                std::string value = trim_copy(arg.substr(colon + 1));

                if (key == "list") list_expr = value;
                else if (key == "match") match_expr = value;
                else if (key == "newItem") new_item_expr = value;
                else return json(nullptr);
            }
            if (list_expr.empty() || match_expr.empty() || new_item_expr.empty())
            {
                return json(nullptr);
            }
        }
        else
        {
            list_expr = trim_copy(args[0]);
            match_expr = trim_copy(args[1]);
            new_item_expr = trim_copy(args[2]);
        }

        if (!starts_with_ci(match_expr, "function"))
        {
            return std::nullopt;
        }

        size_t fn_lparen = match_expr.find('(');
        size_t fn_rparen = match_expr.find(')', fn_lparen == std::string::npos ? 0 : fn_lparen + 1);
        if (fn_lparen == std::string::npos || fn_rparen == std::string::npos || fn_rparen <= fn_lparen)
        {
            return json(nullptr);
        }

        std::string params_str = match_expr.substr(fn_lparen + 1, fn_rparen - fn_lparen - 1);
        auto params = split_top_level_arguments(params_str);
        if (params.size() != 2)
        {
            return json(nullptr);
        }
        std::string p1 = trim_copy(params[0]);
        std::string p2 = trim_copy(params[1]);
        if (p1.empty() || p2.empty())
        {
            return json(nullptr);
        }

        std::string predicate_expr = trim_copy(match_expr.substr(fn_rparen + 1));
        if (predicate_expr.empty())
        {
            return json(nullptr);
        }

        json list_val = Evaluator::evaluate(list_expr, input, eval_ctx);
        json new_item_val = Evaluator::evaluate(new_item_expr, input, eval_ctx);
        if (list_val.is_null()) return json(nullptr);
        if (!list_val.is_array())
        {
            list_val = json::array({list_val});
        }

        json result = list_val;
        for (size_t i = 0; i < list_val.size(); ++i)
        {
            json local_ctx = input;
            if (!local_ctx.is_object())
            {
                local_ctx = json::object();
            }
            local_ctx[p1] = list_val[i];
            local_ctx[p2] = new_item_val;

            json pred = Evaluator::evaluate(predicate_expr, local_ctx, eval_ctx);
            if (!pred.is_boolean())
            {
                return json(nullptr);
            }

            if (pred.get<bool>())
            {
                result[i] = new_item_val;
            }
        }

        return result;
    }

    } // namespace

    json Evaluator::evaluate(std::string_view expression, const json& input, const EvaluationContext& eval_ctx)
    {
        if (auto sort_with_lambda = try_evaluate_sort_with_lambda_precedes(expression, input, eval_ctx);
            sort_with_lambda.has_value())
        {
            return sort_with_lambda.value();
        }

        if (auto filter_or_projection = try_evaluate_list_filter_or_projection(expression, input, eval_ctx);
            filter_or_projection.has_value())
        {
            return filter_or_projection.value();
        }

        if (auto list_replace_match = try_evaluate_list_replace_match_overload(expression, input, eval_ctx);
            list_replace_match.has_value())
        {
            return list_replace_match.value();
        }

        // AST-based evaluation path (all FEEL features supported)
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
            throw std::runtime_error(std::string("FEEL expression evaluation failed: ").append(expression) + " - " + e.what());
        }
    }
}
