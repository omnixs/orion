# TCK Failure Analysis — Snapshot at 3120/3527

## Quick Stats
- **Passed**: 3120 / 3527 (88.5%)
- **Failed**: 407
- **Baseline file**: `dat/tck-baselines/2.1.1/tck_results.csv`
- **Date**: 2026-08-03

## Top Failure Categories (estimated from prior analysis)

| Test Suite | Est. Failures | Category | Difficulty |
|-----------|--------------|----------|------------|
| `0070-feel-instance-of` | ~34 | Type system | Hard |
| `1156-range-function` | ~27 | Missing range() impl | Medium |
| `0082-feel-coercion` | ~21 | BKM/coercion | Hard |
| `0068-feel-equality` | ~19 | IANA timezone | Hard |
| `0036-dt-variable-input` | ~14 | Variable in unary test | Medium-Hard |
| `0074-feel-properties` | ~14 | Range properties | Medium (after range) |
| `1111-feel-matches` | ~12 | Regex XPath compat | Medium |
| `0034-drg-scopes` | 12 | DRG resolution | Hard |
| `0057-feel-context` | ~5 | Context evaluation | Medium |
| `0020-vacation-days` | 7 | BKM invocation | Hard |
| `1146-feel-context-put` | 6 | Inter-decision deps | Hard |
| Lambda-related (sort, etc.) | scattered | Function-as-value | Hard |

## Recommended Investigation Order

1. **`1156-range-function`** — Self-contained, new function implementation
2. **`1111-feel-matches`** — Regex flag adjustments  
3. **`0057-feel-context`** — May be quick fixes
4. **`0036-dt-variable-input`** — Attempted & reverted, needs fresh approach
5. **`0074-feel-properties`** — After range is done

## Command to Re-analyze Failures

```powershell
# Get failure breakdown by test suite
.\build\Debug\orion_tck_runner.exe --log_level=error 2>&1 > tck_output.txt
# Then analyze tck_output.txt for [FAIL] lines grouped by suite name
```

## Known Design Decisions

- **Duration struct**: `{total_months, total_seconds}` — integer only, no fractional seconds stored
- **Timezone**: Only offset-based (e.g., +05:30, Z). No IANA database.
- **Function registry**: Single signature per function name. Overloads handled specially.
- **Date/time functions**: Use fallback positional binding (not registry) due to multiple signatures.
- **Context put "keys" variant**: Manually bound in ast_node.cpp, not via registry.
