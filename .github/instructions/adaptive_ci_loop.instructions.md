# Adaptive CI Loop for Code Quality Refactoring

## Overview

Intelligent test execution strategy that adapts checkpoint frequency based on:
- **Change risk level** (low/medium/high)
- **Failure history** (recent failures trigger more frequent checks)
- **Success streak** (long streaks allow longer intervals)

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│ Change Applied                                               │
│ ↓                                                            │
│ Debug Build (incremental, -j$(nproc))                       │
│ ↓                                                            │
│ Debug Smoke Test (fast, <1s, runtime checks)                │
│ ↓                                                            │
│ Risk Classification: Low/Medium/High                         │
│ ↓                                                            │
│ Update Checkpoint Tracker                                    │
│   - issues_since_last_check++                                │
│   - consecutive_successes++ (if pass)                        │
│   - Calculate adaptive interval                              │
│ ↓                                                            │
│ IF issues_since_last_check >= checkpoint_interval:           │
│   ├─ Release Build (incremental, -j$(nproc))                 │
│   ├─ Full Unit Tests (Release, all 279 tests)                │
│   └─ Conditional TCK (only if high-risk or failure history)  │
└─────────────────────────────────────────────────────────────┘
```

## Adaptive Interval Calculation

### Base Intervals by Risk Level

| Risk Level | Base Interval | Example Changes |
|------------|---------------|-----------------|
| Low | 20-30 issues | string_view, const, [[nodiscard]], naming |
| Medium | 10-15 issues | modernization, error handling refactor |
| High | 5-10 issues | FEEL evaluator, DMN parser, BKM manager |

### Dynamic Adjustments

```python
# After failure
checkpoint_interval = max(5, current_interval / 2)
consecutive_successes = 0

# After 20+ consecutive successes
checkpoint_interval = min(30, current_interval * 1.5)

# After high-risk change
checkpoint_interval = min(checkpoint_interval, 10)
```

## Risk Classification Heuristics

### Automatic Detection

```python
def classify_risk(changed_files: list[str]) -> str:
    high_risk_patterns = [
        "src/bre/feel/evaluator.cpp",
        "src/bre/feel/expr.cpp",
        "src/bre/feel/unary.cpp",
        "src/bre/feel/parser.cpp",
        "src/bre/dmn_parser.cpp",
        "src/bre/bkm_manager.cpp"
    ]
    
    medium_risk_patterns = [
        "src/bre/feel/",  # Other FEEL files
        "src/bre/ast_node.cpp",
        "src/api/engine.cpp"
    ]
    
    low_risk_patterns = [
        "*.hpp",  # Header-only changes
        "test_*.cpp",  # Test code
        "*.md"  # Documentation
    ]
    
    if any(file in changed_files for file in high_risk_patterns):
        return "high"
    elif any(pattern in file for pattern in medium_risk_patterns for file in changed_files):
        return "medium"
    else:
        return "low"
```

## TCK Execution Strategy

### Conditional TCK (Avoid Unnecessary 15-minute Runs)

**Execute TCK if:**
- High-risk files changed (FEEL evaluator, DMN parser)
- Recent failure in last 10 issues
- Checkpoint interval >= 30 (periodic long-horizon check)
- Final validation (always)

**Skip TCK if:**
- All changes are low-risk (string_view, const, naming)
- Consecutive successes > 15
- No FEEL/DMN/BKM changes
- Time saving: ~15 minutes per skip

### Example Session

```
Issue 1-10: string_view changes (low-risk)
  → Checkpoint at 10: Unit tests (pass), TCK skipped (low-risk)
  
Issue 11-25: string_view + const (low-risk, streak)
  → Checkpoint at 25: Unit tests (pass), TCK skipped (15+ streak)
  
Issue 26: FEEL evaluator change (high-risk)
  → Risk bump: interval → 10
  → Checkpoint at 30: Unit tests (pass), TCK executed (high-risk)
  
Issue 31-40: string_view (low-risk, post-TCK)
  → Checkpoint at 42: Unit tests (pass), TCK skipped (low-risk)
  
Final: Full validation
  → Unit tests + TCK (always executed)
```

**Result**: 2 TCK runs instead of 5 → **Saved ~45 minutes**

## Checkpoint State Tracking

### File: `dat/log/checkpoint_state.json`

```json
{
  "issues_since_last_check": 12,
  "consecutive_successes": 18,
  "checkpoint_interval": 15,
  "last_failure_issue": 7,
  "last_checkpoint_issue": 10,
  "risk_level": "medium",
  "tck_executed_count": 2,
  "tck_skipped_count": 2,
  "session_start": "2025-11-22T09:00:00Z",
  "changed_files": [
    "src/bre/feel/evaluator.cpp",
    "src/bre/feel/lexer.cpp",
    "include/orion/bre/feel/evaluator.hpp"
  ]
}
```

### Failure Log: `dat/log/ci_failures.log`

```
2025-11-22 09:15:32 | Issue 7 | build | src/bre/evaluator.cpp:234 | Missing #include <string>
2025-11-22 09:45:18 | Issue 23 | test | test_unary_matches | Lifetime issue with string_view
2025-11-22 10:30:00 | Session Complete | 47 issues | 42 fixed | 5 skipped | 0 regressions
```

## Build Strategy: Debug vs Release

### Debug Build (Smoke Tests)
- **Purpose**: Fast runtime validation (<1s)
- **Checks**: Assertions, UB detection, dangling references
- **Frequency**: Every change
- **Configuration**: `-g -O0 -fsanitize=address,undefined` (optional)

### Release Build (Full Validation)
- **Purpose**: Performance-accurate, full regression detection
- **Checks**: All 279 unit tests, TCK (conditional)
- **Frequency**: Adaptive checkpoints (5-30 issues)
- **Configuration**: `-O3 -DNDEBUG`

## Performance Gains

| Optimization | Time Saved | Cumulative |
|--------------|------------|------------|
| Ninja vs Make | ~60s/build | ~10 min/session |
| ccache (90% hit) | ~40s/build | ~15 min/session |
| Parallel builds (-j8) | ~30s/build | ~8 min/session |
| Adaptive TCK skipping | ~15 min/skip | ~30-45 min/session |
| Debug smoke vs full | ~5s/issue | ~8 min/session |
| **Total** | - | **~75-90 min saved** |

**For 100-issue session**: Traditional fixed-10 = ~4 hours, Adaptive = ~2.5 hours

## Integration with Agent

The `code-quality-refactor` agent automatically:
1. Initializes checkpoint tracker in Phase 1
2. Classifies risk after each change
3. Executes Debug smoke tests every change
4. Runs adaptive checkpoints (Release unit + conditional TCK)
5. Updates intervals based on failures/successes
6. Archives artifacts and logs at session end

No manual intervention required - fully autonomous.
