---
template: add_dmn_feature.md
agent:
status: not-started
category: feature
priority: high
estimated-effort: "6-12 hours"
actual-effort: ""
---

## Task: Replace std::regex usage (Issue #17)

Replace internal uses of `std::regex` with CTRE for compile-time / fixed patterns and PCRE2 for FEEL `matches()` (runtime patterns), while keeping the public Orion API **backward compatible** and unchanged for correctness.

## Context

**Issue:** https://github.com/omnixs/orion/issues/17

**Background / Problem (from issue #17):**
- `std::regex` is used for fixed-pattern parsing/validation in multiple locations.
- FEEL `matches(string, pattern)` currently constructs a `std::regex` from a runtime pattern; this causes runtime compilation overhead and unpredictable allocation behavior in some integration environments.
- `std::regex` performance and behavior varies significantly between standard library implementations.

## Scope

### Included in this task

- [ ] Identify and remove all fixed-pattern `std::regex` usage by migrating those call sites to CTRE.
- [ ] Replace FEEL `matches()` implementation to use PCRE2 instead of `std::regex`.
- [ ] Add precompilation for `matches()` patterns when the pattern argument is statically determinable (e.g. constant string literal).
- [ ] Add bounded caching (LRU or similar) for dynamically provided patterns, to avoid repeated compilation and unbounded growth.
- [ ] Preserve current FEEL `matches()` semantics: invalid patterns return `null`.
- [ ] Keep the public Orion API backward compatible:
  - No existing API must change semantics.
  - Any initialization or warm-up must be optional and not required for correctness.

### Explicitly excluded (per issue #17)

- Custom allocator hooks for PCRE2 (potential follow-up).
- Broad regex dialect translation layer.
- Exposure of regex-engine-specific concepts in the public API.

## Detailed Instructions

### Step 1: Codebase inventory (evidence-based)
- Locate every usage of `std::regex` in the repository (including helper utilities).
- Categorize each use:
  - Fixed compile-time pattern → CTRE.
  - Runtime pattern → PCRE2.
- Record findings in this task file (short list of file paths + purpose).

### Step 2: Add dependencies + wiring
- Add CTRE dependency for compile-time regex usage.
- Add PCRE2 dependency for runtime regex usage in FEEL `matches()`.
- Integrate via vcpkg/CMake as appropriate for this repository.

### Step 3: Replace fixed-pattern regex with CTRE
- Migrate each fixed-pattern call site from `std::regex` to CTRE.
- Ensure behavior stays equivalent for the given patterns:
  - Full match vs search behavior must match existing logic.
- Prefer minimal refactors; keep intent readable.

### Step 4: Implement PCRE2-based FEEL `matches()`
- Replace `std::regex` usage in FEEL `matches()` with PCRE2.
- Maintain semantics:
  - Full-string matching behavior (equivalent to `std::regex_match`).
  - Invalid pattern compilation returns `null`.
- Regex dialect:
  - Use PCRE2 syntax for `matches()` patterns.
  - Document the supported dialect/behavior (no broad translation layer; out of scope).
- Implement an internal wrapper type for compiled regex objects (owning compiled code and relevant metadata).
- Ensure PCRE2 usage is fully encapsulated (no API exposure).

### Step 5: Precompilation + bounded cache
- During model load / expression preparation:
  - Detect FEEL `matches()` calls where the pattern argument is a constant string literal (or otherwise statically determinable).
  - Compile the pattern once and store the compiled regex in the model/expression node.
- For truly dynamic patterns:
  - Add a bounded cache:
    - Key: pattern string (plus relevant flags if applicable).
    - Value: compiled PCRE2 code object.
    - Eviction: LRU (or similar).
    - Hard limit: configurable maximum entries.
      - Implement configurability via a backward-compatible engine/options surface (not environment variables).
      - The configuration must not expose regex-engine-specific types or PCRE2 details.

### Step 6: Internal warm-up / deterministic initialization 
- Add an optional warm-up mechanism that:
  - Compiles and executes a trivial regex match once.
  - Reduces first-use latency and initialization variability.
- Requirements:
  - Must not be required for correctness.
  - Must not be mandatory for existing users (backward compatible).
  - Must be exposed via an optional public API entry point (exact placement/signature to be chosen to fit the existing public headers, but must not expose PCRE2/regex-engine concepts).

### Step 7: Tests

Add or extend tests to cover:
- `matches("abc", "a.*")` true/false cases.
- Invalid pattern returns `null`.
- Cache behavior: same pattern compiled once (observable via instrumentation or targeted unit test structure).
- Precompile behavior: pattern compiled at model load and reused during evaluation.

## Success Criteria

- [ ] No remaining `std::regex` usage in fixed-pattern code paths.
- [ ] FEEL `matches()` no longer uses `std::regex`.
- [ ] Invalid FEEL patterns reliably return `null`.
- [ ] Existing unit tests pass.
- [ ] No regressions in TCK tests.
- [ ] Public Orion API remains backward compatible (no breaking changes).

## Validation Steps

1. Build Debug per `.github/instructions/build.md`.
2. Run unit tests per `.github/instructions/run_unit_tests.md`.
3. Run TCK runner per `.github/instructions/run_tck_tests.md`.
4. If a baseline exists for the current version, run regression detection and confirm no regressions.

## Reference Documentation

- Issue: https://github.com/omnixs/orion/issues/17
- Build: `.github/instructions/build.md`
- Unit tests: `.github/instructions/run_unit_tests.md`
- TCK: `.github/instructions/run_tck_tests.md`
- Coding standards: `CODING_STANDARDS.md`

## Retrospective

(To be completed after execution)

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
