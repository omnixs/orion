# Performance Optimization Template

## Objective

Use profiling tools to identify and fix performance bottlenecks through data-driven optimization.

## Scope

Performance targets (specify in task instance):
- [ ] Component to optimize
- [ ] Expected improvement (e.g., 20% faster evaluation)
- [ ] Baseline metrics captured

## Key Process Steps

### 1. Establish Baseline
Follow [build instructions](../instructions/build.md) for Release build and [performance test instructions](../instructions/run_perf_tests.md) for benchmarking:

**Windows (PowerShell):**
```powershell
# Build Release with debug symbols (see build.md for CMake presets)
cmake --build build --config Release

# Capture baseline (see run_perf_tests.md for options and interpretation)
.\build\Release\orion-bench.exe --benchmark_repetitions=20 --benchmark_out=baseline.json
```

**Linux (Bash):**
```bash
# Build Release (see build.md for details)
cmake --build build-release -j$(nproc)

# Capture baseline (see run_perf_tests.md for options and interpretation)
./build-release/orion-bench --benchmark_repetitions=20 --benchmark_out=baseline.json
```

### 2. Profile with VTune (or equivalent)

**Windows (PowerShell):**
```powershell
# Run VTune hotspots analysis
vtune -collect hotspots -result-dir vtune_results/baseline -- .\build\Release\orion-bench.exe
```

**Linux (Bash):**
```bash
# Run VTune hotspots analysis (or perf on Linux)
vtune -collect hotspots -result-dir vtune_results/baseline -- ./build-release/orion-bench
# Alternative: perf record -g ./build-release/orion-bench
```

### 3. Identify Bottlenecks
- Analyze top hotspots (>10% CPU time)
- Trace call paths (top-down tree)
- Validate with benchmarks

### 4. Implement Optimization
- One optimization at a time
- Verify correctness first (tests pass)
- Measure improvement

### 5. Validate Results
Follow [performance test instructions](../instructions/run_perf_tests.md) for statistical analysis:

**Windows (PowerShell):**
```powershell
# Re-run benchmarks (see run_perf_tests.md for statistical requirements)
.\build\Release\orion-bench.exe --benchmark_repetitions=20 --benchmark_out=optimized.json

# Compare (requires Python with benchmark tools)
python tools\scripts\compare_benchmarks.py baseline.json optimized.json
```

**Linux (Bash):**
```bash
# Re-run benchmarks (see run_perf_tests.md for statistical requirements)
./build-release/orion-bench --benchmark_repetitions=20 --benchmark_out=optimized.json

# Compare (requires Python with benchmark tools)
python tools/scripts/compare_benchmarks.py baseline.json optimized.json
```

## Success Criteria

- [ ] Improvement >5% (statistically significant, p<0.05)
- [ ] No functional regressions
- [ ] Baseline comparison documented
- [ ] Code remains maintainable

## Critical Notes

- Measurement noise: Require p<0.05 for validity
- Profile production code path, not cold paths
- Test first, optimize second

## Retrospective

(This section will be filled after task completion with learnings and improvements)
