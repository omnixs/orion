---
name: Task
about: Create a new task for AI-driven development
title: '[TASK] '
labels: 'task'
assignees: ''
---

# Task File Template

**Instructions:** Copy this template to `.github/tasks/<ISSUE_NUMBER>_<snake_case_title>.md` and fill in all sections.

---

## Frontmatter (YAML)

```yaml
---
template: <template-name>.md  # Which template (improve_quality, add_dmn_feature, fix_bug, improve_perf, etc.)
agent: <agent-name>            # Which agent should execute (code-quality-refactor, code-reviewer, performance-optimizer, or leave blank for general agent)
status: not-started            # not-started | in-progress | completed
category: <category>           # code-quality | feature | bug-fix | ci-cd | performance | documentation
priority: <level>              # low | medium | high | critical
estimated-effort: "X-Y hours"  # Estimated time range
actual-effort: ""              # Filled after completion
---
```

## Task: [Title]

Brief one-sentence summary of what this task accomplishes.

## Context

**Background and Motivation:**
- Why is this task needed?
- What problem does it solve?
- What is the current state?

**Related Issues/PRs:**
- Related to issue #XXX
- Depends on PR #XXX
- Blocks issue #XXX

## Scope

**Included in this task:**
- [ ] Item 1
- [ ] Item 2
- [ ] Item 3

**Explicitly excluded:**
- Item A (reason: will be handled in separate task)
- Item B (reason: out of scope for this iteration)

## Detailed Instructions

### Step 1: [First Major Step]
- Specific action 1
- Specific action 2
- Expected outcome

### Step 2: [Second Major Step]
- Specific action 1
- Specific action 2
- Expected outcome

[Continue with additional steps as needed]

## Success Criteria

- [ ] Criterion 1 (specific, measurable)
- [ ] Criterion 2 (specific, measurable)
- [ ] Criterion 3 (specific, measurable)
- [ ] All unit tests pass
- [ ] No regressions in TCK tests
- [ ] Code follows CODING_STANDARDS.md

## Validation Steps

1. Build succeeds in both Debug and Release
2. Unit tests pass: `./build/tst_orion --log_level=test_suite`
3. TCK tests pass: `./build/orion_tck_runner --log_level=error`
4. Performance benchmarks show no regression (if applicable)
5. Code review checklist passes

## Reference Documentation

- [Template File](../prompts/<template-name>.md) - If using a standard template
- [CODING_STANDARDS.md](../../CODING_STANDARDS.md) - Project coding standards
- [Build Instructions](../instructions/build.md) - Build and configuration
- [Unit Test Instructions](../instructions/run_unit_tests.md) - Testing
- [TCK Test Instructions](../instructions/run_tck_tests.md) - Compliance
- Other relevant docs...

## Retrospective

(This section will be filled after task completion with learnings and improvements)

### What worked well:
- 

### What was unclear or problematic:
- 

### Suggestions for improvement:
- 

### Actual effort:
- 

### Blockers encountered:
- 
