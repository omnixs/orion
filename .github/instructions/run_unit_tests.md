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
