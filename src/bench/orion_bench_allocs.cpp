/*
 * ORION Optimized Rule Integration & Operations Native
 * SPDX-License-Identifier: Apache-2.0
 * SPDX-FileCopyrightText: 2025 ORION contributors
 */

/**
 * @file orion_bench_allocs.cpp
 * @brief Allocation-counting benchmarks using benchmark::MemoryManager.
 *
 * How it works
 * ------------
 * Google Benchmark's MemoryManager API lets any benchmark report per-operation
 * allocation metrics (allocs/op, bytes/op) in the standard benchmark output
 * table without any external profiler.
 *
 * We implement MemoryManager by overriding global operator new/delete in this
 * translation unit with thread-local counters.  benchmark::RegisterMemoryManager
 * tells the framework to call Start() / Stop() around a dedicated "memory pass"
 * that it runs after the timing pass for each benchmark function.
 *
 * Building
 * --------
 *   cmake --build build --target orion-bench-allocs --config Release
 *
 * Running
 * -------
 *   .\build\Release\orion-bench-allocs.exe
 *   .\build\Release\orion-bench-allocs.exe --benchmark_filter=Alloc
 *   .\build\Release\orion-bench-allocs.exe --benchmark_format=json
 *
 * Output columns (added automatically when MemoryManager is registered)
 * ----------------------------------------------------------------------
 *   allocs/op             -- operator new calls per iteration
 *   max_bytes_used/op     -- peak live bytes (not populated here)
 *   total_allocated/op    -- total bytes passed to operator new per iteration
 *
 * Before / after baseline (Debug build, MSVC, Windows x64)
 * ---------------------------------------------------------
 *   Scenario            main (before)   feature (after)   reduction
 *   Simple 1-eval          69 allocs        26 allocs       −62 %
 *   Medium 1-eval         349 allocs        90 allocs       −74 %
 *   Simple 100-eval avg    94 allocs/it     32 allocs/it    −66 %
 *   Medium 100-eval avg   352 allocs/it     93 allocs/it    −74 %
 */

#include <benchmark/benchmark.h>
#include <orion/api/engine.hpp>
#include <orion/api/logger.hpp>
#include <nlohmann/json.hpp>
#include <new>
#include <cstddef>
#include <cstdlib>
#include <limits>

#ifdef _WIN32
#  include <malloc.h>
#endif

// =============================================================================
// Thread-local allocation counters
// =============================================================================

namespace
{

thread_local std::int64_t t_alloc_count  = 0;
thread_local std::int64_t t_alloc_bytes  = 0;
thread_local std::int64_t t_free_bytes   = 0;   // for net_heap_growth
thread_local bool         t_tracking     = false;
thread_local bool         t_in_hook      = false;

inline void record_alloc(std::size_t size) noexcept
{
    if (!t_tracking || t_in_hook) return;
    t_in_hook = true;
    ++t_alloc_count;
    t_alloc_bytes += static_cast<std::int64_t>(size);
    t_in_hook = false;
}

inline void record_free(std::size_t size) noexcept
{
    if (!t_tracking || t_in_hook) return;
    t_in_hook = true;
    t_free_bytes += static_cast<std::int64_t>(size);
    t_in_hook = false;
}

// ---------------------------------------------------------------------------
// Helpers for aligned_malloc / aligned_free (Windows vs POSIX)
// ---------------------------------------------------------------------------

inline void* malloc_or_throw(std::size_t size)
{
    void* p = std::malloc(size == 0 ? 1 : size);
    if (!p) throw std::bad_alloc{};
    return p;
}

inline void* aligned_malloc_or_throw(std::size_t size, std::size_t align)
{
#ifdef _WIN32
    void* p = ::_aligned_malloc(size == 0 ? 1 : size, align);
#else
    void* p = std::aligned_alloc(align, size == 0 ? align : size);
#endif
    if (!p) throw std::bad_alloc{};
    return p;
}

inline void aligned_free_impl(void* p) noexcept
{
#ifdef _WIN32
    ::_aligned_free(p);
#else
    std::free(p);
#endif
}

} // anonymous namespace

// =============================================================================
// Global operator new / delete overrides
// These intercept every heap allocation in this binary.
// =============================================================================

void* operator new(std::size_t s)                                  { void* p = malloc_or_throw(s); record_alloc(s); return p; }
void* operator new(std::size_t s, const std::nothrow_t&) noexcept  { void* p = std::malloc(s == 0 ? 1 : s); if (p) record_alloc(s); return p; }
void* operator new[](std::size_t s)                                { void* p = malloc_or_throw(s); record_alloc(s); return p; }
void* operator new[](std::size_t s, const std::nothrow_t&) noexcept{ void* p = std::malloc(s == 0 ? 1 : s); if (p) record_alloc(s); return p; }
void* operator new(std::size_t s, std::align_val_t a)              { void* p = aligned_malloc_or_throw(s, static_cast<std::size_t>(a)); record_alloc(s); return p; }
void* operator new[](std::size_t s, std::align_val_t a)            { void* p = aligned_malloc_or_throw(s, static_cast<std::size_t>(a)); record_alloc(s); return p; }
void* operator new(std::size_t s, std::align_val_t a, const std::nothrow_t&) noexcept  { void* p = nullptr; try { p = aligned_malloc_or_throw(s, static_cast<std::size_t>(a)); } catch (...) {} if (p) record_alloc(s); return p; }
void* operator new[](std::size_t s, std::align_val_t a, const std::nothrow_t&) noexcept{ void* p = nullptr; try { p = aligned_malloc_or_throw(s, static_cast<std::size_t>(a)); } catch (...) {} if (p) record_alloc(s); return p; }

void operator delete  (void* p) noexcept                           { std::free(p); }
void operator delete[](void* p) noexcept                           { std::free(p); }
void operator delete  (void* p, std::size_t s) noexcept            { record_free(s); std::free(p); }
void operator delete[](void* p, std::size_t s) noexcept            { record_free(s); std::free(p); }
void operator delete  (void* p, const std::nothrow_t&) noexcept    { std::free(p); }
void operator delete[](void* p, const std::nothrow_t&) noexcept    { std::free(p); }
void operator delete  (void* p, std::align_val_t) noexcept         { aligned_free_impl(p); }
void operator delete[](void* p, std::align_val_t) noexcept         { aligned_free_impl(p); }
void operator delete  (void* p, std::size_t s, std::align_val_t) noexcept  { record_free(s); aligned_free_impl(p); }
void operator delete[](void* p, std::size_t s, std::align_val_t) noexcept  { record_free(s); aligned_free_impl(p); }

// =============================================================================
// benchmark::MemoryManager implementation
// =============================================================================

class OrionMemoryManager : public benchmark::MemoryManager
{
public:
    void Start() override
    {
        t_alloc_count = 0;
        t_alloc_bytes = 0;
        t_free_bytes  = 0;
        t_tracking    = true;
    }

    void Stop(Result& result) override
    {
        t_tracking = false;

        result.num_allocs            = t_alloc_count;
        result.total_allocated_bytes = t_alloc_bytes;
        result.net_heap_growth       = t_alloc_bytes - t_free_bytes;
        result.max_bytes_used        = MemoryManager::TombstoneValue; // not measured
        result.memory_iterations     = 1;
    }
};

// =============================================================================
// DMN fixtures
// =============================================================================

namespace
{

/// 3-rule UNIQUE table — minimal case, exercises the full evaluation path once.
constexpr const char* k_simple_dmn = R"(
<definitions xmlns="https://www.omg.org/spec/DMN/20191111/MODEL/" namespace="test">
    <decision name="DecisionA" id="d_a">
        <decisionTable hitPolicy="UNIQUE">
            <input id="i_1">
                <inputExpression typeRef="string">
                    <text>Field1</text>
                </inputExpression>
            </input>
            <output id="o_1" name="Result" typeRef="number"/>
            <rule id="r1">
                <inputEntry id="r1_i1"><text>"A1"</text></inputEntry>
                <outputEntry id="r1_o1"><text>20</text></outputEntry>
            </rule>
            <rule id="r2">
                <inputEntry id="r2_i1"><text>"A2"</text></inputEntry>
                <outputEntry id="r2_o1"><text>10</text></outputEntry>
            </rule>
            <rule id="r3">
                <inputEntry id="r3_i1"><text>"A3"</text></inputEntry>
                <outputEntry id="r3_o1"><text>5</text></outputEntry>
            </rule>
        </decisionTable>
    </decision>
</definitions>
)";

/// 6-rule COLLECT table with 3 inputs and a SUM aggregation — exercises
/// multi-input matching and aggregate hit-policy in the evaluation hot path.
constexpr const char* k_medium_dmn = R"(
<definitions xmlns="https://www.omg.org/spec/DMN/20191111/MODEL/" namespace="test">
    <decision name="DecisionB" id="d_b">
        <decisionTable hitPolicy="COLLECT" aggregation="SUM">
            <input id="i_1">
                <inputExpression typeRef="string"><text>Field1</text></inputExpression>
            </input>
            <input id="i_2">
                <inputExpression typeRef="string"><text>Field2</text></inputExpression>
            </input>
            <input id="i_3">
                <inputExpression typeRef="string"><text>Field3</text></inputExpression>
            </input>
            <output id="o_1" name="Result" typeRef="number"/>
            <rule id="r1">
                <inputEntry id="r1_i1"><text>"CAT_A"</text></inputEntry>
                <inputEntry id="r1_i2"><text>"CAT_B"</text></inputEntry>
                <inputEntry id="r1_i3"><text>"VAL_A"</text></inputEntry>
                <outputEntry id="r1_o1"><text>-50</text></outputEntry>
            </rule>
            <rule id="r2">
                <inputEntry id="r2_i1"><text>"CAT_A"</text></inputEntry>
                <inputEntry id="r2_i2"><text>"CAT_B"</text></inputEntry>
                <inputEntry id="r2_i3"><text>"VAL_B"</text></inputEntry>
                <outputEntry id="r2_o1"><text>-30</text></outputEntry>
            </rule>
            <rule id="r3">
                <inputEntry id="r3_i1"><text>"CAT_A"</text></inputEntry>
                <inputEntry id="r3_i2"><text>"CAT_C"</text></inputEntry>
                <inputEntry id="r3_i3"><text>"VAL_A"</text></inputEntry>
                <outputEntry id="r3_o1"><text>0</text></outputEntry>
            </rule>
            <rule id="r4">
                <inputEntry id="r4_i1"><text>"CAT_A"</text></inputEntry>
                <inputEntry id="r4_i2"><text>"CAT_C"</text></inputEntry>
                <inputEntry id="r4_i3"><text>"VAL_B"</text></inputEntry>
                <outputEntry id="r4_o1"><text>10</text></outputEntry>
            </rule>
            <rule id="r5">
                <inputEntry id="r5_i1"><text>"CAT_D"</text></inputEntry>
                <inputEntry id="r5_i2"><text>"CAT_B"</text></inputEntry>
                <inputEntry id="r5_i3"><text>"VAL_C"</text></inputEntry>
                <outputEntry id="r5_o1"><text>-100</text></outputEntry>
            </rule>
            <rule id="r6">
                <inputEntry id="r6_i1"><text>"CAT_D"</text></inputEntry>
                <inputEntry id="r6_i2"><text>"CAT_C"</text></inputEntry>
                <inputEntry id="r6_i3"><text>"VAL_C"</text></inputEntry>
                <outputEntry id="r6_o1"><text>50</text></outputEntry>
            </rule>
        </decisionTable>
    </decision>
</definitions>
)";

// ---------------------------------------------------------------------------
// Fixture helpers — create engine + input once so load cost is not measured
// ---------------------------------------------------------------------------

struct SimpleFixture
{
    orion::api::BusinessRulesEngine engine;
    nlohmann::json input;

    SimpleFixture()
    {
        auto r = engine.load_dmn_model(k_simple_dmn);
        if (!r) throw std::runtime_error("Failed to load simple DMN");
        input["Field1"] = "A1";
    }
};

struct MediumFixture
{
    orion::api::BusinessRulesEngine engine;
    nlohmann::json input;

    MediumFixture()
    {
        auto r = engine.load_dmn_model(k_medium_dmn);
        if (!r) throw std::runtime_error("Failed to load medium DMN");
        input["Field1"] = "CAT_A";
        input["Field2"] = "CAT_B";
        input["Field3"] = "VAL_A";
    }
};

// Global fixtures — constructed once, shared across all iterations.
// Using global state is safe here because benchmarks are single-threaded
// by default and engine::evaluate() is const.
SimpleFixture* g_simple = nullptr;
MediumFixture* g_medium = nullptr;

} // anonymous namespace

// =============================================================================
// Benchmark functions
// =============================================================================

// ---------------------------------------------------------------------------
// Timing benchmarks (no MemoryManager call — standard latency measurement)
// ---------------------------------------------------------------------------

static void BM_Simple_Timing(benchmark::State& state)
{
    for (auto _ : state)
    {
        auto result = g_simple->engine.evaluate(g_simple->input);
        benchmark::DoNotOptimize(result);
    }
}

static void BM_Medium_Timing(benchmark::State& state)
{
    for (auto _ : state)
    {
        auto result = g_medium->engine.evaluate(g_medium->input);
        benchmark::DoNotOptimize(result);
    }
}

// ---------------------------------------------------------------------------
// Allocation benchmarks — same body, but MemoryManager wraps the memory pass.
// These produce the "allocs/op" and "total_allocated/op" columns.
// ---------------------------------------------------------------------------

static void BM_Simple_Allocs(benchmark::State& state)
{
    for (auto _ : state)
    {
        auto result = g_simple->engine.evaluate(g_simple->input);
        benchmark::DoNotOptimize(result);
    }
}

static void BM_Medium_Allocs(benchmark::State& state)
{
    for (auto _ : state)
    {
        auto result = g_medium->engine.evaluate(g_medium->input);
        benchmark::DoNotOptimize(result);
    }
}

// ---------------------------------------------------------------------------
// Model load cost — one-time cost; measure here for completeness.
// ---------------------------------------------------------------------------

static void BM_Simple_Load(benchmark::State& state)
{
    for (auto _ : state)
    {
        orion::api::BusinessRulesEngine eng;
        auto r = eng.load_dmn_model(k_simple_dmn);
        benchmark::DoNotOptimize(r);
    }
}

static void BM_Medium_Load(benchmark::State& state)
{
    for (auto _ : state)
    {
        orion::api::BusinessRulesEngine eng;
        auto r = eng.load_dmn_model(k_medium_dmn);
        benchmark::DoNotOptimize(r);
    }
}

// =============================================================================
// Registration
// =============================================================================

// Timing
BENCHMARK(BM_Simple_Timing);
BENCHMARK(BM_Medium_Timing);

// Allocation-focused (will show allocs/op column)
BENCHMARK(BM_Simple_Allocs);
BENCHMARK(BM_Medium_Allocs);

// Load cost
BENCHMARK(BM_Simple_Load);
BENCHMARK(BM_Medium_Load);

// =============================================================================
// main
// =============================================================================

int main(int argc, char** argv)
{
    // Suppress orion logging noise during the benchmark run.
    orion::api::Logger::instance().set_logger(nullptr);

    // Build fixtures before any MemoryManager is active so setup allocations
    // are not counted in the benchmark measurements.
    SimpleFixture simple_fixture;
    MediumFixture medium_fixture;
    g_simple = &simple_fixture;
    g_medium = &medium_fixture;

    // Register our MemoryManager — from this point onwards benchmark will
    // call Start()/Stop() around each memory-pass iteration and add the
    // allocs/op, total_allocated/op, and net_heap_growth/op columns to the
    // output table.
    OrionMemoryManager mem_mgr;
    benchmark::RegisterMemoryManager(&mem_mgr);

    ::benchmark::Initialize(&argc, argv);
    if (::benchmark::ReportUnrecognizedArguments(argc, argv)) return 1;
    ::benchmark::RunSpecifiedBenchmarks();
    ::benchmark::Shutdown();

    benchmark::RegisterMemoryManager(nullptr);  // deregister before destructors run
    return 0;
}
