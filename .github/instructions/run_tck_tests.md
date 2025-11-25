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

## Regression Detection (v1.0.0+)

### Overview

The TCK runner supports regression detection by comparing current test results against a baseline. This ensures that previously passing tests continue to pass as the codebase evolves.

### Exit Codes

- **0** - All tests passed OR no regressions detected
- **1** - Expected test failures (normal during development)
- **2** - Regression detected (previously passing test now fails)
- **3** - Level 2 compliance failure (when `--level2-strict` enabled)

### Generating a Baseline

Create a baseline for the current version:

**Windows:**
```powershell
# Extract version from CMakeLists.txt
$VERSION = (Select-String -Path CMakeLists.txt -Pattern "project\(orion VERSION ([0-9]+\.[0-9]+\.[0-9]+)" | ForEach-Object { $_.Matches.Groups[1].Value })

# Run tests and generate baseline
.\build\Release\orion_tck_runner.exe `
  --output-csv "dat\tck-baselines\$VERSION\tck_results.csv" `
  --output-properties "dat\tck-baselines\$VERSION\tck_results.properties" `
  --log_level=error

# Review results before committing
Write-Host "Generated baseline for version $VERSION"
```

**Linux:**
```bash
# Extract version from CMakeLists.txt
VERSION=$(grep "project(orion VERSION" CMakeLists.txt | sed -E 's/.*VERSION ([0-9]+\.[0-9]+\.[0-9]+).*/\1/')

# Create baseline directory
mkdir -p "dat/tck-baselines/$VERSION"

# Run tests and generate baseline
./build/orion_tck_runner \
  --output-csv "dat/tck-baselines/$VERSION/tck_results.csv" \
  --output-properties "dat/tck-baselines/$VERSION/tck_results.properties" \
  --log_level=error

# Review results before committing
echo "Generated baseline for version $VERSION"
```

### Running with Regression Detection

**Windows:**
```powershell
# Detect regressions against current version baseline
$VERSION = (Select-String -Path CMakeLists.txt -Pattern "project\(orion VERSION ([0-9]+\.[0-9]+\.[0-9]+)" | ForEach-Object { $_.Matches.Groups[1].Value })

.\build\Release\orion_tck_runner.exe `
  --baseline "dat\tck-baselines\$VERSION\tck_results.csv" `
  --regression-check `
  --level2-strict `
  --log_level=error

# Check exit code
if ($LASTEXITCODE -eq 0) {
    Write-Host "✓ No regressions detected"
} elseif ($LASTEXITCODE -eq 1) {
    Write-Host "⚠ Expected test failures (normal)"
} elseif ($LASTEXITCODE -eq 2) {
    Write-Host "✗ REGRESSION DETECTED - Previously passing tests now fail!"
    exit 1
} elseif ($LASTEXITCODE -eq 3) {
    Write-Host "✗ LEVEL 2 COMPLIANCE FAILURE - Critical DMN standard violation!"
    exit 1
}
```

**Linux:**
```bash
# Detect regressions against current version baseline
VERSION=$(grep "project(orion VERSION" CMakeLists.txt | sed -E 's/.*VERSION ([0-9]+\.[0-9]+\.[0-9]+).*/\1/')

./build/orion_tck_runner \
  --baseline "dat/tck-baselines/$VERSION/tck_results.csv" \
  --regression-check \
  --level2-strict \
  --log_level=error

# Check exit code
EXIT_CODE=$?
if [ $EXIT_CODE -eq 0 ]; then
    echo "✓ No regressions detected"
elif [ $EXIT_CODE -eq 1 ]; then
    echo "⚠ Expected test failures (normal)"
elif [ $EXIT_CODE -eq 2 ]; then
    echo "✗ REGRESSION DETECTED - Previously passing tests now fail!"
    exit 1
elif [ $EXIT_CODE -eq 3 ]; then
    echo "✗ LEVEL 2 COMPLIANCE FAILURE - Critical DMN standard violation!"
    exit 1
fi
```

### Regression Detection Options

**`--baseline <path>`**
- Path to baseline CSV file for comparison
- Example: `--baseline dat/tck-baselines/1.0.0/tck_results.csv`
- Without this, tests run normally without regression detection

**`--regression-check`**
- Enable regression detection mode
- Compares current results against baseline
- Exit code 2 if any previously passing tests now fail
- Must be used with `--baseline`

**`--level2-strict`**
- Fail the build on ANY Level 2 test failures
- Level 2 represents core DMN 1.5 compliance
- Exit code 3 if any Level 2 tests fail
- Use in CI/CD to enforce compliance standards

**`--output-csv <path>`**
- Generate CSV file with detailed test results
- Format: `"test_dir","test_case_id","result_node_id","result","detail"`
- Use for creating new baselines or analysis

**`--output-properties <path>`**
- Generate summary statistics file
- Contains: total_tests, passed_tests, failed_tests, pass_rate, level2_*
- Useful for CI/CD reporting and dashboards

### Workflow Examples

**Development Workflow:**
```powershell
# During development - allow expected failures
.\build\Release\orion_tck_runner.exe --log_level=error
# Exit code 1 is acceptable (normal failures during development)
```

**Pre-Commit Check:**
```powershell
# Before committing - check for regressions
$VERSION = "1.0.0"  # Current version
.\build\Release\orion_tck_runner.exe `
  --baseline "dat\tck-baselines\$VERSION\tck_results.csv" `
  --regression-check `
  --log_level=error

if ($LASTEXITCODE -eq 2) {
    Write-Error "Cannot commit: regression detected!"
    exit 1
}
```

**Release Validation:**
```powershell
# Before release - strict Level 2 compliance required
.\build\Release\orion_tck_runner.exe `
  --baseline "dat\tck-baselines\$VERSION\tck_results.csv" `
  --regression-check `
  --level2-strict `
  --output-csv "dat\release_results.csv" `
  --output-properties "dat\release_stats.properties" `
  --log_level=error

if ($LASTEXITCODE -ne 0) {
    Write-Error "Release validation failed!"
    exit 1
}
```

### CI/CD Integration

The full CI workflow (`.github/workflows/ci-full.yml`) automatically:
1. Detects the current version from `CMakeLists.txt`
2. Checks if a baseline exists for that version in `dat/tck-baselines/`
3. If baseline exists:
   - Runs with `--regression-check` and `--level2-strict`
   - **Fails the build** on exit codes 2 or 3
4. If no baseline:
   - Runs normally with `continue-on-error: true`
   - Allows failures during active development

### Baseline Management

**Baseline Directory Structure:**
```
dat/tck-baselines/
├── README.md              # Documentation
├── 1.0.0/                # Version 1.0.0 baseline
│   ├── tck_results.csv
│   └── tck_results.properties
└── 1.1.0/                # Version 1.1.0 baseline
    ├── tck_results.csv
    └── tck_results.properties
```

**When to Create New Baselines:**
- **Major/Minor version bump**: New baseline required
- **Patch version bump**: Update existing baseline if tests improve
- **Feature additions**: Update baseline if new tests pass
- **Bug fixes**: Update baseline if previously failing tests now pass

**Baseline Update Process:**
1. Run tests and generate new baseline
2. Review diff to ensure only intended improvements
3. Commit baseline with descriptive message
4. Reference issue/PR that caused the improvement

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

**Regression detection warnings:**

*"Regression check enabled but baseline is empty or could not be loaded"*
- Verify baseline file path is correct
- Check baseline CSV format matches expected structure
- Ensure baseline file exists and is readable

*"Exit code 2 - Regression detected"*
- Review which tests regressed: check CSV output for FAIL results with passing baseline
- Investigate code changes that may have broken previously working functionality
- Update baseline ONLY if regressions are intentional improvements elsewhere

*"Exit code 3 - Level 2 compliance failure"*
- Level 2 failures are **critical** - these are core DMN 1.5 features
- Must be fixed before release
- Review DMN specification compliance
- Do NOT update baseline to ignore Level 2 failures

**Baseline version mismatch:**
- Ensure baseline version matches current `CMakeLists.txt` version
- Create new baseline for new versions
- Old baselines are kept for historical reference only
