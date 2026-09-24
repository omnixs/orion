---
template: improve_perf.md
agent: performance-optimizer
status: in-progress
optimization: "feel-parse-and-evaluate"
component: "FEEL expression evaluation (Evaluator::evaluate)"
category: performance
priority: high
estimated-effort: ""
actual-effort: ""
---

# Task: Performance Optimization — FEEL Parse-and-Evaluate Hot Path

## Objective

Reduce the latency of the one-shot FEEL evaluation path
`orion::bre::feel::Evaluator::evaluate(std::string_view, const json&, const EvaluationContext&)`,
which performs lexing, parsing, and AST evaluation on every call.

## Scope

**Target workload:** parse **plus** evaluate (cold expressions that force tokenization and parsing).
Cached/loaded-model evaluation is kept as a regression suite, not as the primary target.

**Included:**
- `src/bre/feel/evaluator.cpp` — entry point, fast-path probes, pre-scans
- `src/bre/feel/lexer.cpp` — tokenization
- `src/bre/feel/parser.cpp` — recursive-descent parsing / AST construction
- `src/bre/ast_node.cpp` — AST evaluation
- New direct FEEL benchmark target + allocation coverage

**Excluded:**
- New FEEL features or grammar changes that alter behavior
- Public API signature changes
- DRG / engine-level optimizations unrelated to FEEL
- Dependency or build-system changes beyond registering the benchmark target

## Success Criteria

- [ ] At least one primary cold parse-plus-evaluate benchmark improves > 2% CPU time with `p < 0.05`
- [ ] Aggregate target benchmark set improves
- [ ] No statistically significant regression > 2% in other FEEL or engine benchmarks
- [ ] No increase in `net_heap_growth`; no material allocation regression
- [ ] All unit tests pass (`*** No errors detected`)
- [ ] TCK regression check exit code is not 2 and not 3
- [ ] Public API and FEEL semantics unchanged
- [ ] Code follows `CODING_STANDARDS.md` and the code review checklist

## Baseline Metrics

| Metric | Value |
|---|---|
| Branch | `feature/feel-performance-optimization` |
| Compiler / config | MSVC 14.51 (VS 18), Release, x64-windows-static |
| Machine | 20 CPUs @ 1382 MHz reported |
| Unit tests | 478 cases, `*** No errors detected` |
| TCK | 3253/3549 (91.7%), exit code 1 (expected failures, no regression) |
| Benchmark files | `build/feel_base_r*.json`, `build/feel_opt_r*.json` |

### Allocation baseline (deterministic, direct FEEL path)

| Benchmark | allocs/iter |
|---|---|
| Tokenize `1 + 7 * 2 - 3` | 3.25 |
| Parse (incl. tokenize) | 18.25 |
| Eval arithmetic | 18.375 |
| Eval `age >= 21 and priority > 5` | 20.25 |

Decomposition showed parsing, not evaluation, dominates short-expression cost:
for `arith_simple` tokenize was ~210 ns and parse ~1311 ns of a ~1569 ns total.

## Execution Phases

1. Branch + task file, verified green baseline (build, unit tests, TCK)
2. Add `orion-bench-feel` direct FEEL benchmark (lexer / lexer+parser / full path; cold + warm)
3. Capture baseline timing + allocation JSON (>= 20 repetitions)
4. Profile and rank hotspots by measured contribution
5. Implement one optimization at a time, re-measuring after each
6. Full functional validation (unit + TCK)
7. Full performance validation + statistical comparison
8. Report, ask for user feedback, fill retrospective

## Progress Tracking

- **Baseline captured:** yes (build, unit tests, TCK, allocations, timing)
- **Benchmarks added:** `orion-bench-feel` (13 cases x Tokenize/Parse/Eval/EvalWarm) + 4 FEEL allocation benchmarks
- **Hotspots identified:** parser child-vector regrowth, evaluator probe string copies, lexer token-vector regrowth
- **Optimizations implemented:** 3 (see Results)
- **Validation status:** unit tests pass, TCK unchanged, no significant benchmark regressions

## Results

### Optimizations implemented

1. **Parser child-vector reserve** (`src/bre/feel/parser.cpp`)
   Fixed-arity node construction sites (`and`, `or`, comparison, additive,
   multiplicative, exponentiation, `in`, `between`, conditional) reserved exact
   child capacity instead of growing the vector on each `push_back`.
   Effect: parse allocations 18.25 -> 15.25 per expression.

2. **Evaluator probe rejection on a view** (`src/bre/feel/evaluator.cpp`)
   All three `try_evaluate_*` pre-parse probes copied the entire expression via
   `trim_copy` *before* their cheap prefix test. Added `trim_view` and moved the
   rejection ahead of any copy.
   Effect: 3 fewer allocations per evaluation for expressions longer than the
   SSO buffer (20.25 -> 17.25 on the comparison expression).

3. **Lexer token-vector reserve heuristic** (`src/bre/feel/lexer.cpp`)
   `size/4 + 1` under-reserved for operator-dense expressions, causing two to
   three regrowths. Changed to `size/3 + 3`.
   Effect: tokenize allocations 3.25 -> 1.25.

### Allocation results (deterministic)

| Benchmark | Before | After | Change |
|---|---|---|---|
| Tokenize | 3.25 | 1.25 | -61.5% |
| Parse | 18.25 | 13.25 | -27.4% |
| Eval arithmetic | 18.375 | 13.375 | -27.2% |
| Eval comparison | 20.25 | 13.25 | -34.6% |

`net_heap_growth` remained 0 for every benchmark.

### Timing results — cold parse-plus-evaluate (`Eval/*`)

Measured with interleaved A/B rounds using two preserved binaries, 20
repetitions each, CPU time.

| Case | Round 1 | Round 2 |
|---|---|---|
| var_compare | -16.48% *** | -17.76% *** |
| conditional | -13.19% *** | -16.92% *** |
| arith_nested | -16.25% *** | -13.73% *** |
| arith_simple | -8.74% *** | -14.39% *** |
| string_funcs | -9.57% *** | -12.39% *** |
| regex_matches | -11.38% *** | -9.29% *** |
| func_nested | -9.52% *** | -8.36% *** |
| temporal_compare | -14.46% *** | -8.31% *** |
| list_aggregate | -22.18% *** | -6.86% *** |
| quantified | -12.89% *** | -6.45% ** |
| for_loop | -9.27% *** | -1.60% (ns) |
| list_filter | -10.27% *** | +1.21% (ns) |
| context_access | -8.12% (ns) | -2.80% (ns) |
| **Average of significant** | **-12.85%** | **-11.45%** |

No statistically significant regressions in either round.

### Measurement methodology finding

The host is too noisy for single-shot comparisons. Running the **identical**
baseline binary twice produced apparent "significant regressions" of up to
+9.59% with `***` markers, because Google Benchmark's repetition stddev
captures within-run variance only and ignores between-run drift.

Consequences:
- All timing conclusions use interleaved A/B rounds with preserved binaries.
- The engine benchmark (`orion-bench`) comparison was **inconclusive within
  noise**: deltas flipped sign between rounds.
- Allocation counts were used as the decisive regression instrument for the
  engine path, since they are deterministic:
  `BM_Simple_Allocs` 19.25 allocs / 14776 bytes and `BM_Medium_Allocs`
  154.25 allocs / 124344 bytes were **identical** before and after, confirming
  the decision-table path (which uses cached ASTs) is unaffected.

### Accepted trade-off

In the diagnostic `Tokenize/`-only track, two of thirteen cases regressed
(`conditional` +6.5%, `regex_matches` +3.7%). These expressions are
string-literal heavy, so any reserve above `size/4` over-allocates for them.
The same expressions improve 13-17% end-to-end in the composite `Eval/` track,
and `Tokenize` is never a standalone production path. No composite
(`Parse`/`Eval`/`EvalWarm`) benchmark regresses significantly.

### Test Results

- **Unit tests:** 478 cases, `*** No errors detected`
- **TCK:** 3253/3549 passed, identical to baseline; exit code 1 (not 2, not 3)
- **Regressions:** none

## Risks

| Risk | Severity | Mitigation |
|---|---|---|
| `string_view` / AST lifetime issues | High | Focused lifetime tests; keep ownership explicit |
| Behavior change in FEEL semantics | High | Full unit suite + TCK regression check after each step |
| Benchmark measures cache instead of parsing | Medium | Cold expression ring defeats any expression cache |
| Measurement noise | Medium | >= 20 repetitions, aggregates, reject CV > 20% |

## Retrospective

(To be filled after user feedback.)
