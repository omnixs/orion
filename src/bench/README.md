# ORION BRE Benchmark Suite

## Overview

This directory contains performance benchmarks for the ORION Business Rules Engine using real DMN TestExample models from the official DMN TCK (Technology Compatibility Kit).

## Benchmark Models

### 1. **calc-discount A.1** - UNIQUE Hit Policy (6 test cases)
- **Model**: `dat/dmn-tck/TestExamples/calc-discount/A.1.dmn`
- **Feature**: Decision table with UNIQUE hit policy
- **Rules**: 6 rules covering age bands and priority service combinations
- **Inputs**: 
  - `age` (number): Customer age
  - `priority` (boolean): Whether priority service is requested
- **Output**: `price` (number): Calculated ticket price
- **Use Case**: Ticket pricing system with age-based discounts and priority surcharge

**Test Scenarios**:
- `BM_CalcDiscount_A1_Infant_NoPriority` - Age 1, no priority → $0
- `BM_CalcDiscount_A1_Infant_Priority` - Age 1, priority → $10
- `BM_CalcDiscount_A1_Child_NoPriority` - Age 10, no priority → $20
- `BM_CalcDiscount_A1_Child_Priority` - Age 10, priority → $30
- `BM_CalcDiscount_A1_Adult_NoPriority` - Age 25, no priority → $40
- `BM_CalcDiscount_A1_Adult_Priority` - Age 25, priority → $50

### 2. **calc-discount A.2** - COLLECT+SUM Hit Policy (1 test case)
- **Model**: `dat/dmn-tck/TestExamples/calc-discount/A.2.dmn`
- **Feature**: Decision table with COLLECT hit policy and SUM aggregation
- **Rules**: 4 rules (3 age-based, 1 priority-based)
- **Inputs**: Same as A.1
- **Output**: Aggregated price (base price + priority surcharge)
- **Use Case**: Additive pricing model where multiple rules can match

**Test Scenario**:
- `BM_CalcDiscount_A2_CollectSum` - Age 19, priority → $40 + $10 = $50

### 3. **order-discount** - Volume-based Discount (5 test cases)
- **Model**: `dat/dmn-tck/TestExamples/order-discount/order-discount.dmn`
- **Feature**: Simple decision table with range expressions
- **Rules**: 5 rules covering different order amount ranges
- **Input**: `amount` (number): Order amount
- **Output**: `discount` (number): Discount percentage
- **Use Case**: E-commerce volume discount calculation

**Test Scenarios**:
- `BM_OrderDiscount_Small` - Amount $250 → 0% discount
- `BM_OrderDiscount_Medium` - Amount $750 → 2% discount
- `BM_OrderDiscount_Large` - Amount $1,500 → 3% discount
- `BM_OrderDiscount_Larger` - Amount $3,000 → 5% discount
- `BM_OrderDiscount_Largest` - Amount $6,000 → 8% discount

## Running Benchmarks

### Build Requirements

The benchmark suite requires the Google Benchmark library, which is installed via vcpkg:

```bash
# Benchmark library is listed in vcpkg.json
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 \
  -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake \
  -DVCPKG_TARGET_TRIPLET=x64-windows-static
cmake --build build --config Release
```

### Run All Benchmarks

```bash
.\build\Release\orion-bench.exe
```

### Run Specific Benchmarks

```bash
# Run only calc-discount A.1 benchmarks
.\build\Release\orion-bench.exe --benchmark_filter=".*CalcDiscount_A1.*"

# Run only calc-discount A.2 benchmarks
.\build\Release\orion-bench.exe --benchmark_filter=".*CalcDiscount_A2.*"

# Run only order-discount benchmarks
.\build\Release\orion-bench.exe --benchmark_filter=".*OrderDiscount.*"
```

### Benchmark Options

```bash
# Run for minimum 1 second per benchmark
.\build\Release\orion-bench.exe --benchmark_min_time=1.0s

# Output results in JSON format
.\build\Release\orion-bench.exe --benchmark_format=json

# Run benchmarks with specific number of iterations
.\build\Release\orion-bench.exe --benchmark_repetitions=10
```

## Interpreting Results

### Sample Output

```
Benchmark                                     Time             CPU   Iterations
-------------------------------------------------------------------------------
BM_CalcDiscount_A1_Infant_NoPriority      85123 ns        82500 ns         8000
BM_CalcDiscount_A1_Adult_Priority        122456 ns       120000 ns         5600
BM_OrderDiscount_Small                    42789 ns        41250 ns        16000
```

### Metrics

- **Time**: Wall clock time per iteration (includes system overhead)
- **CPU**: CPU time per iteration (actual computation time)
- **Iterations**: Number of times the benchmark was executed

### Performance Characteristics

**Expected Performance (Release build)**:
- **Simple decision tables** (order-discount): 40-100 μs per evaluation
- **Medium complexity** (calc-discount A.1): 80-150 μs per evaluation
- **Complex/aggregation** (calc-discount A.2): 100-200 μs per evaluation

**Debug vs Release**:
- Debug builds are 5-10x slower due to lack of optimizations
- Always use Release builds for performance measurements
- Google Benchmark will warn if DEBUG build is detected

## Performance Analysis

### Factors Affecting Performance

1. **Number of Rules**: More rules = longer evaluation time (linear scan)
2. **Rule Complexity**: Range expressions `[500..999]` vs simple `< 500`
3. **Hit Policy**: COLLECT+SUM requires evaluating all rules vs UNIQUE/FIRST stops at first match
4. **Input Matching**: Early matches (first rules) are faster than late matches
5. **String Parsing**: FEEL expression parsing overhead (no pre-compilation yet)

### Optimization Opportunities

Current implementation uses **direct string evaluation** without AST pre-parsing:
- Each evaluation re-parses FEEL expressions from strings
- No caching of parsed decision tables
- No compilation phase

**Potential improvements** (see performance analysis in conversation):
- Pre-parse expressions into AST during model load → **10-100x speedup**
- Cache parsed decision tables → Eliminate redundant XML parsing
- Compile decision tables to native code → Maximum performance

## Benchmarking Best Practices

### 1. Stable Environment
- Close background applications
- Disable CPU frequency scaling if possible
- Run multiple iterations to reduce noise

### 2. Release Builds Only
```bash
cmake --build build --config Release
.\build\Release\orion-bench.exe
```

### 3. Statistical Significance
```bash
# Run with repetitions to get mean and standard deviation
.\build\Release\orion-bench.exe --benchmark_repetitions=10 --benchmark_report_aggregates_only=true
```

### 4. Baseline Comparison
```bash
# Save baseline results
.\build\Release\orion-bench.exe --benchmark_format=json > baseline.json

# Compare after changes
.\build\Release\orion-bench.exe --benchmark_format=json > new.json
python compare.py baseline.json new.json
```

## Adding New Benchmarks

To add benchmarks for additional DMN models:

1. **Embed the DMN XML** as a raw string literal
2. **Create input data** strings for test cases
3. **Write benchmark function**:
   ```cpp
   static void BM_MyModel_Scenario(benchmark::State& state) {
       for (auto _ : state) {
           std::string result = evaluate(kMyModel_DMN, kMyModel_Input, {});
           benchmark::DoNotOptimize(result);
       }
   }
   ```
4. **Register the benchmark**: `BENCHMARK(BM_MyModel_Scenario);`
5. **Rebuild**: `cmake --build build --config Release`

## References

- **DMN TCK TestExamples**: `dat/dmn-tck/TestExamples/`
- **Google Benchmark Documentation**: https://github.com/google/benchmark
- **DMN 1.5 Specification**: `docs/formal-24-01-01.pdf`
- **ORION Performance Analysis**: See conversation for detailed performance discussion

## Notes

### Why These Models?

These three TestExamples were chosen because:
1. ✅ **Fully supported** by current ORION engine (Level-2 compliance)
2. ✅ **Representative use cases** (pricing, discounts, aggregation)
3. ✅ **Different complexity levels** (simple to moderate)
4. ✅ **Official OMG test cases** (standards compliance)
5. ✅ **Complete test data** (input/output examples provided)

### Models NOT Included

- **calc-discount/A.3.dmn**: Requires context expressions (not yet implemented)
- **lending1.dmn**: Requires context expressions and advanced FEEL features (Level-3)

These will be added to benchmarks once context expression support is implemented.
