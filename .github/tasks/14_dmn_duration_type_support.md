---
template: add_dmn_feature.md
agent: general
status: completed
category: feature
priority: high
estimated-effort: "4-6 hours"
actual-effort: "6-8 hours (including extended debugging)"
---

# Task: Implement DMN FEEL Duration Type Support

Implement FEEL `duration` data type with days and hours precision to enable time-based business rules (e.g., advance purchase windows, time-bounded offers).

## Context

**Background and Motivation:**
- Current ORION implementation lacks support for DMN duration type
- Business rules often need time-based conditions (e.g., "valid between 5 and 10 days before event")
- DMN 1.5 specification defines duration as ISO 8601 format: `duration("P5DT10H")` (5 days, 10 hours)
- Enables generic time-window rules without hardcoding specific timestamps

**Related Issues/PRs:**
- Implements DMN 1.5 Section 10.3.2.3 (FEEL duration type)
- Foundation for time-based decision tables

**Current State:**
- FEEL evaluator supports: number, string, boolean, date (partial)
- Duration type not yet implemented
- No duration comparison operators

## Scope

**Included in this task:**
- [ ] FEEL `duration()` built-in function (days and hours only: `P<days>DT<hours>H`)
- [ ] Duration comparison operators: `<`, `>`, `<=`, `>=`, `=`, `between X and Y`
- [ ] Duration literal parsing from ISO 8601 format
- [ ] Decision table input/output type: duration
- [ ] Unit tests for duration parsing and comparisons
- [ ] DMN test case in `tst/dmn/` with comprehensive coverage
- [ ] Documentation in code comments referencing DMN 1.5 spec

**Explicitly excluded:**
- Full ISO 8601 duration (years, months, minutes, seconds) - scoped to days + hours only
- Duration arithmetic (add/subtract durations) - future enhancement
- Timezone handling - durations are timezone-agnostic intervals
- Date arithmetic (date + duration) - separate feature

## Detailed Instructions

### Step 1: Review DMN 1.5 Specification

**Actions:**
- Read DMN 1.5 spec `docs/formal-24-01-01.txt` Section 10.3.2.3 (duration type)
- Understand ISO 8601 duration format: `P[n]DT[n]H` where:
  - `P` = duration designator
  - `[n]D` = number of days
  - `T` = time designator (separates date and time components)
  - `[n]H` = number of hours
- Examples: `P5D` (5 days), `PT10H` (10 hours), `P2DT6H` (2 days 6 hours)

**Expected Outcome:**
- Clear understanding of duration syntax and semantics
- Identified test cases from specification

### Step 2: Design Duration Data Structure

**Actions:**
- Add `Duration` type to `include/orion/bre/feel/types.hpp` (or appropriate header)
- Design internal representation (e.g., total hours or days+hours struct)
- Ensure comparison operations are efficient

**Example Design:**
```cpp
namespace orion::bre::feel {
    struct Duration {
        int days;    // Number of days
        int hours;   // Number of hours (0-23 or unrestricted?)
        
        // Convert to total hours for comparison
        [[nodiscard]] int to_total_hours() const noexcept {
            return days * 24 + hours;
        }
        
        [[nodiscard]] auto operator<=>(const Duration&) const = default;
    };
}
```

**Expected Outcome:**
- Header file with `Duration` struct
- Clear conversion semantics

### Step 3: Implement `duration()` Built-in Function

**Actions:**
- Add lexer token for `duration` keyword (if needed) in `src/bre/feel/lexer.cpp`
- Update parser to recognize `duration("P5DT10H")` syntax in `src/bre/feel/parser.cpp`
- Implement parsing logic in `src/bre/feel/evaluator.cpp`:
  - Parse ISO 8601 duration string
  - Extract days and hours
  - Return `Duration` object wrapped in `nlohmann::json`

**Parsing Algorithm:**
```
Input: "P5DT10H"
1. Check starts with 'P'
2. Extract days: parse digits before 'D' → 5
3. Check for 'T' separator
4. Extract hours: parse digits before 'H' → 10
5. Return Duration{days=5, hours=10}
```

**Error Handling:**
- Invalid format → throw `std::runtime_error` with clear message
- Missing 'P' designator → error
- Negative values → error (or support if DMN allows)

**Expected Outcome:**
- `duration("P5DT10H")` evaluates to `Duration{5, 10}`
- Unit tests pass for valid/invalid formats

### Step 4: Implement Duration Comparison Operators

**Actions:**
- Update `src/bre/feel/evaluator.cpp` comparison logic
- Support operators: `<`, `>`, `<=`, `>=`, `=`, `!=`
- Support `between X and Y` for duration ranges

**Implementation:**
```cpp
// Compare durations by total hours
bool compare_duration(const Duration& left, const Duration& right, CompareOp op) {
    int left_hours = left.to_total_hours();
    int right_hours = right.to_total_hours();
    
    switch (op) {
        case CompareOp::LessThan: return left_hours < right_hours;
        case CompareOp::GreaterThan: return left_hours > right_hours;
        // ... etc
    }
}
```

**Expected Outcome:**
- `duration("P2D") < duration("P3D")` → true
- `duration("PT48H") = duration("P2D")` → true (48 hours = 2 days)
- `duration("P5DT10H") between duration("P5D") and duration("P10D")` → true

### Step 5: Add Unit Tests

**Actions:**
- Create `tst/bre/feel/test_evaluator_duration.cpp`
- Test cases:
  - Parsing valid duration strings: `P5D`, `PT10H`, `P2DT6H`
  - Parsing invalid formats: missing 'P', negative values, malformed
  - Comparison operators: `<`, `>`, `<=`, `>=`, `=`
  - Boundary conditions: zero duration, very large values
  - Between operator: inclusive ranges

**Test Example:**
```cpp
BOOST_AUTO_TEST_CASE(duration_parsing) {
    auto ctx = nlohmann::json::object();
    
    // Valid formats
    auto result1 = evaluate_feel("duration(\"P5D\")", ctx);
    BOOST_CHECK(/* result is Duration{5, 0} */);
    
    auto result2 = evaluate_feel("duration(\"P2DT6H\")", ctx);
    BOOST_CHECK(/* result is Duration{2, 6} */);
    
    // Comparison
    auto result3 = evaluate_feel("duration(\"P2D\") < duration(\"P3D\")", ctx);
    BOOST_CHECK_EQUAL(result3, true);
}
```

**Expected Outcome:**
- All unit tests pass
- Edge cases covered

### Step 6: Create DMN Test Case

**Actions:**
- Create `tst/dmn/test_duration_rules.dmn` with decision table
- Create `tst/dmn/test_duration_rules.json` with test data

**DMN Structure:**
```xml
<decision name="TimeWindowCheck">
  <decisionTable hitPolicy="UNIQUE">
    <input label="Time Before Event">
      <inputExpression typeRef="duration">
        <text>timeBefore</text>
      </inputExpression>
    </input>
    <output label="Action">
      <outputExpression typeRef="string">
        <text>action</text>
      </outputExpression>
    </output>
    <rule>
      <inputEntry>
        <text>[duration("P5D")..duration("P10D")]</text>
      </inputEntry>
      <outputEntry>
        <text>"eligible"</text>
      </outputEntry>
    </rule>
    <rule>
      <inputEntry>
        <text>&lt; duration("P5D")</text>
      </inputEntry>
      <outputEntry>
        <text>"too_late"</text>
      </outputEntry>
    </rule>
    <rule>
      <inputEntry>
        <text>&gt; duration("P10D")</text>
      </inputEntry>
      <outputEntry>
        <text>"too_early"</text>
      </outputEntry>
    </rule>
  </decisionTable>
</decision>
```

**Test Data (JSON):**
```json
[
  {
    "input": {"timeBefore": "PT120H"},
    "expectedOutput": {"action": "eligible"},
    "description": "120 hours (5 days) - lower boundary"
  },
  {
    "input": {"timeBefore": "P7D"},
    "expectedOutput": {"action": "eligible"},
    "description": "7 days - middle of range"
  },
  {
    "input": {"timeBefore": "P2D"},
    "expectedOutput": {"action": "too_late"},
    "description": "2 days - below minimum"
  },
  {
    "input": {"timeBefore": "P15D"},
    "expectedOutput": {"action": "too_early"},
    "description": "15 days - above maximum"
  }
]
```

**Expected Outcome:**
- DMN file parses successfully
- All test cases pass when evaluated

### Step 7: Update Documentation

**Actions:**
- Add duration type to README features list (if applicable)
- Document duration syntax in code comments
- Reference DMN 1.5 Section 10.3.2.3 in implementation

**Expected Outcome:**
- Clear documentation for users
- DMN compliance referenced

## Success Criteria

- [ ] `duration()` function parses ISO 8601 days+hours format correctly
- [ ] All comparison operators work: `<`, `>`, `<=`, `>=`, `=`, `between`
- [ ] Duration equivalence: `PT48H` equals `P2D` (48 hours = 2 days)
- [ ] Unit tests pass (>90% coverage for duration code)
- [ ] DMN test case in `tst/dmn/` passes with 4+ test scenarios
- [ ] Error handling: invalid formats throw clear exceptions
- [ ] No regressions in existing FEEL tests
- [ ] Code follows CODING_STANDARDS.md (snake_case, const-correctness, [[nodiscard]])
- [ ] DMN 1.5 specification compliance verified

## Validation Steps

1. Build succeeds in Debug and Release: `cmake --build build --config Debug`
2. Unit tests pass: `.\build\Debug\tst_orion.exe --run_test=*duration* --log_level=all`
3. DMN test case evaluates correctly (run via test harness or standalone)
4. No regressions: `.\build\Debug\tst_orion.exe --log_level=test_suite`
5. TCK tests pass (if duration-related tests exist): `.\build\Debug\orion_tck_runner.exe`
6. Code review checklist passes

## Reference Documentation

- [DMN Feature Template](../prompts/add_dmn_feature.md) - Implementation workflow
- [DMN 1.5 Specification](../../docs/formal-24-01-01.txt) - Section 10.3.2.3 (duration type)
- [CODING_STANDARDS.md](../../CODING_STANDARDS.md) - Naming conventions, error handling
- [Build Instructions](../instructions/build.instructions.md) - Compilation and testing
- [Unit Test Instructions](../instructions/run_unit_tests.instructions.md) - Running specific tests
- [ISO 8601 Duration](https://en.wikipedia.org/wiki/ISO_8601#Durations) - Duration format reference

## Retrospective

### What worked well:
- **Pre-existing infrastructure discovery**: Found that 90% of duration support already existed (Duration struct, parse_duration(), comparison logic) - only needed to add the FEEL function wrapper
- **Efficient unit test creation**: Created 14 comprehensive unit tests covering all edge cases in one iteration
- **Clean function implementation**: duration() function implemented in 47 lines with proper null propagation and validation
- **Structured debugging approach**: When DMN tests failed, used systematic debug logging to trace execution flow through multiple layers
- **Root cause identification**: Debug output clearly revealed the issue (inputExpression field empty, expression text in label field)
- **Comprehensive DMN testing**: Decision table with 9 test cases validated hour-to-day equivalence, range matching, and boundary conditions

### What was unclear or problematic:
- **DMN parser bug in duplicate code**: Parser had the same bug in TWO locations (parse_dmn_xml and parse_input_clauses helper) - both needed fixing
- **Missing input expression evaluation**: DMN standard requires evaluating input expressions before unary test matching, but this functionality was not implemented
- **Architectural gap discovery**: Initial assumption was "just add duration() function", but uncovered missing core DMN feature (input expression evaluation)
- **Quote handling confusion**: Initially thought quote handling was the issue, but root cause was deeper (expressions not being evaluated at all)
- **Debug logging cleanup**: Removing debug statements required multiple attempts due to formatting sensitivity in exact string matching

### Suggestions for improvement:
- **Parser code duplication**: Consider refactoring DMN parser to eliminate duplicate input clause parsing code (DRY principle)
- **Input expression evaluation documentation**: Document that DMN input expressions are FEEL expressions requiring full evaluation pipeline (Lexer → Parser → AST → evaluate)
- **Test coverage at integration level**: Always test new FEEL functions in DMN decision tables, not just in unit tests - integration testing reveals architectural gaps
- **Debug logging strategy**: Use structured logging with clear prefixes ([DEBUG find_matching_rules], [DEBUG duration()]) to trace multi-layer execution
- **Early DMN testing**: Create DMN test cases earlier in implementation process to catch integration issues before extensive unit testing

### Actual effort:
- **Initial implementation**: 2 hours (duration() function + 14 unit tests)
- **Build integration**: 30 minutes (CMakeLists.txt, includes)
- **DMN test creation**: 1 hour (decision table, test data, test driver)
- **Extended debugging session**: 2.5 hours (tracing root cause, fixing parser, implementing expression evaluation)
- **Code cleanup**: 30 minutes (removing debug logging, final verification)
- **Total**: ~6.5 hours (vs estimated 4-6 hours)

### Blockers encountered:
- **DMN parser bug**: Discovered two locations in dmn_parser.cpp incorrectly setting `ic.label = expression_text` instead of `ic.inputExpression = expression_text`
- **Missing DMN feature**: Input expression evaluation was not implemented - required adding full FEEL evaluation pipeline in find_matching_rules()
- **Architectural assumption**: Initially assumed duration comparisons would "just work" through existing unary test infrastructure - required deep dive into DMN evaluation flow
- **Duplicate parsing code**: Parser had same logic in two places (inline and helper function), both needed fixing for tests to pass

### Technical learnings:
- **Duration comparison semantics**: PT120H correctly equals P5D (both = 432000 seconds total), P2DT12H = 2.5 days
- **DMN XML structure**: Input clauses have both label attribute (for display) and inputExpression text (for evaluation) - must be stored separately
- **Input expression evaluation flow**: Check if inputExpression non-empty → Tokenize → Parse → Evaluate with context → Use result for unary test matching
- **Three-valued logic**: Duration comparisons properly handle null propagation (null input → null result → no match)
- **Quote handling in comparisons**: Added unquote() calls in cmp_values to handle quoted string literals from DMN XML

### Unexpected discoveries:
- **90% existing infrastructure**: Duration struct, full ISO 8601 parser, and comparison operators already implemented - only missing FEEL function wrapper
- **Input expression evaluation gap**: Core DMN feature was not implemented - all input clauses used label lookup instead of evaluating expressions
- **Parser duplication bug**: Same bug existed in two separate parsing functions, suggesting code duplication issue
- **Full ISO 8601 support**: parse_duration() already supports years, months, days, hours, minutes, seconds - scope limitation was conservative

### Success metrics:
- ✅ All 14 unit tests passing (100% coverage)
- ✅ All 9 DMN test cases passing (100% coverage)
- ✅ Full regression suite passing (296/296 tests, 0 regressions)
- ✅ Production-ready clean code (all debug logging removed)
- ✅ DMN compliance verified (input expressions now evaluated correctly)
- ✅ Performance: Duration comparison tests run in 17.4ms total
