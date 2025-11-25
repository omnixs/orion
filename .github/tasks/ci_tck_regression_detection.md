---
template: custom
agent: none
status: completed
category: ci-cd
priority: high
estimated-effort: "10-14 hours"
actual-effort: "~6 hours"
---

# Task: Implement TCK Regression Detection

## Context

ORION's TCK (Test Compatibility Kit) tests currently run with `continue-on-error: true`, meaning they never fail the CI workflow even when regressions occur. With ~14% pass rate (484/3535 tests), we need to:
- Detect when previously passing tests start failing (regressions)
- Ensure Level 2 DMN compliance remains at 100%
- Track which specific tests pass/fail across versions

This task implements baseline storage and regression detection in the TCK runner and CI workflows.

## Requirements

### 1. TCK Baseline Storage and Tracking

**Current State:**
- TCK results stored in submodule: `dat/dmn-tck/TestResults/Orion/0.1.0/`
- Submodule changes are git-ignored, so baselines aren't tracked
- Version mismatch: CMakeLists.txt = `1.0.0`, test results = `0.1.0`

**Required Changes:**

#### 1.1 Create Baseline Directory Structure
```
dat/tck-baselines/
├── README.md                    # Explains baseline purpose and generation
└── 1.0.0/                      # Version from CMakeLists.txt
    ├── tck_results.csv         # Detailed test results
    └── tck_results.properties  # Summary (pass rate, counts)
```

#### 1.2 Baseline README.md Content
The README should document:
- Purpose of baselines (regression detection in CI)
- File format (CSV with test IDs and pass/fail, properties with summary stats)
- How baselines are generated (manually or via release workflow)
- Version numbering (matches CMakeLists.txt semantic versioning)
- How to compare baselines across versions
- CI workflow integration details

See the full content specification in the original combined task file.

#### 1.3 Initial Baseline Migration
- Copy current results from `dat/dmn-tck/TestResults/Orion/0.1.0/` to `dat/tck-baselines/1.0.0/`
- Verify CMakeLists.txt version = `1.0.0`
- Commit baseline files to repository

### 2. TCK Runner Regression Detection

**Current Behavior:**
- `orion_tck_runner` exits with code 1 if any tests fail (3051 failures expected)
- No baseline comparison capability
- No differentiation between expected failures and regressions

**Required Changes:**

#### 2.1 Add Command-Line Options

Extend `src/apps/orion_tck_runner.cpp` with new options:

```cpp
--baseline <path>              // Path to baseline tck_results.csv
--regression-check             // Enable regression detection mode
--level2-strict                // Fail if any Level 2 test fails
--output-csv <path>            // Generate results CSV (for baseline creation)
--output-properties <path>     // Generate summary properties file
```

#### 2.2 Implement Exit Code Strategy

**Exit Codes:**
- **0**: All tests passed OR (regression mode) no regressions detected
- **1**: TCK test failures (current behavior, normal during development)
- **2**: Regression detected (previously passing test now fails)
- **3**: Level 2 compliance failure (when `--level2-strict` enabled)

**Decision Logic:**
```cpp
if (level2_strict && level2_failures > 0) {
    return 3;  // Level 2 compliance failure
}
if (regression_check && regressions_detected > 0) {
    return 2;  // Regression detected
}
if (total_failures > 0) {
    return 1;  // Normal failures
}
return 0;  // All passed or no regressions
```

#### 2.3 Baseline Comparison Implementation

**Baseline CSV Format:**
```csv
TestID,Level,Result,Duration
0001-input-data-string,2,PASS,45
0002-input-data-number,2,PASS,32
0003-input-data-boolean,2,PASS,28
...
```

**Regression Detection Logic:**
1. Load baseline CSV into map: `test_id -> (level, result)`
2. Run current TCK tests, track results
3. For each test in baseline with `PASS` result:
   - If current result is `FAIL` → regression detected
   - Collect all regressions for reporting
4. Report regressions with test IDs and levels
5. Return appropriate exit code

**Level 2 Strict Mode:**
- Check if any Level 2 test failed in current run
- Exit with code 3 if any Level 2 failures (regardless of baseline)

### 3. CI Workflow Integration

#### 3.1 Update `ci-full.yml` - Windows

Replace current TCK step:
```yaml
- name: Run TCK Tests (Windows)
  if: runner.os == 'Windows'
  run: .\build\Release\orion_tck_runner.exe --log_level=error
  working-directory: ${{ github.workspace }}
  timeout-minutes: 15
  continue-on-error: true
```

With regression-aware step:
```yaml
- name: Run TCK Tests with Regression Detection (Windows)
  if: runner.os == 'Windows'
  run: |
    $VERSION = (Select-String 'project\(orion VERSION ([0-9.]+)' CMakeLists.txt).Matches.Groups[1].Value
    $BASELINE = "dat/tck-baselines/$VERSION/tck_results.csv"
    
    if (Test-Path $BASELINE) {
      Write-Host "Running TCK with regression detection (baseline: $VERSION)"
      .\build\Release\orion_tck_runner.exe --log_level=error --baseline $BASELINE --regression-check --level2-strict
      $exitCode = $LASTEXITCODE
      if ($exitCode -eq 2) {
        Write-Error "TCK Regression Detected - previously passing tests now fail"
        exit 2
      } elseif ($exitCode -eq 3) {
        Write-Error "Level 2 DMN Compliance Failure"
        exit 3
      } elseif ($exitCode -eq 1) {
        Write-Warning "TCK tests have expected failures (Level 3 incomplete)"
        exit 0  # Don't fail workflow for expected failures
      }
    } else {
      Write-Host "No baseline found for version $VERSION - running without regression check"
      .\build\Release\orion_tck_runner.exe --log_level=error
      exit 0  # Don't fail workflow when no baseline exists
    }
  working-directory: ${{ github.workspace }}
  timeout-minutes: 15
```

#### 3.2 Update `ci-full.yml` - Linux

Similar implementation with bash syntax:
```yaml
- name: Run TCK Tests with Regression Detection (Linux)
  if: runner.os == 'Linux'
  run: |
    VERSION=$(grep -oP 'project\(orion VERSION \K[0-9.]+' CMakeLists.txt)
    BASELINE="dat/tck-baselines/$VERSION/tck_results.csv"
    
    if [ -f "$BASELINE" ]; then
      echo "Running TCK with regression detection (baseline: $VERSION)"
      ./build/orion_tck_runner --log_level=error --baseline "$BASELINE" --regression-check --level2-strict
      exitCode=$?
      if [ $exitCode -eq 2 ]; then
        echo "ERROR: TCK Regression Detected - previously passing tests now fail"
        exit 2
      elif [ $exitCode -eq 3 ]; then
        echo "ERROR: Level 2 DMN Compliance Failure"
        exit 3
      elif [ $exitCode -eq 1 ]; then
        echo "WARNING: TCK tests have expected failures (Level 3 incomplete)"
        exit 0
      fi
    else
      echo "No baseline found for version $VERSION - running without regression check"
      ./build/orion_tck_runner --log_level=error
      exit 0
    fi
  working-directory: ${{ github.workspace }}
  timeout-minutes: 15
```

### 4. Documentation Updates

#### 4.1 Update `run_tck_tests.md`

Add new sections to `.github/instructions/run_tck_tests.md`:

**Regression Detection Options:**
```markdown
### Regression Detection Mode

Run TCK tests with baseline comparison:

```powershell
# Windows
.\build\orion_tck_runner.exe --baseline dat/tck-baselines/1.0.0/tck_results.csv --regression-check --level2-strict

# Linux
./build/orion_tck_runner --baseline dat/tck-baselines/1.0.0/tck_results.csv --regression-check --level2-strict
```

**Options:**
- `--baseline <path>` - Path to baseline CSV for comparison
- `--regression-check` - Enable regression detection (exit 2 if regressions found)
- `--level2-strict` - Fail on any Level 2 test failures (exit 3)
- `--output-csv <path>` - Generate results CSV (for creating new baselines)
- `--output-properties <path>` - Generate summary properties file

**Exit Codes:**
- `0` - All tests passed OR no regressions detected
- `1` - Expected test failures (normal during development)
- `2` - Regression detected (previously passing test now fails)
- `3` - Level 2 compliance failure (DMN Level 2 must stay at 100%)
```

Add examples and troubleshooting for regression detection scenarios.

**Note:** This file is referenced in:
- `copilot-instructions.md`
- `code-quality-refactor.agent.md`
- `add_dmn_feature.md`
- `fix_bug.md`
- `improve_quality.md`

## Implementation Plan

### Phase 1: Baseline Infrastructure (2-3 hours)
1. Create `dat/tck-baselines/` directory structure
2. Write `dat/tck-baselines/README.md`
3. Copy results from `dat/dmn-tck/TestResults/Orion/0.1.0/` to `dat/tck-baselines/1.0.0/`
4. Verify CMakeLists.txt version = `1.0.0`
5. Commit baseline files

### Phase 2: TCK Runner Implementation (5-7 hours)
1. Add command-line option parsing for new flags
2. Implement baseline CSV parser
3. Implement regression detection logic with test ID tracking
4. Implement Level 2 strict checking
5. Implement CSV and properties file output generation
6. Add exit code logic (0/1/2/3)
7. Add regression report output (list of regressed tests)
8. Test locally with baseline:
   - Test with matching baseline (exit 0)
   - Test with regression (modify code to fail passing test, verify exit 2)
   - Test Level 2 failure (verify exit 3)

### Phase 3: CI Workflow Integration (2-3 hours)
1. Update `.github/workflows/ci-full.yml` for Windows
2. Update `.github/workflows/ci-full.yml` for Linux
3. Test workflow on feature branch:
   - Create a regression (modify code to fail passing test)
   - Verify workflow fails with exit 2
   - Verify error message is clear

### Phase 4: Documentation (1-2 hours)
1. Update `.github/instructions/run_tck_tests.md` with:
   - New command-line options
   - Exit code meanings
   - Regression detection examples
   - Troubleshooting section
2. Update `dat/tck-baselines/README.md` if needed based on implementation

## Success Criteria

- [ ] Baseline directory exists with README and version 1.0.0 results
- [ ] `orion_tck_runner` accepts `--baseline`, `--regression-check`, `--level2-strict` flags
- [ ] `orion_tck_runner` accepts `--output-csv` and `--output-properties` flags
- [ ] Exit code 0: No regressions or all tests pass
- [ ] Exit code 1: Expected failures (current behavior)
- [ ] Exit code 2: Regression detected (with clear error message)
- [ ] Exit code 3: Level 2 compliance failure
- [ ] `ci-full.yml` workflow fails on exit code 2 or 3
- [ ] `ci-full.yml` workflow succeeds on exit code 0 or 1
- [ ] Documentation updated in `run_tck_tests.md`
- [ ] Tested locally with baseline comparison

## Testing Strategy

### Local Testing
```bash
# Generate baseline from current state
./build/orion_tck_runner --output-csv baseline.csv --output-properties baseline.properties

# Test regression detection (should pass)
./build/orion_tck_runner --baseline baseline.csv --regression-check --level2-strict
echo $?  # Should be 0 or 1

# Create artificial regression (modify code to fail a passing test)
# Then test again
./build/orion_tck_runner --baseline baseline.csv --regression-check --level2-strict
echo $?  # Should be 2
```

### CI Testing
1. Push to feature branch with baseline
2. Verify workflow runs successfully
3. Create commit that fails a passing test
4. Verify workflow fails with exit code 2
5. Verify error message identifies regression

## Dependencies

**Code Changes:**
- `src/apps/orion_tck_runner.cpp` - Add regression detection logic

**Workflow Changes:**
- `.github/workflows/ci-full.yml` - Add regression checking steps

**New Files:**
- `dat/tck-baselines/README.md`
- `dat/tck-baselines/1.0.0/tck_results.csv`
- `dat/tck-baselines/1.0.0/tck_results.properties`

**Documentation Updates:**
- `.github/instructions/run_tck_tests.md`

**Libraries Needed:**
- CSV parsing (can use simple C++ string parsing or lightweight library)
- Command-line argument parsing (already exists in orion_tck_runner)

## Notes

- This task focuses solely on regression detection, not release automation
- Baseline files should be committed to the repository (not in submodule)
- Level 2 tests must always pass (100% compliance required)
- Level 3 tests are allowed to fail, but regressions are not acceptable
- Exit codes allow CI to differentiate between expected failures and problems

## Future Enhancements (Out of Scope)

- Automated baseline updates via release workflow
- Performance regression detection
- Regression reporting dashboard
- Baseline comparison UI tool

## Retrospective

### What Went Well
1. **Phased Approach**: Breaking the task into 4 phases (Infrastructure → Implementation → CI Integration → Documentation) made progress trackable and ensured nothing was missed
2. **CSV Format Insight**: Discovered actual CSV format ("test_dir","test_case_id","result_node_id","result","detail") early by examining existing output code, avoiding potential rework
3. **Robust Parsing**: Implemented proper CSV parser with quoted-field handling instead of naive string splitting, making the system resilient to edge cases
4. **Exit Code Strategy**: Clear exit code hierarchy (3 > 2 > 1 > 0) makes CI workflow logic straightforward
5. **Comprehensive Testing**: Local testing with baseline generation/comparison caught the CSV format mismatch before CI integration

### Challenges Encountered
1. **CSV Format Mismatch**: Initial baseline loading assumed 4-column format (TestID,Level,Result,Duration) but actual output was 5 columns with quotes. Fixed by reading existing write_csv_result() code first
2. **Test ID Construction**: Needed to create unique IDs from test_case_id + result_node_id since multiple outputs exist per test case
3. **Level Detection**: DMN level not explicitly in CSV - had to parse from test_dir path (compliance-level-2/ vs compliance-level-3/)
4. **PowerShell vs Bash**: Dual-platform CI required careful translation of version extraction and conditional logic between Windows/Linux

### Key Learnings
1. **Read Before Writing**: Examining existing CSV output code (write_csv_result) before implementing baseline parsing saved significant rework
2. **Test Data Structures Matter**: The 5-column CSV with composite test IDs required more complex parsing than initially planned
3. **Exit Codes as API**: Well-designed exit codes (0/1/2/3) make CI integration clean and self-documenting
4. **Baseline Path Strategy**: Using version from CMakeLists.txt as single source of truth simplifies version management

### Process Improvements for Future Tasks
1. **Code Archaeology First**: Always search for existing related code (e.g., CSV writing) before designing new code that interacts with it
2. **Format Documentation**: Document data formats (CSV schema, test ID format) in comments near the parsing/writing code
3. **Local Testing Script**: Create a test script that validates all exit codes locally before pushing to CI
4. **Incremental Commits**: Each phase was committed separately, making it easy to review and roll back if needed

### Time Breakdown
- **Phase 1** (Baseline Infrastructure): 1 hour - Faster than estimated, directory structure and README straightforward
- **Phase 2** (TCK Runner Implementation): 3.5 hours - Close to estimate, CSV format discovery added time but comprehensive testing paid off
- **Phase 3** (CI Workflow Integration): 1 hour - Faster than estimated due to clear exit code design
- **Phase 4** (Documentation): 0.5 hours - Comprehensive examples written quickly by adapting local test commands
- **Total**: ~6 hours vs 10-14 hour estimate (40-60% time saving due to phased approach and early format discovery)

### Impact
- **Regression Protection**: CI now catches when previously passing tests start failing
- **Level 2 Compliance**: Strict enforcement ensures DMN 1.5 core features always work
- **Development Confidence**: Developers can refactor knowing regressions will be caught
- **Baseline Tracking**: Git-tracked baselines provide version history of test improvements
- **Exit Code Clarity**: CI logs clearly differentiate between expected failures (1), regressions (2), and compliance issues (3)
