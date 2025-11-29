# GitHub Actions Workflows

## Active Workflows

### `ci-fast.yml` - Fast PR Checks
**Purpose**: Quick feedback for development iterations  
**Triggers**: Every push, every PR  
**Platforms**: Windows + Linux  
**Configuration**: Debug build  
**Tests**: Unit tests only  
**Duration**: ~3-5 minutes  

**What it does:**
- Builds ORION in Debug mode
- Runs all unit tests (Boost.Test suite)
- Validates basic functionality across platforms
- Provides fast feedback for development

---

### `ci-full.yml` - Full Validation
**Purpose**: Comprehensive validation before releases  
**Triggers**: 
- Push to `main` branch
- Git tags matching `v*` (releases)
- Manual trigger (`workflow_dispatch`)
- Nightly schedule (3 AM UTC)

**Platforms**: Windows + Linux  
**Configuration**: Release build  
**Tests**: Unit tests + Full TCK compliance suite  
**Duration**: ~15-25 minutes  

**What it does:**
- Builds ORION in Release mode
- Runs all unit tests
- Runs complete DMN TCK compliance test suite (~1887 tests)
- **Regression detection**: Compares results against baseline if exists
- **Level 2 strict mode**: Enforces 100% Level 2 DMN compliance
- Uploads build artifacts and TCK results
- Validates production-ready configuration

**Regression Detection:**
- If baseline exists for current version → enforces no regressions (exit codes 2/3 fail build)
- If no baseline exists → allows failures (development mode)

**Exit code handling:**
- **0**: All tests passed or no regressions
- **1**: Expected failures (allowed)
- **2**: Regression detected → **FAILS BUILD**
- **3**: Level 2 compliance failure → **FAILS BUILD**

---

### `release.yml` - Automated Release
**Purpose**: Automated release creation with artifacts and baselines  
**Triggers**: Manual only (`workflow_dispatch`)  
**Platforms**: Windows + Linux  
**Configuration**: Release build  
**Duration**: ~20-30 minutes  

**Inputs:**
- `version_bump`: Choose `patch`, `minor`, or `major` for semantic versioning
- `release_notes`: Markdown-formatted release description

**What it does:**

**Phase 1 - Version Management:**
- Reads current version from `CMakeLists.txt`
- Calculates new version (e.g., 1.0.0 → 1.0.1 for patch)
- Updates `CMakeLists.txt` with new version
- Creates git tag (`vX.Y.Z`)
- Commits and pushes to `main` branch

**Phase 2 - Build Artifacts:**
- Builds for Windows and Linux in Release mode
- Runs full test suite (unit + TCK)
- Generates TCK baseline for new version
- Packages artifacts with:
  - Static libraries (`liborion_lib.a` / `orion_lib.lib`)
  - Headers (`include/orion/`)
  - CMake configuration files
  - Executables (`orion_app`, `orion_tck_runner`)
  - TCK baseline
  - License files
- Generates SHA256 checksums

**Phase 3 - GitHub Release:**
- Creates GitHub Release with tag `vX.Y.Z`
- Uploads platform archives (`.tar.gz` for Linux, `.zip` for Windows)
- Uploads checksums
- Publishes release notes

**Phase 4 - Baseline Update:**
- Extracts TCK baseline from Linux build
- Commits to `dat/tck-baselines/{version}/`
- Enables regression detection for future CI runs

**Outputs:**
- GitHub Release with downloadable artifacts
- Version bump commit on `main`
- Git tag `vX.Y.Z`
- TCK baseline in repository

---

## Optional Workflows (Manual Trigger)

These workflows are **disabled by default**. Run them manually or enable on PRs by uncommenting triggers.

### `quality-clang-tidy.yml` - Static Analysis
**Purpose**: Code quality enforcement via clang-tidy  
**Triggers**: Manual only (workflow_dispatch)  
**Platform**: Linux only (MSVC compile_commands.json incompatible)  
**Duration**: ~5-10 minutes  

**What it does:**
- Runs clang-tidy static analysis on all source files
- Checks naming conventions (CamelCase, snake_case)
- Validates const-correctness, modernization, readability
- Enforces CODING_STANDARDS.md rules

**Enable on PRs**: Uncomment `pull_request` trigger in the file

**Known Issue**: Clang-tidy can be slow on some machines. Use manual trigger to avoid blocking PRs.

---

### `quality-format.yml` - Format Check
**Purpose**: Consistent code formatting  
**Triggers**: Manual only (workflow_dispatch)  
**Platform**: Linux (fast, no build needed)  
**Duration**: ~30 seconds  

**What it does:**
- Checks code formatting against `.clang-format` rules
- Validates consistent style across codebase
- Reports formatting violations

**Enable on PRs**: Uncomment `pull_request` trigger in the file

**Setup Required**: Add `.clang-format` configuration file:
```bash
clang-format -style=llvm -dump-config > .clang-format
# Then customize the settings
```

---

## CI/CD Strategy

**Development Workflow:**
1. **Work on feature** → Push commits
2. **`ci-fast.yml` runs** → Get feedback in 3-5 min
3. **Fix issues, iterate** → Fast feedback loop
4. **Merge to main** → `ci-full.yml` validates comprehensively
5. **Tag release** → `ci-full.yml` creates artifacts

**Quality Checks (Optional):**
- Run `quality-clang-tidy.yml` manually before important merges
- Run `quality-format.yml` to check formatting
- Enable on PRs when ready for stricter enforcement

---

## GitHub Actions Limitations

**Free Tier (Private Repos):**
- 2,000 minutes/month (Windows uses 2x multiplier)
- ~10 full CI runs per month before limit

**Free Tier (Public Repos):**
- **Unlimited minutes** ✅
- No cost concerns once repository is public

**Current Usage Estimate:**
- Fast check: ~5 min × 2 platforms × 2 multiplier (Windows) = ~15 minutes per run
- Full validation: ~20 min × 2 platforms × 2 multiplier = ~80 minutes per run
- With caching: Reduce by ~50% after first run

---

## Customization

### Change Triggers
Edit the `on:` section in each workflow file.

### Add/Remove Platforms
Modify the `matrix.os` array in job definitions.

### Adjust Build Settings
Change CMake configuration flags in the configure steps.

### Enable Optional Workflows
Uncomment the `pull_request:` trigger in quality workflows.

---

## Troubleshooting

**vcpkg dependencies fail:**
- Check `vcpkg.json` is committed
- Verify vcpkg commit ID in workflow is current

**Tests timeout:**
- Increase `timeout-minutes` in test steps
- TCK tests can take 10-15 minutes on slower runners

**Build fails on Windows:**
- Ensure Visual Studio 2022 components are available
- Check triplet is `x64-windows-static`

**Build fails on Linux:**
- Verify GCC 13 is used (required for C++23 std::expected)
- Check triplet is `x64-linux`

---

## Manual Workflow Execution

**Via GitHub UI:**
1. Go to Actions tab
2. Select workflow from left sidebar
3. Click "Run workflow" button
4. Choose branch and run

**Via GitHub CLI:**
```bash
# Trigger fast checks
gh workflow run ci-fast.yml

# Trigger full validation
gh workflow run ci-full.yml

# Trigger clang-tidy
gh workflow run quality-clang-tidy.yml

# Trigger format check
gh workflow run quality-format.yml
```

---

*Last updated: November 24, 2025*
