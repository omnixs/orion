# Master Task: Implement All Missing FEEL Expressions & Features

## Objective

Implement **all** missing FEEL expressions and built-in functions as defined in **Chapters 9 and 10** of the DMN 1.5 specification (`D:\Orion\docs\formal-24-01-01.txt`). Upon completion, the ORION engine must pass all applicable TCK Level 3 compliance tests with no regressions.

---

## Sources & References

| Resource | Path | Purpose |
|---|---|---|
| DMN 1.5 Spec | `D:\Orion\docs\formal-24-01-01.txt` | Authoritative specification for FEEL semantics |
| Gap Analysis | `D:\Orion\.github\tasks\feel_gap_analysis.md` | Structured table of all missing features grouped by phase |
| Add DMN Feature | `D:\Orion\.github\prompts\add_dmn_feature.md` | Template for implementing new FEEL features |
| Fix Bug | `D:\Orion\.github\prompts\fix_bug.md` | Template for debugging and fixing issues |
| Improve Quality | `D:\Orion\.github\prompts\improve_quality.md` | Template for code quality improvement |
| Improve Performance | `D:\Orion\.github\prompts\improve_perf.md` | Template for performance optimization |
| Build Instructions | `D:\Orion\.github\instructions\build.instructions.md` | How to configure and build |
| Unit Test Instructions | `D:\Orion\.github\instructions\run_unit_tests.instructions.md` | How to run Boost.Test unit tests |
| TCK Test Instructions | `D:\Orion\.github\instructions\run_tck_tests.instructions.md` | How to run TCK, baselines, regressions |
| Coding Standards | `D:\Orion\CODING_STANDARDS.md` | Naming, error handling, memory management |
| Code Review Checklist | `D:\Orion\.github\instructions\code_review_checklist.instructions.md` | Quality gates for all changes |
| Architecture | `D:\Orion\docs\architecture.md` | Component overview |

---

## Baseline Metrics (Before Work Begins)

Record these and compare after each phase:

| Metric | Current Value |
|---|---|
| Unit Tests | 404 (all passing) |
| TCK Total | 512/3535 (14.5%) |
| TCK Level 2 | 125/125 (100%) |
| TCK Level 3 | 378/3351 (11.3%) |

---

## Architecture Quick Reference

The codebase has a two-track FEEL evaluation system. All modifications must be consistent across both tracks.

### AST Evaluation Pipeline (literal expressions)
```
Input string → Lexer → Tokens → Parser → AST → ASTNode::evaluate() → Result
```

**Key files to modify for adding a function:**
1. `src/bre/feel/functions.hpp` / `src/bre/feel/functions.cpp` — Add `evaluate_X_function()` implementation
2. `src/bre/ast_node.cpp` — Add dispatch case in `FUNCTION_CALL` handler (search for `funcName ==`)
3. `src/bre/feel/function_registry.cpp` — Verify function signature is registered (most already are)

**Key files to modify for adding a language construct (e.g., `for`, `some`, `every`, `instance of`):**
1. `include/orion/bre/feel/lexer.hpp` — Add token type if needed (e.g., `KEYWORD` for new keywords)
2. `src/bre/feel/lexer.cpp` — Add keyword recognition
3. `include/orion/bre/ast_node.hpp` — Add new `ASTNodeType` enum value
4. `src/bre/ast_node.cpp` — Add `evaluate()` logic for new node type
5. `src/bre/feel/parser.cpp` — Add parsing rule

### Unary Test Evaluation (decision table input columns)
```
Input string → unary_test_matches() → regex/dispatch → bool match result
```
**File:** `src/bre/feel/unary.cpp` — Only modify if the new feature can appear in unary test contexts.

### Type System
**Files:** `include/orion/bre/feel/types.hpp`, `src/bre/feel/types.cpp`
- Currently: `Date{y,m,d}`, `Time{h,m,s}`, `DateTime{date,time}`, `Duration{total_months, total_seconds}`
- Types are parsed from ISO strings for comparison
- No timezone support currently
- Must be extended for temporal property access (`.year`, `.month`, etc.) and temporal arithmetic

### Existing Implementation Pattern (Follow This)

When implementing a new function, follow the exact pattern of existing implementations. For example, `abs()`:

1. **Declaration** in `functions.hpp`:
   ```cpp
   std::string evaluate_abs_function(const std::vector<std::string>& args);
   ```

2. **Implementation** in `functions.cpp`:
   ```cpp
   std::string evaluate_abs_function(const std::vector<std::string>& args) {
       if (args.empty() || args[0] == "null") return "null";
       // ... implementation ...
   }
   ```

3. **Dispatch** in `ast_node.cpp` (inside `case ASTNodeType::FUNCTION_CALL:`):
   ```cpp
   else if (funcName == "abs") {
       return evaluate_abs_function(evaluatedArgs);
   }
   ```

4. **Registration** in `function_registry.cpp` (usually already done):
   ```cpp
   registry_["abs"] = {{"n"}, 1, 1};
   ```

---

## Execution Plan — 8 Phases

Each phase is an independent sub-task (except where dependencies are noted). An orchestrating agent must execute phases in order and verify completion of each before starting the next.

### Mandatory Quality Gates Per Phase

Every sub-task (phase) MUST pass ALL of these gates before the phase is considered complete:

1. **Build Gate**: `cmake --build build --config Debug` succeeds with zero errors
2. **Unit Test Gate**: ALL existing + new unit tests pass (`tst_orion.exe --log_level=test_suite` → `*** No errors detected`)
3. **TCK Regression Gate**: Run TCK with regression detection against the v2.1.1 baseline — exit code must NOT be 2 (regression). See `run_tck_tests.instructions.md` for commands.
4. **TCK Progress Gate**: The specific TCK test groups listed for the phase must show improvement. Document before/after pass counts.
5. **Code Quality Gate**: Code follows `CODING_STANDARDS.md` and passes the code review checklist.

### CRITICAL RULES

> ❌ **NEVER** disable, skip, trivialize, or weaken an existing test to make it pass
> ❌ **NEVER** add `#if 0`, `BOOST_TEST_DISABLED`, `return;` at start of test, or `BOOST_WARN` instead of `BOOST_CHECK/REQUIRE`
> ❌ **NEVER** hardcode expected values or test-specific logic in production code
> ❌ **NEVER** bypass safety checks (e.g., `--no-verify`) or discard unfamiliar code
> ✅ **ALWAYS** fix the implementation to match the spec, not the test to match the implementation
> ✅ **ALWAYS** use generic solutions — no domain-specific assumptions
> ✅ **ALWAYS** preserve all existing test assertions unchanged

---

### Phase 1: Trivial Function Dispatch (odd, even, number, string, is)

**Effort:** Low | **Dependencies:** None | **Expected TCK gain:** ~60 tests

**Functions to implement:**
- `odd(number)` → returns `true` if number is odd (integer check: no fractional part and last digit is odd)
- `even(number)` → returns `true` if number is even (integer check: no fractional part and last digit is even)
- `number(from, grouping separator, decimal separator)` → parse formatted string to number
- `string(from)` → convert any value to its string representation
- `is(value1, value2)` → semantic equality check (same type and value)

**Implementation steps:**
1. Add `evaluate_odd_function`, `evaluate_even_function`, `evaluate_number_function`, `evaluate_string_function`, `evaluate_is_function` to `functions.hpp` and `functions.cpp`
2. Add dispatch cases for each in `ast_node.cpp` FUNCTION_CALL handler
3. Verify registrations exist in `function_registry.cpp` (they should already be there)
4. Add unit tests for each function in `tst/bre/feel/`

**Spec reference:** DMN 1.5 §10.3.4.1 (Conversion functions), §10.3.4.5 (Numeric)

**TCK validation groups:** `0054-feel-even-function`, `0055-feel-odd-function`, `0058-feel-number-function`, `0103-feel-is-function`

---

### Phase 2: List Functions (23 functions)

**Effort:** Medium | **Dependencies:** None | **Expected TCK gain:** ~200+ tests

Split into two sub-phases:

#### Phase 2A: Aggregation Functions (9 functions)
`count`, `sum`, `min`, `max`, `mean`, `product`, `median`, `stddev`, `mode`

All take a list (or variadic arguments), return a single value. Straightforward to implement.

**Implementation notes:**
- These operate on the JSON array representation of lists (e.g., `["1","2","3"]`)
- `min`/`max` must also handle variadic form: `min(1, 2, 3)` in addition to `min([1, 2, 3])`
- Must handle edge cases: empty list, null elements, non-numeric elements (return null)

#### Phase 2B: List Manipulation Functions (14 functions)
`list contains`, `append`, `concatenate`, `insert before`, `remove`, `reverse`, `index of`, `sublist`, `union`, `distinct values`, `flatten`, `sort`, `list replace`

**Implementation notes:**
- `sort(list, precedes)` — The second argument is a function reference. If user-defined functions (Phase 7B) aren't implemented yet, support only the built-in `<` comparison by accepting a comparison lambda or deferring full `sort` to Phase 7.
- Functions with spaces in names (e.g., `list contains`, `insert before`) — verify the parser and function dispatch handle multi-word function names correctly. The function registry already registers these names.
- `flatten` must handle nested lists recursively

**Spec reference:** DMN 1.5 §10.3.4.4

**TCK validation groups:** `0009-append-flatten`, `0010-concatenate`, `0011-insert-remove`, `0012-list-functions`, `0013-sort`, `0061-feel-median-function`, `0062-feel-mode-function`, `0063-feel-stddev-function`, `0069-feel-list`, `0094-feel-product-function`, `1155-list-replace-function`

---

### Phase 3: Context Functions (5 functions)

**Effort:** Medium | **Dependencies:** None | **Expected TCK gain:** ~50+ tests

**Functions to implement:**
- `get value(m, key)` → extract value from context by key
- `get entries(m)` → return list of `{key, value}` pairs
- `context(entries)` → construct context from list of `{key, value}` pairs
- `context put(context, key, value)` → add/update entry in context
- `context merge(contexts)` → merge list of contexts

**Implementation notes:**
- Contexts are represented as JSON objects internally
- Use `nlohmann::json` (already a project dependency) for manipulation
- Be careful with key collision rules in `context merge` (later context wins)
- `context put` with a nested key path (e.g., `context put(ctx, ["a", "b"], val)`) — check if the DMN spec requires this

**Spec reference:** DMN 1.5 §10.3.4.6

**TCK validation groups:** `0057-feel-context`, `0080-feel-getvalue-function`, `0081-feel-getentries-function`, `1145-feel-context-function`, `1146-feel-context-put-function`, `1147-feel-context-merge-function`

---

### Phase 4: Date/Time Type System & Functions

**Effort:** High | **Dependencies:** None, but foundational for Phases 5-6 | **Expected TCK gain:** ~300+ tests

This is the most impactful single phase. Split into five sub-phases:

#### Phase 4A: First-class temporal types
Enhance the type system so that temporal values flow through the AST evaluator as typed values, not raw strings.

**Implementation approach:**
- Extend the value representation in the evaluator to distinguish between string `"2023-01-01"` and date `date("2023-01-01")`
- Options: tagged value wrapper, JSON metadata, or internal type enum
- The current approach of returning ISO strings from `date()` / `duration()` must be enhanced so that property access (`.year`, `.month`) and arithmetic work

#### Phase 4B: Temporal constructor functions (5 functions)
- `time(hour, minute, second)` and `time(from)` — from string or date-and-time
- `date and time(date, time)` and `date and time(from)` — from string or components
- `years and months duration(from, to)` — compute duration between two dates
- `now()` — current date and time (use `std::chrono::system_clock::now()`)
- `today()` — current date

#### Phase 4C: Temporal property access
- Properties: `.year`, `.month`, `.day`, `.hour`, `.minute`, `.second`, `.time offset`, `.timezone`
- Duration properties: `.years`, `.months`, `.days`, `.hours`, `.minutes`, `.seconds`
- Requires `PROPERTY_ACCESS` AST node to resolve temporal property names on typed values

#### Phase 4D: Temporal accessor functions (4 functions)
- `day of year(date)`, `day of week(date)`, `month of year(date)`, `week of year(date)`
- These return specific calendar information (day of week as string like "Monday", etc.)

#### Phase 4E: Temporal arithmetic
- `date + duration`, `date - duration`, `date - date = duration`
- `time + duration`, `time - duration`, `time - time = duration`
- `datetime + duration`, `datetime - duration`, `datetime - datetime = duration`
- Requires extending `BINARY_OP` evaluation to handle temporal operands

**Spec reference:** DMN 1.5 §10.3.2.3 (Date/Time), §10.3.4.2 (Temporal functions)

**TCK validation groups:** `0007-date-time`, `0074-feel-properties`, `0095-feel-day-of-year-function`, `0096-feel-day-of-week-function`, `0097-feel-month-of-year-function`, `0098-feel-week-of-year-function`, `0100-arithmetic`, `1115-feel-date-function`, `1116-feel-time-function`, `1117-feel-date-and-time-function`, `1120-feel-duration-function`, `1121-feel-years-and-months-duration-function`, `1148-feel-now-function`, `1149-feel-today-function`

---

### Phase 5: FEEL Language Constructs (Parser + AST + Evaluator)

**Effort:** High | **Dependencies:** Phase 2 (list functions used in iterations) | **Expected TCK gain:** ~500+ tests

These require new AST node types, new parser rules, and new evaluator logic. Each is a significant parser extension.

#### Phase 5A: `for` expression
- Syntax: `for x in expr1 return expr2`
- Also: `for x in expr1, y in expr2 return expr3` (nested iteration)
- New AST node type needed (e.g., `FOR_EXPRESSION`)
- Parser must recognize `for` keyword, parse variable bindings, `in`, `return`
- Evaluator iterates over list, binds variable in scope, evaluates return expression, collects results into new list

#### Phase 5B: Quantified expressions (`some`/`every`)
- Syntax: `some x in list satisfies condition` → `true` if any element satisfies
- Syntax: `every x in list satisfies condition` → `true` if all elements satisfy
- Similar to `for` but returns boolean instead of list

#### Phase 5C: Filter expression
- Syntax: `list[condition]` — filter list elements where condition is true
- Syntax: `list[index]` — extract element by numeric index
- Must add parsing after primary expressions to handle `[` bracket
- Inside the filter, `item` refers to the current element being tested

#### Phase 5D: `between` operator
- Syntax: `x between a and b` → equivalent to `x >= a and x <= b`
- Can be desugared at parse time to a conjunction of comparisons

#### Phase 5E: `in` operator (membership/range test)
- Syntax: `x in [1, 2, 3]` — list membership
- Syntax: `x in (1..10)` — range test
- Syntax: `x in <5, >=10` — unary test list
- This reuses unary test matching logic from `unary.cpp`

#### Phase 5F: `instance of` operator
- Syntax: `x instance of number` / `x instance of string` / etc.
- Requires runtime type introspection of values
- Type names: `number`, `string`, `boolean`, `date`, `time`, `date and time`, `years and months duration`, `days and time duration`, `list`, `context`, `Any`

**Spec reference:** DMN 1.5 §10.3.1.5 (for), §10.3.2.6.1 (some/every), §10.3.2.5 (filter), §10.3.1.3 (between), §10.3.1.4 (in), §10.3.2.12 (instance of)

**TCK validation groups:** `0001-filter`, `0003-iteration`, `0016-some-every`, `0033-for-loops`, `0068-feel-equality`, `0070-feel-instance-of`, `0071-feel-between`, `0072-feel-in`, `0084-feel-for-loops`, `1150-boxed-conditional`, `1151-boxed-filter`, `1152-boxed-for`, `1153-boxed-some`, `1154-boxed-every`, `1161-feel-any-instance-of`

---

### Phase 6: Range Functions (14 functions)

**Effort:** Medium | **Dependencies:** Phase 4 (temporal types for range comparisons) | **Expected TCK gain:** ~56 tests

**Functions to implement:**
`before`, `after`, `meets`, `met by`, `overlaps`, `overlaps before`, `overlaps after`, `finishes`, `finished by`, `includes`, `during`, `starts`, `started by`, `coincides`

**Implementation notes:**
- All functions take two arguments (either points or ranges)
- A range is `[start..end]` or `(start..end)` notation (inclusive/exclusive brackets)
- These functions must work with numbers, dates, times, and date-times
- Allen's interval algebra: each function maps to a specific temporal/spatial relationship
- See DMN 1.5 Table 80 for the formal definitions of each function
- All 14 functions are already registered in `function_registry.cpp`

**Spec reference:** DMN 1.5 §10.3.4.7

**TCK validation groups:** `1130-feel-interval-functions`, `1156-range-function`

---

### Phase 7: Advanced Features

**Effort:** High | **Dependencies:** Phases 1-6 | **Expected TCK gain:** ~100+ tests

These features are less commonly used but required for full compliance.

#### Phase 7A: At-literals
- Syntax: `@"2023-01-01"`, `@"PT5H"`, `@"10:30:00@Europe/Paris"`
- Parser must recognize `@"..."` as a temporal/duration literal (not a string)
- Maps to appropriate typed value based on content format

#### Phase 7B: User-defined functions / Lambda
- Syntax: `function(x, y) x + y`
- First-class function values
- Required for `sort()` comparator and general expressiveness
- Careful: closure semantics over local variables

#### Phase 7C: Type coercion
- Singleton list ↔ element coercion: `[1]` should equal `1` in certain contexts
- String to number coercion where expected by the spec
- Date string to date coercion with `date()` constructor

#### Phase 7D: Comments in FEEL
- `// line comment` and `/* block comment */`
- Lexer must skip comments and not emit tokens for them

#### Phase 7E: Unicode support
- FEEL strings may contain Unicode characters
- String functions (`string length`, `substring`, etc.) should count Unicode code points, not bytes

#### Phase 7F: NaN and Infinity handling
- Division by zero → `null` (not NaN or crash)
- Ensure no `NaN` or `Infinity` leaks into output

**TCK validation groups:** `0021-singleton-list`, `0030-user-defined-functions`, `0031-user-defined-functions`, `0073-feel-comments`, `0077-feel-nan`, `0078-feel-infinity`, `0082-feel-coercion`, `0083-feel-unicode`, `0092-feel-lambda`, `0093-feel-at-literals`

---

### Phase 8: Fix Existing Implementation Edge Cases

**Effort:** Medium | **Dependencies:** None (can execute in parallel with Phases 1-3) | **Expected TCK gain:** ~100+ tests

These are functions that ARE dispatched and implemented but have failing TCK tests due to incomplete handling.

#### Phase 8A: `replace()` function — regex support
- Current implementation does plain string replacement
- DMN spec requires regex replacement (XPath/XQuery regex syntax)
- Use `<regex>` standard library with appropriate flag translation
- TCK: `1109-feel-replace-function`

#### Phase 8B: `matches()` function — ensure full regex support
- Verify regex flag handling (`i`, `s`, `m`, `x`)
- TCK: `1108-feel-matches-function`

#### Phase 8C: Numeric precision edge cases
- `decimal()` rounding mode edge cases
- `round up/down/half up/half down` — verify IEEE 754 tie-breaking rules
- TCK: `1100-feel-decimal-function`, `1140`-`1144` (round functions)

#### Phase 8D: `substring()` edge cases
- Negative start position behavior per spec (count from end)
- Length exceeding string bounds
- TCK: `1103-feel-substring-function`

#### Phase 8E: String function output format
- Many string function TCK tests return results inside context expressions
- Root cause: when a DMN decision output uses an expression like `{"result": upper case("hello")}`, the evaluator must handle context-level function calls
- TCK: `1104`-`1110` (all showing 0/N despite function implementations existing)

#### Phase 8F: Logical operator edge cases
- `and`/`or` with non-boolean operands (should return `null`, not error)
- Negation of `null` should return `null`
- TCK: `0064-feel-conjunction` (5/18), `0065-feel-disjunction` (5/18), `0066-feel-negation` (2/6)

---

## Sub-Task Execution Protocol

For each phase, the executing agent MUST follow this protocol:

### Step 1: Read context
- Read `D:\Orion\.github\tasks\feel_gap_analysis.md` for the gap details
- Read the relevant section of the DMN spec (`D:\Orion\docs\formal-24-01-01.txt`) for the features being implemented
- Read the existing implementation files to understand current patterns
- Read the applicable instruction files (build, unit tests, TCK)

### Step 2: Implement
- Follow the existing code patterns exactly (see Architecture Quick Reference above)
- Follow `CODING_STANDARDS.md` for naming, error handling, memory management
- Create feature branch: `git checkout -b feature/feel-phase-N-description`

### Step 3: Write unit tests
- Add tests in `tst/bre/feel/` following existing test file patterns
- Test normal cases, edge cases (null inputs, empty lists, type mismatches), and error cases
- Use `BOOST_AUTO_TEST_CASE` within appropriate test suites

### Step 4: Verify — Build
- Follow `D:\Orion\.github\instructions\build.instructions.md`
- Build must complete with ZERO errors

### Step 5: Verify — Unit Tests
- Follow `D:\Orion\.github\instructions\run_unit_tests.instructions.md`
- ALL unit tests must pass (old AND new): `*** No errors detected`

### Step 6: Verify — TCK (No Regressions)
- Follow `D:\Orion\.github\instructions\run_tck_tests.instructions.md`
- Run with regression detection against baseline
- Exit code must NOT be 2 (regression detected)
- Document TCK pass count improvement for this phase's target test groups

### Step 7: Fix issues (loop)
- If any gate fails, diagnose the root cause and fix
- Use `D:\Orion\.github\prompts\fix_bug.md` methodology for debugging
- Re-run Steps 4-6 after each fix
- Do NOT proceed to next phase until all gates pass

### Step 8: Record results
- Document in the phase's task file:
  - Functions/features implemented
  - Unit tests added (count and file paths)
  - TCK before/after for each target test group
  - Any spec ambiguities encountered and decisions made
  - Any regressions encountered and how they were resolved

---

## Orchestration Rules

1. **Sequential phase execution**: Execute phases 1 → 2 → 3 → 4 → 5 → 6 → 7 → 8, except Phase 8 may start in parallel with Phase 1.
2. **Phase gate**: Do NOT start phase N+1 until phase N passes all quality gates.
3. **Regression is a blocker**: If a regression is detected at any point, STOP and fix it before adding more features.
4. **After all phases complete**: Run full TCK suite and generate a new baseline. Compare against starting baseline (512/3535). Target: at least 2000/3535 (56%+).
5. **If tests fail that aren't in the phase's scope**: Investigate but do NOT fix unrelated failures. Document them for future phases.
6. **Branch strategy**: One feature branch per phase. Merge to main after phase passes all gates. Next phase branches from updated main.
7. **Never modify TCK test data**: The files in `dat/dmn-tck/` are external compliance tests and must NOT be modified.

---

## Final Verification

After all 8 phases are complete:

1. Build the project with `--config Release`
2. Run ALL unit tests
3. Run full TCK suite with regression detection against v2.1.1 baseline
4. Generate new TCK baseline for the new version
5. Run performance benchmarks and compare against previous benchmarks
6. Produce a final report:
   - Total TCK pass rate (before/after)
   - Per-phase TCK improvements
   - Functions implemented (count)
   - Language constructs added
   - Any remaining gaps and why they were deferred

---

## Out of Scope

The following are explicitly NOT part of this task:

- DMN Chapter 11 features (Decision Services, Imports)
- External Java function invocation (`0076-feel-external-java`)
- DRG infrastructure changes (scoping, multi-model)
- Performance optimization beyond ensuring no major regressions
- API changes to the public `BusinessRulesEngine` interface
- Changes to the build system or dependencies (unless required for a feature)
