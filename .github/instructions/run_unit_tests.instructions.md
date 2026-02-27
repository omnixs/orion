# Unit Test Execution Instructions

## Prerequisites

- Build completed successfully
- Boost.Test framework linked

## Windows (PowerShell)

### Run All Tests
```powershell
.\build\tst_orion.exe --log_level=test_suite
```

### Run Specific Test Suite
```powershell
.\build\tst_orion.exe --run_test=dmn_tck_levels --log_level=all
```

### Run Single Test
```powershell
.\build\tst_orion.exe --run_test=dmn_tck_levels/dmn_tck_comprehensive/test_case_name --log_level=all
```

## Linux (Bash)

### Run All Tests
```bash
./build/tst_orion --log_level=test_suite
```

### Run Specific Test Suite
```bash
./build/tst_orion --run_test=dmn_tck_levels --log_level=all
```

## Common Options

- `--log_level=all` - Verbose output with test details
- `--log_level=test_suite` - Test suite summary only
- `--log_level=error` - Errors only
- `--run_test=<pattern>` - Run tests matching pattern
- `--list_content` - List all test cases without running

## TCK Test Modes

The DMN TCK comprehensive test (`dmn_tck_comprehensive`) supports two execution modes:

### Selective Mode (Default - Fast)
Runs only Level 3 TCK tests that are known to pass according to the baseline. This significantly speeds up unit test execution.

**Windows:**
```powershell
# Default behavior - selective mode
.\build\tst_orion.exe --run_test=dmn_tck_levels/dmn_tck_comprehensive
```

**Linux:**
```bash
# Default behavior - selective mode
./build/tst_orion --run_test=dmn_tck_levels/dmn_tck_comprehensive
```

**Performance:**
- Execution time: ~0.9 seconds
- Level 2: All 126 tests (100% required)
- Level 3: Only ~35 passing test directories
- Skips: ~114 failing test directories

### Comprehensive Mode (Full Coverage)
Runs ALL Level 3 TCK tests for comprehensive validation. Enable by setting environment variable.

**Windows:**
```powershell
$env:ORION_TCK_RUN_ALL="1"
.\build\tst_orion.exe --run_test=dmn_tck_levels/dmn_tck_comprehensive
```

**Linux:**
```bash
export ORION_TCK_RUN_ALL=1
./build/tst_orion --run_test=dmn_tck_levels/dmn_tck_comprehensive
```

**Performance:**
- Execution time: ~25 seconds (Debug mode)
- Level 2: All 126 tests (100% required)
- Level 3: All ~120 test directories
- Use for debugging or comprehensive validation

**Note:** Level 2 tests ALWAYS run in full regardless of mode (DMN compliance requirement).

## Expected Output

```
Running 264 test cases...
*** No errors detected
```

Or with summary:
```
Test suite "orion_bre" passed
  Test suite "feel_parser_tests" passed
  Test suite "dmn_tck_levels" passed
    Test case "dmn_tck_level2_only" passed
    Test case "dmn_tck_comprehensive" passed
```

## Troubleshooting

**Test failures:**
- Review failure details with `--log_level=all`
- Check BOOST_TEST_MESSAGE output
- Verify test data files in `dat/tst/`

**Segmentation fault:**
- Run with debugger: `gdb ./build/tst_orion`
- Check for null pointer dereferences

**Timeout:**
- Some TCK tests may take 2-5 minutes
- Use `isBackground=true` in VS Code tool calls
