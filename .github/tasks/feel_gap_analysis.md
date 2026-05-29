# FEEL Gap Analysis — ORION DMN Engine

<!-- Structured reference for missing FEEL features. -->
<!-- Generated: 2026-05-28 | Baseline: TCK v2.1.1 | Source: codebase analysis -->

## Current Baseline Metrics

| Metric | Value |
|---|---|
| TCK Total Tests | 3535 |
| TCK Passed | 512 (14.5%) |
| Level 2 | 125/125 (100%) |
| Level 3 | 378/3351 (11.3%) |
| Unit Tests | 404 (all passing) |
| Functions Registered | 86 |
| Functions Dispatched + Implemented | 29 (33.7%) |

## Architecture Quick Reference

| Component | File(s) | Role |
|---|---|---|
| Lexer | `include/orion/bre/feel/lexer.hpp`, `src/bre/feel/lexer.cpp` | Tokenization |
| Parser | `include/orion/bre/feel/parser.hpp`, `src/bre/feel/parser.cpp` | Token → AST |
| AST Nodes | `include/orion/bre/ast_node.hpp`, `src/bre/ast_node.cpp` | AST node types + `evaluate()` |
| Function Dispatch | `src/bre/ast_node.cpp` (FUNCTION_CALL case) | Routes function name → C++ impl |
| Function Impls | `include/orion/bre/feel/functions.hpp`, `src/bre/feel/functions.cpp` | Built-in function bodies |
| Function Registry | `include/orion/bre/feel/function_registry.hpp`, `src/bre/feel/function_registry.cpp` | Named parameter signatures |
| Type System | `include/orion/bre/feel/types.hpp`, `src/bre/feel/types.cpp` | Date/Time/Duration parsing |
| Unary Tests | `include/orion/bre/feel/unary.hpp`, `src/bre/feel/unary.cpp` | Decision table input matching |
| Evaluator Entry | `include/orion/bre/feel/evaluator.hpp`, `src/bre/feel/evaluator.cpp` | Top-level `Evaluator::evaluate()` |
| Parameter Binder | `include/orion/bre/feel/parameter_binder.hpp`, `src/bre/feel/parameter_binder.cpp` | Named/positional param resolution |

---

## PHASE 1: Trivial Function Dispatch (odd, even, number, string, is)

**Effort: Low | Dependencies: None | TCK Impact: ~60 tests**

These functions are already registered in `function_registry.cpp` but have NO implementation in `functions.cpp` and NO dispatch case in `ast_node.cpp`.

| Function | Signature | TCK Group | TCK Baseline |
|---|---|---|---|
| `odd(number)` | 1 param | `0055-feel-odd-function` | 12/17 |
| `even(number)` | 1 param | `0054-feel-even-function` | 12/17 |
| `number(from, grouping separator, decimal separator)` | 3 params | `0058-feel-number-function` | 10/17 |
| `string(from)` | 1 param | (no isolated TCK group) | — |
| `is(value1, value2)` | 2 params | `0103-feel-is-function` | 0/10 |

### Implementation pattern:
1. Add `evaluate_X_function()` to `functions.hpp` / `functions.cpp`
2. Add `else if (funcName == "X")` dispatch in `ast_node.cpp` FUNCTION_CALL case
3. Add unit tests in `tst/bre/feel/`

---

## PHASE 2: List Functions (23 functions)

**Effort: Medium | Dependencies: None | TCK Impact: ~200+ tests**

All 23 functions are registered but have NO implementation and NO dispatch. This is the largest gap.

### Aggregation functions (simple — operate on flat lists):

| Function | Signature | TCK Group |
|---|---|---|
| `count(list)` | 1 param | `0012-list-functions` |
| `sum(list)` | 1 param | `0012-list-functions` |
| `min(list)` | 1 param (also variadic) | `0012-list-functions` |
| `max(list)` | 1 param (also variadic) | `0012-list-functions` |
| `mean(list)` | 1 param | `0012-list-functions` |
| `product(list)` | 1 param | `0094-feel-product-function` |
| `median(list)` | 1 param | `0061-feel-median-function` |
| `stddev(list)` | 1 param | `0063-feel-stddev-function` |
| `mode(list)` | 1 param | `0062-feel-mode-function` |

### List manipulation functions:

| Function | Signature | TCK Group |
|---|---|---|
| `list contains(list, element)` | 2 params | `0012-list-functions` |
| `append(list, items...)` | variadic | `0009-append-flatten` |
| `concatenate(list...)` | variadic | `0010-concatenate` |
| `insert before(list, position, newItem)` | 3 params | `0011-insert-remove` |
| `remove(list, position)` | 2 params | `0011-insert-remove` |
| `reverse(list)` | 1 param | `0012-list-functions` |
| `index of(list, match)` | 2 params | `0012-list-functions` |
| `sublist(list, start position, length?)` | 2-3 params | `0012-list-functions` |
| `union(list...)` | variadic | `0012-list-functions` |
| `distinct values(list)` | 1 param | `0012-list-functions` |
| `flatten(list)` | 1 param | `0009-append-flatten` |
| `sort(list, precedes)` | 2 params (2nd is function) | `0013-sort` |
| `list replace(list, position, newItem)` | 3 params | `1155-list-replace-function` |

**Note:** `0012-list-functions` currently 0/19 — this phase should bring it to 19/19.

---

## PHASE 3: Context Functions (5 functions)

**Effort: Medium | Dependencies: None | TCK Impact: ~50+ tests**

| Function | Signature | TCK Group | TCK Baseline |
|---|---|---|---|
| `get value(m, key)` | 2 params | `0080-feel-getvalue-function` | 0/5 |
| `get entries(m)` | 1 param | `0081-feel-getentries-function` | 0/9 |
| `context(entries)` | 1 param | `1145-feel-context-function` | partial |
| `context put(context, key, value)` | 3 params | `1146-feel-context-put-function` | partial |
| `context merge(contexts)` | 1 param | `1147-feel-context-merge-function` | partial |

---

## PHASE 4: Date/Time Type System & Functions (9 functions)

**Effort: High | Dependencies: None, but foundational for Phases 5-6 | TCK Impact: ~300+ tests**

The current type system represents dates/times/durations as ISO strings. A first-class type system is needed.

### 4A: Complete temporal constructors:

| Function | TCK Group | TCK Baseline |
|---|---|---|
| `time(from)` | `1116-feel-time-function` | 0/? |
| `date and time(from)` | `1117-feel-date-and-time-function` | 0/? |
| `years and months duration(from, to)` | `1121-feel-years-and-months-duration-function` | 0/? |
| `now()` | `1148-feel-now-function` | 0/? |
| `today()` | `1149-feel-today-function` | 0/? |

### 4B: Temporal accessor functions:

| Function | TCK Group | TCK Baseline |
|---|---|---|
| `day of year(date)` | `0095-feel-day-of-year-function` | 0/? |
| `day of week(date)` | `0096-feel-day-of-week-function` | 0/? |
| `month of year(date)` | `0097-feel-month-of-year-function` | 0/? |
| `week of year(date)` | `0098-feel-week-of-year-function` | 0/? |

### 4C: Temporal property access (`.year`, `.month`, `.day`, `.hour`, etc.):
- TCK: `0074-feel-properties` (0/50+)
- TCK: `0007-date-time` (partial)

### 4D: Temporal arithmetic (date + duration, date - date, etc.):
- TCK: `0100-arithmetic` (partial — large test group)

---

## PHASE 5: FEEL Language Constructs (Parser + AST + Evaluator)

**Effort: High | Dependencies: Phase 2 (list functions used in iterations) | TCK Impact: ~500+ tests**

These require new AST node types, new parser methods, and new evaluator logic.

### 5A: `for` expression
- Syntax: `for x in list return expr`
- New AST node type: `FOR_EXPRESSION`
- Parser: `parse_for_expression()` method
- TCK: `0033-for-loops`, `0084-feel-for-loops`, `0003-iteration`, `1152-boxed-for`

### 5B: Quantified expressions
- Syntax: `some x in list satisfies expr` / `every x in list satisfies expr`
- New AST node types: `SOME_EXPRESSION`, `EVERY_EXPRESSION`
- TCK: `0016-some-every`, `1153-boxed-some`, `1154-boxed-every`

### 5C: Filter expression
- Syntax: `list[condition]` / `list[index]`
- New AST node type: `FILTER_EXPRESSION`
- Parser: handle `[` after primary expression
- TCK: `0001-filter`, `1151-boxed-filter`

### 5D: `between` operator
- Syntax: `x between a and b`
- Can be desugared to `x >= a and x <= b`
- TCK: `0071-feel-between` (0/large)

### 5E: `in` operator (membership/range test)
- Syntax: `x in [list]` / `x in (1..10)`
- TCK: `0072-feel-in` (0/large)

### 5F: `instance of` operator
- Syntax: `x instance of number`
- Requires type introspection
- TCK: `0070-feel-instance-of` (0/large)

---

## PHASE 6: Range Functions (14 functions)

**Effort: Medium | Dependencies: Phase 4 (temporal types) | TCK Impact: ~56 tests**

| Function | Signature |
|---|---|
| `before(point1, point2)` | 2 params |
| `after(point1, point2)` | 2 params |
| `meets(range1, range2)` | 2 params |
| `met by(range1, range2)` | 2 params |
| `overlaps(range1, range2)` | 2 params |
| `overlaps before(range1, range2)` | 2 params |
| `overlaps after(range1, range2)` | 2 params |
| `finishes(point, range)` | 2 params |
| `finished by(range, point)` | 2 params |
| `includes(range, point)` | 2 params |
| `during(point, range)` | 2 params |
| `starts(point, range)` | 2 params |
| `started by(range, point)` | 2 params |
| `coincides(point1, point2)` | 2 params |

All 14 functions are already registered in `function_registry.cpp`.

---

## PHASE 7: Advanced Features

**Effort: High | Dependencies: Phases 1-6 | TCK Impact: ~100+ tests**

### 7A: User-defined functions / Lambda
- Syntax: `function(x) x + 1`
- TCK: `0030-user-defined-functions`, `0031-user-defined-functions`, `0092-feel-lambda`

### 7B: At-literals
- Syntax: `@"2023-01-01"`, `@"PT5H"`, `@"10:30:00"`
- TCK: `0093-feel-at-literals`

### 7C: Type coercion
- DMN implicit coercions (singleton list ↔ element, etc.)
- TCK: `0082-feel-coercion`, `0021-singleton-list`

### 7D: Comments in FEEL
- `// line comment` and `/* block comment */`
- TCK: `0073-feel-comments`

### 7E: Unicode support
- TCK: `0083-feel-unicode`

### 7F: NaN and Infinity handling
- TCK: `0077-feel-nan`, `0078-feel-infinity`

---

## PHASE 8: Fix Existing Implementations (Quality)

**Effort: Medium | Dependencies: None (can run in parallel) | TCK Impact: ~100+ tests**

### 8A: `replace()` function — currently does plain string replacement, must use regex
- TCK: `1109-feel-replace-function`

### 8B: Numeric precision edge cases
- `decimal()` function TCK failures: `1100-feel-decimal-function`
- Rounding mode edge cases for `round up/down/half up/half down`

### 8C: `substring()` negative index handling
- TCK: `1103-feel-substring-function`

### 8D: String function TCK failures (context-output format)
- Many string function TCK tests use context expressions as output
- TCK groups: `1104` through `1110` — all show 0/N despite functions being implemented
- Root cause: DMN test outputs like `{"result": upper case("hello")}` fail because context-level expression evaluation returns null

### 8E: Logical operator edge cases with non-boolean types
- TCK: `0064-feel-conjunction` (5/18), `0065-feel-disjunction` (5/18), `0066-feel-negation` (2/6)

---

## Out of Scope (Not FEEL Chapter 9/10)

These are DMN infrastructure features, not FEEL expression features:

| Feature | TCK Group | Reason |
|---|---|---|
| Decision Services | `0085-decision-services` | DMN Chapter 11 |
| Import | `0086-import`, `0089-nested-inputdata-imports` | DMN Chapter 11 |
| External Java functions | `0076-feel-external-java` | Platform-specific |
| DRG Scopes | `0034-drg-scopes` | DMN infrastructure |
| BKM implicit/explicit params | `0037`, `0038` | DMN infrastructure |
| Chapter 11 example | `0087-chapter-11-example` | DMN Chapter 11 |

---

## TCK Test Groups by Phase

### Phase 1 targets: `0054`, `0055`, `0058`, `0103`
### Phase 2 targets: `0009`, `0010`, `0011`, `0012`, `0013`, `0061`, `0062`, `0063`, `0069`, `0094`, `1155`
### Phase 3 targets: `0057`, `0080`, `0081`, `1145`, `1146`, `1147`
### Phase 4 targets: `0007`, `0074`, `0095`, `0096`, `0097`, `0098`, `0100`, `1115`, `1116`, `1117`, `1120`, `1121`, `1148`, `1149`
### Phase 5 targets: `0001`, `0003`, `0016`, `0033`, `0068`, `0070`, `0071`, `0072`, `0084`, `1150`, `1151`, `1152`, `1153`, `1154`, `1161`
### Phase 6 targets: `1130`, `1156`
### Phase 7 targets: `0021`, `0030`, `0031`, `0073`, `0077`, `0078`, `0082`, `0083`, `0092`, `0093`
### Phase 8 targets: `0064`, `0065`, `0066`, `0099`, `1100`, `1103`, `1104`, `1105`, `1106`, `1107`, `1108`, `1109`, `1110`, `1140`, `1141`, `1142`, `1143`, `1144`
