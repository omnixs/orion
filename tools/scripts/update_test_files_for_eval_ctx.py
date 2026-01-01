#!/usr/bin/env python3
"""
Script to automatically update test files to add RegexCache and EvaluationContext.

This script:
1. Adds #include for regex_cache.hpp
2. Adds RegexCache and EvaluationContext variables at the start of each test case
3. Updates all Evaluator::evaluate() calls to pass eval_ctx
4. Updates all ast->evaluate() calls to pass eval_ctx
"""

import re
import sys
from pathlib import Path

def update_test_file(file_path):
    """Update a single test file."""
    print(f"Processing {file_path}...")
    
    with open(file_path, 'r') as f:
        content = f.read()
    
    original_content = content
    
    # Step 1: Add include if not present
    if '#include <orion/bre/feel/regex_cache.hpp>' not in content:
        # Find the last #include line
        include_pattern = r'(#include <orion/bre/feel/evaluator\.hpp>)'
        content = re.sub(
            include_pattern,
            r'\1\n#include <orion/bre/feel/regex_cache.hpp>',
            content
        )
    
    # Step 2: For each BOOST_AUTO_TEST_CASE, add RegexCache and EvaluationContext if not present
    # Pattern to match test case start
    test_case_pattern = r'(BOOST_AUTO_TEST_CASE\([^)]+\)\s*\{)'
    
    def add_regex_cache_to_test(match):
        test_start = match.group(1)
        # Check if this test case already has regex_cache
        # Look ahead to see if there's already a RegexCache declaration
        return test_start
    
    # Step 3: Update Evaluator::evaluate calls
    # Pattern: Evaluator::evaluate("expr", context) -> Evaluator::evaluate("expr", context, eval_ctx)
    # Only update calls with exactly 2 arguments
    evaluator_pattern = r'Evaluator::evaluate\(([^,]+),\s*([^)]+)\)(?!,)'
    content = re.sub(
        evaluator_pattern,
        r'Evaluator::evaluate(\1, \2, eval_ctx)',
        content
    )
    
    # Step 4: Update ast->evaluate calls
    # Pattern: ast->evaluate(context) -> ast->evaluate(context, eval_ctx)
    ast_pattern = r'((?:ast|funcNode)->evaluate\([^)]+)\)(?!,)'
    content = re.sub(
        ast_pattern,
        r'\1, eval_ctx)',
        content
    )
    
    # Step 5: Update evaluator.evaluate calls (instance methods)
    instance_pattern = r'evaluator\.evaluate\(([^,]+),\s*([^)]+)\)(?!,)'
    content = re.sub(
        instance_pattern,
        r'evaluator.evaluate(\1, \2, eval_ctx)',
        content
    )
    
    if content != original_content:
        with open(file_path, 'w') as f:
            f.write(content)
        print(f"  ✓ Updated {file_path}")
        return True
    else:
        print(f"  - No changes needed for {file_path}")
        return False

def main():
    # Get all test files in tst/bre/feel/
    test_dir = Path(__file__).parent.parent.parent / 'tst' / 'bre' / 'feel'
    test_files = list(test_dir.glob('test_*.cpp'))
    
    if not test_files:
        print(f"No test files found in {test_dir}")
        return 1
    
    updated_count = 0
    for test_file in sorted(test_files):
        if update_test_file(test_file):
            updated_count += 1
    
    print(f"\nUpdated {updated_count}/{len(test_files)} files")
    return 0

if __name__ == '__main__':
    sys.exit(main())
