# Contributing to ORION — Optimized Rule Integration & Operations Native

Thanks for your interest in contributing! By contributing, you agree that your
contributions are licensed under the Apache License, Version 2.0.

## Developer Certificate of Origin (DCO)

All commits must be signed off with the Developer Certificate of Origin (DCO 1.1). This certifies that you have the right to submit your contribution under the Apache-2.0 license. Full DCO text: https://developercertificate.org/

### Quick Start

**Automatic sign-off (recommended):**
```bash
cd /path/to/orion
git config --local format.signoff true
```

**Manual sign-off:**
```bash
git commit -s -m "Your commit message"
```

This adds `Signed-off-by: Your Name <your.email@example.com>` to your commit.

**Note**: PRs without DCO sign-offs will not be merged.

## Source file headers

- Every C++ source/header file must include the SPDX header:
  - `SPDX-License-Identifier: Apache-2.0`
  - A copyright line like: 2025 ORION contributors
- If you modify a file, keep a one-line notice in the header that the file was modified and refer to VCS history for details.
- Do **not** remove upstream file headers from third-party code; append a local
  addendum below them.

## Third-party code

- Include third-party license notices in `THIRD-PARTY-NOTICES.txt`.
- Only use dependencies with licenses compatible with Apache-2.0.

## Code style & CI

- Prefer modern C++ (C++20/23 where possible), RAII, and strong typing.
- Ensure unit tests pass locally before opening a PR.
- CI must be green for merge; add tests for new features or bug fixes.

## Security

See `SECURITY.md` for how to report vulnerabilities.
