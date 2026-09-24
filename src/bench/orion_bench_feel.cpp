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
 * @file orion_bench_feel.cpp
 * @brief Direct FEEL expression benchmarks for the parse-and-evaluate hot path.
 *
 * Scope
 * -----
 * These benchmarks target `feel::Evaluator::evaluate()` directly, bypassing the
 * DMN engine, so that lexing + parsing + AST evaluation are measured without
 * decision-table overhead.
 *
 * Tracks per expression case
 * --------------------------
 *   Tokenize/<case>  -- lexer only
 *   Parse/<case>     -- lexer + parser (AST construction, no evaluation)
 *   Eval/<case>      -- full parse + evaluate, COLD  (primary metric)
 *   EvalWarm/<case>  -- full parse + evaluate, one fixed expression string
 *
 * Cold vs warm
 * ------------
 * The cold tracks cycle through a ring of textually distinct but structurally
 * identical expressions. Any expression/AST cache keyed on the expression text
 * cannot serve these from cache, so parsing stays inside the measurement.
 * The warm track reuses a single string and is reported separately.
 *
 * Running
 * -------
 *   .\build\Release\orion-bench-feel.exe
 *   .\build\Release\orion-bench-feel.exe --benchmark_filter=Eval/
 *   .\build\Release\orion-bench-feel.exe --benchmark_repetitions=20 \
 *       --benchmark_report_aggregates_only=true \
 *       --benchmark_out=feel.json --benchmark_out_format=json
 */

#include <benchmark/benchmark.h>
#include <orion/api/logger.hpp>
#include <orion/bre/ast_node.hpp>
#include <orion/bre/evaluation_context.hpp>
#include <orion/bre/feel/evaluator.hpp>
#include <orion/bre/feel/lexer.hpp>
#include <orion/bre/feel/parser.hpp>
#include <orion/bre/feel/regex_cache.hpp>
#include <nlohmann/json.hpp>

#include <cstdio>
#include <functional>
#include <memory>
#include <string>
#include <vector>

using json = nlohmann::json;
using orion::bre::EvaluationContext;
using orion::bre::feel::Evaluator;
using orion::bre::feel::Lexer;
using orion::bre::feel::Parser;
using orion::bre::feel::RegexCache;

namespace
{

/// Ring size must stay a power of two so the index step is a mask, not a modulo.
constexpr std::size_t k_ring_size = 64;

EvaluationContext& eval_ctx()
{
    static RegexCache cache;
    static EvaluationContext ctx(cache);
    return ctx;
}

struct FeelCase
{
    const char* name;
    std::function<std::string(int)> make_expr;
    json input;
};

struct PreparedCase
{
    std::string name;
    std::vector<std::string> ring;
    json input;
};

std::string two_digit(int n)
{
    char buf[8];
    std::snprintf(buf, sizeof(buf), "%02d", n);
    return buf;
}

std::vector<FeelCase> make_cases()
{
    const json empty = json::object();
    const json person = json{{"age", 30}, {"priority", 7}};

    std::vector<FeelCase> cases;

    cases.push_back({"arith_simple",
                     [](int n) { return "1 + " + std::to_string(n) + " * 2 - 3"; },
                     empty});

    cases.push_back({"arith_nested",
                     [](int n) {
                         const std::string v = std::to_string(n);
                         return "((1 + " + v + ") * (3 - 4)) / (2 + " + v + ")";
                     },
                     empty});

    cases.push_back({"var_compare",
                     [](int n) { return "age >= " + std::to_string(n) + " and priority > 5"; },
                     person});

    cases.push_back({"conditional",
                     [](int n) {
                         return "if age > " + std::to_string(n) + R"( then "high" else "low")";
                     },
                     person});

    cases.push_back({"func_nested",
                     [](int n) { return "abs(floor(-" + std::to_string(n) + ".5)) + sqrt(16)"; },
                     empty});

    cases.push_back({"string_funcs",
                     [](int n) {
                         return R"(string length(upper case("abc)" + std::to_string(n) + R"(")))";
                     },
                     empty});

    cases.push_back({"list_aggregate",
                     [](int n) {
                         return "sum([1, 2, " + std::to_string(n) + ", 4]) + count([1, 2, 3])";
                     },
                     empty});

    cases.push_back({"list_filter",
                     [](int n) { return "[1, 2, 3, 4, " + std::to_string(n) + "][item > 2]"; },
                     empty});

    cases.push_back({"context_access",
                     [](int n) { return "{a: " + std::to_string(n) + ", b: 2}.a"; },
                     empty});

    cases.push_back({"temporal_compare",
                     [](int n) {
                         return R"(date("2023-01-)" + two_digit(1 + (n % 28)) +
                                R"(") < date("2024-01-01"))";
                     },
                     empty});

    cases.push_back({"regex_matches",
                     [](int n) {
                         return R"(matches("abc)" + std::to_string(n) + R"(", "abc[0-9]+"))";
                     },
                     empty});

    cases.push_back({"quantified",
                     [](int n) {
                         return "some x in [1, 2, " + std::to_string(n) + "] satisfies x > 2";
                     },
                     empty});

    cases.push_back({"for_loop",
                     [](int n) {
                         return "for x in [1, 2, " + std::to_string(n) + "] return x * 2";
                     },
                     empty});

    return cases;
}

std::vector<std::unique_ptr<PreparedCase>> g_cases;

// Evaluating once up-front both validates the expression and keeps first-call
// lazy initialisation (regex JIT, static tables) out of the timed region.
bool warm_up(const PreparedCase& pc, std::string& error)
{
    try
    {
        for (const auto& expr : pc.ring)
        {
            json result = Evaluator::evaluate(expr, pc.input, eval_ctx());
            benchmark::DoNotOptimize(result);
        }
    }
    catch (const std::exception& e)
    {
        error = e.what();
        return false;
    }
    return true;
}

void bm_tokenize(benchmark::State& state, const PreparedCase* pc)
{
    Lexer lexer;
    std::size_t i = 0;
    for (auto _ : state)
    {
        auto tokens = lexer.tokenize(pc->ring[i]);
        benchmark::DoNotOptimize(tokens);
        i = (i + 1) & (k_ring_size - 1);
    }
}

void bm_parse(benchmark::State& state, const PreparedCase* pc)
{
    Lexer lexer;
    std::size_t i = 0;
    for (auto _ : state)
    {
        auto tokens = lexer.tokenize(pc->ring[i]);
        Parser parser;
        auto ast = parser.parse(tokens);
        benchmark::DoNotOptimize(ast);
        i = (i + 1) & (k_ring_size - 1);
    }
}

void bm_eval_cold(benchmark::State& state, const PreparedCase* pc)
{
    std::size_t i = 0;
    for (auto _ : state)
    {
        json result = Evaluator::evaluate(pc->ring[i], pc->input, eval_ctx());
        benchmark::DoNotOptimize(result);
        i = (i + 1) & (k_ring_size - 1);
    }
}

void bm_eval_warm(benchmark::State& state, const PreparedCase* pc)
{
    const std::string& expr = pc->ring[0];
    for (auto _ : state)
    {
        json result = Evaluator::evaluate(expr, pc->input, eval_ctx());
        benchmark::DoNotOptimize(result);
    }
}

void register_all()
{
    for (const auto& c : make_cases())
    {
        auto pc = std::make_unique<PreparedCase>();
        pc->name = c.name;
        pc->input = c.input;
        pc->ring.reserve(k_ring_size);
        for (int n = 0; n < static_cast<int>(k_ring_size); ++n)
        {
            pc->ring.push_back(c.make_expr(n));
        }

        std::string error;
        if (!warm_up(*pc, error))
        {
            std::fprintf(stderr, "SKIP %s: %s\n", pc->name.c_str(), error.c_str());
            continue;
        }

        const PreparedCase* raw = pc.get();
        benchmark::RegisterBenchmark(("Tokenize/" + pc->name).c_str(), bm_tokenize, raw)
            ->Unit(benchmark::kNanosecond);
        benchmark::RegisterBenchmark(("Parse/" + pc->name).c_str(), bm_parse, raw)
            ->Unit(benchmark::kNanosecond);
        benchmark::RegisterBenchmark(("Eval/" + pc->name).c_str(), bm_eval_cold, raw)
            ->Unit(benchmark::kNanosecond);
        benchmark::RegisterBenchmark(("EvalWarm/" + pc->name).c_str(), bm_eval_warm, raw)
            ->Unit(benchmark::kNanosecond);

        g_cases.push_back(std::move(pc));
    }
}

} // namespace

int main(int argc, char** argv)
{
    orion::api::Logger::instance().set_logger(nullptr);

    register_all();

    ::benchmark::Initialize(&argc, argv);
    if (::benchmark::ReportUnrecognizedArguments(argc, argv)) return 1;
    ::benchmark::RunSpecifiedBenchmarks();
    ::benchmark::Shutdown();
    return 0;
}
