# Continuation Instructions — FEEL Phase 2+ Implementation

## Original Task Reference

**Task File:** `D:\Orion\.github\tasks\Generated_task_prompt.md`  
**Gap Analysis:** `D:\Orion\.github\tasks\feel_gap_analysis.md`  
**Branch:** `feature/feel-phase-2-list-functions`  
**Restoration Instructions:** See `check_point/restoration_instruction.prompt.md`

---

## Current State (as of 2026-08-03)

### TCK Score: 3120 / 3527 (88.5%) — up from 512 (14.5%) at project start

### Latest Phase 2 Delta (2026-08-03, uncommitted)

- Added multi-word parser support for `list replace` in `src/bre/feel/parser.cpp`
- Added singleton coercion to `list replace` in `src/bre/feel/functions.cpp`
- Added broader multi-word parser support for list/string/context/round functions
- Improved TCK:
	- `1155-list-replace-function`: 18/22 -> 19/22
	- `0012-list-functions`: remains 18/19 (parser regression fixed)
	- Full TCK: 3118/3527 -> 3120/3527 (`failed=409 -> 407`), no regression (exit code 1)

### Build & Test Commands

```powershell
# Ensure cmake is in PATH (new terminal sessions may need this)
$env:PATH += ";C:\Program Files\Microsoft Visual Studio\18\Professional\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin"

# Build
cmake --build build --config Debug

# Unit tests (must show "*** No errors detected")
.\build\Debug\tst_orion.exe --log_level=test_suite

# TCK regression check (exit code 1 = OK, exit code 2 = REGRESSION)
.\build\Debug\orion_tck_runner.exe --baseline "dat\tck-baselines\2.1.1\tck_results.csv" --regression-check --log_level=error

# TCK specific test suite (--verbose for details)
.\build\Debug\orion_tck_runner.exe --test "TEST_NAME" --verbose

# Update baseline after confirmed improvements
.\build\Debug\orion_tck_runner.exe --output-csv "dat\tck-baselines\2.1.1\tck_results.csv"
```

---

## 1. What Has Been Implemented

### Session Commits (9 commits, TCK 3007 → 3118)

| Commit | Summary | TCK After |
|--------|---------|-----------|
| `368d874` | Parse list inputs in TCK test XML | 3007 |
| `567160a` | JSON-aware deep comparison in TCK runner | 3042 |
| `effac3e` | string+temporal returns null, P0Y formatting | 3063 |
| `096621d` | Parse empty xsd:string as empty string not null | 3076 |
| `883a180` | Implement regex-based replace() function | 3094 |
| `569a39b` | Parse fractional seconds in durations | 3096 |
| `e7cada2` | Nested list parsing in expected values, string join coercion | 3110 |
| `deb7908` | FEEL-style string() formatting for lists and contexts | 3112 |
| `bb72fd1` | Context put nested keys, xml2json nested components, keys param alias | 3118 |

### In-Progress (not yet committed)

- `src/bre/feel/parser.cpp`
- `src/bre/feel/functions.cpp`

### Earlier Commits (from prior sessions, same branch)

| Commit | Summary |
|--------|---------|
| `4a168b8` | Phase 1: odd/even/number/string/is functions |
| `efcce50` | Phase 2+3: list/aggregation/context functions |
| `6bc42f5` | Phase 3: context functions + TCK runner fix |
| `c7bf22e` | Phases 2-5,8: FEEL functions, parser constructs, TCK perf fix |
| `aa0abd0` | Phase 4: Date/Time temporal functions |
| `e253b2e` | in-operator, duration arithmetic, date/time arithmetic |
| `d823b3c` | instance-of temporal types, postfix property access |
| `5e83007` | equality null semantics |
| `75ac2d3` | singleton coercion, temporal normalization |
| `7f791a8` | multi-word function names (date and time, etc.) |
| `4b33f79` | normalize +00:00/-00:00 to Z |
| `e6d720c` | abs duration, negative year datetime arithmetic |
| `e242379` | time 4-arg form, years-and-months-duration negative fix |
| `e9c3ed7` | datetime subtraction, date-dtDuration floor, unary negation |
| `0b422b0` | three-valued logic for and/or |
| `5c6bf3a` | multi-word function parsing (day/month/week of year) |
| `a4f93ed` | for-loop bare range, FEEL comments |

---

## 2. How It Has Been Implemented

### Architecture Pattern for Function Implementation

1. **Declare** in `include/orion/bre/feel/functions.hpp`
2. **Implement** in `src/bre/feel/functions.cpp`
3. **Dispatch** in `src/bre/ast_node.cpp` (FUNCTION_CALL case, search `funcName ==`)
4. **Register** in `src/bre/feel/function_registry.cpp` (parameter names for named-param support)

### Key Implementation Details

- **JSON library**: nlohmann::json (aliased as `json`)
- **Temporal types**: `feel::Duration{total_months, total_seconds}` — NO fractional seconds in struct, but string parsing handles them
- **Date/Time functions** (date, time, date and time): NOT registered in function_registry.cpp — use fallback positional binding in parameter_binder.cpp because they have multiple overloaded signatures
- **Context put**: Has special handling in `ast_node.cpp` for the "keys" (list) variant vs "key" (string) variant, since function registry only supports single signature per name
- **TCK runner enhancements**: `src/common/xml2json.cpp` and `src/apps/orion_tck_runner.cpp` were modified to properly parse nested expected values, list inputs, and perform deep JSON comparison
- **FEEL string formatting**: `string()` function uses FEEL-style output (unquoted keys, spaces after separators) rather than JSON dump

### Key Source Files Modified

| File | Role |
|------|------|
| `src/bre/feel/functions.cpp` | All built-in FEEL function implementations (~3300 lines) |
| `src/bre/ast_node.cpp` | Function dispatch + special context put handling |
| `src/bre/feel/parameter_binder.cpp` | Named parameter binding |
| `src/bre/feel/function_registry.cpp` | Function signature metadata |
| `src/common/xml2json.cpp` | TCK test XML → JSON parsing (nested components, lists) |
| `src/apps/orion_tck_runner.cpp` | TCK runner comparison logic |
| `src/bre/feel/types.cpp` | Temporal type parsing (fractional seconds, durations) |

---

## 3. Challenges Faced

### Resolved
- **Decision table variable input (0036)**: Attempted complex fix with AST parsing of unary tests — caused 7+ regressions. Reverted entirely. String-based `unary_test_matches` can't resolve variable paths like `Complex.aNumber`.
- **Context put dual signatures**: Function registry only supports one signature per name. Solved with manual binding in `ast_node.cpp` for the "keys" list variant.
- **TCK runner nested expected values**: Parser only checked `<value>` children. Fixed with recursive `parse_expected_components()`.
- **FEEL string() formatting**: JSON `dump()` doesn't match FEEL spec output. Implemented custom recursive formatter.
- **Duration fractional seconds**: Struct only has `total_seconds` (integer). Parse and format fractional seconds in string handling but don't store them.

### Unresolved / Known Limitations
- **Inter-decision dependencies (DRG scope)**: Tests like 1146/014-016, nested010-012 fail because they reference other decisions. Needs DRG scope resolution infrastructure.
- **@timezone IANA support**: Tests like 0068 need timezone database (America/New_York → UTC offset). Not implemented.
- **Lambda/user-defined functions**: No support for anonymous functions. Affects sort(), list-replace(), user-defined function tests.
- **instance-of type system**: Partial — basic types work but complex types (list<T>, context types) don't.
- **BKM invocation**: Business Knowledge Model tests (0020, 0082) need full BKM invocation support.

---

## 4. What Still Needs To Be Done

### Remaining 409 Failures — Grouped by ROI

#### High ROI (fixable with moderate effort)

| Test Suite | Failures | Root Cause | Approach |
|-----------|----------|------------|----------|
| `0036-dt-variable-input` | ~14 | Unary tests can't resolve variable paths | Need AST-based unary test evaluation that resolves variables from input context |
| `0057-feel-context` | ~5 | Context expression evaluation gaps | Review specific failures |
| `1111-feel-matches` | ~12 | XPath regex compat, unicode case folding | PCRE2 flag adjustments |
| `1156-range-function` | ~27 | `range()` function not implemented | Implement range object support |
| `0074-feel-properties` | ~14 | Range properties need range objects | Depends on range implementation |

#### Medium ROI (significant effort)

| Test Suite | Failures | Root Cause |
|-----------|----------|------------|
| `0070-feel-instance-of` | ~34 | Complex type system (list<T>, function types) |
| `0082-feel-coercion` | ~21 | BKM/type coercion infrastructure |
| `0068-feel-equality` | ~19 | IANA timezone support |

#### Low ROI (major infrastructure needed)

| Area | Tests Affected | Required Infrastructure |
|------|---------------|----------------------|
| Lambda/closures | sort, list replace, filter | Function-as-value, closure support |
| BKM invocation | 0020 (7), 0034 (12) | Full DRG resolution |
| User-defined functions | scattered | Function definition AST |

### Recommended Next Steps (in priority order)

1. **Range function + range properties** (~41 tests): Implement `range()` built-in and range object type with `.start`, `.end`, `.start included`, `.end included` properties
2. **Matches/replace regex improvements** (~12 tests): Fix XPath regex incompatibilities with PCRE2
3. **Context expression evaluation** (~5 tests): Fix remaining `0057-feel-context` failures
4. **Decision table variable input** (~14 tests): Re-attempt with careful AST evaluation that handles all comparison types
5. **Instance-of improvements** (~34 tests): Add list<T> and context type checking

### Test Suites to Investigate for Quick Wins

Run these to see exact failure details:
```powershell
.\build\Debug\orion_tck_runner.exe --test "0057" --verbose
.\build\Debug\orion_tck_runner.exe --test "1111" --verbose
.\build\Debug\orion_tck_runner.exe --test "1156" --verbose
.\build\Debug\orion_tck_runner.exe --test "0074" --verbose
```

---

## Environment Notes

- **OS**: Windows 10/11
- **Compiler**: MSVC 2022 (Visual Studio 18 / Professional)
- **CMake**: Located at `C:\Program Files\Microsoft Visual Studio\18\Professional\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin`
- **Build Config**: Debug
- **Test framework**: Boost.Test
- **Working directory**: `D:\Orion`
- **Branch**: `feature/feel-phase-2-list-functions`

---

## Critical Rules (from copilot-instructions.md)

1. **Never disable/skip/weaken existing tests**
2. **Never hardcode expected values**
3. **Always fix implementation to match spec**
4. **Build → Unit Test → TCK regression check (sequential, each must pass)**
5. **Exit code 1 from TCK = OK (failures exist but no regression); Exit code 2 = REGRESSION (must fix)**
6. **Use simple commands only — no pipes for complex operations (VS Code tools instead)**
