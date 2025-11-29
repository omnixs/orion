# Testing & TCK Guide

## 1. Overview

Orion includes:
- **Unit tests** (C++ based)
- **DMN Technology Compatibility Kit (TCK)** tests
- **TCK harness** for verifying correctness against the DMN specification

This document explains how to run all tests and interpret results.

---

## 2. Building Tests

### Windows (Visual Studio)
```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 `
  -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Debug
```

### Linux (Ninja + GCC)
```bash
cmake -S . -B build-debug -G Ninja `
  -DCMAKE_BUILD_TYPE=Debug `
  -DCMAKE_TOOLCHAIN_FILE=~/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build-debug -j$(nproc)
```

Test binaries are generated:
- Windows: `build\Debug\tst_orion.exe`
- Linux: `build-debug/tst_orion`

---

## 3. Running Unit Tests

**Windows:**
```powershell
.\build\Debug\tst_orion.exe --log_level=test_suite
```

**Linux:**
```bash
./build-debug/tst_orion --log_level=test_suite
```

Expected output:
```
Running 279 test cases...
*** No errors detected
```

---

## 4. Running DMN TCK Tests

### 4.1 Quick (Selective) TCK Run
Runs Level 2 (all 126 tests) + passing Level 3 tests (~35 test directories):

**Windows:**
```powershell
.\build\Debug\tst_orion.exe --run_test=dmn_tck_levels/dmn_tck_comprehensive
```

**Linux:**
```bash
./build-debug/tst_orion --run_test=dmn_tck_levels/dmn_tck_comprehensive
```

Expected runtime: ~0.9 seconds

### 4.2 Full TCK Run (All 3,535 Test Cases)
Runs ALL Level 2 + Level 3 tests (comprehensive validation):

**Windows:**
```powershell
$env:ORION_TCK_RUN_ALL="1"
.\build\Debug\tst_orion.exe --run_test=dmn_tck_levels/dmn_tck_comprehensive
```

**Linux:**
```bash
export ORION_TCK_RUN_ALL=1
./build-debug/tst_orion --run_test=dmn_tck_levels/dmn_tck_comprehensive
```

Expected runtime: ~25 seconds (Debug mode)

Note:
- Level 2 tests ALWAYS run in full (DMN compliance requirement)
- Full mode runs all ~120 Level 3 test directories
- Use for comprehensive validation or debugging

---

## 5. TCK Results

The TCK runner outputs results to console with:
- Per-test-directory summaries (e.g., "0032-conditionals: 6/6 passed (100.0%)")
- Overall statistics:
  - Level 2: X/126 tests
  - Level 3: X/3,535 tests
  - Success rate percentages

Example output:
```
[info] Finished: test_cases=3535 passed=484 failed=3051
[info] DMN TCK Comprehensive Summary: 484/3535 (13.7% success rate)
```

For detailed diagnostics, check console output during test execution.

---

## 6. Debugging Failures

If a TCK or unit test fails:

1. Locate failing test case in output directory.
2. Inspect JSON evaluation logs.
3. Compare expected vs actual FEEL expression results.
4. If needed, re-run the specific test in verbose mode.

---

## 7. Tips for Contributors

- Before PR submission: run **unit tests** + **quick TCK**.
- For major changes: run **full TCK**.
- Ensure no new warnings appear under Clang‑Tidy.
- Include new tests when adding FEEL functions or rule types.

---

## 8. Summary

Orion's comprehensive test suite includes:
- **279 unit tests** (C++ test cases)
- **3,535 TCK tests** (DMN specification compliance)
- **100% Level 2 compliance** (126/126 tests)
- **13.7% Level 3 coverage** (484/3,535 tests)

Contributors can confidently validate correctness for any code change using these automated tests.
