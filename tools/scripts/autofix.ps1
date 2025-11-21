Param()

# Single-run rebuild & test (no iteration).
# Rebuild sequence:
#  cmake -E remove_directory build
#  cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows-static
#  cmake --build build --config Debug
# Test:
#  .\build\Debug\tst_bre.exe --log_level=all

$ErrorActionPreference = 'Continue'
$RepoRoot = Split-Path -Parent $MyInvocation.MyCommand.Path | Split-Path -Parent
Set-Location $RepoRoot

function Invoke-Rebuild {
  Write-Host "[Rebuild] Clean" -ForegroundColor Cyan
  & cmake -E remove_directory build | Out-Null
  Write-Host "[Rebuild] Configure" -ForegroundColor Cyan
  $cfgArgs = @('cmake','-S','.', '-B','build','-G','Visual Studio 17 2022','-A','x64','-DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake','-DVCPKG_TARGET_TRIPLET=x64-windows-static')
  $cfgOut = & $cfgArgs[0] $cfgArgs[1..($cfgArgs.Length-1)] 2>&1
  $cfgCode = $LASTEXITCODE
  if($cfgCode -ne 0){ Write-Host '--- Configure FAILED ---' -ForegroundColor Red; ($cfgOut | Select -Last 80) | ForEach-Object { Write-Host $_ }; return @{ code=$cfgCode; stage='configure'; out=$cfgOut } }
  Write-Host "[Rebuild] Build Debug" -ForegroundColor Cyan
  $buildOut = & cmake --build build --config Debug 2>&1
  $bCode = $LASTEXITCODE
  if($bCode -ne 0){ Write-Host '--- Build FAILED ---' -ForegroundColor Red; ($buildOut | Select -Last 80) | ForEach-Object { Write-Host $_ }; return @{ code=$bCode; stage='build'; out=($cfgOut + $buildOut) } }
  return @{ code=0; stage='ok'; out=($cfgOut + $buildOut) }
}

function Invoke-Tests {
  $exe = Join-Path $RepoRoot 'build/Debug/tst_bre.exe'
  if(-not (Test-Path $exe)){ return @{ code=1; out="Missing test exe: $exe" } }
  Write-Host "[Test] Run" -ForegroundColor Cyan
  $out = & $exe --log_level=all 2>&1
  return @{ code=$LASTEXITCODE; out=$out }
}

function Parse-Failures($TestOutput){ $fails=@(); foreach($l in $TestOutput -split "`n"){ if($l -match '(FAILED|Failure|Assertion)'){ $fails+=$l.Trim() } } return $fails }

# Execute once
$re = Invoke-Rebuild
if($re.code -ne 0){ Write-Host "[Summary] Configure/Build failed." -ForegroundColor Red; exit 1 }
$te = Invoke-Tests
$fails = Parse-Failures $te.out
if($te.code -eq 0 -and $fails.Count -eq 0){ Write-Host "[Summary] All tests passed." -ForegroundColor Green; exit 0 }
Write-Warning ("[Summary] testExit={0} failures={1}" -f $te.code,$fails.Count)
Write-Host "--- Test Output (tail) ---" -ForegroundColor Yellow
($te.out | Select -Last 120) | ForEach-Object { Write-Host $_ }
exit $te.code

# Build and run comprehensive DMN TCK tests
Write-Host "`n=== Building DMN TCK Tests ===" -ForegroundColor Cyan
cmake --build build --config Release --target test_bre
if ($LASTEXITCODE -ne 0) {
    Write-Host "Build failed for DMN TCK tests!" -ForegroundColor Red
    exit 1
}

Write-Host "`n=== Running Level 2 Tests (Comprehensive Mode) ===" -ForegroundColor Green
ctest --test-dir build -R "dmn_tck_levels_xml/level2$" -V --output-on-failure

Write-Host "`n=== Running Level 2 Tests (Strict Mode) ===" -ForegroundColor Yellow  
ctest --test-dir build -R "dmn_tck_levels_xml/level2_strict" -V --output-on-failure

<# USAGE:
  powershell.exe -ExecutionPolicy Bypass -File scripts\autofix.ps1
#>
