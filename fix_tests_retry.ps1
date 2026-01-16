# Fix test files to use eval_feel - with retry logic

$files = @(
    'tst\bre\feel\test_evaluator_builtin_math.cpp',
    'tst\bre\feel\test_feel_matches.cpp', 
    'tst\bre\feel\test_parser_subtraction.cpp',
    'tst\bre\feel\test_named_parameters.cpp',
    'tst\bre\feel\test_tck_named_params_quick.cpp'
)

foreach ($f in $files) {
    if (-not (Test-Path $f)) {
        Write-Host "Skipping $f (not found)"
        continue
    }
    
    $retries = 5
    $success = $false
    
    for ($i = 0; $i -lt $retries; $i++) {
        try {
            $content = Get-Content $f -Raw -ErrorAction Stop
            
            # Apply replacements
            $content = $content -replace 'Evaluator::evaluate\(', 'eval_feel('
            $content = $content -replace '(?m)^\s*Evaluator eval;\r?\n', ''
            $content = $content -replace '(?m)^\s*orion::bre::feel::Evaluator eval;\r?\n', ''
            $content = $content -replace 'eval\.evaluate\(', 'eval_feel('
            
            # Try to write
            $stream = [System.IO.File]::Open($f, 'Open', 'Write', 'None')
            $writer = New-Object System.IO.StreamWriter($stream)
            $writer.Write($content)
            $writer.Close()
            $stream.Close()
            
            Write-Host "✓ Fixed $f"
            $success = $true
            break
        }
        catch {
            if ($i -lt ($retries - 1)) {
                Write-Host "  Retry $($i+1) for $f..."
                Start-Sleep -Milliseconds 500
            }
        }
    }
    
    if (-not $success) {
        Write-Host "✗ Failed to fix $f after $retries attempts"
    }
}

Write-Host "`nDone!"
