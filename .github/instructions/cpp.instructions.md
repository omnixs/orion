# C++23 Project Instructions

These instructions apply only to C++ source files (`.cpp` and `.h`) in the project.  
They combine C++23-specific guidance with repository expectations relevant to C++ code.

---

## Language Standard
- Target **C++23** exclusively.
- Use modern C++23 language features and library facilities where appropriate.

---

## Modern C++ Practices
- Prefer **std::ranges** and algorithms over raw loops for selection, transformation, and aggregation.
- Use **std::expected** for recoverable errors.
- Use **std::span** for non-owning ranges.
- Prefer smart pointers (`std::unique_ptr`, `std::shared_ptr`) for ownership; avoid raw owning pointers.
- Use `constexpr`, `consteval`, and `constinit` where applicable.
- Use `[[nodiscard]]` on functions whose return values should not be ignored.
- Favor strong typing, structured bindings, and concepts to improve correctness and readability.
- Avoid legacy constructs: C-style arrays, `NULL`, and preprocessor macros (except include guards).

---

## Loop & Function Guidelines
- Keep functions **small and single-purpose** (~20–25 lines).
- Extract helper functions when a function grows too large.
- Raw loops are acceptable only for:
  - Index-aware logic
  - Performance-critical inner loops
  - State machines or complex early-exit logic
- Prefer algorithms and ranges for clarity and intent expression.

---

## Libraries & Headers
- Prefer **standard library facilities** over third-party libraries unless justified.
- Recommended headers:
  - `<algorithm>`, `<ranges>`, `<optional>`, `<expected>`, `<span>`, `<memory>`, `<string_view>`, `<variant>`
- Avoid storing `std::string_view` as a member unless the referenced lifetime is guaranteed.

---

## Error Handling
- Prefer `std::expected` for recoverable errors.
- Use exceptions only for truly exceptional or unrecoverable cases.

---

## Ownership, Lifetime & Thread Safety
- Clearly document lifetime expectations for API inputs and outputs.
- Avoid returning views to temporaries.
- Document thread-safety guarantees for public APIs; assume functions are not implicitly thread-safe.
- Prefer caller-provided synchronization unless otherwise stated.

---

## Static and Global State (library code)
- Every function-local `static`, namespace-scope variable, `static` data member and `thread_local` must be `constexpr` or `constinit`.
- Lookup tables: `constexpr std::array` (sorted at compile time) + `std::ranges::lower_bound`/`find`; never `std::map`/`unordered_map`/`set`/`vector`/`string`/`json` in statics.
- No allocating singletons, `std::call_once`, or `thread_local` caches; caches belong to an engine/`EvaluationContext` instance.
- Rationale and examples: `CODING_STANDARDS.md` section "Static and Global State". Verify with `tools/scripts/check_static_init.ps1`.

---

## Performance Guidance
- Avoid allocations on hot paths; use zero-copy parsing where possible.
- Reserve container capacity if the size is known in advance.
- Minimize unnecessary copies and conversions in performance-sensitive code.

---

## Testing & CI
- Add unit tests for behavioral changes.
- Run unit tests, TCK, `clang-tidy`, and formatting checks as part of PR workflow.
- Commits and PRs should include descriptive messages and reference design notes where applicable.

---

## References
- Follow the repository’s `CODING_STANDARDS.md`.
- Check build/test/TCK instructions in `.github/instructions/`.
- Use `clang-tidy` and CI checks to validate changes.
