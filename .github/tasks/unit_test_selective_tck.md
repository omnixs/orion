---
template: improve_quality.md
agent: code-quality-refactor
status: not-started
category: code-quality
priority: medium
estimated-effort: "2-3 hours"
actual-effort: ""
---

# Task: Optimize Unit Tests to Skip Failing Level 3 TCK Tests

## Context

The unit test suite (`tst_orion`) executes all DMN TCK tests (Level 2 + Level 3) in Debug mode, which:
- Takes considerable time (~2-3 minutes for full suite)
- Wastes CI resources running tests that will fail
- Slows down local development iterations
- Level 3 has ~13.5% pass rate (476/3527 tests pass)

The current implementation runs **all** Level 3 tests regardless of whether they're known to fail. Since we already have TCK baseline results from `orion_tck_runner`, we can use this data to selectively skip tests that are known to fail.

## Requirements

### 1. Baseline-Aware Test Filtering

**Goal**: Skip Level 3 TCK tests that are known to fail according to the baseline.

**Approach**:
- Read the TCK baseline CSV (`dat/tck-baselines/{version}/tck_results.csv`)
- Build a set of test directories that have ALL tests passing
- Only run Level 3 test directories that are in the "passing" set
- Always run ALL Level 2 tests (non-negotiable - 100% compliance required)

**Benefits**:
- Faster unit test execution (skip ~86% of failing Level 3 tests)
- Reduced CI costs
- Faster local development iterations
- Still validates all passing functionality

### 2. Optional Filtering via Environment Variable

**Goal**: Make selective filtering optional for flexibility.

**Implementation**:
```cpp
// Default: Run only passing Level 3 tests (fast mode)
// Set ORION_TCK_RUN_ALL=1 to run all tests (comprehensive mode)
const char* run_all_env = std::getenv("ORION_TCK_RUN_ALL");
bool run_all_tests = (run_all_env != nullptr && std::string(run_all_env) == "1");
```

**Usage**:
- **Local development**: Default (selective) → fast iterations
- **CI validation**: Set `ORION_TCK_RUN_ALL=1` → comprehensive validation
- **Debugging**: Set `ORION_TCK_RUN_ALL=1` → investigate specific failures

### 3. Baseline Parsing

**File**: `dat/tck-baselines/{version}/tck_results.csv`

**Format**:
```csv
"test_dir","test_case_id","result_node_id","result","detail"
"compliance-level-3/0001-filter","0001-filter-test-01","001","ERROR","..."
"compliance-level-3/0032-conditionals","0032-conditionals-test-01","001","SUCCESS",""
```

**Logic**:
```cpp
// Build set of test directories where ALL tests pass
std::set<std::string> get_passing_test_dirs(const fs::path& baseline_csv) {
    std::set<std::string> passing_dirs;
    std::map<std::string, std::pair<int, int>> dir_stats; // dir -> {total, passed}
    
    // Parse baseline CSV
    // Count total and passed tests per directory
    
    // Only include directories where passed == total
    for (const auto& [dir, stats] : dir_stats) {
        if (stats.second == stats.first && stats.first > 0) {
            passing_dirs.insert(dir);
        }
    }
    
    return passing_dirs;
}
```

### 4. Test Execution Logic

**File**: `tst/bre/tck/test_tck_runner.cpp`

**Modified logic**:
```cpp
BOOST_AUTO_TEST_CASE(dmn_tck_comprehensive) {
    auto tck_base = find_tck_root();
    bool run_all = should_run_all_tests();
    
    std::set<std::string> passing_dirs;
    if (!run_all) {
        // Load baseline and determine passing test directories
        auto baseline_path = find_baseline_for_version();
        if (fs::exists(baseline_path)) {
            passing_dirs = get_passing_test_dirs(baseline_path);
            BOOST_TEST_MESSAGE("Selective mode: Running only " << passing_dirs.size() 
                              << " passing Level 3 test directories");
        } else {
            BOOST_TEST_MESSAGE("Baseline not found, running all tests");
            run_all = true;
        }
    }
    
    // Level 2: ALWAYS run all tests (non-negotiable)
    process_level2_tests();
    
    // Level 3: Selective execution
    for (auto& entry : fs::directory_iterator(level3_path)) {
        std::string test_name = entry.path().filename().string();
        
        if (!run_all && passing_dirs.count(test_name) == 0) {
            BOOST_TEST_MESSAGE("[SKIP] " << test_name << " (known to fail in baseline)");
            continue;
        }
        
        process_test_case_directory(entry.path());
    }
}
```

## Success Criteria

- [ ] Baseline CSV parsing function implemented
- [ ] Passing test directory detection logic implemented
- [ ] Environment variable `ORION_TCK_RUN_ALL` controls filtering
- [ ] Default behavior: Skip failing Level 3 tests
- [ ] ALL Level 2 tests always run (unchanged)
- [ ] Unit tests pass with selective mode (default)
- [ ] Unit tests pass with comprehensive mode (`ORION_TCK_RUN_ALL=1`)
- [ ] Execution time reduced by ~50-70% in selective mode
- [ ] CI workflow updated to use selective mode for speed
- [ ] Documentation updated in test file comments

## Expected Performance Impact

**Before** (all tests):
- Unit test execution: ~2-3 minutes
- Level 3 tests: 3527 tests, 476 pass, 3051 fail
- Debug mode overhead: Significant

**After** (selective mode):
- Unit test execution: ~45-90 seconds (estimated)
- Level 3 tests: ~476 tests (only known passing)
- Reduction: Skip 3051 failing tests → ~86% time savings

**Comprehensive mode** (`ORION_TCK_RUN_ALL=1`):
- Same as before (for validation/debugging)

## Implementation Plan

### Phase 1: Baseline Parsing (30 min)
1. Create `get_passing_test_dirs()` function
2. Parse CSV format with row-based unique IDs
3. Calculate per-directory pass/fail statistics
4. Return set of 100% passing directories

### Phase 2: Environment Variable Control (15 min)
1. Add `should_run_all_tests()` function
2. Check `ORION_TCK_RUN_ALL` environment variable
3. Default to selective mode (fast)

### Phase 3: Test Filtering Logic (30 min)
1. Modify `dmn_tck_comprehensive` test case
2. Load baseline if in selective mode
3. Skip Level 3 tests not in passing set
4. Always run Level 2 (unchanged)

### Phase 4: Testing & Validation (45 min)
1. Test selective mode (default)
2. Test comprehensive mode (`ORION_TCK_RUN_ALL=1`)
3. Verify Level 2 always runs
4. Measure execution time improvements
5. Update CI workflow

### Phase 5: Documentation (15 min)
1. Update test file comments
2. Update `.github/instructions/run_unit_tests.md`
3. Document environment variable usage

## Files to Modify

1. **`tst/bre/tck/test_tck_runner.cpp`**
   - Add baseline parsing functions
   - Add environment variable check
   - Modify `dmn_tck_comprehensive` test case
   - Add skip logic for Level 3 tests

2. **`.github/workflows/ci-full.yml`** (optional)
   - Set `ORION_TCK_RUN_ALL=0` (or leave default) for faster CI

3. **`.github/instructions/run_unit_tests.md`**
   - Document `ORION_TCK_RUN_ALL` environment variable
   - Explain selective vs comprehensive modes

## Testing Strategy

**Unit Test Validation**:
```powershell
# Selective mode (default)
.\build\Debug\tst_orion.exe --run_test=dmn_tck_levels --log_level=test_suite

# Comprehensive mode
$env:ORION_TCK_RUN_ALL="1"
.\build\Debug\tst_orion.exe --run_test=dmn_tck_levels --log_level=test_suite
```

**Timing Comparison**:
```powershell
# Measure selective mode
Measure-Command { .\build\Debug\tst_orion.exe --run_test=dmn_tck_levels --log_level=error }

# Measure comprehensive mode
$env:ORION_TCK_RUN_ALL="1"
Measure-Command { .\build\Debug\tst_orion.exe --run_test=dmn_tck_levels --log_level=error }
```

## Retrospective

(To be filled after task completion)

### What Went Well
- 

### What Was Challenging
- 

### Learnings
- 

### User Feedback
- 

### Recommendations for Future Tasks
- 
