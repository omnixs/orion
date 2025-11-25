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

## Development Workflow

### Baseline Updates During Development

When implementing features that improve TCK test coverage, update the baseline for the **current version**:

```bash
# 1. Implement feature and verify tests pass
./build-release/orion_tck_runner --log_level=error

# 2. Generate updated baseline (current version directory)
./build-release/orion_tck_runner \
  --output-csv dat/tck-baselines/1.0.0/tck_results.csv \
  --output-properties dat/tck-baselines/1.0.0/tck_results.properties \
  --log_level=error

# 3. Review improvements
git diff dat/tck-baselines/1.0.0/tck_results.properties
# Should show:
# -level3_passed=350
# +level3_passed=375  (+25 tests!)

# 4. Commit updated baseline with feature
git add src/ dat/tck-baselines/1.0.0/tck_results.*
git commit -m "feat: Implement FEEL string functions - improves Level 3 coverage

Added support for:
- substring()
- string length()
- upper case()

TCK Impact:
- level3_passed: 350 → 375 (+25 tests)
- level3_pass_rate: 10.4% → 11.1%
- New passing tests: 1103-feel-substring-function (11/11)

Updated baseline to reflect improvements."
```

**Guidelines:**
- **Update existing baseline** for current version during development
- **Create new baseline directory** only during release (automated)
- Update baseline when tests **improve**, never to hide regressions
- CI regression detection will catch attempts to ignore failures

### CI Behavior

CI automatically adapts based on baseline presence:

**With baseline** (e.g., `dat/tck-baselines/1.0.0/` exists):
```yaml
# Strict mode - fails on regressions
--baseline dat/tck-baselines/1.0.0/tck_results.csv
--regression-check    # Exit code 2 if regressions detected
--level2-strict       # Exit code 3 if Level 2 fails (must be 100%)
```

**Without baseline** (development of new version):
```yaml
# Permissive mode - allows failures
--log_level=error
# Continue on error until baseline established
```

## Release Process

ORION uses an automated release workflow to ensure consistent, reproducible releases.

### Creating a Release

Releases are created via the automated GitHub Actions workflow:

1. **Navigate to Actions**: Go to [GitHub Actions → Release](../../actions/workflows/release.yml)
2. **Click "Run workflow"**
3. **Select version bump type**:
   - **`patch`** (X.Y.Z → X.Y.Z+1) - Bug fixes only, backwards compatible
   - **`minor`** (X.Y.Z → X.Y+1.0) - New features, backwards compatible
   - **`major`** (X.Y.Z → X+1.0.0) - Breaking changes
4. **Enter release notes**: Markdown supported, will appear in GitHub Release
5. **Click "Run workflow"**

### What Happens Automatically

The release workflow automatically:

1. **Version Management**:
   - Updates version in `CMakeLists.txt`
   - Commits version bump to `main` branch
   - Creates git tag (`vX.Y.Z`)
   - Pushes tag and commit to repository

2. **Build & Test**:
   - Builds for both Windows and Linux in Release mode
   - Runs full unit test suite (`tst_orion`)
   - Runs TCK compliance tests (`orion_tck_runner`)
   - Generates TCK baseline for the new version

3. **Packaging**:
   - Creates platform-specific archives (`.tar.gz` for Linux, `.zip` for Windows)
   - Includes libraries, headers, CMake config, executables, and TCK baseline
   - Generates SHA256 checksums for verification

4. **Release**:
   - Creates GitHub Release with tag `vX.Y.Z`
   - Uploads all platform archives and checksums
   - Publishes release notes

5. **Baseline Update**:
   - Commits TCK baseline to `dat/tck-baselines/{version}/`
   - Enables regression detection for future CI runs

### Release Artifacts

Each release includes the following for both platforms:

```
orion-X.Y.Z-{platform}/
├── lib/                      # Static libraries
│   ├── liborion_lib.a        # Linux
│   └── orion_lib.lib         # Windows
├── include/                  # Public headers
│   └── orion/
│       ├── api/
│       ├── bre/
│       └── common/
├── bin/                      # Executables
│   ├── orion_app             # CLI application
│   └── orion_tck_runner      # TCK test runner
├── cmake/                    # CMake integration
│   ├── orionConfig.cmake
│   └── orionConfigVersion.cmake
├── tck-baseline/             # TCK test results
│   ├── tck_results.csv
│   └── tck_results.properties
├── licenses/                 # License information
│   ├── LICENSE
│   ├── NOTICE
│   └── THIRD-PARTY-NOTICES.txt
└── README.md
```

### Semantic Versioning

ORION follows [Semantic Versioning 2.0.0](https://semver.org/):

- **Major version** (X.0.0): Incompatible API changes
  - Breaking changes to public API
  - Removal of deprecated features
  - Major architectural changes

- **Minor version** (X.Y.0): New functionality, backwards compatible
  - New features added
  - Enhancements to existing features
  - New FEEL/DMN capabilities

- **Patch version** (X.Y.Z): Bug fixes, backwards compatible
  - Bug fixes only
  - Security patches
  - Documentation updates

### Pre-release Checklist

Before triggering a release, ensure:

- [ ] All CI checks pass on `main` branch
- [ ] No regressions in TCK tests (regression detection active)
- [ ] Level 2 DMN compliance at 100%
- [ ] Documentation is up-to-date
- [ ] CHANGELOG.md updated (if present)
- [ ] Breaking changes documented in release notes
- [ ] Migration guide prepared for major versions

### Post-release

After a successful release:

1. **Verify release**: Check that GitHub Release was created with all artifacts
2. **Test downloads**: Download and verify checksums match
3. **Update dependencies**: If ORION is consumed by other projects, update version references
4. **Announce**: Share release announcement in relevant channels

### Troubleshooting

**Workflow fails during build:**
- Check CI logs for compilation errors
- Verify all tests pass locally in Release mode
- Ensure vcpkg dependencies are available

**Version bump incorrect:**
- Verify `CMakeLists.txt` had correct starting version
- Check workflow input selection (patch/minor/major)

**Baseline not committed:**
- Ensure TCK runner generated baseline successfully
- Check workflow permissions (`contents: write`)

## Security

See `SECURITY.md` for how to report vulnerabilities.
