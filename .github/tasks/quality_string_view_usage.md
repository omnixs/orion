---
template: improve_quality.md
agent: code-quality-refactor
status: not-started
pattern: "const std::string&"
replacement: "std::string_view"
scope: "function and method parameters (read-only)"
category: code-quality
priority: medium
estimated-effort: "8-12 hours"
---

# Task: Improve Code Quality - String View Usage

## Context

The FEEL evaluator frequently passes `const std::string&` parameters for read-only string operations. Modern C++23 recommends `std::string_view` for non-owning string parameters to:
- Avoid unnecessary copies
- Enable compile-time string operations
- Support substring operations without allocation

## Scope

**Target Files:**
- **All source files** in `src/` (bre, api, apps, common)
- **All header files** in `include/orion/`
- **All test files** in `tst/` (bre, common)

**Pattern to Find:**
- All instances of `const std::string&` in function/method parameters
- Focus on read-only parameters (not stored as members)
- Exclude: return types, member variables, cases requiring ownership

**Expected Coverage:**
- Production code: src/bre/, src/api/, src/common/
- Test code: tst/bre/, tst/common/
- Headers: include/orion/bre/, include/orion/api/, include/orion/common/

## Acceptance Criteria

- [ ] All read-only string parameters use `std::string_view`
- [ ] String storage (class members) still uses `std::string`
- [ ] All clang-tidy warnings resolved
- [ ] No test regressions (unit + TCK)
- [ ] Performance maintained or improved
- [ ] Code follows CODING_STANDARDS.md Section 3 (Modern C++)

## Execution Instructions

**Agent Required:** `code-quality-refactor`

**Task Execution:**
Execute the refactoring task defined in this file. The agent will automatically:
- Capture TCK baseline for regression detection
- Search entire codebase for `const std::string&` instances
- Enumerate all findings with file:line:signature details
- For each instance:
  - Apply the change
  - Build in Release configuration
  - Run unit tests
  - Auto-revert on build/test failures and continue
- Compare final TCK results with baseline
- Update this file with progress and results
- Handoff to code-reviewer for final review

## Expected Impact

- **Code Quality:** Modernization, better const-correctness
- **Performance:** Potential 1-3% improvement in string-heavy operations
- **Maintainability:** Clearer ownership semantics

## Progress Tracking
<!-- Agent will update this section during execution -->
- **Total instances found:** 
- **Completed:** 
- **Skipped (build failures):** 
- **Skipped (test failures):** 
- **Remaining:** 

## Results
<!-- Agent will update after completion -->
- **Baseline TCK:** 
- **Final TCK:** 
- **Regressions:** None / [list]
- **Unit Tests:** Pass / Fail

## Risks

- **Low Risk:** Non-breaking change, internal APIs only
- **Compilation:** May need explicit string conversions where string_view passed to std::string-taking APIs

## Retrospective

(This section will be filled after task execution with:
- What worked well
- What needed clarification
- Improvements for template/process)
