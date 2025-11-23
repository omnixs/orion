# 0126-logical-variable-resolution

This test case validates proper variable resolution within FEEL logical expressions (AND, OR, NOT) in the ORION DMN engine, specifically addressing DMN 1.5 Section 10.3.2.10 (Logical expressions) combined with Section 10.3.2.3 (Name matching and variable resolution).

## Files:
- **DMN Model:** 0126-logical-variable-resolution.dmn
- **Test Case:** 0126-logical-variable-resolution-test-01.xml

## Coverage:
This test ensures proper implementation of:
  - Section 10.3.2.10 (FEEL logical expressions: and, or, not)
  - Section 10.3.2.3 (Name matching and variable resolution)
  - Section 8.5 (Ternary logic for null values)

## Purpose:
Tests that variables in logical expressions are properly resolved from the evaluation context before applying ternary logic semantics. This addresses the specific case where expressions like "A and B" fail to resolve variables A and B from the input context.

## Requirements:
- All test cases in this directory should be executed and reported separately in both `tst_bre` and `orion_tck_runner` runners.