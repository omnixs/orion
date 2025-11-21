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
```powershell
# Build Release with debug symbols (see build.md for CMake presets)
cmake --preset release

# Capture baseline (see run_perf_tests.md for options and interpretation)
.\build\orion-bench.exe --benchmark_repetitions=20 --benchmark_out=baseline.json
```

### 2. Profile with VTune (or equivalent)
```powershell
# Run VTune hotspots analysis
vtune -collect hotspots -result-dir vtune_results/baseline -- .\build\orion-bench.exe
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
```powershell
# Re-run benchmarks (see run_perf_tests.md for statistical requirements)
.\build\orion-bench.exe --benchmark_repetitions=20 --benchmark_out=optimized.json

# Compare (requires Python with benchmark tools)
python tools\compare.py baseline.json optimized.json
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
