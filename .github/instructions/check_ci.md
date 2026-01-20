# CI Feedback Loop: Poll + Download Logs

This repo’s fast iteration loop relies on getting CI results quickly after a push.

This guide standardizes how to:
- Find the GitHub Actions workflow run(s) for a specific commit SHA
- Wait/poll until completion
- Download the workflow **log archive ZIP**
- Extract logs into `.github/temp/` for local inspection

## Recommended (Automated)

Use the provided script:

```powershell
# From repo root
powershell -ExecutionPolicy Bypass -File tools/scripts/check_ci.ps1
```

This will:
- Detect `owner/repo` from `remote.origin.url`
- Use current `HEAD` SHA by default
- Poll all workflow runs that match `head_sha == HEAD`
- Download each run’s log archive and extract to `.github/temp/ci-logs/<sha>/...`

### Authentication

The script requires an API token via one of:
- `GH_TOKEN` (preferred)
- `GITHUB_TOKEN`

If neither is set, the script will attempt to use your GitHub CLI login (from `gh auth login -h github.com`).

The token must be able to read workflow runs and logs (Actions read permissions). For private repositories, the token must also have access to the repository.

Example:

```powershell
$env:GH_TOKEN = "<your token>"
powershell -ExecutionPolicy Bypass -File tools/scripts/check_ci.ps1
```

### Useful Options

```powershell
# Watch a specific SHA
powershell -ExecutionPolicy Bypass -File tools/scripts/check_ci.ps1 -Sha <commit-sha>

# Filter to a specific workflow run name substring
powershell -ExecutionPolicy Bypass -File tools/scripts/check_ci.ps1 -WorkflowName "CI - Fast Checks"

# Validate local detection (no API calls)
powershell -ExecutionPolicy Bypass -File tools/scripts/check_ci.ps1 -DryRun

# Increase timeout (seconds)
powershell -ExecutionPolicy Bypass -File tools/scripts/check_ci.ps1 -TimeoutSeconds 3600
```

## Manual (CLI)

If you prefer the GitHub CLI, you can watch workflow runs, but note that `gh run watch` has authentication limitations with some fine-grained tokens.

## Output Location

- Extracted logs: `.github/temp/ci-logs/<sha>/...`

This directory is treated as scratch space and should not be committed.
