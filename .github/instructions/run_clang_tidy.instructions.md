# Clang-Tidy Static Analysis Instructions

## Prerequisites

- Build completed successfully with `CMAKE_EXPORT_COMPILE_COMMANDS=ON`
- `compile_commands.json` exists in build directory
- clang-tidy installed (comes with LLVM/Clang toolchain)

## Project Configuration

The project uses `.clang-tidy` configuration file in the root directory:

**Enabled Check Categories:**
- `bugprone-*` - Bug-prone code patterns
- `clang-analyzer-*` - Static analysis checks
- `cppcoreguidelines-*` - C++ Core Guidelines compliance
- `modernize-*` - Modern C++ feature usage
- `performance-*` - Performance optimizations
- `readability-*` - Code readability improvements
- `portability-*` - Cross-platform portability

**Naming Conventions (Enforced):**
- Classes: CamelCase (e.g., `BusinessRulesEngine`)
- Functions/Methods: lower_case (snake_case, e.g., `evaluate_expression`)
- Variables: lower_case (snake_case, e.g., `decision_table`)

**Disabled Checks:**
- `modernize-use-trailing-return-type` - Project doesn't require trailing returns
- `readability-magic-numbers` - Too many false positives
- `cppcoreguidelines-avoid-magic-numbers` - Too restrictive for constants

## Running Clang-Tidy

### Windows (PowerShell)

**Analyze Single File:**
```powershell
# Using relative path (from project root)
clang-tidy src\bre\bkm_manager.cpp -p build\

# Using absolute path (more reliable in scripts)
clang-tidy C:\workspace\orion\src\bre\bkm_manager.cpp -p C:\workspace\orion\build\
```

**Analyze Multiple Files:**
```powershell
# All source files in a directory
Get-ChildItem -Path src\bre -Filter *.cpp -Recurse | ForEach-Object {
    clang-tidy $_.FullName -p build\
}

# Specific file pattern
clang-tidy src\bre\feel\*.cpp -p build\
```

**Analyze Specific Category:**
```powershell
# Only naming conventions
clang-tidy src\bre\bkm_manager.cpp -p build\ --checks='-*,readability-identifier-naming'

# Only modernization issues
clang-tidy src\bre\bkm_manager.cpp -p build\ --checks='-*,modernize-*'

# Only performance issues
clang-tidy src\bre\bkm_manager.cpp -p build\ --checks='-*,performance-*'
```

**List All Enabled Checks:**
```powershell
clang-tidy --list-checks -p build\
```

### Linux (Bash)

**Analyze Single File:**
```bash
# Using relative path (from project root)
clang-tidy src/bre/bkm_manager.cpp -p build-debug/

# Using absolute path (more reliable in scripts)
clang-tidy /workspace/orion/src/bre/bkm_manager.cpp -p /workspace/orion/build-debug/
```

**Analyze Multiple Files:**
```bash
# All source files in a directory
find src/bre -name "*.cpp" -exec clang-tidy {} -p build-debug/ \;

# All source files in project
find src -name "*.cpp" -exec clang-tidy {} -p build-debug/ \;

# Specific file pattern
clang-tidy src/bre/feel/*.cpp -p build-debug/
```

**Analyze Specific Category:**
```bash
# Only naming conventions
clang-tidy src/bre/bkm_manager.cpp -p build-debug/ --checks='-*,readability-identifier-naming'

# Only modernization issues
clang-tidy src/bre/bkm_manager.cpp -p build-debug/ --checks='-*,modernize-*'

# Only performance issues
clang-tidy src/bre/bkm_manager.cpp -p build-debug/ --checks='-*,performance-*'
```

**List All Enabled Checks:**
```bash
clang-tidy --list-checks -p build-debug/
```

## Common Options

**`-p <build-path>`** (Required)
- Path to build directory containing `compile_commands.json`
- Provides compilation flags and include paths to clang-tidy
- **MUST** be generated with `CMAKE_EXPORT_COMPILE_COMMANDS=ON` (see [build.md](./build.md))

**`--checks=<pattern>`** (Optional)
- Override enabled checks from `.clang-tidy` file
- Use `-*,category-*` to enable only specific category
- Examples:
  - `-*,modernize-*` - Only modernization checks
  - `-*,readability-identifier-naming` - Only naming checks
  - `*` - All available checks (very verbose)

**`--fix` or `--fix-errors`** (Use with caution)
- Automatically apply fixes for detected issues
- `--fix` - Apply fixes for warnings
- `--fix-errors` - Apply fixes for errors and warnings
- **WARNING**: Always review changes before committing
- **RECOMMENDED**: Run on single files, not entire codebase

**`--format-style=<style>`** (Optional)
- Reformat fixed code using clang-format style
- Values: `llvm`, `google`, `chromium`, `mozilla`, `webkit`, `file` (uses `.clang-format`)
- Project uses `FormatStyle: none` in `.clang-tidy` (manual formatting)

## Expected Output

**No Issues Found:**
```
47 warnings generated.
Suppressed 47 warnings (47 in non-user code).
Use -header-filter=.* to display errors from all non-system headers.
```

**Issues Found:**
```
C:\workspace\orion\src\bre\bkm_manager.cpp:45:23: warning: parameter 'name' can be declared as std::string_view [modernize-use-string-view]
void add_bkm(const std::string& name) {
                      ^
C:\workspace\orion\src\bre\bkm_manager.cpp:67:15: warning: method 'get_bkm' can be marked const [readability-make-member-function-const]
BKM* get_bkm(std::string_view name) {
              ^
                                       const
```

**Output Format:**
- File path and line number
- Warning or error severity
- Diagnostic message
- Check name in brackets (e.g., `[modernize-use-string-view]`)
- Code context with suggested fix (if available)

## Integration with Code Quality Workflow

### Manual Analysis (One-Time Check)

**Before committing changes:**
```powershell
# Windows: Analyze all changed files
git diff --name-only --diff-filter=d | Where-Object { $_ -match '\.cpp$' } | ForEach-Object {
    clang-tidy $_ -p build\
}
```

```bash
# Linux: Analyze all changed files
git diff --name-only --diff-filter=d | grep '\.cpp$' | xargs -I {} clang-tidy {} -p build-debug/
```

### Iterative Refactoring (Agent Workflow)

See [improve_quality.md](../prompts/improve_quality.md) template for systematic quality improvement.

**Agent usage pattern:**
1. Run clang-tidy to generate issue list
2. Enumerate all issues in structured format
3. Apply fixes iteratively with build/test verification
4. Re-run clang-tidy to verify all issues resolved

**Agent command example:**
```bash
# Generate issue list for specific category
clang-tidy src/bre/*.cpp -p build-debug/ --checks='-*,modernize-use-string-view' > issues.txt

# Parse issues.txt to enumerate findings
# Apply fixes one-by-one with verification
# Re-run to confirm all resolved
```

## CI/CD Integration

The project includes a GitHub Actions workflow for automated clang-tidy checks:

**Workflow:** `.github/workflows/quality-clang-tidy.yml`
- Runs on: Manual trigger or scheduled
- Scope: All source files in `src/` and `tst/`
- Configuration: Uses `.clang-tidy` from repository root
- Failure: Fails workflow if any issues found

**Local pre-push check:**
```powershell
# Windows: Quick check before pushing
clang-tidy src\bre\*.cpp -p build\ --quiet
```

```bash
# Linux: Quick check before pushing
find src -name "*.cpp" -exec clang-tidy {} -p build-debug/ --quiet \;
```

## Troubleshooting

**"compile_commands.json not found"**
- Ensure CMake was configured with `CMAKE_EXPORT_COMPILE_COMMANDS=ON`
- See [build.md](./build.md) for proper CMake configuration
- Re-run CMake configuration if needed

**"too many warnings from system headers"**
- Project uses `HeaderFilterRegex: '(include/orion|src|tst)/.*'` to filter non-project files
- Suppressed warnings from vcpkg dependencies and system headers
- Use `-header-filter=.*` to see all warnings (not recommended)

**"unknown warning option"**
- Some checks require specific clang-tidy version
- Project targets clang-tidy 14+ (comes with Clang 14+)
- Update LLVM toolchain if using older version

**"conflicting check suggestions"**
- Some checks may conflict (e.g., `modernize-*` vs `readability-*`)
- Use judgment to apply appropriate fix
- Consult [CODING_STANDARDS.md](../../CODING_STANDARDS.md) for project conventions
- Can disable specific checks in `.clang-tidy` if consistently problematic

**Slow performance on large codebase**
- Run on individual files instead of entire codebase
- Use parallel execution with xargs/PowerShell parallel
- Consider using compilation database filtering

## Reference Documentation

- [CODING_STANDARDS.md](../../CODING_STANDARDS.md) - Complete coding standards and naming conventions
- [Code Review Checklist](./code_review_checklist.md) - Manual review criteria
- [Code Quality Template](../prompts/improve_quality.md) - Systematic quality improvement workflow
- [Clang-Tidy Documentation](https://clang.llvm.org/extra/clang-tidy/) - Official clang-tidy reference
