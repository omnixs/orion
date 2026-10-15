# check_no_chrono_symbols.ps1
# Verify that an Orion library uses std::chrono exactly when its build variant allows it.
#
# Lists the symbol table of the static library (dumpbin on Windows, nm elsewhere) and looks for
# std::chrono / time zone database symbols, defined or referenced (including template instances
# pulled in from third-party headers).
#
# Without -CMakeCache the library must not use std::chrono (the ORION_ENABLE_TZDB=OFF contract).
# With -CMakeCache the expectation follows the configured ORION_TZDB_ENABLED: 0 = no symbols
# allowed, 1 = symbols required (proves the time zone code is compiled and the check finds it).
#
# Usage:
#   .\tools\scripts\check_no_chrono_symbols.ps1 -Library build\Debug\orion_lib.lib
#   pwsh ./tools/scripts/check_no_chrono_symbols.ps1 -Library build/liborion_lib.a -CMakeCache build/CMakeCache.txt
#
# Exit code: 0 = symbols match the expectation, 1 = they do not.

[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$Library,

    [Parameter(Mandatory = $false)]
    [string]$CMakeCache,

    # std::chrono in MSVC-decorated (incl. back-referenced std), Itanium-mangled (libstdc++/libc++)
    # or demangled form, plus the MSVC STL's extern "C" __std_tzdb_* helpers.
    [Parameter(Mandatory = $false)]
    [string]$Pattern = '@chrono@(?:std@|\d)|St6chrono|St3__16chrono|\bstd::(?:__\w+::)?chrono::|tzdb|locate_zone'
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

if (-not (Test-Path -LiteralPath $Library)) {
    throw "Library not found: $Library"
}

$expectChrono = $false
if ($CMakeCache) {
    $entry = Select-String -LiteralPath $CMakeCache -Pattern '^ORION_TZDB_ENABLED_EFFECTIVE:INTERNAL=([01])$'
    if ($null -eq $entry) {
        throw "ORION_TZDB_ENABLED_EFFECTIVE not found in $CMakeCache (configure with the current CMakeLists.txt)"
    }
    $expectChrono = $entry.Matches[0].Groups[1].Value -eq '1'
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
                    Object  = ''
                    Display = $line.Trim()
                }
            }
        }
    )
}
else {
    # -A prefixes every line with "<archive>:<member>:" so hits can be attributed to a source file.
    $symbols = @(& nm -A -C $Library)
    if ($LASTEXITCODE -ne 0) {
        throw "Could not list symbols of $Library"
    }

    $prefix = "${Library}:"
    $symbolRecords = @(
        foreach ($line in $symbols) {
            if (-not $line.StartsWith($prefix)) { continue }
            # GNU nm's n/N entries are debugging symbols, not compiled or referenced symbols.
            if ($line.Substring($prefix.Length) -match '^(?<object>[^:]+):(?:[0-9A-Fa-f]+)?\s+(?<type>[A-Za-z?])\s+(?<name>.+?)\s*$' `
                -and $Matches['type'] -notin @('n', 'N')) {
                [pscustomobject]@{
                    Name    = $Matches['name']
                    Object  = $Matches['object']
                    Display = "$($Matches['object']): $($Matches['type']) $($Matches['name'])"
                }
            }
        }
    )
}

if ($symbolRecords.Count -eq 0) {
    throw "Could not parse any symbols from $Library"
}

$hitRecords = @($symbolRecords | Where-Object { $_.Name -match $Pattern })
$hits = @($hitRecords | ForEach-Object { $_.Display } | Sort-Object -Unique)
$variant = if ($CMakeCache) { "ORION_TZDB_ENABLED=$([int]$expectChrono)" } else { "ORION_ENABLE_TZDB=OFF" }

if ($expectChrono) {
    if ($hits.Count -eq 0) {
        Write-Host "[FAIL] $Library has no std::chrono symbols although $variant." -ForegroundColor Red
        Write-Host "Named time zones are not compiled in, or -Pattern no longer matches this toolchain's symbols." -ForegroundColor Yellow
        exit 1
    }
    Write-Host "[OK] $Library uses std::chrono as expected for $variant ($($hits.Count) symbol(s))." -ForegroundColor Green
    exit 0
}

if ($hits.Count -gt 0) {
    Write-Host "[FAIL] $Library uses std::chrono although $variant ($($hits.Count) symbol(s)):" -ForegroundColor Red
    $objects = @($hitRecords | Where-Object { $_.Object } | Group-Object Object | Sort-Object Count -Descending)
    if ($objects.Count -gt 0) {
        Write-Host "  By object file:"
        $objects | ForEach-Object { Write-Host "    $($_.Name): $($_.Count)" }
    }
    $hits | Select-Object -First 20 | ForEach-Object { Write-Host "    $_" }
    Write-Host "Library code must only use std::chrono inside '#if ORION_TZDB_ENABLED', including via third-party headers (see CODING_STANDARDS.md)." -ForegroundColor Yellow
    exit 1
}

Write-Host "[OK] No std::chrono symbols in $Library for $variant ($($symbolRecords.Count) symbols checked)." -ForegroundColor Green
exit 0
