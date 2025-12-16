---
template: add_dmn_feature.md
agent: ""
status: not-started
category: feature
priority: medium
estimated-effort: "6-10 hours"
actual-effort: ""
---

# Task: Add DMN Namespace Support

Add support for parsing and storing DMN namespace declarations from `<definitions>` elements to improve DMN 1.5 compliance and model identity handling.

## Context

**Background and Motivation:**
- The Orion DMN engine currently ignores namespace declarations in DMN models
- When a DMN file specifies `namespace="http://example.com/dmn"` in its definitions element, the engine doesn't store or validate this namespace information
- This could lead to issues with model identity and integration with other DMN tools
- Basic requirement for proper DMN 1.5 compliance

**Current State:**
- DMN parser reads decision tables, literal expressions, and BKMs but skips namespace attributes
- Engine works functionally but doesn't respect namespace context
- No validation of namespace-qualified references

**Related Issues/PRs:**
- GitHub feature request: DMN namespace support
- Foundation for future qualified name and import support

## Scope

**Included in this task:**
- [ ] Parse namespace attribute from DMN `<definitions>` element
- [ ] Add namespace field to DmnModel structure
- [ ] Store namespace information in BusinessRulesEngine
- [ ] Update dmn_parser.cpp to extract namespace from XML
- [ ] Add unit tests for namespace parsing
- [ ] Validate namespace is preserved through load/evaluate cycle

**Explicitly excluded:**
- DMN Import element support (future task)
- Qualified name resolution in FEEL expressions (future task)
- Multi-model namespace collision handling (future task)
- Namespace-qualified typeRef attributes (future task)

## Detailed Instructions

### Step 1: Update DMN Model Structure

**File: `include/orion/bre/dmn_model.hpp`**
- Add `std::string namespace_uri` field to appropriate model structure
- Document the field purpose and DMN 1.5 reference
- Consider where namespace should be stored (DmnModel vs BusinessRulesEngine)

**Expected outcome:** Model can store namespace information

### Step 2: Update DMN Parser

**File: `src/bre/dmn_parser.cpp`**
- Locate existing XML parsing logic for `<definitions>` element
- Extract `namespace` attribute using rapidxml
- Pass namespace information to model construction
- Handle cases where namespace attribute is missing (use empty string or default)

**Expected outcome:** Parser extracts namespace from DMN XML

### Step 3: Update Engine API

**File: `src/api/engine.cpp`**
- Modify `load_dmn_model` to capture and store namespace information
- Ensure namespace is available for future use
- Consider logging namespace information for debugging

**Expected outcome:** Engine preserves namespace through load process

### Step 4: Add Unit Tests

**File: `tst/bre/test_dmn_parser.cpp` (or new test file)**
- Test parsing DMN with namespace declaration
- Test parsing DMN without namespace (default behavior)
- Verify namespace is correctly extracted and stored
- Test namespace with various URI formats

**Expected outcome:** Comprehensive test coverage for namespace parsing

### Step 5: Integration Testing

- Create test DMN file with namespace declaration
- Verify end-to-end namespace preservation
- Test with existing airline ticketing or other test models
- Ensure no regression in models without namespace

**Expected outcome:** Namespace support works in full evaluation pipeline

## Success Criteria

- [ ] DMN models with `namespace="..."` declarations are parsed correctly
- [ ] Namespace information is stored and accessible within the engine
- [ ] Models without namespace declarations continue to work unchanged
- [ ] All existing unit tests pass (no regressions)
- [ ] New unit tests demonstrate namespace parsing functionality
- [ ] Code follows CODING_STANDARDS.md (naming conventions, error handling)

## Validation Steps

1. Build succeeds in both Debug and Release: `cmake --build build --config Debug/Release`
2. Unit tests pass: `.\build\Debug\tst_orion.exe --log_level=test_suite`
3. TCK tests show no regressions: `.\build\Debug\orion_tck_runner.exe --log_level=error`
4. Test with namespace-enabled DMN files works correctly
5. Existing DMN models (without namespace) continue to function

## Reference Documentation

- [DMN Feature Template](../prompts/add_dmn_feature.md) - Standard DMN feature implementation process
- [CODING_STANDARDS.md](../../CODING_STANDARDS.md) - Project coding standards and naming conventions
- [Build Instructions](../instructions/build.md) - Build and configuration
- [Unit Test Instructions](../instructions/run_unit_tests.md) - Testing procedures
- [DMN 1.5 Specification](../../docs/formal-24-01-01.txt) - Section 6.3.3 Import metamodel, namespace handling

## DMN 1.5 Specification References

- **Section 6.3.3**: Import metamodel and namespace declarations
- **Section 6.2.1**: Definitions metamodel with namespace attribute
- **Page 2394**: "The DMN namespace indicates that the imported element is a DMN Definitions element"
- **Page 2402**: "namespace-qualified names, such as typeRefs specifying imported ItemDefinitions"

## Implementation Notes

- Use rapidxml API for XML attribute parsing (consistent with existing parser)
- Follow existing error handling patterns in dmn_parser.cpp
- Consider namespace as optional attribute (DMN spec allows empty namespace)
- Ensure thread-safety if namespace is stored at engine level
- Document namespace behavior in API comments

## Retrospective

(This section will be filled after task completion with learnings and improvements)

### What worked well:


### What was unclear or problematic:


### Suggestions for improvement:


### Actual effort:


### Blockers encountered:
