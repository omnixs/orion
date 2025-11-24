# setup-clang-tidy.ps1
# Setup script to generate compile_commands.json for clang-tidy
#
# This script must be run from "x64 Native Tools Command Prompt for VS 2022"
# or it will invoke one for you if possible.

$ErrorActionPreference = 'Stop'

$RepoRoot = Resolve-Path (Join-Path $PSScriptRoot "..\..")
$BuildDir = Join-Path $RepoRoot "build-ninja"

Write-Host "═══════════════════════════════════════════════════════════════" -ForegroundColor Cyan
Write-Host "  Setup Clang-Tidy for Orion" -ForegroundColor Cyan
Write-Host "═══════════════════════════════════════════════════════════════" -ForegroundColor Cyan
Write-Host ""

# Check if we're in a Visual Studio Developer environment
$vsEnv = $env:VSINSTALLDIR
if (-not $vsEnv) {
    Write-Host "[INFO] Visual Studio environment not detected" -ForegroundColor Yellow
    Write-Host "[INFO] Attempting to locate and initialize VS 2022 environment..." -ForegroundColor Cyan
    
    # Try to find vcvars64.bat
    $vcvarsPath = "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat"
    if (-not (Test-Path $vcvarsPath)) {
        $vcvarsPath = "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
    }
    if (-not (Test-Path $vcvarsPath)) {
        $vcvarsPath = "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat"
    }
    
    if (Test-Path $vcvarsPath) {
        Write-Host "[INFO] Found vcvars64.bat at: $vcvarsPath" -ForegroundColor Green
        Write-Host "[INFO] Running CMake with VS environment..." -ForegroundColor Cyan
        Write-Host ""
        
        # Build the command to run
        $cmakeCmd = "cmake -S . -B build-ninja -G Ninja -DUSE_CLANG=ON -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows-static -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_BUILD_TYPE=Release"
        
        # Run in cmd with vcvars
        $fullCmd = "`"$vcvarsPath`" && cd /d `"$RepoRoot`" && $cmakeCmd"
        
        cmd /c $fullCmd
        
        if ($LASTEXITCODE -ne 0) {
            Write-Host "[ERROR] CMake configuration failed" -ForegroundColor Red
            exit 1
        }
    }
    else {
        Write-Host "[ERROR] Could not find Visual Studio 2022 installation" -ForegroundColor Red
        Write-Host ""
        Write-Host "Please run this script from 'x64 Native Tools Command Prompt for VS 2022'" -ForegroundColor Yellow
        Write-Host "or ensure Visual Studio 2022 is installed." -ForegroundColor Yellow
        exit 1
    }
}
else {
    Write-Host "[OK] Visual Studio environment detected: $vsEnv" -ForegroundColor Green
    Write-Host ""
    
    # We're already in a VS environment, just run cmake
    Push-Location $RepoRoot
    try {
        cmake -S . -B build-ninja -G Ninja `
            -DUSE_CLANG=ON `
            -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake `
            -DVCPKG_TARGET_TRIPLET=x64-windows-static `
            -DCMAKE_EXPORT_COMPILE_COMMANDS=ON `
            -DCMAKE_BUILD_TYPE=Release
        
        if ($LASTEXITCODE -ne 0) {
            Write-Host "[ERROR] CMake configuration failed" -ForegroundColor Red
            exit 1
        }
    }
    finally {
        Pop-Location
    }
}

# Verify compile_commands.json was created
$compileCommandsPath = Join-Path $BuildDir "compile_commands.json"
if (Test-Path $compileCommandsPath) {
    $entryCount = (Get-Content $compileCommandsPath | ConvertFrom-Json).Count
    Write-Host ""
    Write-Host "═══════════════════════════════════════════════════════════════" -ForegroundColor Cyan
    Write-Host "[SUCCESS] Setup complete!" -ForegroundColor Green
    Write-Host ""
    Write-Host "  compile_commands.json created: $compileCommandsPath" -ForegroundColor Green
    Write-Host "  Contains $entryCount compilation entries" -ForegroundColor Green
    Write-Host ""
    Write-Host "You can now run clang-tidy:" -ForegroundColor Cyan
    Write-Host "  .\tools\scripts\run-clang-tidy.ps1" -ForegroundColor Yellow
    Write-Host "  .\tools\scripts\run-clang-tidy.ps1 -File src\common\log.cpp" -ForegroundColor Yellow
    Write-Host "  .\tools\scripts\run-clang-tidy.ps1 -Path src\bre" -ForegroundColor Yellow
    Write-Host "  .\tools\scripts\run-clang-tidy.ps1 -Fix" -ForegroundColor Yellow
    Write-Host ""
}
else {
    Write-Host "[ERROR] compile_commands.json was not created" -ForegroundColor Red
    exit 1
}
