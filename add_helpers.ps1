# Add eval_feel helper to all test files that don't have it

$testFiles = Get-ChildItem "tst\bre\feel\test_*.cpp"

$helperCode = @'
namespace {
    json eval_feel(std::string_view expression, const json& context = json::object()) {
        static thread_local RegexCache cache(100);
        EvaluationContext eval_ctx;
        eval_ctx.regex_cache = &cache;
        return Evaluator::evaluate(expression, context, eval_ctx);
    }
}

'@

foreach ($file in $testFiles) {
    $content = Get-Content $file.FullName -Raw
    
    # Check if helper already exists
    if ($content -match 'namespace \{[^}]*eval_feel') {
        Write-Host "✓ $($file.Name) already has helper"
        continue
    }
    
    # Find where to insert (after includes, before BOOST_AUTO_TEST_SUITE)
    if ($content -match '(?s)(.*?)(BOOST_AUTO_TEST_SUITE)') {
        $before = $matches[1]
        $suite = $matches[2]
        $after = $content.Substring($matches[0].Length)
        
        $newContent = $before + $helperCode + $suite + $after
        Set-Content $file.FullName -Value $newContent -NoNewline
        Write-Host "✓ Added helper to $($file.Name)"
    } else {
        Write-Host "✗ Could not find BOOST_AUTO_TEST_SUITE in $($file.Name)"
    }
}

Write-Host "`nDone!"
