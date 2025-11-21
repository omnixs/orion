# Copilot Instructions for ORION Business Rules Engine

## Project Overview

ORION is a native C++ DMN™ Level 1 rule engine focused on decision tables with an emphasis on performance, correctness, and clean integration. The project prioritizes **generic solutions only** - no hardcoded values, domain-specific assumptions, or test-specific code patterns.

## Quick Start Resources

**Build & Test:**
- [Build Instructions](./instructions/build.md) - CMake, vcpkg, compilation
- [Unit Tests](./instructions/run_unit_tests.md) - Boost Test execution
- [TCK Tests](./instructions/run_tck_tests.md) - DMN compliance validation
- [Performance Tests](./instructions/run_perf_tests.md) - Benchmarking

**Development:**
- [DMN Feature Template](./prompts/add_dmn_feature.md) - Implement new FEEL features
- [Code Quality Template](./prompts/improve_quality.md) - Iterative quality improvements
- [Performance Template](./prompts/improve_perf.md) - Profile and optimize
- [Bug Fix Template](./prompts/fix_bug.md) - Debug and fix issues

**Standards:**
- [CODING_STANDARDS.md](../CODING_STANDARDS.md) - Naming, error handling, memory management
- DMN 1.5 Spec: `docs/formal-24-01-01.txt` - Official OMG specification

## Architecture Overview

### Core Components (`src/bre/`)
- **`engine.hpp/cpp`** - Main `BusinessRulesEngine` with stateful model management
- **`dmn_parser.hpp/cpp`** - DMN XML parsing (rapidxml)
- **`feel_evaluator.hpp/cpp`** - FEEL expression evaluation
- **`dmn_model.hpp`** - Data structures: `DecisionTable`, `LiteralDecision`
- **`hit_policy.hpp/cpp`** - FIRST, UNIQUE, COLLECT with aggregations
- **`bkm_manager.hpp/cpp`** - Business Knowledge Model management

### Key Usage Pattern

```cpp
// ✅ CORRECT: Load once, evaluate many (9-45 μs/eval)
orion::bre::BusinessRulesEngine engine;
std::string error;
engine.load_dmn_model(dmn_xml, error);  // One-time: 100-200μs

for (auto& request : requests) {
    std::string result = engine.evaluate(request_json);  // 9-45μs each
}

// ❌ WRONG: Re-parse every time (100-150 μs/eval - 10x slower!)
for (auto& request : requests) {
    std::string result = evaluate(dmn_xml, request_json);  // DEPRECATED
}
```

**Performance:** 22,000-110,000 evaluations/second with cached models

## Development Workflow

### Adding Features
See [DMN Feature Template](./prompts/add_dmn_feature.md) for full process. Quick steps:
1. Update headers in `include/orion/bre/`
2. Implement in `src/bre/`
3. Add tests in `tst/bre/`
4. Update `CMakeLists.txt` if needed
5. Validate DMN 1.5 compliance

### Working with Tests
- **Unit Tests**: Boost Test in `tst/bre/`, focus on edge cases
- **TCK Tests**: Official DMN compliance in `dat/dmn-tck/TestCases/`
- **NEVER hardcode test data** - parse from XML test files
- Validate DMN spec compliance, not just implementation

```cpp
// ✅ CORRECT: Parse from official test files
nlohmann::json test_input = parseTestInputFromXML(test_xml_file);

// ❌ WRONG: Hardcoded data
test_input = {{"Monthly Salary", 10000}};
```

### Code Standards
- **Naming**: CamelCase classes, snake_case functions (enforced by clang-tidy)
- **Error Handling**: `ContractViolation` for programming errors, `std::expected` for business logic
- **Memory**: RAII, smart pointers, move semantics
- See [CODING_STANDARDS.md](../CODING_STANDARDS.md) for complete guidelines

## Task Retrospective Process

After completing tasks from `.github/tasks/`:

1. **Ask for User Feedback** - What worked? What was unclear?
2. **Analyze Execution** - Review conversation for blockers/ambiguities
3. **Document in Task File** - Add to Retrospective section with user feedback + analysis
4. **Update Template** - If patterns emerge across multiple tasks

**Quality Criteria:** Be specific (reference exact issues), actionable (suggest improvements), brief (3-5 bullets max)

## Command Execution Rules

**WHY:** VS Code AI requires simple commands for automation. Complex shell operations (pipes, redirection, chaining) need manual approval.

### ❌ Never Use
- **Pipes**: `command1 | command2` → Use `read_file` + analysis
- **Redirection**: `command > file` → Capture output, create markdown if needed
- **Chaining**: `cmd1 && cmd2` → Run separately, verify each
- **Echo/Cat**: `echo "text" && cmd` → Report in your response
- **Text Processing**: `awk/sed/grep` → Use VS Code tools

### ✅ Always Use
- **Simple commands**: `cmake --build build`, `./build/tst_orion --log_level=test_suite`
- **File tools**: `read_file`, `replace_string_in_file`, `grep_search`, `file_search`
- **One at a time**: Run command → verify → report → next command

### Tool Alternatives
| Instead of | Use |
|-----------|-----|
| `cat file.log` | `read_file("file.log")` |
| `grep "error" file` | `grep_search(query="error", includePattern="file")` |
| `find . -name "*.cpp"` | `file_search(query="**/*.cpp")` |
| `cmd \| grep pattern` | Run cmd, analyze in response |
| `cmd > output.log` | Run cmd, create markdown in `docs/replies/` |

## Common Workflows

### Build & Test Cycle
```powershell
# 1. Build
cmake --build build

# 2. Unit tests (verify build succeeded first)
.\build\tst_orion.exe --log_level=test_suite

# 3. TCK tests (verify unit tests passed first)
.\build\orion_tck_runner.exe --log_level=error

# 4. Performance check (optional)
.\build\orion-bench.exe --benchmark_repetitions=3
```

### Debug Failing Test
1. Run test with verbose: `.\build\tst_orion.exe --run_test=test_name --log_level=all`
2. Read source files to understand code
3. Add `BOOST_TEST_MESSAGE` or `spdlog::debug` for tracing
4. Fix issue using file edit tools
5. Re-run test to verify

### Code Quality Iteration
See [Code Quality Template](./prompts/improve_quality.md). Quick version:
1. Analyze: `clang-tidy file.cpp -p build/`
2. Fix issues in file
3. Build & test
4. Commit if all pass

---

## Dependencies

**Build System:**
- CMake 3.20+, vcpkg for dependencies
- C++23 standard (fallback to C++20)
- See [build.md](./instructions/build.md) for setup

**Core Libraries:**
- **spdlog** - Logging
- **nlohmann-json** - JSON processing
- **rapidxml** - XML parsing (header-only)
- **boost-test** - Unit testing
- **benchmark** - Performance (optional)

See [CODING_STANDARDS.md](../CODING_STANDARDS.md) for dependency guidelines.