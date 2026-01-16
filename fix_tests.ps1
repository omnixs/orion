# Fix test files to use eval_feel helper instead of Evaluator instance

$testFiles = Get-ChildItem "tst\bre\feel\*.cpp" -Recurse

foreach ($file in $testFiles) {
    $content = Get-Content $file.FullName -Raw
    $original = $content
    
    # Remove lines with "Evaluator eval;" or "orion::bre::feel::Evaluator eval;"
    $content = $content -replace '(?m)^\s*(orion::bre::feel::)?Evaluator eval;\r?\n', ''
    
    # Replace eval.evaluate( with eval_feel(
    $content = $content -replace 'eval\.evaluate\(', 'eval_feel('
    
    # Replace Evaluator::evaluate( with eval_feel(  (for static calls)
    $content = $content -replace 'Evaluator::evaluate\(', 'eval_feel('
    
    if ($content -ne $original) {
        Set-Content $file.FullName -Value $content -NoNewline
        Write-Host "Fixed: $($file.Name)"
    }
}

Write-Host "Done!"
