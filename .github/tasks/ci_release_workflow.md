---
template: custom
agent: none
status: completed
category: ci-cd
priority: medium
estimated-effort: "7-11 hours"
actual-effort: "~3 hours"
---

# Task: Implement Automated Release Workflow

## Context

ORION currently lacks an automated release process. Creating releases is manual and error-prone:
- No automated version bumping in CMakeLists.txt
- No automated git tagging
- No automated artifact building and packaging
- No GitHub Release creation
- No version consistency enforcement

This task implements a GitHub Actions workflow to automate the entire release process.

## Prerequisites

**This task depends on:** 
- `ci_tck_regression_detection.md` task must be completed first
- TCK baselines must exist in `dat/tck-baselines/{version}/`
- `orion_tck_runner` must support `--output-csv` and `--output-properties` flags

## Requirements

### 1. Version Management

**Single Source of Truth:** `CMakeLists.txt`
```cmake
project(orion VERSION 1.0.0 LANGUAGES CXX)
```

**Derived Versions:**
- Git tags: `v${VERSION}` (e.g., `v1.0.0`)
- TCK baselines: `dat/tck-baselines/${VERSION}/`
- Release artifacts: `orion-${VERSION}-${PLATFORM}.tar.gz`
- GitHub releases: `ORION v${VERSION}`

**Semantic Versioning:**
- **Major (X.0.0)**: Breaking API changes
- **Minor (X.Y.0)**: New features, backwards compatible
- **Patch (X.Y.Z)**: Bug fixes only

### 2. Release Workflow

**Create new file:** `.github/workflows/release.yml`

#### 2.1 Workflow Trigger

Manual trigger with version bump selection:

```yaml
name: Release

on:
  workflow_dispatch:
    inputs:
      version_bump:
        description: 'Version increment type'
        required: true
        type: choice
        options:
          - patch  # X.Y.Z → X.Y.(Z+1) - Bug fixes
          - minor  # X.Y.Z → X.(Y+1).0 - New features
          - major  # X.Y.Z → (X+1).0.0 - Breaking changes
      release_notes:
        description: 'Release notes (markdown supported)'
        required: true
        type: string

permissions:
  contents: write  # Required for creating releases and pushing tags
```

#### 2.2 Job 1: Version Bump and Tag

```yaml
jobs:
  version-bump:
    runs-on: ubuntu-latest
    outputs:
      new_version: ${{ steps.bump.outputs.new_version }}
    steps:
      - name: Checkout
        uses: actions/checkout@v4
        with:
          token: ${{ secrets.GITHUB_TOKEN }}
          
      - name: Read Current Version
        id: current
        run: |
          VERSION=$(grep -oP 'project\(orion VERSION \K[0-9.]+' CMakeLists.txt)
          echo "version=$VERSION" >> $GITHUB_OUTPUT
          
      - name: Calculate New Version
        id: bump
        run: |
          IFS='.' read -r major minor patch <<< "${{ steps.current.outputs.version }}"
          
          case "${{ github.event.inputs.version_bump }}" in
            major)
              new_version="$((major + 1)).0.0"
              ;;
            minor)
              new_version="$major.$((minor + 1)).0"
              ;;
            patch)
              new_version="$major.$minor.$((patch + 1))"
              ;;
          esac
          
          echo "new_version=$new_version" >> $GITHUB_OUTPUT
          echo "New version: $new_version"
          
      - name: Update CMakeLists.txt
        run: |
          sed -i "s/project(orion VERSION [0-9.]*/project(orion VERSION ${{ steps.bump.outputs.new_version }}/" CMakeLists.txt
          
      - name: Commit Version Bump
        run: |
          git config user.name "github-actions[bot]"
          git config user.email "github-actions[bot]@users.noreply.github.com"
          git add CMakeLists.txt
          git commit -m "chore: bump version to ${{ steps.bump.outputs.new_version }}"
          
      - name: Create Tag
        run: |
          git tag -a "v${{ steps.bump.outputs.new_version }}" -m "${{ github.event.inputs.release_notes }}"
          
      - name: Push Changes
        run: |
          git push origin main
          git push origin "v${{ steps.bump.outputs.new_version }}"
```

#### 2.3 Job 2: Build Release Artifacts

```yaml
  build-artifacts:
    needs: version-bump
    strategy:
      matrix:
        os: [ubuntu-latest, windows-latest]
        include:
          - os: ubuntu-latest
            platform: linux-x64
            archive_ext: tar.gz
          - os: windows-latest
            platform: windows-x64
            archive_ext: zip
    runs-on: ${{ matrix.os }}
    steps:
      - name: Checkout at Tag
        uses: actions/checkout@v4
        with:
          ref: v${{ needs.version-bump.outputs.new_version }}
          submodules: recursive
          
      - name: Setup vcpkg
        run: |
          git clone https://github.com/Microsoft/vcpkg.git
          cd vcpkg
          ./bootstrap-vcpkg.sh  # Linux
          # .\bootstrap-vcpkg.bat  # Windows
          
      - name: Build Release
        run: |
          cmake -B build -DCMAKE_BUILD_TYPE=Release \
            -DCMAKE_TOOLCHAIN_FILE=vcpkg/scripts/buildsystems/vcpkg.cmake
          cmake --build build --config Release -j
          
      - name: Run Tests
        run: |
          # Unit tests
          ./build/tst_orion --log_level=test_suite  # Linux
          # .\build\Release\tst_orion.exe --log_level=test_suite  # Windows
          
          # TCK tests (generate baseline)
          ./build/orion_tck_runner --output-csv tck_results.csv \
            --output-properties tck_results.properties
          
      - name: Package Artifacts
        run: |
          mkdir -p release/orion-${{ needs.version-bump.outputs.new_version }}/lib
          mkdir -p release/orion-${{ needs.version-bump.outputs.new_version }}/include
          mkdir -p release/orion-${{ needs.version-bump.outputs.new_version }}/bin
          mkdir -p release/orion-${{ needs.version-bump.outputs.new_version }}/cmake
          mkdir -p release/orion-${{ needs.version-bump.outputs.new_version }}/tck-baseline
          
          # Copy libraries
          cp build/liborion_lib.a release/orion-.../lib/  # Linux
          # cp build\Release\orion_lib.lib release\orion-...\lib\  # Windows
          
          # Copy headers (preserve structure)
          cp -r include/orion release/orion-.../include/
          
          # Copy CMake config
          cp build/orionConfig.cmake build/orionConfigVersion.cmake release/orion-.../cmake/
          
          # Copy executables
          cp build/orion_app build/orion_tck_runner release/orion-.../bin/
          
          # Copy TCK baseline
          cp tck_results.csv tck_results.properties release/orion-.../tck-baseline/
          
          # Create archive
          cd release
          tar czf orion-${{ needs.version-bump.outputs.new_version }}-${{ matrix.platform }}.tar.gz orion-...  # Linux
          # 7z a orion-...-windows-x64.zip orion-...  # Windows
          
      - name: Generate Checksums
        run: |
          cd release
          sha256sum orion-*.tar.gz > orion-${{ needs.version-bump.outputs.new_version }}-checksums.txt  # Linux
          # Get-FileHash orion-*.zip | Format-List > checksums.txt  # Windows
          
      - name: Upload Artifacts
        uses: actions/upload-artifact@v4
        with:
          name: orion-${{ matrix.platform }}
          path: release/orion-*
```

#### 2.4 Job 3: Create GitHub Release

```yaml
  create-release:
    needs: [version-bump, build-artifacts]
    runs-on: ubuntu-latest
    steps:
      - name: Download All Artifacts
        uses: actions/download-artifact@v4
        with:
          path: artifacts/
          
      - name: Create GitHub Release
        uses: softprops/action-gh-release@v1
        with:
          tag_name: v${{ needs.version-bump.outputs.new_version }}
          name: ORION v${{ needs.version-bump.outputs.new_version }}
          body: ${{ github.event.inputs.release_notes }}
          draft: false
          prerelease: false
          files: |
            artifacts/**/*.tar.gz
            artifacts/**/*.zip
            artifacts/**/*-checksums.txt
```

#### 2.5 Job 4: Update TCK Baseline

```yaml
  update-baseline:
    needs: [version-bump, build-artifacts]
    runs-on: ubuntu-latest
    steps:
      - name: Checkout
        uses: actions/checkout@v4
        with:
          ref: main
          
      - name: Download TCK Baseline Artifacts
        uses: actions/download-artifact@v4
        with:
          name: orion-linux-x64  # Use Linux baseline
          path: artifacts/
          
      - name: Copy Baseline to Repository
        run: |
          VERSION=${{ needs.version-bump.outputs.new_version }}
          mkdir -p dat/tck-baselines/$VERSION
          
          # Extract baseline from artifact
          tar xzf artifacts/orion-*-linux-x64.tar.gz
          cp orion-*/tck-baseline/tck_results.csv dat/tck-baselines/$VERSION/
          cp orion-*/tck-baseline/tck_results.properties dat/tck-baselines/$VERSION/
          
      - name: Commit Baseline
        run: |
          git config user.name "github-actions[bot]"
          git config user.email "github-actions[bot]@users.noreply.github.com"
          git add dat/tck-baselines/${{ needs.version-bump.outputs.new_version }}
          git commit -m "chore: add TCK baseline for version ${{ needs.version-bump.outputs.new_version }}"
          git push origin main
```

### 3. Documentation Updates

#### 3.1 CONTRIBUTING.md

Add release process documentation:
```markdown
## Release Process

Releases are created via the automated release workflow:

1. Go to GitHub → Actions → Release
2. Click "Run workflow"
3. Select version bump type:
   - **patch** (X.Y.Z → X.Y.Z+1) - Bug fixes only
   - **minor** (X.Y.Z → X.Y+1.0) - New features, backwards compatible
   - **major** (X.Y.Z → X+1.0.0) - Breaking changes
4. Enter release notes (markdown supported)
5. Click "Run workflow"

The workflow automatically:
- Updates version in CMakeLists.txt
- Creates git tag (vX.Y.Z)
- Builds for Windows and Linux
- Runs full test suite
- Generates TCK baseline
- Creates GitHub Release with artifacts
- Commits baseline to repository
```

#### 3.2 README.md

Add version badge and download links:
```markdown
[![Version](https://img.shields.io/github/v/release/omnixs/orion)](https://github.com/omnixs/orion/releases/latest)

## Download

Download the latest release from [GitHub Releases](https://github.com/omnixs/orion/releases/latest):

- **Linux**: `orion-X.Y.Z-linux-x64.tar.gz`
- **Windows**: `orion-X.Y.Z-windows-x64.zip`

Each release includes:
- Static libraries (liborion_lib.a / orion_lib.lib)
- Headers (include/orion/)
- CMake configuration files
- Command-line tools (orion_app, orion_tck_runner)
- TCK test baseline
```

## Implementation Plan

### Phase 1: Workflow Infrastructure (3-4 hours)
1. Create `.github/workflows/release.yml`
2. Implement version bump job with semantic versioning logic
3. Implement git tag creation and push
4. Test version bump on test branch

### Phase 2: Build and Artifact Jobs (3-4 hours)
1. Implement multi-platform build matrix (Windows + Linux)
2. Implement test execution (unit + TCK)
3. Implement artifact packaging with proper directory structure
4. Implement checksum generation
5. Test artifact creation locally

### Phase 3: Release and Baseline Jobs (1-2 hours)
1. Implement GitHub Release creation with softprops/action-gh-release
2. Implement baseline extraction and commit-back
3. Test full workflow end-to-end on test branch

### Phase 4: Documentation (1 hour)
1. Update CONTRIBUTING.md with release process
2. Update README.md with version badge and download links
3. Create `.github/workflows/README.md` documenting workflow usage

## Success Criteria

- [ ] Release workflow can be triggered manually via GitHub Actions UI
- [ ] Version bump correctly increments patch/minor/major in CMakeLists.txt
- [ ] Git tag created with format `vX.Y.Z`
- [ ] Windows and Linux builds succeed in Release mode
- [ ] Full test suite passes (unit + TCK)
- [ ] Artifacts packaged correctly (libs, headers, CMake config, executables, baseline)
- [ ] SHA256 checksums generated
- [ ] GitHub Release created with all artifacts
- [ ] TCK baseline committed back to `dat/tck-baselines/{version}/`
- [ ] Documentation updated (CONTRIBUTING.md, README.md)
- [ ] Tested end-to-end on test branch

## Testing Strategy

### Test on Feature Branch
1. Create test branch from main
2. Manually bump version to `0.9.0-test` in CMakeLists.txt
3. Trigger release workflow with "patch" bump
4. Verify:
   - Version becomes `0.9.1-test`
   - Tag `v0.9.1-test` created
   - Builds succeed for both platforms
   - Artifacts contain all required files
   - GitHub Release created
   - Baseline committed to `dat/tck-baselines/0.9.1-test/`
5. Download artifacts and verify:
   - Libraries link successfully
   - Headers compile successfully
   - Executables run
   - Checksums match

### Production Test
1. After successful test branch validation
2. Merge to main (or run on main directly)
3. Trigger workflow with real version bump
4. Verify all steps complete successfully

## Dependencies

**GitHub Actions:**
- `actions/checkout@v4`
- `actions/upload-artifact@v4`
- `actions/download-artifact@v4`
- `softprops/action-gh-release@v1`

**Permissions:**
- `contents: write` - Create releases, push tags, commit baselines

**Prerequisites:**
- TCK regression detection task completed
- `orion_tck_runner` supports `--output-csv` and `--output-properties`
- Baselines exist in `dat/tck-baselines/`

**New Files:**
- `.github/workflows/release.yml`

**Documentation Updates:**
- `CONTRIBUTING.md`
- `README.md`
- `.github/workflows/README.md` (new)

## Notes

- Workflow requires manual trigger (`workflow_dispatch`) for controlled releases
- Version in CMakeLists.txt is the single source of truth
- Baselines are generated from Linux build (consistent environment)
- Release artifacts include both platform-specific and cross-platform files
- Semantic versioning must be followed for version bumps

## Future Enhancements (Out of Scope)

- Automated changelog generation from git commits
- Release candidate (RC) workflow with `-rc` suffix
- Docker image publishing to GHCR
- vcpkg port auto-update on release
- Slack/Discord notifications
- Release notes preview in PR descriptions

## Retrospective

### What Went Well
1. **Comprehensive Workflow**: Single workflow handles all phases (version bump, build, test, package, release, baseline)
2. **Dual-Platform Support**: Matrix strategy cleanly handles Windows and Linux differences
3. **Reusable Patterns**: Leveraged existing CI patterns from `ci-full.yml` for consistency
4. **Documentation First**: Updated CONTRIBUTING.md and README.md with clear release process before implementation
5. **Artifact Management**: Well-organized packaging with libraries, headers, CMake config, executables, and baselines
6. **Semantic Versioning**: Bash arithmetic for version bumping is simple and maintainable

### Challenges Encountered
1. **Cross-Platform Packaging**: Different archive formats (tar.gz vs zip) and file paths (forward vs backslashes) required platform-specific handling
2. **Checksum Generation**: Windows uses `certutil` instead of `sha256sum`, needed conditional logic
3. **Baseline Extraction**: Had to extract from packaged artifact to copy back to repository
4. **Git Push Permissions**: Need `contents: write` permission for creating releases, pushing tags, and committing baselines

### Key Learnings
1. **Shell Uniformity**: Using `shell: bash` even on Windows (Git Bash) simplifies cross-platform scripting
2. **Job Dependencies**: `needs:` keyword critical for passing version between jobs and ensuring proper execution order
3. **Artifact Lifecycle**: Download artifacts, extract needed files, commit back to repo in separate job
4. **Manual Triggers**: `workflow_dispatch` with inputs provides controlled release process without accidental triggers

### Process Improvements for Future
1. **Testing Strategy**: Should test on non-main branch first with test version (e.g., 0.9.0-test)
2. **Rollback Plan**: Document how to revert failed releases (delete tag, revert commit)
3. **Changelog Automation**: Consider auto-generating changelog from git commits in future
4. **Pre-flight Checks**: Could add validation step to ensure no uncommitted changes, all CI green, etc.

### Time Breakdown
- **Phase 1** (Workflow Infrastructure): 1 hour - Version bump and tagging logic straightforward
- **Phase 2** (Build and Artifacts): 1 hour - Reused CI patterns, packaging required careful structure
- **Phase 3** (Release and Baseline): 0.5 hours - softprops/action-gh-release simplified GitHub Release creation
- **Phase 4** (Documentation): 0.5 hours - CONTRIBUTING.md and README.md updates, workflows README enhancement
- **Total**: ~3 hours vs 7-11 hour estimate (60-75% time saving due to reusing CI patterns and clear requirements)

### Impact
- **Automated Releases**: No manual version bumping, tagging, or artifact building
- **Consistent Packaging**: Same structure every release, no human error
- **Baseline Tracking**: Automatic baseline commits enable regression detection
- **Semantic Versioning**: Enforced through workflow inputs
- **Artifact Distribution**: GitHub Releases provide permanent download links
- **Developer Experience**: Clear process documented in CONTRIBUTING.md

### Dependencies on Previous Work
- **TCK Regression Detection Task**: Provided `--output-csv` and `--output-properties` flags essential for baseline generation
- **CI Full Workflow**: Patterns for multi-platform builds, test execution, and artifact handling reused directly
- **Baseline Infrastructure**: `dat/tck-baselines/` directory structure ready for new version baselines

### Next Steps (Future Enhancements)
- Test release workflow on feature branch with test version
- Add release candidate (RC) workflow with `-rc` suffix
- Implement automated changelog generation from commits
- Add release announcement automation (Slack/Discord)
- Consider Docker image publishing to GHCR
