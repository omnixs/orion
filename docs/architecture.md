# Orion Architecture Overview

## 1. Purpose
Orion is a native C++23 Decision Model and Notation (DMN) Level 1–2 rules engine designed for high‑performance backend environments.  
This document provides a concise overview of its architecture and major components.

---

## 2. High‑Level Architecture

### 2.1 Components

#### **1. Model Loader**
- Parses DMN XML into an in-memory representation.
- Validates structure (tables, inputs, outputs, rule cells).
- Performs basic semantic checks (types, rule shape, FEEL syntax).

#### **2. FEEL Subset Engine**
- Parses FEEL expressions used in:
  - Input entries
  - Output entries
  - Unary tests
- Supports a curated subset of FEEL functions.
- Produces a small, efficient AST for evaluation.

#### **3. Decision Table Evaluator**
- Core of the engine.
- Evaluates rows in order, applying hit-policy logic.
- Makes no assumptions about external systems — pure in-memory evaluation.

#### **4. Runtime Context**
- Stores input variables, intermediate values, output mappings.
- Designed for deterministic execution.
- Future extension point for thread-safety options.

#### **5. TCK Harness (dmn-tck)**
- Loads TCK test models and expected results.
- Runs them against the evaluator.
- Produces JSON logs for debugging and verification.

---

## 3. Data Flow

```
DMN XML
   ↓ (Model Loader)
Parsed Model
   ↓ (Validation)
Validated Model
   ↓ (Evaluation + FEEL)
Decision Result
```

Step-by-step:

1. **Load XML** using lightweight parsing.
2. **Construct model**: tables, inputs, outputs, rule rows.
3. **Validate** (shape, types, rule completeness).
4. **Evaluate** a decision using provided input variables.
5. **Return** structured output (variant-based C++ types).

---

## 4. Repository Structure

```
include/orion/       → Public API headers
src/                 → Engine implementation
tst/                 → Tests + TCK harness
dat/dmn-tck/         → DMN TCK test suite
docs/                → Documentation
tools/scripts/       → Utility tools and CLIs
```

### 4.1 Core Components (`src/bre/`)
- **`engine.hpp/cpp`** - Main `BusinessRulesEngine` with stateful model management
- **`dmn_parser.hpp/cpp`** - DMN XML parsing (rapidxml)
- **`feel_evaluator.hpp/cpp`** - FEEL expression evaluation
- **`dmn_model.hpp`** - Data structures: `DecisionTable`, `LiteralDecision`
- **`hit_policy.hpp/cpp`** - FIRST, UNIQUE, COLLECT with aggregations
- **`bkm_manager.hpp/cpp`** - Business Knowledge Model management

### 4.2Key Usage Pattern

```cpp
// ✅ CORRECT: Load once, evaluate many (9-45 μs/eval)
orion::bre::BusinessRulesEngine engine;
auto result = engine.load_dmn_model(dmn_xml);  // One-time: 100-200μs
if (!result) {
    std::cerr << "Error: " << result.error() << std::endl;
    return 1;
}

for (auto& request : requests) {
    nlohmann::json result = engine.evaluate(request);  // 9-45μs each
}

// ❌ WRONG: Re-load model every time (100-150 μs/eval - 10x slower!)
for (auto& request : requests) {
    orion::api::BusinessRulesEngine temp_engine;
    temp_engine.load_dmn_model(dmn_xml);
    nlohmann::json result = temp_engine.evaluate(request);  // Inefficient!
}
```

---

## 5. Build & Runtime

- **C++23**
- **Header + static library** design
- Build using **CMake** (with optional vcpkg)
- Engine is **pure compute**, no external dependencies at runtime

---

## 6. Error Handling & Logging

- Error messages surfaced via structured `orion::error` types.
- Evaluation errors produce clear diagnostics (invalid types, FEEL mismatch, rule failures).
- Logging available for debugging and TCK output (JSON format).

---

## 7. Planned Future Architecture Extensions
- Extended FEEL coverage
- Multi-decision model graph
- Thread-safe or thread-local evaluation modes
- More granular diagnostics for FEEL parsing

---

This document is intended as a short orientation for engineers, contributors, and reviewers.
