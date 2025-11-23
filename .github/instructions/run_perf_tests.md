# Performance Benchmark Instructions

## Prerequisites

- Build completed in RelWithDebInfo or Release mode
- google-benchmark library linked

## Windows (PowerShell)

### Run All Benchmarks
```powershell
.\build\orion-bench.exe --benchmark_repetitions=3
```

### Run With Statistical Analysis
```powershell
.\build\orion-bench.exe `
  --benchmark_repetitions=20 `
  --benchmark_out=results.json `
  --benchmark_out_format=json
```

### Run Specific Benchmark
```powershell
.\build\orion-bench.exe --benchmark_filter=CalcDiscount
```

## Linux (Bash)

### Run All Benchmarks
```bash
./build/orion-bench --benchmark_repetitions=3
```

### Run With Statistical Analysis
```bash
./build/orion-bench \
  --benchmark_repetitions=20 \
  --benchmark_out=results.json \
  --benchmark_out_format=json
```

## Common Options

- `--benchmark_repetitions=N` - Number of repetitions (default: 3, recommend: 20+)
- `--benchmark_min_time=Xs` - Minimum time per benchmark (e.g., 5s)
- `--benchmark_filter=<pattern>` - Run specific benchmarks
- `--benchmark_out=<file>` - Save results to file
- `--benchmark_out_format=json` - JSON output format
- `--benchmark_report_aggregates_only=true` - Show only mean/median/stddev

## Expected Output

```
Run on (8 X 3600 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x4)
  L1 Instruction 32 KiB (x4)
  L2 Unified 256 KiB (x4)
  L3 Unified 8192 KiB (x1)

BM_CalcDiscount_A1_Infant_Priority        12500 ns    12450 ns    56000
BM_ParseSimpleExpression                    180 ns      175 ns   3890000
```

With statistics:
```
BM_CalcDiscount_mean                      12450 ns
BM_CalcDiscount_median                    12380 ns
BM_CalcDiscount_stddev                      850 ns
BM_CalcDiscount_cv                         6.83 %
```

## Comparing Results

```powershell
# Generate baseline
.\build\orion-bench.exe --benchmark_out=baseline.json --benchmark_repetitions=20

# After changes
.\build\orion-bench.exe --benchmark_out=current.json --benchmark_repetitions=20

# Compare
python tools\compare.py baseline.json current.json
```

## Troubleshooting

**High variance (CV > 20%):**
- Close background applications
- Set high-performance power plan
- Increase repetitions (--benchmark_repetitions=50)
- Use longer min_time (--benchmark_min_time=10s)

**Benchmark not found:**
- Use `--benchmark_list_tests` to see all benchmarks
- Check filter pattern

**Invalid JSON output:**
- Ensure file path is writable
- Check for benchmark crashes
