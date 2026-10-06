# Code Review Checklist

## Overview

This checklist defines the quality gates for all code changes in the ORION project. Use this for:
- Inline review checks during refactoring iterations
- Comprehensive code reviews before merge
- Self-review before committing changes

## Core Review Criteria

### 1. Naming & Style (clang-tidy enforced)

**Classes:**
- [ ] CamelCase naming (e.g., `BusinessRulesEngine`, `DecisionTable`)
- [ ] Proper namespace hierarchy (`orion::api::`, `orion::bre::`)

**Functions & Variables:**
- [ ] snake_case naming (e.g., `evaluate_expression`, `load_dmn_model`)
- [ ] Private members have trailing underscore (e.g., `bkms_`, `model_name_`)

**Constants:**
- [ ] ALL_CAPS for static constants (e.g., `MAX_RULES`, `MONTHS_PER_YEAR`)

**Formatting:**
- [ ] 4 spaces per indentation level (NEVER tabs)
- [ ] Consistent brace style

### 2. Correctness

**Logic Flow:**
- [ ] Correct algorithm implementation
- [ ] No off-by-one errors
- [ ] Proper null/empty checks
- [ ] Three-valued logic for DMN (null propagation)

**Error Handling:**
- [ ] `ContractViolation` for programming errors (precondition violations)
- [ ] `std::runtime_error` for business logic errors (current, migrate to `std::expected`)
- [ ] Proper error messages with context
- [ ] No silent failures

**Lifetime Management:**
- [ ] No dangling references
- [ ] Proper use of move semantics
- [ ] RAII for all resources
- [ ] Smart pointers for ownership

### 3. Standards Compliance (CODING_STANDARDS.md)

**Modern C++:**
- [ ] `[[nodiscard]]` on functions returning values that shouldn't be ignored
- [ ] `noexcept` where appropriate (destructors, move operations, non-throwing functions)
- [ ] `std::string_view` for read-only string parameters
- [ ] Const-correctness (const methods, const references, const local variables)
- [ ] Range-based for loops where applicable
- [ ] Structured bindings for pairs/tuples

**Memory Management:**
- [ ] `std::unique_ptr` for exclusive ownership
- [ ] `std::shared_ptr` only when truly needed
- [ ] No raw `new`/`delete`
- [ ] RAII for all resources

**Static and Global State (CRITICAL - see CODING_STANDARDS.md "Static and Global State"):**
- [ ] Every new function-local `static`, namespace-scope variable, `static` data member and `thread_local` is `constexpr` or `constinit`
- [ ] No lazily built lookup tables (`std::map`/`unordered_map`/`set`/`vector`/`string`/`json` in statics) - use `constexpr std::array` + `std::ranges::lower_bound`/`find`
- [ ] No allocating singletons (`static Foo instance;`), `std::call_once`/`once_flag`, or `thread_local` caches
- [ ] Caches are owned by an engine/`EvaluationContext` instance
- [ ] New uses of standard library facilities with process-lifetime caches (`std::chrono::locate_zone`, `std::regex`, `std::locale`) are documented in CODING_STANDARDS.md
- [ ] Any `// static-init-ok:` exemption is justified and approved
- [ ] `tools/scripts/check_static_init.ps1` passes

**No Hardcoded Values (CRITICAL for BRE):**
- [ ] No test-specific function names (e.g., "PMT", "Payment")
- [ ] No test-specific property names (e.g., "fee", "amount", "rate")
- [ ] No test-specific numeric values (e.g., 600000, 0.0375, 360)
- [ ] No test-specific string literals (e.g., "Hello World", "John Doe")
- [ ] Generic constants only (e.g., `MONTHS_PER_YEAR = 12`)

**DMN 1.5 Compliance:**
- [ ] Conforms to DMN specification sections
- [ ] Generic patterns work with any DMN model
- [ ] Proper hit policy implementation
- [ ] FEEL expression semantics correct

### 4. Performance Impact

**Allocations:**
- [ ] No unnecessary string copies
- [ ] `std::string_view` used for read-only operations
- [ ] Move semantics for large objects
- [ ] Reserve capacity for vectors when size known

**Efficiency:**
- [ ] No allocations in hot paths
- [ ] Efficient data structure choices
- [ ] Consider cache locality for performance-critical code

**String Operations:**
- [ ] Use `string_view` for parsing, tokenization, comparisons
- [ ] Convert to `std::string` only when storing
- [ ] No repeated allocations in loops

## Special Focus Areas

### For String_view Changes

**Lifetime Safety (CRITICAL):**
```cpp
// ✅ GOOD: Safe usage
void process(std::string_view name) {
    if (name.empty()) return;
    std::string stored_name(name);  // Convert to string when storing
    cache_[stored_name] = ...;
}

// ❌ BAD: Dangling reference
std::string_view get_name() {
    std::string temp = "value";
    return temp;  // DANGER: temp destroyed, view is invalid
}

// ❌ BAD: Storing string_view
class Foo {
    std::string_view name_;  // DANGER: What does this point to?
public:
    Foo(std::string_view n) : name_(n) {}  // Caller's string must outlive Foo
};
```

**Checklist:**
- [ ] Not returned from functions returning local variables
- [ ] Not stored as class members (unless lifetime guaranteed)
- [ ] Converted to `std::string` when storing in containers
- [ ] Safe with structured bindings (underlying object lives long enough)

### For FEEL Evaluator Changes

**DMN Compliance:**
- [ ] Reference DMN 1.5 spec sections in comments
- [ ] Verify against TCK test cases
- [ ] Generic patterns (no test-specific logic)
- [ ] Null handling follows three-valued logic
- [ ] Proper type coercion rules

**Example:**
```cpp
// ✅ GOOD: Generic, DMN-compliant
if (left_val.is_null() || right_val.is_null()) {
    return nlohmann::json{};  // DMN 1.5 Section 10.3.2.9: null propagation
}

// ❌ BAD: Hardcoded test logic
if (expression.find("PMT") != std::string::npos) {
    // Special case for loan payment...
}
```

### For Performance-Critical Code

**Hot Path Optimization:**
- [ ] Profile before optimizing
- [ ] No allocations in loops
- [ ] Cache-friendly data structures
- [ ] Consider `string_view` for all string operations
- [ ] Benchmark changes (use orion-bench)

**Example:**
```cpp
// ✅ GOOD: Zero-allocation parsing
std::vector<std::string_view> tokenize(std::string_view input) {
    std::vector<std::string_view> tokens;
    tokens.reserve(10);  // Reserve expected size
    // Parse without allocating strings...
}

// ❌ BAD: Allocates for every token
std::vector<std::string> tokenize(const std::string& input) {
    std::vector<std::string> tokens;
    // Each token allocates a new string...
}
```

### For Lookup Tables, Caches and Singletons

Hosts embed ORION in processes that fence/profile heap usage per command. A static built on first
use allocates inside the host's command and lives until process exit, so it is reported as a leak.

**Checklist:**
- [ ] Lookup tables are `constexpr` (compile-time sorted `std::array` + `std::ranges::lower_bound`)
- [ ] Caches live in an engine/`EvaluationContext` instance, never in a static or `thread_local`
- [ ] Global objects (if unavoidable) have a non-allocating `constexpr` constructor and are `constinit`
- [ ] Mutable global state is thread-safe (`std::atomic`, `std::atomic<std::shared_ptr<T>>`)

**Example:**
```cpp
// ❌ BAD: lazily allocated on first call, never freed before process exit
static const std::set<std::string> builtins = {"abs", "floor"};

// ✅ GOOD: no heap, no runtime initialization, no init guard
static constexpr auto builtins = std::to_array<std::string_view>({"abs", "floor"});
bool is_builtin = std::ranges::find(builtins, name) != builtins.end();
```

### For Parser Changes

**Error Handling:**
- [ ] Helpful error messages with context
- [ ] Include location information (line, column)
- [ ] Suggest fixes when possible
- [ ] No cryptic error codes

**Robustness:**
- [ ] Handle malformed input gracefully
- [ ] No crashes on invalid input
- [ ] Proper recovery strategies
- [ ] Comprehensive test coverage for edge cases

## Review Feedback Format

When providing review feedback, use this structure:

```
Issue: [Brief description]
Location: [file:line or file:function]
Category: [Correctness/Performance/Standards/Security]

Current:
  [Current code snippet]

Problem:
[Detailed explanation of what's wrong and why]

Suggested fix:
  [Corrected code snippet]

Reference: [CODING_STANDARDS.md section or DMN spec section]
```

## Severity Levels

**CRITICAL:** Must fix before merge
- Memory leaks, dangling references
- Static/thread-local variables that are not `constexpr`/`constinit` (lazy or dynamic initialization)
- Hardcoded test values in production code
- DMN spec violations
- Security issues

**WARNING:** Should fix before merge
- Performance issues (unnecessary allocations)
- Missing const-correctness
- Missing [[nodiscard]]
- Non-idiomatic code

**SUGGESTION:** Consider for improvement
- Refactoring opportunities
- Additional comments for clarity
- Alternative implementations

## Pass/Fail Criteria

**PASS if:**
- All CRITICAL issues resolved
- No CODING_STANDARDS.md violations
- No hardcoded test values
- `tools/scripts/check_static_init.ps1` passes
- All tests pass (unit + TCK)
- No performance regressions

**FAIL if:**
- Any CRITICAL issue unresolved
- Hardcoded values detected
- Runtime-initialized static or thread-local state introduced
- DMN compliance violations
- Test failures or regressions

---

*Reference: CODING_STANDARDS.md, DMN 1.5 Specification (docs/formal-24-01-01.txt)*
