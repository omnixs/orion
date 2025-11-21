# Bug Fix Template

## Objective

Fix identified bug with minimal changes and comprehensive verification.

## Bug Details (specify in task instance)

- [ ] Description
- [ ] Reproduction steps
- [ ] Expected vs actual behavior
- [ ] Affected components

## Fix Process

### 1. Reproduce
- Create minimal test case
- Verify bug exists

### 2. Root Cause Analysis
- Use debugger/logging
- Identify exact failure point

### 3. Fix Implementation
- Minimal change to fix root cause
- Follow CODING_STANDARDS.md

### 4. Verification
Follow [unit test instructions](../instructions/run_unit_tests.md), [TCK test instructions](../instructions/run_tck_tests.md), and [performance test instructions](../instructions/run_perf_tests.md):
```powershell
# Unit tests (see run_unit_tests.md for verbose output options)
.\build\tst_orion.exe --log_level=all

# TCK tests (see run_tck_tests.md for error-only output)
.\build\orion_tck_runner.exe --log_level=error

# Regression check (see run_perf_tests.md for repetition guidelines)
.\build\orion-bench.exe --benchmark_repetitions=3
```

## Success Criteria

- [ ] Bug no longer reproducible
- [ ] Test case added to prevent regression
- [ ] No new bugs introduced
- [ ] All tests pass

## Retrospective

(This section will be filled after task completion with learnings and improvements)
