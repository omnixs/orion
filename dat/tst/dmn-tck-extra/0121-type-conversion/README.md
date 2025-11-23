# 0121-type-conversion

## Purpose
This test case validates FEEL/DMN type conversion for arithmetic expressions, specifically:
- String to number conversion (e.g., "123.45" + 1.55 → 125.0)
- Boolean to number conversion (e.g., true + 2 → 3)

## DMN Specification Reference
- **DMN 1.5 Section 10.3.2.2 (FEEL type conversion)**: "If a value is a string and a number is expected, the string is converted to a number if possible. If a value is a boolean and a number is expected, true is converted to 1 and false to 0."
- **DMN 1.5 Section 10.3.2.4 (Arithmetic operators)**: "Operands are converted to numbers as needed."

## Files
- `type-conversion.dmn`: DMN model for type conversion
- `type-conversion-test.xml`: Test cases for type conversion

## Coverage Note
This test targets the type conversion logic recently added to the FEEL evaluator. It is designed to ensure compliance with the DMN 1.5 specification, even if it does not increase overall TCK coverage due to missing or non-compliant features in the reference TCK suite.
