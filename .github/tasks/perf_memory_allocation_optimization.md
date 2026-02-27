---
template: improve_perf.md
agent: performance-optimizer
status: not-started
category: performance
priority: critical
estimated-effort: "16-24 hours"
actual-effort: ""
---

# Task: Memory Allocation Optimization — Profiled Production Hotspots

Eliminate excessive heap allocations in the Orion evaluation hot path identified through production memory profiling of a host application request that triggers many `evaluate()` calls.

## Context

**Background and Motivation:**
- Production memory profiling on a single host-application request revealed **~4,340 allocations / ~315KB per `evaluate()` call** inside the Orion library code paths alone
- The profiled request triggers **27 `evaluate()` calls** via the host application, totaling **~117K allocations / ~8.5MB** just for the Orion rule engine portion
- The root causes are: redundant re-computation of immutable data, unnecessary JSON deep copies, and missing caching of parsed FEEL expressions
- All optimizations must preserve **identical behavior** — same outputs for same inputs

**Profiling Source:**
- File: `temp/memory_profiling.prof`  
- Format: `(#allocations, allocated_bytes)` with hierarchical stack traces
- Single host-application request triggering 27 decision table evaluations

**Related Tasks:**
- Different from `perf_reduce_allocations.md` (which targets arena allocation for AST node construction)
- This task targets **redundant computation and unnecessary copying** in the evaluation hot path

## Profiling Analysis Summary

### Allocation breakdown per `evaluate()` call (inside `orion::` namespace):

| Hotspot | Function | Allocations | Bytes | Root Cause |
|---------|----------|-------------|-------|------------|
| **H1** | `Lexer::tokenize` in `find_matching_rules` | 596 | 51KB | Re-lex/re-parse identical `inputExpression` per rule (N rules × M inputs) |
| **H2** | `ASTNode::evaluate` / `resolveVariable` | 405 + 217 | 24KB + 10KB | JSON copies from `resolveVariable` (4 speculative string allocs per call), JSON value copies |
| **H3** | `DecisionTable::evaluate` line 635 | 342 | 24KB | Deep-copy JSON result via `nlohmann::json::basic_json` copy ctor (`_Copy_nodes`) |
| **H4** | `DRGEvaluator::get_evaluation_order` | 313 | 21KB | Rebuilds full dependency graph + topological sort on **every** `evaluate()` call |
| **H5** | `evaluate_with_drg` line 292 | 498 | 35KB | Deep-copy JSON input context for result accumulation |
| **H6** | `evaluate_with_drg` line 272 | 542 | 38KB | DRG evaluation order + dependency graph recomputation |
| **H7** | `find_matching_rules` output building | 162 + 108 + 108 | 11KB + 7KB + 10KB | `matching_outputs` vector growth, JSON result copies |
| **H8** | `create_bkm_map` per literal decision | est. ~100 | est. ~5KB | Full BKM map copy on every literal decision evaluation |

## Scope

**Included in this task (8 optimizations):**

- [ ] **P0-A: Cache DRG evaluation order** — Compute `get_evaluation_order()` once at model load, store result
- [ ] **P0-B: Cache parsed ASTs for input expressions** — Pre-parse `inputExpression` at model load or first evaluation; reuse cached ASTs in `find_matching_rules`
- [ ] **P1-A: Cache BKM map** — Return `const&` from BKM manager instead of copying all BKMs every call
- [ ] **P1-B: Avoid JSON deep copy in `evaluate_with_drg`** — Mutate context in-place or use layered/overlay context instead of deep-copying input
- [ ] **P2-A: Lazy variable resolution in `resolveVariable`** — Try exact match first; only generate underscored/lowercase/nospace variants on miss
- [ ] **P2-B: Reserve token vector + avoid string copy in Lexer** — `reserve()` token vector, use `string_view` for `input_` member
- [ ] **P2-C: Use `string_view` for `Token::text`** — Non-owning reference into source expression instead of per-token heap allocation
- [ ] **P3-A: Return `const json&` / `const json*` from `get_value_from_label`** — Eliminate JSON value copies per input lookup

**Explicitly excluded:**
- Arena allocation for AST nodes (covered by `perf_reduce_allocations.md`)
- Changes to the FEEL parser grammar or AST structure
- Changes to the public API (`BusinessRulesEngine::evaluate` signature)
- Any change to output values or evaluation semantics

## Detailed Instructions

### Phase 0: Baseline Capture

1. **Build Release**
   - Follow [Build Instructions](../instructions/build.instructions.md)
   - `cmake --build build --config Release`

2. **Capture performance baseline**
   - Follow [Performance Test Instructions](../instructions/run_perf_tests.instructions.md)
   - `.\build\Release\orion-bench.exe --benchmark_repetitions=20 --benchmark_out=baseline_memopt.json`

3. **Run all tests to confirm green baseline**
   - Follow [Unit Test Instructions](../instructions/run_unit_tests.instructions.md)
   - Follow [TCK Test Instructions](../instructions/run_tck_tests.instructions.md)

### Phase 1: P0 — Critical Optimizations (Biggest Wins)

#### Step 1: Cache DRG Evaluation Order (P0-A)

**Target files:**
- `src/bre/drg_evaluator.cpp` — `get_evaluation_order()`, `build_dependency_graph()`, `topological_sort()`
- `include/orion/bre/drg_evaluator.hpp` — Add cached member

**What to change:**
- Add a `std::vector<std::string> cached_evaluation_order_` member to `DRGEvaluator`
- Compute and cache the evaluation order in the constructor (after `has_cycles()` check already builds the graph)
- Make `get_evaluation_order()` return `const std::vector<std::string>&` (const reference to cached data)
- Also cache the dependency graph if `build_dependency_graph()` is called from other places
- Consider also caching a lookup map (`std::unordered_map<std::string, const Decision*>`) to replace the O(N) linear `find_decision()` scan

**Expected impact:** Eliminates ~855 allocations / ~59KB per `evaluate_with_drg()` call

**Verification:** Build → Unit tests → TCK tests

#### Step 2: Cache Parsed ASTs for Input Expressions (P0-B)

**Target files:**
- `src/bre/dmn_model.cpp` — `find_matching_rules()` lines 110-116
- `include/orion/bre/dmn_model.hpp` — `DecisionTable` or `InputClause` struct
- `src/bre/feel/evaluator.cpp` — Same pattern for the `Evaluator::evaluate` path

**What to change:**
- `inputExpression` values are static per decision table (set at DMN load time, never change at evaluation time)
- For `DecisionTable::find_matching_rules()`: Pre-parse all `inputExpression` strings into ASTs at model load time (or lazily on first evaluation) and store the ASTs alongside the `InputClause`
- Add an `std::shared_ptr<ASTNode> inputExpression_ast` field to `InputClause` (or equivalent caching struct)
- Similarly, for `outputEntries_ast` — verify these are already cached (the field name suggests they may be); if not, cache them too
- For the FEEL `Evaluator::evaluate()` path: Consider adding an expression cache (`std::unordered_map<std::string, std::shared_ptr<ASTNode>>`) so repeated evaluations of the same expression reuse the parsed AST
- **CRITICAL:** All `inputExpression` values within a decision table are identical for the same column across all rules — this is the parsing that gets repeated N×M times

**Expected impact:** Eliminates ~596 allocations / ~51KB per evaluation (tokenize) + parser allocations

**Verification:** Build → Unit tests → TCK tests

### Phase 2: P1 — High Impact Optimizations

#### Step 3: Cache BKM Map (P1-A)

**Target files:**
- `src/bre/bkm_manager.cpp` — `create_bkm_map()`
- `include/orion/bre/bkm_manager.hpp`
- `src/api/engine.cpp` — `try_evaluate_literal_decision()`

**What to change:**
- BKMs are loaded at construction time and never change during evaluation
- Replace `create_bkm_map()` with a cached map built once at construction or first use
- Return `const std::map<std::string, BusinessKnowledgeModel>&` instead of a new map each time
- Alternatively, provide a `const BusinessKnowledgeModel* find_bkm(std::string_view name) const` method for direct lookup

**Expected impact:** Eliminates full map + string + vector copies per literal decision evaluation

**Verification:** Build → Unit tests → TCK tests

#### Step 4: Avoid JSON Deep Copy in `evaluate_with_drg` (P1-B)

**Target files:**
- `src/api/engine.cpp` — `evaluate_with_drg()` lines ~263-266, ~292

**What to change:**
- Currently: `json augmented_context = data;` deep-copies the entire input, then `augmented_context[name] = result;` for each decision result
- Option A (preferred): Build a thin overlay/adapter that wraps the original `data` as read-only and only stores new decision results in a separate map. During lookup, check the overlay first, then fall back to the original data.
- Option B (simpler): Move the input json into the augmented context if possible (requires caller cooperation), or accumulate results in a separate `json` object and merge at the end
- Option C (minimal): At minimum, use `json augmented_context = std::move(data)` if the caller doesn't need the original (check signature — if `const json&`, this won't work as-is)
- Whichever approach is chosen, the evaluation result must be identical

**Expected impact:** Eliminates ~500-1000 allocations / ~35-70KB per `evaluate_with_drg()` call

**Verification:** Build → Unit tests → TCK tests. **Pay extra attention** to tests with multiple dependent decisions (DRG chains).

### Phase 3: P2 — Medium Impact Optimizations

#### Step 5: Lazy Variable Resolution (P2-A)

**Target files:**
- `src/bre/ast_node.cpp` — `resolveVariable()` lines ~41-86

**What to change:**
- Currently creates 4 string variants (underscored, lowercase, lowercase+underscored, nospace) **unconditionally** before checking if any match
- Refactor to try exact match first (most common case), then generate variants one at a time only on miss
- Early return on first match to avoid generating remaining variants
- Consider caching the resolved variable name mapping if the same variable is looked up repeatedly

The resolution order should be:
1. Exact match → return immediately
2. Underscored variant → return if found
3. Lowercase variant → return if found
4. Lowercase+underscored → return if found
5. No-space variant → return if found
6. Return null

**Expected impact:** Eliminates 3 of 4 string allocations in the common case (exact match hits)

**Verification:** Build → Unit tests → TCK tests

#### Step 6: Optimize Lexer Token Vector (P2-B)

**Target files:**
- `src/bre/feel/lexer.cpp` — `tokenize()` lines ~87-143
- `include/orion/bre/feel/lexer.hpp` — `input_` member type

**What to change:**
- Add `tokens.reserve(expression.size() / 2)` or similar heuristic to avoid repeated vector reallocation
- Change `input_` from `std::string` to `std::string_view` (the caller owns the expression string; lexer only needs read access)
- Verify that `position_` and `input_` lifetimes are compatible (tokenize is synchronous, so `string_view` is safe)

**Expected impact:** Reduces vector reallocation from ~5-6 to 1, eliminates 1 string copy per tokenize

**Verification:** Build → Unit tests → TCK tests

#### Step 7: Use `string_view` for Token Text (P2-C)

**Target files:**
- `include/orion/bre/feel/lexer.hpp` — `Token` struct, `text` field
- `src/bre/feel/lexer.cpp` — All tokenization methods that set `text`
- Any code that consumes `Token::text` downstream (parser, evaluator)

**What to change:**
- Change `Token::text` from `std::string` to `std::string_view` referencing the source expression
- This requires that the source expression outlives the tokens (which it does — expressions are stored in the DMN model)
- Update all token creation sites to create `string_view` from the expression substring
- **CAUTION:** Verify no downstream code takes ownership of `Token::text` (e.g., stores it in a `std::string` member). If any code does, those specific paths need special handling.

**Expected impact:** Eliminates heap allocation per token (typical expression = 5-15 tokens)

**Verification:** Build → Unit tests → TCK tests. Run with **AddressSanitizer** to detect any use-after-free from dangling `string_view`.

### Phase 4: P3 — Lower Impact Optimizations

#### Step 8: Return const Reference from `get_value_from_label` (P3-A)

**Target files:**
- `include/orion/bre/dmn_model.hpp` — `get_value_from_label()`

**What to change:**
- Currently returns `nlohmann::json` by value, which deep-copies the JSON value
- Change to return `const nlohmann::json&` for the direct-lookup case (`ctx.find(label)`)
- For the dotted-path case (nested lookup), a value return is unavoidable — split into two code paths or return `std::optional<std::reference_wrapper<const json>>`
- Update all call sites to use `const auto&` to capture the return value

**Expected impact:** Eliminates JSON deep-copy per input variable lookup in the simple (non-dotted) case

**Verification:** Build → Unit tests → TCK tests

### Iterative Fix Loop (MANDATORY per step)

> **After EVERY step (Steps 1-8), execute this loop before moving to the next step.**

```
REPEAT:
  1. Build Debug  → if FAIL → analyze error → fix → GOTO 1
  2. Run unit tests → if FAIL → analyze failure → fix → GOTO 1
  3. Run TCK tests  → if FAIL → analyze failure → fix → GOTO 1
  4. If all pass → proceed to next step

  MAX RETRIES per step: 5
  If retry limit reached → REVERT all changes for this step, mark as SKIPPED, continue to next step
```

**Rules:**
- Never move to the next optimization step while the current step has failing tests
- Each retry must address the **specific** failure (read error output, trace root cause)
- If a fix for step N breaks a previously passing step, fix the regression before continuing
- After Steps 2 and 4 (end of P0 and P1), do a **full Release build + all tests** checkpoint
- Log each retry with: what failed, why, what was changed

### Phase 5: Final Verification

1. **Build Release** per [Build Instructions](../instructions/build.instructions.md)
2. **Run all unit tests** per [Unit Test Instructions](../instructions/run_unit_tests.instructions.md)
3. **Run all TCK tests** per [TCK Test Instructions](../instructions/run_tck_tests.instructions.md)
4. **Run performance benchmarks** per [Performance Test Instructions](../instructions/run_perf_tests.instructions.md)
   - Compare against baseline: `python tools/scripts/compare_benchmarks.py baseline_memopt.json optimized_memopt.json`
5. **Code review** per [Code Review Checklist](../instructions/code_review_checklist.instructions.md)

## Success Criteria

- [ ] All 8 optimizations implemented and verified
- [ ] All unit tests pass (279+ tests, 0 failures)
- [ ] All TCK tests pass (126/126 Level-2, no regressions)
- [ ] Performance benchmarks show measurable improvement (target: >30% reduction in allocations per evaluation)
- [ ] Evaluation output is **byte-for-byte identical** to pre-optimization output for all test cases
- [ ] Code follows [CODING_STANDARDS.md](../../CODING_STANDARDS.md)
- [ ] No new compiler warnings

## Risk Assessment

| Risk | Severity | Mitigation |
|------|----------|------------|
| `string_view` lifetime issues (P2-B, P2-C) | High | Use AddressSanitizer in debug builds; verify expression lifetimes |
| DRG evaluation order caching invalidation | Medium | Verify evaluation order is immutable after `load_dmn_model()`; no mutation path exists |
| `const json&` return creating dangling references | Medium | Ensure callers don't hold references across mutation points |
| JSON overlay/layered context correctness (P1-B) | Medium | Extensive DRG chain tests; verify decision interdependency results |
| BKM map caching thread safety | Low | Orion is currently single-threaded per engine instance; document assumption |

## Reference Documentation

- [Performance Template](../prompts/improve_perf.md) - Profiling and optimization methodology
- [CODING_STANDARDS.md](../../CODING_STANDARDS.md) - Project coding standards
- [Build Instructions](../instructions/build.instructions.md) - Build and configuration
- [Unit Test Instructions](../instructions/run_unit_tests.instructions.md) - Testing
- [TCK Test Instructions](../instructions/run_tck_tests.instructions.md) - Compliance
- [Performance Test Instructions](../instructions/run_perf_tests.instructions.md) - Benchmarking
- [Code Review Checklist](../instructions/code_review_checklist.instructions.md) - Quality gates
- Profiling data: `temp/memory_profiling.prof`

## Retrospective

(This section will be filled after task completion)

### What worked well:
- 

### What was unclear or problematic:
- 

### Suggestions for improvement:
- 

### Actual effort:
- 

### Blockers encountered:
- 
