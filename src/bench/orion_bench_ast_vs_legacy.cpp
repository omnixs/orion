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

/*
 * ORION Business Rules Engine - AST vs Legacy Benchmark
 * Compares performance of AST-based evaluation vs legacy string-based evaluation
 * Uses long-running TCK test cases that are normally skipped
 */

#include <benchmark/benchmark.h>
#include <orion/api/engine.hpp>
#include <orion/common/xml2json.hpp>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <sstream>

using namespace orion::api;
using json = nlohmann::json;
namespace fs = std::filesystem;

// Helper to find TCK root
static fs::path find_tck_root() {
    if (const char* env = std::getenv("ORION_TCK_ROOT")) {
        fs::path p(env);
        if (fs::exists(p / "TestCases")) return fs::canonical(p);
    }
    std::vector<fs::path> candidates = {
        fs::path("dat") / "dmn-tck",
        fs::path("..") / "dat" / "dmn-tck",
        fs::path("../../dat") / "dmn-tck"
    };
    for (auto& c : candidates) {
        if (fs::exists(c / "TestCases")) return fs::canonical(c);
    }
    fs::path cur = fs::current_path();
    for (int i = 0; i < 6; ++i) {
        fs::path probe = cur / "dat" / "dmn-tck";
        if (fs::exists(probe / "TestCases")) return fs::canonical(probe);
        if (cur.has_parent_path()) cur = cur.parent_path();
        else break;
    }
    return {};
}

// Helper to read file
static std::string read_file(const fs::path& p) {
    std::ifstream f(p, std::ios::binary);
    if (!f) throw std::runtime_error("Cannot open " + p.string());
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

// ============================================================================
// Benchmark: 0105-feel-math (33 test cases - arithmetic operations)
// Tests: Addition, subtraction, multiplication, division, exponentiation
// ============================================================================

static void BM_FeelMath_AST(benchmark::State& state) {
    auto tck_root = find_tck_root();
    if (tck_root.empty()) {
        state.SkipWithError("TCK root not found");
        return;
    }
    
    auto test_path = tck_root / "TestCases" / "compliance-level-2" / "0105-feel-math";
    auto dmn_file = test_path / "0105-feel-math.dmn";
    auto test_file = test_path / "0105-feel-math-test-01.xml";
    
    if (!fs::exists(dmn_file) || !fs::exists(test_file)) {
        state.SkipWithError("Test files not found");
        return;
    }
    
    std::string dmn_xml = read_file(dmn_file);
    std::string test_xml = read_file(test_file);
    
    // Parse test cases
    auto test_cases = orion::common::parse_test_xml(test_xml);
    if (test_cases.empty()) {
        state.SkipWithError("No test cases found");
        return;
    }
    
    // Load model once (Phase 3: parse and cache AST)
    BusinessRulesEngine engine;
    std::string error;
    if (!engine.load_dmn_model(dmn_xml, error)) {
        state.SkipWithError(("Failed to load model: " + error).c_str());
        return;
    }
    
    // Benchmark: Evaluate all test cases repeatedly
    for (auto _ : state) {
        for (const auto& test_case : test_cases) {
            std::string input_json = test_case.input.dump();
            auto result = engine.evaluate(input_json);
            benchmark::DoNotOptimize(result);
        }
    }
    
    state.SetLabel("AST with caching");
    state.counters["test_cases"] = test_cases.size();
}
BENCHMARK(BM_FeelMath_AST)->Unit(benchmark::kMicrosecond);

// Benchmark with legacy evaluator (would need to disable AST at compile time)
// This is conceptual - actual legacy benchmark would require recompilation
static void BM_FeelMath_ComparisonNote(benchmark::State& state) {
    for (auto _ : state) {
        // Placeholder to document what we'd measure:
        // - Legacy: Parse expression on EVERY evaluation (33 test cases × iterations)
        // - AST: Parse ONCE during load_dmn_model, then reuse cached AST
        benchmark::DoNotOptimize(state.iterations());
    }
    state.SetLabel("Note: Legacy would parse 33× per iteration");
}
BENCHMARK(BM_FeelMath_ComparisonNote)->Unit(benchmark::kMicrosecond);

// ============================================================================
// Benchmark: 0106-feel-ternary-logic (18 test cases - null propagation)
// Tests: Logical AND, OR with null values (DMN ternary logic)
// ============================================================================

static void BM_TernaryLogic_AST(benchmark::State& state) {
    auto tck_root = find_tck_root();
    if (tck_root.empty()) {
        state.SkipWithError("TCK root not found");
        return;
    }
    
    auto test_path = tck_root / "TestCases" / "compliance-level-2" / "0106-feel-ternary-logic";
    auto dmn_file = test_path / "0106-feel-ternary-logic.dmn";
    auto test_file = test_path / "0106-feel-ternary-logic-test-01.xml";
    
    if (!fs::exists(dmn_file) || !fs::exists(test_file)) {
        state.SkipWithError("Test files not found");
        return;
    }
    
    std::string dmn_xml = read_file(dmn_file);
    std::string test_xml = read_file(test_file);
    
    auto test_cases = orion::common::parse_test_xml(test_xml);
    if (test_cases.empty()) {
        state.SkipWithError("No test cases found");
        return;
    }
    
    BusinessRulesEngine engine;
    std::string error;
    if (!engine.load_dmn_model(dmn_xml, error)) {
        state.SkipWithError(("Failed to load model: " + error).c_str());
        return;
    }
    
    for (auto _ : state) {
        for (const auto& test_case : test_cases) {
            std::string input_json = test_case.input.dump();
            auto result = engine.evaluate(input_json);
            benchmark::DoNotOptimize(result);
        }
    }
    
    state.SetLabel("AST with caching");
    state.counters["test_cases"] = test_cases.size();
}
BENCHMARK(BM_TernaryLogic_AST)->Unit(benchmark::kMicrosecond);

// ============================================================================
// Benchmark: Multi-output decision tables with different hit policies
// ============================================================================

static void BM_MultiOutput_CollectSum_AST(benchmark::State& state) {
    auto tck_root = find_tck_root();
    if (tck_root.empty()) {
        state.SkipWithError("TCK root not found");
        return;
    }
    
    auto test_path = tck_root / "TestCases" / "compliance-level-2" / "0115-sum-collect-hitpolicy";
    auto dmn_file = test_path / "0115-sum-collect-hitpolicy.dmn";
    auto test_file = test_path / "0115-sum-collect-hitpolicy-test-01.xml";
    
    if (!fs::exists(dmn_file) || !fs::exists(test_file)) {
        state.SkipWithError("Test files not found");
        return;
    }
    
    std::string dmn_xml = read_file(dmn_file);
    std::string test_xml = read_file(test_file);
    
    auto test_cases = orion::common::parse_test_xml(test_xml);
    if (test_cases.empty()) {
        state.SkipWithError("No test cases found");
        return;
    }
    
    BusinessRulesEngine engine;
    std::string error;
    if (!engine.load_dmn_model(dmn_xml, error)) {
        state.SkipWithError(("Failed to load model: " + error).c_str());
        return;
    }
    
    for (auto _ : state) {
        for (const auto& test_case : test_cases) {
            std::string input_json = test_case.input.dump();
            auto result = engine.evaluate(input_json);
            benchmark::DoNotOptimize(result);
        }
    }
    
    state.SetLabel("AST with caching");
    state.counters["test_cases"] = test_cases.size();
}
BENCHMARK(BM_MultiOutput_CollectSum_AST)->Unit(benchmark::kMicrosecond);

// ============================================================================
// Benchmark: String operations and concatenation
// ============================================================================

static void BM_StringConcat_AST(benchmark::State& state) {
    auto tck_root = find_tck_root();
    if (tck_root.empty()) {
        state.SkipWithError("TCK root not found");
        return;
    }
    
    auto test_path = tck_root / "TestCases" / "compliance-level-2" / "0008-LX-arithmetic";
    auto dmn_file = test_path / "0008-LX-arithmetic.dmn";
    auto test_file = test_path / "0008-LX-arithmetic-test-01.xml";
    
    if (!fs::exists(dmn_file) || !fs::exists(test_file)) {
        state.SkipWithError("Test files not found");
        return;
    }
    
    std::string dmn_xml = read_file(dmn_file);
    std::string test_xml = read_file(test_file);
    
    auto test_cases = orion::common::parse_test_xml(test_xml);
    if (test_cases.empty()) {
        state.SkipWithError("No test cases found");
        return;
    }
    
    BusinessRulesEngine engine;
    std::string error;
    if (!engine.load_dmn_model(dmn_xml, error)) {
        state.SkipWithError(("Failed to load model: " + error).c_str());
        return;
    }
    
    for (auto _ : state) {
        for (const auto& test_case : test_cases) {
            std::string input_json = test_case.input.dump();
            auto result = engine.evaluate(input_json);
            benchmark::DoNotOptimize(result);
        }
    }
    
    state.SetLabel("AST with caching");
    state.counters["test_cases"] = test_cases.size();
}
BENCHMARK(BM_StringConcat_AST)->Unit(benchmark::kMicrosecond);

// ============================================================================
// Summary benchmark showing Phase 3 benefit
// ============================================================================

static void BM_Phase3_MultiEvaluation_Benefit(benchmark::State& state) {
    auto tck_root = find_tck_root();
    if (tck_root.empty()) {
        state.SkipWithError("TCK root not found");
        return;
    }
    
    auto test_path = tck_root / "TestCases" / "compliance-level-2" / "0105-feel-math";
    auto dmn_file = test_path / "0105-feel-math.dmn";
    auto test_file = test_path / "0105-feel-math-test-01.xml";
    
    if (!fs::exists(dmn_file) || !fs::exists(test_file)) {
        state.SkipWithError("Test files not found");
        return;
    }
    
    std::string dmn_xml = read_file(dmn_file);
    std::string test_xml = read_file(test_file);
    
    auto test_cases = orion::common::parse_test_xml(test_xml);
    if (test_cases.empty()) {
        state.SkipWithError("No test cases found");
        return;
    }
    
    // Phase 3: Load once, evaluate many times (realistic production usage)
    BusinessRulesEngine engine;
    std::string error;
    
    auto start_load = std::chrono::high_resolution_clock::now();
    if (!engine.load_dmn_model(dmn_xml, error)) {
        state.SkipWithError(("Failed to load model: " + error).c_str());
        return;
    }
    auto end_load = std::chrono::high_resolution_clock::now();
    auto load_time_us = std::chrono::duration_cast<std::chrono::microseconds>(end_load - start_load).count();
    
    // Measure evaluation time (using cached AST)
    size_t total_evaluations = 0;
    for (auto _ : state) {
        for (const auto& test_case : test_cases) {
            std::string input_json = test_case.input.dump();
            auto result = engine.evaluate(input_json);
            benchmark::DoNotOptimize(result);
            total_evaluations++;
        }
    }
    
    state.SetLabel("Load once (parse+cache), evaluate many times");
    state.counters["load_time_us"] = load_time_us;
    state.counters["test_cases"] = test_cases.size();
    state.counters["total_evals"] = total_evaluations;
    state.counters["eval_per_iter"] = test_cases.size();
}
BENCHMARK(BM_Phase3_MultiEvaluation_Benefit)
    ->Unit(benchmark::kMicrosecond)
    ->Iterations(100);  // 100 iterations = 3,300 evaluations total (33 test cases)

BENCHMARK_MAIN();
