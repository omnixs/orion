#!/usr/bin/env python3
"""
Simple benchmark comparison tool for Google Benchmark JSON output.
Compares two benchmark runs and performs statistical significance testing.

Usage:
    python compare_benchmarks.py baseline.json optimized.json
"""

import json
import sys
import math
from typing import Dict, List, Tuple

def load_benchmark(filename: str) -> Dict:
    """Load benchmark JSON file."""
    with open(filename, 'r') as f:
        return json.load(f)

def extract_results(data: Dict) -> Dict[str, Dict]:
    """Extract benchmark results grouped by name."""
    results = {}
    for bench in data['benchmarks']:
        name = bench['name']
        # Remove _mean, _median, _stddev, _cv suffixes
        base_name = name
        for suffix in ['_mean', '_median', '_stddev', '_cv']:
            if base_name.endswith(suffix):
                base_name = base_name[:-len(suffix)]
                break
        
        if base_name not in results:
            results[base_name] = {}
        
        # Store metrics
        if name.endswith('_mean'):
            results[base_name]['mean_time'] = bench.get('real_time', 0)
            results[base_name]['mean_cpu'] = bench.get('cpu_time', 0)
        elif name.endswith('_stddev'):
            results[base_name]['stddev_time'] = bench.get('real_time', 0)
            results[base_name]['stddev_cpu'] = bench.get('cpu_time', 0)
        elif name.endswith('_median'):
            results[base_name]['median_time'] = bench.get('real_time', 0)
            results[base_name]['median_cpu'] = bench.get('cpu_time', 0)
        elif name.endswith('_cv'):
            results[base_name]['cv_time'] = bench.get('real_time', 0)
            results[base_name]['cv_cpu'] = bench.get('cpu_time', 0)
    
    return results

def compute_significance(baseline_mean: float, baseline_std: float,
                        optimized_mean: float, optimized_std: float,
                        n: int = 20) -> Tuple[float, str]:
    """
    Compute statistical significance using Welch's t-test approximation.
    Returns (t_statistic, significance_marker)
    """
    if baseline_std == 0 and optimized_std == 0:
        return (0.0, '')
    
    # Standard error of the difference
    se = math.sqrt((baseline_std**2 / n) + (optimized_std**2 / n))
    
    if se == 0:
        return (0.0, '')
    
    # T-statistic
    t = abs(baseline_mean - optimized_mean) / se
    
    # Approximate significance levels (two-tailed test, df ~= 20-40)
    if t > 3.85:  # p < 0.001
        return (t, '***')
    elif t > 2.85:  # p < 0.01
        return (t, '**')
    elif t > 2.09:  # p < 0.05
        return (t, '*')
    elif t > 1.68:  # p < 0.10
        return (t, '.')
    else:
        return (t, '')

def compare_benchmarks(baseline_file: str, optimized_file: str):
    """Compare two benchmark runs and print results."""
    baseline_data = load_benchmark(baseline_file)
    optimized_data = load_benchmark(optimized_file)
    
    baseline_results = extract_results(baseline_data)
    optimized_results = extract_results(optimized_data)
    
    print("=" * 100)
    print(f"{'Benchmark':<50} {'Time':<12} {'CPU':<12} {'Significance':<12}")
    print("=" * 100)
    
    significant_improvements = []
    
    for name in sorted(baseline_results.keys()):
        if name not in optimized_results:
            continue
        
        baseline = baseline_results[name]
        optimized = optimized_results[name]
        
        # Skip if missing data
        if 'mean_cpu' not in baseline or 'mean_cpu' not in optimized:
            continue
        
        # Compute percentage changes
        time_change = ((optimized['mean_time'] - baseline['mean_time']) / baseline['mean_time']) * 100
        cpu_change = ((optimized['mean_cpu'] - baseline['mean_cpu']) / baseline['mean_cpu']) * 100
        
        # Compute statistical significance for CPU time
        t_stat, sig_marker = compute_significance(
            baseline['mean_cpu'],
            baseline.get('stddev_cpu', 0),
            optimized['mean_cpu'],
            optimized.get('stddev_cpu', 0)
        )
        
        time_str = f"{time_change:+6.2f}%"
        cpu_str = f"{cpu_change:+6.2f}%"
        sig_str = f"{sig_marker:>3} (t={t_stat:.2f})" if sig_marker else f"    (t={t_stat:.2f})"
        
        print(f"{name:<50} {time_str:<12} {cpu_str:<12} {sig_str:<12}")
        
        # Track significant improvements
        if cpu_change < -2.0 and sig_marker in ['*', '**', '***']:
            significant_improvements.append((name, cpu_change, sig_marker))
    
    print("=" * 100)
    print()
    print("Significance markers:")
    print("  *** = p < 0.001 (Very significant)")
    print("  **  = p < 0.01  (Significant)")
    print("  *   = p < 0.05  (Marginally significant)")
    print("  .   = p < 0.10  (Not significant)")
    print()
    
    if significant_improvements:
        print("=" * 100)
        print("SIGNIFICANT IMPROVEMENTS (CPU time):")
        print("=" * 100)
        for name, change, sig in significant_improvements:
            print(f"  {name}: {change:+.2f}% {sig}")
        print()
        
        avg_improvement = sum(change for _, change, _ in significant_improvements) / len(significant_improvements)
        print(f"Average significant improvement: {avg_improvement:+.2f}%")
    else:
        print("=" * 100)
        print("NO STATISTICALLY SIGNIFICANT IMPROVEMENTS DETECTED")
        print("=" * 100)
        print()
        print("Note: For gains < 3%, statistical significance (p < 0.05) is required.")
        print("      Without significance markers (* ** ***), gains may be noise.")

if __name__ == '__main__':
    if len(sys.argv) != 3:
        print("Usage: python compare_benchmarks.py baseline.json optimized.json")
        sys.exit(1)
    
    compare_benchmarks(sys.argv[1], sys.argv[2])
