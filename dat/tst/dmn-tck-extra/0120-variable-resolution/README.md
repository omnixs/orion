# 0120-variable-resolution

This test case validates robust variable resolution in the ORION DMN engine, specifically for DMN input names containing spaces, underscores, and case differences. It ensures that FEEL expressions can correctly reference input data regardless of formatting variations in the DMN model.

## Description
- **DMN Model:** 0120-variable-resolution.dmn
- **Test Case:** 0120-variable-resolution-test-01.xml
- **Purpose:** Confirms that the engine resolves input names such as "Input With Spaces" and matches them to FEEL variable references like `Input_With_Spaces`, `input_with_spaces`, and `input with spaces`.

## Specification Reference
- **OMG DMN 1.5 Specification:**
  - Section 10.3.2.3 (Name matching and variable resolution)
  - Section 10.3.2.4 (FEEL context variable resolution)

## Notes
- This test case is part of the `/dat/tst/dmn-tck-extra/` suite for ORION-specific and edge-case validation.
- All test cases in this directory should be executed and reported separately in both `tst_bre` and `orion_tck_runner` runners.
