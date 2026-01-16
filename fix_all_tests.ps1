# Fix all test files to use eval_feel instead of Evaluator::evaluate

$files = @(
    'tst\bre\feel\test_evaluator_builtin_math.cpp',
    'tst\bre\feel\test_feel_matches.cpp',
    'tst\bre\feel\test_parser_subtraction.cpp',
    'tst\bre\feel\test_named_parameters.cpp',
    'tst\bre\feel\test_tck_named_params_quick.cpp'
)

foreach ($filePath in $files) {
    if (Test-Path $filePath) {
        Write-Host "Processing $filePath..."
        
        $lines = Get-Content $filePath
        $newLines = @()
        
        foreach ($line in $lines) {
            # Skip lines with just "Evaluator eval;" or "orion::bre::feel::Evaluator eval;"
            if ($line -match '^\s*(orion::bre::feel::)?Evaluator eval;\s*$') {
                Write-Host "  Removing: $line"
                continue
            }
            
            # Replace eval.evaluate( with eval_feel(
            if ($line -match 'eval\.evaluate\(') {
                $line = $line -replace 'eval\.evaluate\(', 'eval_feel('
                Write-Host "  Fixed eval.evaluate: $line"
            }
            
            # Replace Evaluator::evaluate( with eval_feel(
            if ($line -match 'Evaluator::evaluate\(') {
                $line = $line -replace 'Evaluator::evaluate\(', 'eval_feel('
                Write-Host "  Fixed Evaluator::evaluate: $line"
            }
            
            # Replace feel::Evaluator::evaluate( with eval_feel(  
            if ($line -match 'feel::Evaluator::evaluate\(') {
                $line = $line -replace 'feel::Evaluator::evaluate\(', 'eval_feel('
                Write-Host "  Fixed feel::Evaluator::evaluate: $line"
            }
            
            $newLines += $line
        }
        
        # Write back to file
        $newLines | Set-Content $filePath -Encoding UTF8
        Write-Host "Done with $filePath`n"
    } else {
        Write-Host "File not found: $filePath`n"
    }
}

Write-Host "All files processed!"
