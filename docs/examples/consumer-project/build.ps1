#!/usr/bin/env pwsh
# PowerShell build script for the consumer example project

$ErrorActionPreference = "Stop"

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$OrionRoot = Join-Path $ScriptDir "..\..\" | Resolve-Path

Write-Host "=== Building Orion Consumer Example ===" -ForegroundColor Cyan
Write-Host "Orion root: $OrionRoot"
Write-Host "Example dir: $ScriptDir"

# Check if vcpkg is available
if (-not $env:VCPKG_ROOT) {
    Write-Host "Error: VCPKG_ROOT environment variable not set" -ForegroundColor Red
    Write-Host "Please set it to your vcpkg installation directory:"
    Write-Host '  $env:VCPKG_ROOT = "C:\path\to\vcpkg"' -ForegroundColor Yellow
    exit 1
}

$VcpkgToolchain = Join-Path $env:VCPKG_ROOT "scripts\buildsystems\vcpkg.cmake"
if (-not (Test-Path $VcpkgToolchain)) {
    Write-Host "Error: vcpkg toolchain file not found at $env:VCPKG_ROOT" -ForegroundColor Red
    exit 1
}

Write-Host "Using vcpkg at: $env:VCPKG_ROOT"

# Configure with vcpkg overlay pointing to orion ports
Write-Host ""
Write-Host "=== Configuring project ===" -ForegroundColor Cyan

$BuildDir = Join-Path $ScriptDir "build"
$OverlayPorts = Join-Path $OrionRoot "ports"

cmake -B $BuildDir -S $ScriptDir `
    -DCMAKE_TOOLCHAIN_FILE="$VcpkgToolchain" `
    -DVCPKG_OVERLAY_PORTS="$OverlayPorts" `
    -DVCPKG_FEATURE_FLAGS=-binarycaching `
    -DCMAKE_BUILD_TYPE=Release

if ($LASTEXITCODE -ne 0) {
    Write-Host "Configuration failed" -ForegroundColor Red
    exit $LASTEXITCODE
}

# Build
Write-Host ""
Write-Host "=== Building project ===" -ForegroundColor Cyan
cmake --build $BuildDir --config Release

if ($LASTEXITCODE -ne 0) {
    Write-Host "Build failed" -ForegroundColor Red
    exit $LASTEXITCODE
}

Write-Host ""
Write-Host "=== Build complete ===" -ForegroundColor Green
Write-Host "Executable: $BuildDir\Release\consumer-example.exe"
Write-Host ""
Write-Host "Run with:"
Write-Host "  cd $ScriptDir"
Write-Host "  .\build\Release\consumer-example.exe"
