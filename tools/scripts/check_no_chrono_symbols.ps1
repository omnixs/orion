# check_no_chrono_symbols.ps1
# Verify that an Orion library built with -DORION_ENABLE_TZDB=OFF does not use std::chrono.
#
# Lists the symbol table of the static library (dumpbin on Windows, nm elsewhere) and fails when
# any std::chrono / time zone database symbol is defined or referenced.
#
# Usage:
#   .\tools\scripts\check_no_chrono_symbols.ps1 -Library build\Release\orion_lib.lib
#   pwsh ./tools/scripts/check_no_chrono_symbols.ps1 -Library build/liborion_lib.a
#
# Exit code: 0 = no std::chrono symbols, 1 = std::chrono symbols found.

[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$Library,

    # Matches the std::chrono namespace only (MSVC-decorated or demangled), not e.g. "synchronous".
    [Parameter(Mandatory = $false)]
    [string]$Pattern = '@chrono@std@@|\bstd::(?:__\w+::)?chrono::|tzdb|locate_zone'
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

if (-not (Test-Path -LiteralPath $Library)) {
    throw "Library not found: $Library"
}

function Get-DumpbinPath {
    $onPath = Get-Command dumpbin.exe -ErrorAction SilentlyContinue
    if ($null -ne $onPath) { return $onPath.Source }

    $vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
    if (-not (Test-Path -LiteralPath $vswhere)) { throw "dumpbin.exe not on PATH and vswhere.exe not found" }
    $found = & $vswhere -latest -prerelease -products * -find "VC\Tools\MSVC\**\bin\Hostx64\x64\dumpbin.exe"
    $first = @($found) | Select-Object -First 1
    if (-not $first) { throw "dumpbin.exe not found via vswhere" }
    return $first
}

if ($env:OS -eq 'Windows_NT') {
    $symbols = @(& (Get-DumpbinPath) /nologo /symbols $Library)
    if ($LASTEXITCODE -ne 0) {
        throw "Could not list symbols of $Library"
    }

    $symbolRecords = @(
        foreach ($line in $symbols) {
            # Match COFF symbol records and inspect only the name after "|".
            if ($line -match '^\s*[0-9A-Fa-f]+\s+[0-9A-Fa-f]+\s+\S+.*\|\s*(?<name>\S.*)$') {
                [pscustomobject]@{
                    Name    = $Matches['name'].Trim()
                    Display = $line.Trim()
                }
            }
        }
    )
}
else {
    $symbols = @(& nm -C $Library)
    if ($LASTEXITCODE -ne 0) {
        throw "Could not list symbols of $Library"
    }

    $symbolRecords = @(
        foreach ($line in $symbols) {
            # GNU nm's n/N entries are debugging symbols, not compiled or referenced symbols.
            if ($line -match '^\s*(?:(?:.+?):\s+)?(?:[0-9A-Fa-f]+\s+)?(?<type>[A-Za-z?])\s+(?<name>.+?)\s*$' `
                -and $Matches['type'] -notin @('n', 'N')) {
                [pscustomobject]@{
                    Name    = $Matches['name'].Trim()
                    Display = $line.Trim()
                }
            }
        }
    )
}

if ($symbolRecords.Count -eq 0) {
    throw "Could not parse any symbols from $Library"
}

$hits = @(
    $symbolRecords |
        Where-Object { $_.Name -match $Pattern } |
        ForEach-Object { $_.Display } |
        Sort-Object -Unique
)
if ($hits.Count -gt 0) {
    Write-Host "[FAIL] $Library uses std::chrono ($($hits.Count) symbol(s)):" -ForegroundColor Red
    $hits | Select-Object -First 20 | ForEach-Object { Write-Host "    $_" }
    Write-Host "Library code must only use std::chrono inside '#if ORION_TZDB_ENABLED' (see CODING_STANDARDS.md)." -ForegroundColor Yellow
    exit 1
}

Write-Host "[OK] No std::chrono symbols in $Library ($(@($symbols).Count) symbol lines checked)." -ForegroundColor Green
exit 0
