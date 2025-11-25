# ORION DMN TCK Test Baselines

This directory contains baseline TCK (Test Compatibility Kit) test results for each released version of ORION.

## Purpose

Baselines are used for **regression detection** in CI workflows:
- Ensures Level 2 DMN compliance remains at 100%
- Detects when previously passing Level 3 tests start failing
- Tracks DMN 1.5 compliance progress over time

## File Format

Each version directory contains:
- `tck_results.csv` - Detailed per-test results (test ID, level, pass/fail status, duration)
- `tck_results.properties` - Summary statistics (total tests, passed, failed, pass rate %)

### CSV Format

```csv
"test_dir","test_case_id","result_node_id","result","detail"
"compliance-level-2/0001-input-data-string","_001","decision001","SUCCESS",""
"compliance-level-2/0002-input-data-number","_001","decision001","SUCCESS",""
"compliance-level-3/0003-iteration","_001","decision001","FAILURE","Expected value mismatch"
```

### Properties Format

Example baseline (actual values vary by implementation progress):

```properties
total_tests=3535
passed_tests=484
failed_tests=3051
pass_rate=13.7
level2_total=3527
level2_passed=476
level2_pass_rate=13.5
```

Note: Level 2 statistics in current baseline reflect mixed Level 2/3 results.
See Issue: Properties file incorrectly assumes all tests are Level 2.

## How Baselines Are Generated

Baselines are created in two ways:

### Manual Generation (Development)
```bash
# Run TCK tests and generate baseline files
./build/orion_tck_runner --output-csv tck_results.csv --output-properties tck_results.properties

# Copy to baseline directory for current version
mkdir -p dat/tck-baselines/$(grep -oP 'VERSION \K[0-9.]+' CMakeLists.txt)
cp tck_results.csv dat/tck-baselines/$(grep -oP 'VERSION \K[0-9.]+' CMakeLists.txt)/
cp tck_results.properties dat/tck-baselines/$(grep -oP 'VERSION \K[0-9.]+' CMakeLists.txt)/
```

### Automated Generation (Release Workflow)
Baselines are automatically created during the release workflow:
1. Release workflow triggered via GitHub Actions (workflow_dispatch)
2. Full TCK test suite runs in Release build configuration
3. Results saved to `dat/tck-baselines/{VERSION}/`
4. Files committed and tagged as part of release

## Version Numbering

Version directories match the version in `CMakeLists.txt` using semantic versioning:
- **Major (X.0.0)**: Breaking API changes
- **Minor (X.Y.0)**: New features, backwards compatible
- **Patch (X.Y.Z)**: Bug fixes only

Example: CMakeLists.txt version `1.2.3` → baseline stored in `dat/tck-baselines/1.2.3/`

## Viewing Compliance Progress

Compare baselines across versions to track progress:

### Compare Pass Rates
```bash
# Windows PowerShell
diff (Get-Content dat\tck-baselines\1.0.0\tck_results.properties) `
     (Get-Content dat\tck-baselines\1.1.0\tck_results.properties)

# Linux/Bash
diff dat/tck-baselines/1.0.0/tck_results.properties \
     dat/tck-baselines/1.1.0/tck_results.properties
```

### Find Newly Passing Tests
```bash
# Windows PowerShell
Compare-Object (Select-String "FAIL" dat\tck-baselines\1.0.0\tck_results.csv) `
               (Select-String "FAIL" dat\tck-baselines\1.1.0\tck_results.csv)

# Linux/Bash
comm -13 <(grep FAIL dat/tck-baselines/1.0.0/tck_results.csv | sort) \
         <(grep FAIL dat/tck-baselines/1.1.0/tck_results.csv | sort)
```

### Find Regressions (Tests That Started Failing)
```bash
# Windows PowerShell
Compare-Object (Select-String "PASS" dat\tck-baselines\1.0.0\tck_results.csv) `
               (Select-String "PASS" dat\tck-baselines\1.1.0\tck_results.csv)

# Linux/Bash
comm -23 <(grep PASS dat/tck-baselines/1.0.0/tck_results.csv | sort) \
         <(grep PASS dat/tck-baselines/1.1.0/tck_results.csv | sort)
```

## CI Workflow Integration

The `ci-full.yml` workflow uses baselines to detect regressions:

### Regression Detection
- **Level 2 tests**: Must maintain 100% pass rate (workflow fails on any Level 2 failure)
- **Level 3 tests**: Workflow fails if overall pass rate decreases from baseline
- **New features**: Pass rate increases are expected and welcomed

### Exit Codes
The `orion_tck_runner` uses specific exit codes when running with baseline:
- **Exit 0**: All tests passed OR no regressions detected
- **Exit 1**: Expected failures (normal during development, workflow continues)
- **Exit 2**: **Regression detected** (previously passing test now fails, **workflow fails**)
- **Exit 3**: **Level 2 compliance failure** (any Level 2 test failed, **workflow fails**)

### Usage in CI
```yaml
# Extract version from CMakeLists.txt
VERSION=$(grep -oP 'project\(orion VERSION \K[0-9.]+' CMakeLists.txt)
BASELINE="dat/tck-baselines/$VERSION/tck_results.csv"

# Run with regression detection if baseline exists
if [ -f "$BASELINE" ]; then
  ./build/orion_tck_runner --baseline "$BASELINE" --regression-check --level2-strict
else
  ./build/orion_tck_runner  # No baseline, run normally
fi
```

See `.github/workflows/ci-full.yml` for implementation details.

## Directory Structure

```
dat/tck-baselines/
├── README.md                    # This file
├── 1.0.0/                      # First baseline
│   ├── tck_results.csv
│   └── tck_results.properties
├── 1.0.1/                      # Patch release baseline
│   ├── tck_results.csv
│   └── tck_results.properties
└── 1.1.0/                      # Minor release baseline
    ├── tck_results.csv
    └── tck_results.properties
```

## Notes

- Baselines are committed to the repository (not stored in the dmn-tck submodule)
- Submodule changes are git-ignored, hence the need for repository-local baselines
- Each release should have a corresponding baseline
- Baselines are immutable once created - never modify past baselines
- If you need to update a baseline, create a new version

## See Also

- [TCK Test Instructions](../../.github/instructions/run_tck_tests.md) - How to run TCK tests
- [CI Full Workflow](../../.github/workflows/ci-full.yml) - Regression detection in CI
- [Release Workflow](../../.github/workflows/release.yml) - Automated baseline generation
