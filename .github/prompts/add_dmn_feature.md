# DMN Feature Implementation Template

## Objective

Implement missing DMN 1.5 FEEL language feature to improve TCK Level 3 compliance.

## Scope

Feature details (specify in task instance):
- [ ] DMN 1.5 specification section
- [ ] Syntax and semantics
- [ ] Expected TCK test impact
- [ ] Example test cases

## Key Implementation Steps

### 1. Specification Review
- Read DMN 1.5 spec section (docs/formal-24-01-01.txt)
- Understand syntax, semantics, edge cases
- Review TCK test cases

### 2. Architecture Design
- AST node type (if needed)
- Lexer tokens (if needed)
- Parser grammar changes
- Evaluator logic

### 3. Implementation
```
Typical files:
- include/orion/bre/feel/lexer.hpp
- src/bre/feel/lexer.cpp
- include/orion/bre/feel/parser.hpp
- src/bre/feel/parser.cpp
- include/orion/bre/feel/ast_node.hpp
- src/bre/ast_node.cpp
```

### 4. Testing
Follow [unit test instructions](../instructions/run_unit_tests.md) and [TCK test instructions](../instructions/run_tck_tests.md):
```powershell
# Unit tests (see run_unit_tests.md for test filtering options)
.\build\tst_orion.exe --run_test=<feature_tests>

# TCK verification (see run_tck_tests.md for test patterns)
.\build\orion_tck_runner.exe --test <tck_pattern>
```

### 5. Documentation
- Update copilot-instructions.md if pattern changes
- Document design decisions

## DMN-Specific Implementation Guidelines

### DMN 1.5 Specification Compliance
- **Reference Document**: Use `docs/formal-24-01-01.pdf` or `docs/formal-24-01-01.txt` for official OMG DMN 1.5 specification
- All decision logic must conform to DMN 1.5 standard
- Cross-reference section numbers when implementing features (e.g., "DMN 1.5 Section 8.1 Decision Table Evaluation")
- Validate behavior against specification requirements before implementation

### FEEL Expression Evaluation
- Handle negative literals, string concatenation, context variables
- DMN constants: `true`, `false`, `null`
- Context-aware variable resolution from input JSON
- Follow FEEL grammar and semantics from DMN 1.5 Chapter 10

### Hit Policy Implementation
- Support FIRST, UNIQUE, COLLECT variants per DMN 1.5 Section 8.1
- Aggregation functions: SUM, COUNT (DMN 1.5 Section 8.1.2)
- Policy override capability via `EvalOptions`

## Success Criteria

- [ ] Specification requirements met
- [ ] Unit tests pass (>90% coverage)
- [ ] TCK tests pass (target specified)
- [ ] No regressions in existing tests
- [ ] Code follows CODING_STANDARDS.md

## Typical Timeline

- Specification review: 1-2 hours
- Design: 1-2 hours
- Implementation: 4-8 hours
- Testing: 2-4 hours
- Total: 8-16 hours (varies by complexity)

## Retrospective

(This section will be filled after task completion with learnings and improvements)
