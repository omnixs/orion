# compare_benchmarks_native.ps1
# Native PowerShell comparison of two Google Benchmark JSON result files.
#
# Mirrors the statistics of tools/scripts/compare_benchmarks.py (Welch's
# t-test approximation and the same significance thresholds) for environments
# where Python is not installed.
#
# Usage:
#   .\tools\scripts\compare_benchmarks_native.ps1 -Baseline base.json -Optimized opt.json
#   .\tools\scripts\compare_benchmarks_native.ps1 -Baseline base.json -Optimized opt.json -Filter '^Eval/'

param(
    [Parameter(Mandatory = $true)][string]$Baseline,
    [Parameter(Mandatory = $true)][string]$Optimized,
    [string]$Filter = '',
    [double]$RegressionThresholdPercent = 2.0
)

if (-not (Test-Path $Baseline))  { Write-Error "Baseline file not found: $Baseline";  exit 1 }
if (-not (Test-Path $Optimized)) { Write-Error "Optimized file not found: $Optimized"; exit 1 }

function Get-Aggregates {
    param([string]$Path)

    $data = Get-Content -Raw -Path $Path | ConvertFrom-Json
    $results = @{}

    foreach ($b in $data.benchmarks) {
        $name = $b.name
        $base = $name
        $kind = $null
        foreach ($suffix in @('_mean', '_median', '_stddev', '_cv')) {
            if ($base.EndsWith($suffix)) {
                $kind = $suffix.Substring(1)
                $base = $base.Substring(0, $base.Length - $suffix.Length)
                break
            }
        }
        if ($null -eq $kind) { continue }

        if (-not $results.ContainsKey($base)) {
            $results[$base] = [ordered]@{ repetitions = $b.repetitions }
        }
        $results[$base]["${kind}_time"] = [double]$b.real_time
        $results[$base]["${kind}_cpu"]  = [double]$b.cpu_time
    }

    return $results
}

function Get-Significance {
    param([double]$BaseMean, [double]$BaseStd, [double]$OptMean, [double]$OptStd, [int]$N)

    if ($BaseStd -eq 0 -and $OptStd -eq 0) { return @{ t = 0.0; marker = '' } }
    if ($N -le 0) { $N = 20 }

    $se = [math]::Sqrt((($BaseStd * $BaseStd) / $N) + (($OptStd * $OptStd) / $N))
    if ($se -eq 0) { return @{ t = 0.0; marker = '' } }

    $t = [math]::Abs($BaseMean - $OptMean) / $se

    $marker = ''
    if     ($t -gt 3.85) { $marker = '***' }
    elseif ($t -gt 2.85) { $marker = '**'  }
    elseif ($t -gt 2.09) { $marker = '*'   }
    elseif ($t -gt 1.68) { $marker = '.'   }

    return @{ t = $t; marker = $marker }
}

$baseResults = Get-Aggregates -Path $Baseline
$optResults  = Get-Aggregates -Path $Optimized

Write-Host ('=' * 104)
Write-Host ("{0,-40} {1,12} {2,12} {3,10} {4,18}" -f 'Benchmark', 'Time', 'CPU', 'CV%', 'Significance')
Write-Host ('=' * 104)

$improvements = @()
$regressions  = @()

foreach ($name in ($baseResults.Keys | Sort-Object)) {
    if ($Filter -and ($name -notmatch $Filter)) { continue }
    if (-not $optResults.ContainsKey($name)) { continue }

    $b = $baseResults[$name]
    $o = $optResults[$name]
    if (-not $b.Contains('mean_cpu') -or -not $o.Contains('mean_cpu')) { continue }

    $timeChange = (($o['mean_time'] - $b['mean_time']) / $b['mean_time']) * 100.0
    $cpuChange  = (($o['mean_cpu']  - $b['mean_cpu'])  / $b['mean_cpu'])  * 100.0

    $baseStd = if ($b.Contains('stddev_cpu')) { $b['stddev_cpu'] } else { 0.0 }
    $optStd  = if ($o.Contains('stddev_cpu')) { $o['stddev_cpu'] } else { 0.0 }
    $n       = if ($b.repetitions) { [int]$b.repetitions } else { 20 }

    $sig = Get-Significance -BaseMean $b['mean_cpu'] -BaseStd $baseStd `
                            -OptMean  $o['mean_cpu'] -OptStd  $optStd -N $n

    # Coefficient of variation of the optimized run, used to flag noisy cases.
    $cv = 0.0
    if ($o['mean_cpu'] -ne 0) { $cv = ($optStd / $o['mean_cpu']) * 100.0 }

    $sigText = if ($sig.marker) { "{0,3} (t={1:F2})" -f $sig.marker, $sig.t } else { "    (t={0:F2})" -f $sig.t }

    Write-Host ("{0,-40} {1,11:+0.00;-0.00}% {2,11:+0.00;-0.00}% {3,9:F1} {4,18}" -f `
        $name, $timeChange, $cpuChange, $cv, $sigText)

    $significant = $sig.marker -in @('*', '**', '***')
    if ($significant -and $cpuChange -lt (-1 * $RegressionThresholdPercent)) {
        $improvements += [pscustomobject]@{ Name = $name; Change = $cpuChange; Marker = $sig.marker }
    }
    if ($significant -and $cpuChange -gt $RegressionThresholdPercent) {
        $regressions += [pscustomobject]@{ Name = $name; Change = $cpuChange; Marker = $sig.marker }
    }
}

Write-Host ('=' * 104)
Write-Host ''
Write-Host 'Significance markers: *** p<0.001   ** p<0.01   * p<0.05   . p<0.10'
Write-Host ''

if ($improvements.Count -gt 0) {
    Write-Host "SIGNIFICANT IMPROVEMENTS (> $RegressionThresholdPercent% CPU, p < 0.05):" -ForegroundColor Green
    foreach ($i in ($improvements | Sort-Object Change)) {
        Write-Host ("  {0}: {1:+0.00;-0.00}% {2}" -f $i.Name, $i.Change, $i.Marker)
    }
    $avg = ($improvements | Measure-Object -Property Change -Average).Average
    Write-Host ("  Average: {0:+0.00;-0.00}%" -f $avg)
} else {
    Write-Host 'NO STATISTICALLY SIGNIFICANT IMPROVEMENTS DETECTED' -ForegroundColor Yellow
}

Write-Host ''

if ($regressions.Count -gt 0) {
    Write-Host "SIGNIFICANT REGRESSIONS (> $RegressionThresholdPercent% CPU, p < 0.05):" -ForegroundColor Red
    foreach ($r in ($regressions | Sort-Object Change -Descending)) {
        Write-Host ("  {0}: {1:+0.00;-0.00}% {2}" -f $r.Name, $r.Change, $r.Marker)
    }
    exit 2
}

Write-Host 'No statistically significant regressions detected.' -ForegroundColor Green
exit 0
