# PowerShell script to list all files in the ORION project
# Usage: .\list-project-files.ps1

Write-Host "=== ORION Project File Structure ===" -ForegroundColor Green
Write-Host "Scanning from: $(Get-Location)" -ForegroundColor Yellow
Write-Host ""

# Function to display directory tree with file counts
function Show-DirectoryTree {
    param(
        [string]$Path = ".",
        [int]$MaxDepth = 3,
        [int]$CurrentDepth = 0,
        [string]$Prefix = ""
    )
    
    if ($CurrentDepth -gt $MaxDepth) { return }
    
    # Directories to exclude from scanning
    $excludeDirs = @('.github', '.vscode', 'build', '.git', 'cmake', 'CMakeFiles', '.vs')
    
    try {
        $items = Get-ChildItem -Path $Path -Force | Sort-Object Name
        # Filter out excluded directories
        $dirs = $items | Where-Object { $_.PSIsContainer -and $_.Name -notin $excludeDirs }
        $files = $items | Where-Object { -not $_.PSIsContainer }
        
        # Show directory info
        if ($CurrentDepth -eq 0) {
            Write-Host "$Path/" -ForegroundColor Cyan
        }
        
        # Show subdirectories
        foreach ($dir in $dirs) {
            $indent = "  " * ($CurrentDepth + 1)
            Write-Host "$indent$($dir.Name)/" -ForegroundColor Cyan
            
            if ($CurrentDepth -lt $MaxDepth) {
                Show-DirectoryTree -Path $dir.FullName -MaxDepth $MaxDepth -CurrentDepth ($CurrentDepth + 1)
            }
        }
        
        # Show files in current directory
        if ($files.Count -gt 0 -and $CurrentDepth -le 2) {
            $indent = "  " * ($CurrentDepth + 1)
            foreach ($file in $files | Select-Object -First 10) {
                Write-Host "$indent$($file.Name)" -ForegroundColor White
            }
            if ($files.Count -gt 10) {
                Write-Host "$indent... and $($files.Count - 10) more files" -ForegroundColor Gray
            }
        }
        
        # Summary for root level
        if ($CurrentDepth -eq 0) {
            Write-Host ""
            Write-Host "Summary:" -ForegroundColor Green
            Write-Host "  Directories: $($dirs.Count)" -ForegroundColor Yellow
            Write-Host "  Files: $($files.Count)" -ForegroundColor Yellow
        }
        
    } catch {
        Write-Host "Error accessing $Path : $($_.Exception.Message)" -ForegroundColor Red
    }
}

# Show overall project structure
Show-DirectoryTree -Path "." -MaxDepth 3

Write-Host ""
Write-Host "=== Key Directories ===" -ForegroundColor Green

# Check for specific important directories
$keyDirs = @(
    "src", "include", "tst", "dat", "build", "bin", 
    "src/bre", "tst/bre", "dat/dmn-tck"
)

foreach ($dir in $keyDirs) {
    if (Test-Path $dir) {
        $itemCount = (Get-ChildItem -Path $dir -Recurse -File | Measure-Object).Count
        Write-Host "  [OK] $dir ($itemCount files)" -ForegroundColor Green
    } else {
        Write-Host "  [--] $dir (not found)" -ForegroundColor Red
    }
}

Write-Host ""
Write-Host "=== DMN TCK Structure (if available) ===" -ForegroundColor Green

$tckPaths = @(
    "dat/dmn-tck",
    "tst/bre/dmn-tck"
)

foreach ($tckPath in $tckPaths) {
    if (Test-Path $tckPath) {
        Write-Host "Found TCK at: $tckPath" -ForegroundColor Cyan
        
        # Show TCK structure specifically
        if (Test-Path "$tckPath/TestCases") {
            $testLevels = Get-ChildItem "$tckPath/TestCases" -Directory
            foreach ($level in $testLevels) {
                $testCount = (Get-ChildItem $level.FullName -Directory | Measure-Object).Count
                Write-Host "  $($level.Name): $testCount test cases" -ForegroundColor Yellow
            }
        }
        
        # Count all test directories
        $allTests = Get-ChildItem $tckPath -Directory -Recurse | Where-Object { $_.Name -match '^\d{4}-' }
        Write-Host "  Total numbered test cases: $($allTests.Count)" -ForegroundColor Yellow
        
        break
    }
}

Write-Host ""
Write-Host "=== File Type Summary ===" -ForegroundColor Green

$extensions = @{}
# Exclude the same directories when counting file types
$excludeDirs = @('.github', '.vscode', 'build', '.git', 'cmake', 'CMakeFiles', '.vs')

Get-ChildItem -Path "." -Recurse -File | Where-Object {
    $shouldInclude = $true
    foreach ($excludeDir in $excludeDirs) {
        if ($_.FullName -like "*\$excludeDir\*" -or $_.FullName -like "*/$excludeDir/*") {
            $shouldInclude = $false
            break
        }
    }
    $shouldInclude
} | ForEach-Object {
    $ext = $_.Extension.ToLower()
    if ($ext -eq "") { $ext = "(no extension)" }
    if ($extensions.ContainsKey($ext)) {
        $extensions[$ext]++
    } else {
        $extensions[$ext] = 1
    }
}

$extensions.GetEnumerator() | Sort-Object Value -Descending | ForEach-Object {
    Write-Host "  $($_.Key): $($_.Value) files" -ForegroundColor White
}

Write-Host ""
Write-Host "Script completed." -ForegroundColor Green