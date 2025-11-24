# run-clang-tidy.ps1
# PowerShell script to run clang-tidy for code quality analysis
#
# Prerequisites:
#   - Run from x64 Native Tools Command Prompt for VS 2022, OR
#   - Run .\tools\scripts\setup-clang-tidy.ps1 first to generate compile_commands.json
#
# Usage:
#   .\tools\scripts\run-clang-tidy.ps1                    # Check all source files
#   .\tools\scripts\run-clang-tidy.ps1 -Fix               # Apply automatic fixes
#   .\tools\scripts\run-clang-tidy.ps1 -Path src\bre      # Check specific directory
#   .\tools\scripts\run-clang-tidy.ps1 -File src\common\log.cpp  # Check single file

Param(
    [string]$Path = "",
    [string]$File = "",
    [switch]$Fix,
    [string]$Checks = "",
    [switch]$ShowAll
)

$ErrorActionPreference = 'Stop'

# Paths
$RepoRoot = Resolve-Path (Join-Path $PSScriptRoot "..\..")
$ClangTidyExe = Join-Path $RepoRoot "tools\llvm-18.1.8\bin\clang-tidy.exe"
$CompileCommandsPath = Join-Path $RepoRoot "build-ninja\compile_commands.json"

Write-Host "═══════════════════════════════════════════════════════════════" -ForegroundColor Cyan
Write-Host "  Orion Clang-Tidy Code Quality Analysis" -ForegroundColor Cyan
Write-Host "═══════════════════════════════════════════════════════════════" -ForegroundColor Cyan
Write-Host ""

# Check prerequisites
if (-not (Test-Path $ClangTidyExe)) {
    Write-Host "[ERROR] clang-tidy.exe not found at: $ClangTidyExe" -ForegroundColor Red
    exit 1
}

if (-not (Test-Path $CompileCommandsPath)) {
    Write-Host "[ERROR] compile_commands.json not found at: $CompileCommandsPath" -ForegroundColor Red
    Write-Host ""
    Write-Host "Please run the setup script first:" -ForegroundColor Yellow
    Write-Host "  .\tools\scripts\setup-clang-tidy.ps1" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "Or manually generate it from x64 Native Tools Command Prompt:" -ForegroundColor Yellow
    Write-Host "  cmake -S . -B build-ninja -G Ninja -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows-static -DCMAKE_EXPORT_COMPILE_COMMANDS=ON" -ForegroundColor Yellow
    exit 1
}

Write-Host "[OK] clang-tidy: $ClangTidyExe" -ForegroundColor Green
Write-Host "[OK] compile_commands.json: $CompileCommandsPath" -ForegroundColor Green

# Get version
$version = & $ClangTidyExe --version 2>&1 | Select-Object -First 1
Write-Host "  Version: $version"
Write-Host ""

# Default checks
if (-not $Checks) {
    $Checks = "bugprone-*,clang-analyzer-*,cppcoreguidelines-*,modernize-*,performance-*,readability-*,portability-*,-modernize-use-trailing-return-type,-readability-magic-numbers,-cppcoreguidelines-avoid-magic-numbers"
}

# Collect files to check
$filesToCheck = @()
if ($File) {
    $filesToCheck = @(Join-Path $RepoRoot $File)
    Write-Host "Checking single file: $File"
}
elseif ($Path) {
    $searchPath = Join-Path $RepoRoot $Path
    $filesToCheck = Get-ChildItem -Path $searchPath -Recurse -Include "*.cpp","*.c" -File | Select-Object -ExpandProperty FullName
    Write-Host "Checking directory: $Path ($($filesToCheck.Count) files)"
}
else {
    # Check all source files
    $srcDirs = @("src", "tst")
    foreach ($dir in $srcDirs) {
        $dirPath = Join-Path $RepoRoot $dir
        if (Test-Path $dirPath) {
            $files = Get-ChildItem -Path $dirPath -Recurse -Include "*.cpp","*.c" -File | Select-Object -ExpandProperty FullName
            $filesToCheck += $files
        }
    }
    Write-Host "Checking all source files ($($filesToCheck.Count) files)"
}

if ($filesToCheck.Count -eq 0) {
    Write-Host "[ERROR] No files found to check" -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "Running clang-tidy..." -ForegroundColor Cyan
Write-Host "  Checks: $Checks" -ForegroundColor Gray
if ($Fix) {
    Write-Host "  [WARNING] Fix mode enabled - files will be modified!" -ForegroundColor Yellow
}
Write-Host ""

# Run clang-tidy on each file
$totalIssues = 0
$filesWithIssues = 0
$fileCount = 0

foreach ($fileToCheck in $filesToCheck) {
    $fileCount++
    $relativePath = $fileToCheck.Replace("$RepoRoot\", "")
    
    Write-Progress -Activity "Running clang-tidy" -Status "$fileCount of $($filesToCheck.Count): $relativePath" -PercentComplete ([int](($fileCount / $filesToCheck.Count) * 100))
    
    # Build clang-tidy arguments
    $args = @(
        "-p=$CompileCommandsPath",
        "--checks=$Checks",
        "--header-filter=(include/orion|src|tst)/.*",
        "--quiet",
        "--warnings-as-errors=''",
        "--extra-arg=-Wno-unknown-warning-option",
        "--extra-arg=-Wno-ignored-optimization-argument",
        "--extra-arg=-Wno-unused-command-line-argument",
        "--extra-arg=-fms-compatibility",
        "--extra-arg=-fms-extensions"
    )
    
    if ($Fix) {
        $args += "--fix"
    }
    
    $args += $fileToCheck
    
    # Run clang-tidy and filter output
    $output = & $ClangTidyExe @args 2>&1 | Where-Object {
        # Keep only warnings/errors from our own code (not external headers)
        if ($_ -match "warning:|error:") {
            $_ -match "(include/orion|src|tst)/"
        }
        elseif ($_ -notmatch "warnings? (and \d+ errors?)? generated" -and
                $_ -notmatch "Suppressed \d+ warnings" -and
                $_ -notmatch "Use -header-filter") {
            $true
        }
        else {
            $false
        }
    }
    
    # Count issues
    $fileIssues = ($output | Select-String -Pattern "warning:|error:" | Measure-Object).Count
    
    if ($fileIssues -gt 0) {
        $filesWithIssues++
        $totalIssues += $fileIssues
        
        Write-Host ""
        Write-Host "[$fileCount/$($filesToCheck.Count)] $relativePath - $fileIssues issues" -ForegroundColor Yellow
        
        if ($ShowAll -or $fileIssues -le 10) {
            $output | ForEach-Object {
                if ($_ -match "warning:") {
                    Write-Host "  $_" -ForegroundColor Yellow
                }
                elseif ($_ -match "error:") {
                    Write-Host "  $_" -ForegroundColor Red
                }
                else {
                    Write-Host "  $_"
                }
            }
        }
        else {
            Write-Host "  (Use -ShowAll to see all $fileIssues issues)" -ForegroundColor Gray
        }
    }
    elseif ($ShowAll) {
        Write-Host "[$fileCount/$($filesToCheck.Count)] $relativePath - OK" -ForegroundColor Green
    }
}

Write-Progress -Activity "Running clang-tidy" -Completed

# Summary
Write-Host ""
Write-Host "═══════════════════════════════════════════════════════════════" -ForegroundColor Cyan
Write-Host "  SUMMARY" -ForegroundColor Cyan
Write-Host "═══════════════════════════════════════════════════════════════" -ForegroundColor Cyan
Write-Host ""
Write-Host "Files checked:      $($filesToCheck.Count)"
Write-Host "Files with issues:  $filesWithIssues"
Write-Host "Total issues found: $totalIssues"
Write-Host ""

if ($totalIssues -eq 0) {
    Write-Host "[OK] No issues found! Code quality is excellent." -ForegroundColor Green
    exit 0
}
else {
    if ($Fix) {
        Write-Host "[INFO] Found $totalIssues issues. Automatic fixes have been applied where possible." -ForegroundColor Yellow
        Write-Host "       Please review the changes and run tests to ensure correctness." -ForegroundColor Yellow
    }
    else {
        Write-Host "[INFO] Found $totalIssues issues. Run with -Fix to apply automatic fixes." -ForegroundColor Yellow
    }
    exit 1
}
