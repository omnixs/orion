# TCK Compliance Test Instructions

## Prerequisites

- Build completed successfully
- TCK test data in `dat/dmn-tck/`
- Environment variable `ORION_TCK_ROOT` set (optional)

## Windows (PowerShell)

### Run All TCK Tests
```powershell
.\build\orion_tck_runner.exe --log_level=error
```

### Run Specific Test Pattern
```powershell
.\build\orion_tck_runner.exe --test 0032-conditionals --log_level=all
```

### Run Level 2 Only
```powershell
.\build\tst_orion.exe --run_test="dmn_tck_levels/dmn_tck_level2_only" --log_level=test_suite
```

## Linux (Bash)

### Run All TCK Tests
```bash
./build/orion_tck_runner --log_level=error
```

### Run Specific Test Pattern
```bash
./build/orion_tck_runner --test 0050-feel-abs-function --log_level=all
```

## Common Options

- `--log_level=error` - Show only failures
- `--log_level=all` - Verbose output
- `--test <pattern>` - Run specific test group
- `--verbose` - Detailed test execution trace

## Expected Output

```
Running TCK compliance tests...
Level-2 Results: 126/126 passed (100.0%)
Level-3 Results: 377/1887 passed (20.0%)
```

With details:
```
TCK Test: 0032-conditionals
  Test 001: ✓ PASS
  Test 002: ✓ PASS
  Test 003: ✓ PASS
  Test 004: ✓ PASS
  Test 005: ✓ PASS
  Test 006: ✓ PASS
Result: 6/6 passed (100.0%)
```

## Troubleshooting

**TCK data not found:**
- Set environment variable: `$env:ORION_TCK_ROOT = "C:\workspace\orion\dat\dmn-tck"`
- Or ensure `dat/dmn-tck/TestCases/` exists

**Test timeouts:**
- TCK suite may take 5-10 minutes
- Use `--test <pattern>` for targeted testing

**Parsing failures:**
- Check XML test case files
- Verify DMN model validity
- Use `--verbose` for detailed trace
