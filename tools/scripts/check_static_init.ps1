# check_static_init.ps1
# Guard against static/thread-local state that needs runtime initialization in the Orion library.
#
# Rule (CODING_STANDARDS.md, "Static and Global State"): every variable with static or thread
# storage duration in library code must be `constexpr` or `constinit`. Lazily (first-use) or
# dynamically initialized statics allocate inside the host's call stack and keep that memory for
# the process lifetime; hosts that fence or profile per-command heap usage report this as a leak.
#
# Scanned: sources listed in ORION_SRC (CMakeLists.txt) plus public and internal library headers.
# A justified exception needs a `// static-init-ok: <reason>` comment on the declaration line or
# on the line directly above it.
#
# Usage:
#   .\tools\scripts\check_static_init.ps1                      # Scan the library
#   .\tools\scripts\check_static_init.ps1 -Files a.cpp,b.hpp   # Scan specific files
#
# Exit code: 0 = no violations, 1 = violations found.

[CmdletBinding()]
param(
    [Parameter(Mandatory = $false)]
    [string[]]$Files
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path

# Comments, string/char literals (incl. raw strings) and preprocessor lines.
$StripPattern = '(?m)//[^\n]*|/\*[\s\S]*?\*/|(?<![\w])(?:u8|u|U|L)?R"([^(\s"\\]{0,16})\([\s\S]*?\)\1"|"(?:\\.|[^"\\\n])*"|(?<![\w])''(?:\\.|[^''\\\n])+''|^[ \t]*#(?:[^\n]*\\\r?\n)*[^\n]*'

$StripEvaluator = [System.Text.RegularExpressions.MatchEvaluator] {
    param($m)
    $value = $m.Value
    $blank = [regex]::Replace($value, '[^\r\n]', ' ')
    $isLiteral = ($value.EndsWith('"') -or $value.EndsWith("'")) -and -not $value.StartsWith('/') -and -not $value.TrimStart().StartsWith('#')
    if ($isLiteral -and $blank.Length -ge 2) {
        # Keep a marker so literals stay visible to the declaration heuristics.
        return '"' + $blank.Substring(1, $blank.Length - 2) + '"'
    }
    return $blank
}

function Get-LibraryFiles {
    $cmake = [System.IO.File]::ReadAllText((Join-Path $RepoRoot "CMakeLists.txt"))
    $match = [regex]::Match($cmake, 'set\(ORION_SRC\s+([^)]*)\)')
    if (-not $match.Success) {
        throw "ORION_SRC not found in CMakeLists.txt"
    }

    $result = [System.Collections.Generic.List[string]]::new()
    foreach ($entry in ($match.Groups[1].Value -split '\s+')) {
        if ($entry -like '*.cpp') {
            $result.Add((Join-Path $RepoRoot $entry))
        }
    }

    $headerDirs = @('include/orion/api', 'include/orion/bre', 'include/orion/bre/feel', 'include/orion/common',
                    'src/common', 'src/bre', 'src/bre/feel')
    foreach ($dir in $headerDirs) {
        $path = Join-Path $RepoRoot $dir
        if (Test-Path -LiteralPath $path) {
            foreach ($header in (Get-ChildItem -LiteralPath $path -Filter '*.hpp' -File)) {
                $result.Add($header.FullName)
            }
        }
    }
    return $result
}

function Split-TopLevel([string]$text) {
    $parts = [System.Collections.Generic.List[string]]::new()
    $depth = 0
    $start = 0
    for ($i = 0; $i -lt $text.Length; $i++) {
        $c = $text[$i]
        if ('<([{'.Contains($c)) { $depth++ }
        elseif ('>)]}'.Contains($c)) { $depth-- }
        elseif ($c -eq ',' -and $depth -eq 0) {
            $parts.Add($text.Substring($start, $i - $start))
            $start = $i + 1
        }
    }
    $parts.Add($text.Substring($start))
    return $parts
}

function Test-IsParameterList([string]$content) {
    $typePattern = '^(?:(?:const|volatile|struct|class|typename|enum|unsigned|signed|long|short)\s+)*[A-Za-z_][\w:]*(?:\s*<.*>)?(?:\s+const)?[\s&*]*(?:[A-Za-z_]\w*)?(?:\s*\[\s*\w*\s*\])?(?:\.\.\.)?$'
    foreach ($item in (Split-TopLevel $content)) {
        $param = $item.Trim()
        if ($param -eq '' -or $param -eq '...') { continue }
        $param = $param -replace '\s*=.*$', ''
        if ($param -match '["'']' -or $param -match '^[-+]?[\d.]') { return $false }
        if ($param -notmatch $typePattern) { return $false }
    }
    return $true
}

function Remove-TemplateHeader([string]$text) {
    $open = $text.IndexOf('<')
    $depth = 0
    for ($i = $open; $i -lt $text.Length; $i++) {
        if ($text[$i] -eq '<') { $depth++ }
        elseif ($text[$i] -eq '>') {
            $depth--
            if ($depth -eq 0) { return $text.Substring($i + 1).Trim() }
        }
    }
    return ''
}

# Returns $true when the statement declares a variable with static/thread storage that is
# neither constexpr nor constinit.
function Test-StatementViolation([string]$statement, [string]$scope) {
    $t = ($statement -replace '\s+', ' ').Trim()
    $t = ($t -replace '^(?:(?:public|private|protected)\s*:\s*)+', '').Trim()
    while ($t.StartsWith('[[')) {
        $end = $t.IndexOf(']]')
        if ($end -lt 0) { break }
        $t = $t.Substring($end + 2).Trim()
    }
    if ($t -match '^template\s*<') { $t = Remove-TemplateHeader $t }
    if ($t -eq '') { return $false }

    $skipKeywords = '^(?:using|typedef|friend|return|static_assert|namespace|class|struct|union|enum|template|goto|break|continue|throw|co_return|co_yield|delete|case|default|concept|requires|if|for|while|switch|do|else|try|catch)\b'
    if ($t -match $skipKeywords) { return $false }

    $hasStorage = $t -match '\b(?:static|thread_local)\b'
    if ($scope -ne 'ns' -and -not $hasStorage) { return $false }
    if ($t -match '\b(?:constexpr|constinit)\b') { return $false }
    if ($t -match '\boperator\b') { return $false }
    if ($scope -eq 'ns' -and $t -match '^extern\b' -and $t -notmatch '=') { return $false }

    # First '(', '=' or '{' outside template brackets decides variable vs. function.
    $angle = 0
    $idx = -1
    for ($i = 0; $i -lt $t.Length; $i++) {
        $c = $t[$i]
        if ($c -eq '<') { $angle++ }
        elseif ($c -eq '>' -and ($i -eq 0 -or $t[$i - 1] -ne '-')) { if ($angle -gt 0) { $angle-- } }
        elseif ($angle -eq 0 -and ($c -eq '(' -or $c -eq '=' -or $c -eq '{')) { $idx = $i; break }
    }

    if ($idx -lt 0) { return (($t -split ' ').Count -ge 2) }
    if ($t[$idx] -ne '(') { return $true }

    $depth = 0
    $close = -1
    for ($i = $idx; $i -lt $t.Length; $i++) {
        if ($t[$i] -eq '(') { $depth++ }
        elseif ($t[$i] -eq ')') {
            $depth--
            if ($depth -eq 0) { $close = $i; break }
        }
    }
    if ($close -lt 0) { return $false }

    $content = $t.Substring($idx + 1, $close - $idx - 1)
    if ($content -match '^\s*[*&]') { return $true }   # pointer/reference to function variable
    if ($content.Trim() -eq '') { return $false }      # function declaration
    return -not (Test-IsParameterList $content)
}

function Get-BraceKind([string]$header) {
    $h = ($header -replace '\s+', ' ').Trim()
    $h = ($h -replace '^(?:(?:public|private|protected)\s*:\s*)+', '').Trim()
    if ($h -eq '') { return 'block' }
    if ($h -match '(?:^|\s)namespace\b' -and $h -notmatch '\(') { return 'ns' }
    if ($h -match '^extern\s*"') { return 'ns' }
    if ($h -match '\benum\b') { return 'enum' }
    if ($h -match '(?:^|\s|>)(?:class|struct|union)\b' -and $h -notmatch '[=(]') { return 'class' }
    if ($h -match ':$' -and $h -notmatch '::$') { return 'block' }
    if ($h -match '(?:=|,|\[|\{|\?|\breturn)$') { return 'init' }
    if ($h -match '\b(?:else|do|try)$') { return 'block' }
    if ($h -notmatch '\(' -and $h -match '[\w>\]]$') { return 'init' }
    return 'block'
}

function Get-LineNumber([System.Collections.Generic.List[int]]$lineStarts, [int]$position) {
    $index = $lineStarts.BinarySearch($position)
    if ($index -lt 0) { $index = (-bnot $index) - 1 }
    return $index + 1
}

function Find-Violations([string]$path) {
    $raw = [System.IO.File]::ReadAllText($path)
    $code = [regex]::Replace($raw, $StripPattern, $StripEvaluator)
    $rawLines = $raw -split "\n"

    $lineStarts = [System.Collections.Generic.List[int]]::new()
    $lineStarts.Add(0)
    foreach ($newline in [regex]::Matches($code, "\n")) { $lineStarts.Add($newline.Index + 1) }

    $violations = [System.Collections.Generic.List[object]]::new()
    $stack = [System.Collections.Generic.List[string]]::new()
    $statementStart = 0
    $paren = 0
    $initDepth = 0

    foreach ($token in [regex]::Matches($code, '[(){}\[\];]')) {
        $c = $token.Value
        $pos = $token.Index

        if ($initDepth -gt 0) {
            if ($c -eq '{') { $initDepth++ }
            elseif ($c -eq '}') { $initDepth-- }
            continue
        }

        if ($c -eq '(' -or $c -eq '[') { $paren++; continue }
        if ($c -eq ')' -or $c -eq ']') { if ($paren -gt 0) { $paren-- }; continue }
        if ($paren -gt 0) { continue }

        if ($c -eq '{') {
            $kind = Get-BraceKind $code.Substring($statementStart, $pos - $statementStart)
            if ($kind -eq 'init') { $initDepth = 1; continue }
            $stack.Add($kind)
            $statementStart = $pos + 1
        }
        elseif ($c -eq '}') {
            if ($stack.Count -gt 0) { $stack.RemoveAt($stack.Count - 1) }
            $statementStart = $pos + 1
        }
        else {
            $statement = $code.Substring($statementStart, $pos - $statementStart)
            $scope = 'ns'
            if ($stack.Contains('block')) { $scope = 'block' }
            elseif ($stack.Count -gt 0 -and $stack[$stack.Count - 1] -eq 'class') { $scope = 'class' }
            elseif ($stack.Count -gt 0 -and $stack[$stack.Count - 1] -eq 'enum') { $scope = 'enum' }

            if ($scope -ne 'enum' -and (Test-StatementViolation $statement $scope)) {
                $offset = $statement.Length - $statement.TrimStart().Length
                $startLine = Get-LineNumber $lineStarts ($statementStart + $offset)
                $endLine = Get-LineNumber $lineStarts $pos
                $suppressed = $false
                for ($line = [Math]::Max(1, $startLine - 1); $line -le $endLine; $line++) {
                    if ($rawLines[$line - 1] -match 'static-init-ok:') { $suppressed = $true; break }
                }
                if (-not $suppressed) {
                    $text = ($statement -replace '\s+', ' ').Trim()
                    if ($text.Length -gt 120) { $text = $text.Substring(0, 117) + '...' }
                    $violations.Add([pscustomobject]@{ Line = $startLine; Scope = $scope; Text = $text })
                }
            }
            $statementStart = $pos + 1
        }
    }
    return $violations
}

if ($Files) {
    $targets = $Files | ForEach-Object { (Resolve-Path -LiteralPath $_).Path }
}
else {
    $targets = Get-LibraryFiles
}

$total = 0
foreach ($file in $targets) {
    if (-not (Test-Path -LiteralPath $file)) {
        throw "File not found: $file"
    }
    $relative = $file
    if ($file.StartsWith($RepoRoot)) { $relative = $file.Substring($RepoRoot.Length).TrimStart('\', '/') }

    foreach ($violation in (Find-Violations $file)) {
        $total++
        Write-Host ("{0}:{1}: {2}-scope variable with static/thread storage must be constexpr or constinit" -f $relative, $violation.Line, $violation.Scope) -ForegroundColor Red
        Write-Host ("    {0}" -f $violation.Text)
    }
}

if ($total -gt 0) {
    Write-Host ""
    Write-Host "[FAIL] $total static initialization violation(s) in $(@($targets).Count) file(s)." -ForegroundColor Red
    Write-Host "See CODING_STANDARDS.md, section 'Static and Global State'." -ForegroundColor Yellow
    exit 1
}

Write-Host "[OK] No static initialization violations in $(@($targets).Count) file(s)." -ForegroundColor Green
exit 0
