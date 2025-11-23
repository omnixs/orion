# Code Quality Improvement Template

## Objective

Systematically improve code quality using automated analysis tools (clang-tidy) in iterative cycles.

## Scope

Target files for quality improvement (specify in task instance):
- [ ] Specific component or directory
- [ ] Identified issues (naming, const-correctness, modernization)
- [ ] Compliance requirements (CODING_STANDARDS.md)

## Key Process Steps

### 1. Analysis
Follow [build instructions](../instructions/build.md) to ensure compile_commands.json exists:

**Both Platforms:**
```bash
# Run static analysis (requires compile_commands.json from CMake)
# The .clang-tidy file in the project root is automatically used
clang-tidy <file> -p build/

# Or with absolute paths (more reliable in scripts)
clang-tidy /path/to/file.cpp -p /path/to/build/

# Check specific category only
clang-tidy <file> -p build/ --checks='-*,readability-identifier-naming'

# List all enabled checks
clang-tidy --list-checks -p build/
```

**Note:** The `.clang-tidy` configuration file in the project root defines:
- Enabled check categories (bugprone, modernize, readability, performance, etc.)
- Naming conventions (CamelCase classes, snake_case functions/variables)
- Header filter regex for project files only

### 2. Fix Application
- Follow CODING_STANDARDS.md priority
- Apply fixes for identified issues
- Maintain backward compatibility

### 3. Verification
Follow [build instructions](../instructions/build.md) and [test instructions](../instructions/run_unit_tests.md):

**Windows (PowerShell):**
```powershell
# Build (see build.md for details)
cmake --build build --config Debug

# Unit tests (see run_unit_tests.md for options)
.\build\Debug\tst_orion.exe --log_level=test_suite

# TCK compliance (see run_tck_tests.md for details)
.\build\Debug\orion_tck_runner.exe --log_level=error
```

**Linux (Bash):**
```bash
# Build (see build.md for details)
cmake --build build-debug -j$(nproc)

# Unit tests (see run_unit_tests.md for options)
./build-debug/tst_orion --log_level=test_suite

# TCK compliance (see run_tck_tests.md for details)
./build-debug/orion_tck_runner --log_level=error
```

### 4. Iteration
- Continue until all issues resolved
- One logical component per commit
- No regressions in tests

## Success Criteria

- [ ] All clang-tidy issues resolved
- [ ] No test regressions (unit + TCK)
- [ ] Code follows CODING_STANDARDS.md
- [ ] Performance maintained (<5% change)

## Critical Constraints

**Forbidden Actions:**
- ❌ No complex shell commands (piping, redirection)
- ❌ No custom build executables
- ❌ No echo/cat for narration

**Allowed Actions:**
- ✅ File editing tools
- ✅ Simple build/test commands
- ✅ Read/search tools

## Retrospective

(This section will be filled after task completion with learnings and improvements)
