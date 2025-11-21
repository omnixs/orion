---
template: improve_perf.md
agent: performance-optimizer
status: not-started
optimization: "arena-allocation"
component: "AST node allocation"
category: performance
priority: high
estimated-effort: "16-24 hours"
expected-improvement: "20-30%"
---

# Task: Performance Optimization - Reduce AST Allocations

## Context

VTune profiling shows 53% of evaluation time spent in memory allocation during AST construction. Current implementation uses `std::unique_ptr<ASTNode>` with individual heap allocations for each node.

**Current Bottleneck:**
- AST nodes allocated individually via `new`
- Deep recursion (5-20 levels) causes pointer chasing
- LLC cache miss rate: 72% (poor locality)

**Proposed Solution:**
- Arena allocation using `std::pmr::monotonic_buffer_resource`
- Allocate all nodes in contiguous memory
- Expected improvement: 20-30% faster parsing/evaluation

## Scope

**Target Files:**
- `include/orion/bre/feel/parser.hpp` - Add arena parameter
- `src/bre/feel/parser.cpp` - Use arena for allocations
- `include/orion/bre/feel/ast_node.hpp` - Custom allocator support
- `src/bre/ast_node.cpp` - Arena-aware construction

**Baseline Metrics (from VTune):**
- Parse time: 2.45s (35.2% of total)
- Allocations: 1.2M calls to malloc
- LLC misses: 1.2M (72% miss rate)
- Peak memory: 128 MB

## Acceptance Criteria

- [ ] Arena allocation implemented for AST nodes
- [ ] Parse time reduced by >20%
- [ ] LLC miss rate reduced by >50%
- [ ] All tests pass (no functional regressions)
- [ ] Memory usage reduced or maintained
- [ ] Code follows CODING_STANDARDS.md Section 5 (Memory Management)

## Execution Instructions

**Using Custom Agent (Recommended):**
1. Switch to **performance-optimizer** agent from dropdown
2. Say: "Analyze performance and generate optimization plan for .github/tasks/perf_reduce_allocations.md"
3. Agent will:
   - Capture baseline benchmarks (20+ repetitions)
   - Profile with VTune (hotspots, memory access)
   - Identify bottlenecks with impact analysis
   - Generate prioritized optimization plan
   - Estimate expected improvements
   - Handoff to default agent for implementation
4. After reviewing plan, say: "Implement the optimizations" (switches to implementation agent)
5. After implementation:
   - Re-run benchmarks and compare
   - Validate with profiler
   - Check for regressions (unit + TCK tests)

**Manual Execution (Alternative):**
Follow the phase-by-phase approach below if not using custom agent.

### Phase 1: Baseline Capture
```powershell
# Build Release with debug symbols
cmake --preset release
cmake --build --preset release-build

# Capture baseline
.\build\orion-bench.exe --benchmark_repetitions=20 --benchmark_out=baseline_arena.json
```

### Phase 2: Arena Implementation
1. Create `ASTArena` class using `std::pmr::monotonic_buffer_resource`
2. Stack-allocate 64KB buffer per parse
3. Modify `Parser` constructor to accept arena
4. Replace `new ASTNode` with arena allocation
5. Update smart pointers to use custom deleters

### Phase 3: Profiling
```powershell
# Re-profile with VTune
vtune -collect hotspots -result-dir vtune_results/after_arena -- .\build\orion-bench.exe

# Compare
vtune -report comparison -result-dir vtune_results/baseline -result-dir vtune_results/after_arena
```

### Phase 4: Validation
```powershell
# Benchmarks
.\build\orion-bench.exe --benchmark_repetitions=20 --benchmark_out=arena_optimized.json

# Compare statistically
python tools\compare.py baseline_arena.json arena_optimized.json

# All tests
.\build\tst_orion.exe --log_level=test_suite
.\build\orion_tck_runner.exe --log_level=error
```

## Expected Impact

- **Performance:** 20-30% faster parsing, 1.5-2× overall speedup
- **Memory:** 25% reduction in peak usage
- **Cache Efficiency:** 50-70% reduction in LLC misses

## Progress Tracking
<!-- Agent will update during execution -->
- **Baseline captured:** 
- **VTune profiling:** 
- **Optimization plan:** 
- **Implementation status:** 
- **Validation status:** 

## Results
<!-- Agent will update after completion -->
### Baseline Metrics
- **Parse time:** 
- **Allocations:** 
- **LLC miss rate:** 
- **Peak memory:** 

### Final Metrics
- **Parse time:** 
- **Allocations:** 
- **LLC miss rate:** 
- **Peak memory:** 
- **Improvement:** 

### Test Results
- **Unit tests:** Pass / Fail
- **TCK tests:** Pass / Fail  
- **Regressions:** None / [list]

## Risks

- **Medium Risk:** Complex lifetime management with arena
- **Correctness:** Need careful testing of AST ownership
- **Regression:** Potential for subtle bugs in tree traversal

## Validation Strategy

- Create comprehensive unit tests for arena allocation
- Run full TCK suite (126/126 Level-2, 377/1887 Level-3)
- Measure with 20+ repetitions for statistical significance
- Revert if improvement <10% or any test failures

## Retrospective

(This section will be filled after task execution with:
- Actual improvement vs expected
- Unexpected challenges
- Refinements to profiling process
- Template improvements)
