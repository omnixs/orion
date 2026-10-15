---
template: fix_bug.md
agent:
status: in-progress
category: bug-fix
priority: high
estimated-effort: "4-6 hours"
actual-effort: ""
---

## Task: Address code review findings on feature/correctness-fixes

Fix the defects, coverage gaps and nits found in the code review of `feature/correctness-fixes` before it is merged into `main`.

## Context

**Background and Motivation:**
- The branch adds the `ORION_ENABLE_TZDB` build option, the `format number` built-in, multi-hit no-match handling and `not (...)` whitespace support.
- Review found: the `std::chrono` symbol guard can never fail, `format number` mis-renders exponent pictures, no-match results are undocumented deviations from DMN 1.5 §10.3.2.10, and the `dat/tst/dmn-tck-extra/` test data was removed without porting its coverage.

## Scope

**Included in this task:**
- [x] `tools/scripts/check_no_chrono_symbols.ps1`: restore failure exit, avoid false positives (e.g. `synchronous`)
- [x] `format number`: exponent pictures (round the mantissa, scale pictures without integer digits), header declaration, document as extension, allocation nits (`std::to_chars`, `string_view` tokens), named constant, remove dead checks
- [x] Decision table no-match: single-hit returns `null` (DMN 1.5 §10.3.2.10); multi-hit returns an empty list / aggregation of an empty list (documented extension); C# returns 0
- [x] Port meaningful coverage of the removed `dmn-tck-extra` models to unit tests (variable names with spaces, logical operators with variables, null arithmetic, no string/boolean to number conversion, `null` input entries, modeler-exported airline model). Not ported: input clause `inputValues` (0122) — the parser never fills `InputClause::inputValues`, so `validate_input_values()` is unreachable from DMN XML
- [x] Remove the dead `dmn-tck-extra` handling from the TCK runners
- [x] `airline_ticketing_simple.dmn`: declare input data and information requirements
- [x] `#error` guard when `ORION_TZDB_ENABLED` is undefined
- [x] Docs: CODING_STANDARDS numbering, remove consumer name references
- [x] TCK baseline for 2.2.5 (README badge target)

**Explicitly excluded:**
- Squashing/rewriting already pushed commits (needs a force push; owner decision)
- `.gitignore` line endings: the branch normalizes CRLF to LF as required by `.gitattributes`, so it stays

## Success Criteria

- [x] `check_no_chrono_symbols.ps1` exits 1 when a `std::chrono` symbol is present and 0 on the `OFF` library
- [x] `format number(0.000123456, "0.00E0")` = `"1.23E-4"`, `format number(23.4, ".00E0")` = `".23E2"`
- [x] Unit tests pass with `ORION_ENABLE_TZDB=ON` and `OFF`
- [x] No TCK regressions; Level 2 at 100%
- [x] No references to consumer products in the library or this PR

## Validation Steps

1. Build Debug and Release
2. `.\build\Debug\tst_orion.exe --log_level=test_suite` (both TZDB variants)
3. `.\build\Release\orion_tck_runner.exe --log_level=error`
4. `.\tools\scripts\check_no_chrono_symbols.ps1` on the `OFF` library

## Reference Documentation

- [Bug Fix Template](../prompts/fix_bug.md)
- [CODING_STANDARDS.md](../../CODING_STANDARDS.md)
- DMN 1.5 §10.3.2.10 (decision table semantics), `docs/formal-24-01-01.txt` line 7994

## Retrospective

(This section will be filled after task completion with learnings and improvements)

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
