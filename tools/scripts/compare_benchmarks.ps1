# compare_benchmarks.ps1
# Compare two Google Benchmark JSON results with statistical analysis
#
# Usage:
#   .\compare_benchmarks.ps1 -Baseline baseline.json -Optimized optimized.json

param(
    [Parameter(Mandatory=$true)]
    [string]$Baseline,
    
    [Parameter(Mandatory=$true)]
    [string]$Optimized,
    
    [string]$PythonScript = "tools\compare.py"
)

# Check if files exist
if (-not (Test-Path $Baseline)) {
    Write-Error "Baseline file not found: $Baseline"
    exit 1
}

if (-not (Test-Path $Optimized)) {
    Write-Error "Optimized file not found: $Optimized"
    exit 1
}

# Check if compare script exists
if (-not (Test-Path $PythonScript)) {
    Write-Error "Compare script not found: $PythonScript"
    Write-Host "Note: Google Benchmark compare.py script should be in tools/"
    exit 1
}

# Run comparison
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Benchmark Comparison" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Baseline:  $Baseline"
Write-Host "Optimized: $Optimized"
Write-Host ""

python $PythonScript benchmarks $Baseline $Optimized

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Interpretation Guide:" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "*** = p < 0.001 (Very significant)"
Write-Host "**  = p < 0.01  (Significant)"
Write-Host "*   = p < 0.05  (Marginally significant)"
Write-Host ".   = p < 0.10  (Not significant)"
Write-Host ""
Write-Host "Gain < 3%  + p >= 0.05: Do NOT claim improvement"
Write-Host "Gain 3-5%  + p < 0.05:  Valid improvement"
Write-Host "Gain > 5%:              Strong improvement"
