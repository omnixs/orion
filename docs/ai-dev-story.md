# Orion AI Development Story

## 1. Motivation

Orion is an experiment in **AI‑only software development**:  
All code was generated using AI tools (e.g., ChatGPT, Copilot), with humans acting as reviewers, architects, and correctness gatekeepers.

The goals:
- Explore how far AI can be pushed in a real system (non‑toy project).
- Validate that an AI‑generated C++ library can meet strict correctness constraints.
- Build an auditable development workflow for safety‑critical or high‑load environments.

---

## 2. Guiding Principles

### 2.1 Human Ownership
- AI generates code, documentation, and test scaffolding.
- Humans remain **fully responsible** for:
  - Architectural decisions  
  - Correctness  
  - Reviewing all code  
  - Ensuring safety & maintainability  

### 2.2 Guardrails
To avoid "blind automation", the following guardrails are in place:

1. **DMN TCK**  
   - Over 3,500 industry-standard tests from the official DMN specification.  
   - Acts as objective validation.

2. **Unit Tests**  
   - C++ tests complement TCK.

3. **Static Analysis**  
   - Clang‑Tidy enforces C++ quality.

4. **Structured Prompts & Task Files**  
   - Located in `.github/prompts` and `.github/tasks`.
   - Each change begins with a task definition.

5. **Reproducible Builds**  
   - CMake + vcpkg + presets.

---

## 3. How AI Contributes

### 3.1 Code Generation
AI generates:
- Parsers  
- Evaluators  
- FEEL expression logic  
- Test harness code  
- Documentation and examples  

### 3.2 Design Aid
AI assists with:
- Architectural exploration  
- Trade‑off analyses  
- CMake improvements  
- Bug investigation proposals  

### 3.3 Documentation
Most `.md` files and examples are AI‑drafted, then human‑refined.

---

## 4. Development Workflow (Simplified)

```
1. Developer opens Issue
2. AI generates corresponding task file (.github/tasks/...md)
3. Developer reviews + adjusts task
4. AI generates code on feature branch and executes tests (unit and tck)
5. Developer reviews
6. Merge when green + correct
```

This keeps the workflow:
- Documented  
- Reproducible  
- Auditable  

---

## 5. Metrics

- **DMN Level 2 Compliance:** 100% (126/126 tests passing)
- **DMN Level 3 Coverage:** 13.7% (484/3,535 tests passing)
- **Unit tests:** 279 C++ test cases
- **Total test cases:** 3,814 (279 unit + 3,535 TCK)
- **Static analysis warnings:** 0 (enforced by clang-tidy)

---

## 6. Summary

Orion demonstrates that AI‑only development can produce a real, standards‑validated, high‑performance C++ library — when combined with strong guardrails and human oversight.
