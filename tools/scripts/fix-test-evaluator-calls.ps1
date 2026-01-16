# Fix test files to use proper Evaluator API with EvaluationContext
# This script adds the eval_feel helper and updates all evaluate() calls

$testFiles = Get-ChildItem -Path "c:\workspace\orion\tst" -Recurse -Filter "*.cpp"

foreach ($file in $testFiles) {
    $content = Get-Content $file.FullName -Raw
    $modified = $false
    
    # Skip if already has eval_feel helper
    if ($content -match "eval_feel\(") {
        Write-Host "Skipping $($file.Name) - already has eval_feel helper"
        continue
    }
    
    # Skip if doesn't use Evaluator
    if ($content -notmatch "Evaluator") {
        continue
    }
    
    Write-Host "Processing $($file.Name)..."
    
    # Add helper after includes, before first test suite
    if ($content -match '(#include\s+<orion/bre/feel/evaluator\.hpp>)') {
        $modified = $true
        $content = $content -replace '(#include\s+<orion/bre/feel/evaluator\.hpp>)', "`$1`n#include <orion/bre/feel/regex_cache.hpp>`n`n// Test helper: evaluate with proper EvaluationContext`nnamespace {`n    json eval_feel(std::string_view expression, const json& context = json::object()) {`n        static thread_local RegexCache cache(100);`n        EvaluationContext eval_ctx;`n        eval_ctx.regex_cache = &cache;`n        return Evaluator::evaluate(expression, context, eval_ctx);`n    }`n}"
    }
    
    # Replace eval.evaluate( with eval_feel(
    if ($content -match 'eval\.evaluate\(') {
        $modified = $true
        $content = $content -replace 'Evaluator eval;\s*\n\s*auto result = eval\.evaluate\(', 'auto result = eval_feel('
        $content = $content -replace 'Evaluator eval;\s*\n\s*json result = eval\.evaluate\(', 'json result = eval_feel('
    }
    
    # Replace Evaluator::evaluate( with eval_feel(
    if ($content -match 'Evaluator::evaluate\(') {
        $modified = $true  
        # Handle both standalone and assignment cases
        $content = $content -replace '(\s+)auto result = Evaluator::evaluate\(', "`$1auto result = eval_feel("
        $content = $content -replace '(\s+)json result = Evaluator::evaluate\(', "`$1json result = eval_feel("
        # Handle any remaining Evaluator::evaluate calls
        $content = $content -replace 'Evaluator::evaluate\(', 'eval_feel('
    }
    
    if ($modified) {
        Set-Content -Path $file.FullName -Value $content -NoNewline
        Write-Host "  Updated $($file.Name)"
    }
}

Write-Host "`nDone! Modified test files to use eval_feel() helper."
