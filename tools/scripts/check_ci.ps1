[CmdletBinding()]
param(
    [Parameter(Mandatory = $false)]
    [string]$Owner,

    [Parameter(Mandatory = $false)]
    [string]$Repo,

    [Parameter(Mandatory = $false)]
    [string]$Sha,

    [Parameter(Mandatory = $false)]
    [string]$WorkflowName,

    [Parameter(Mandatory = $false)]
    [switch]$DryRun,

    [Parameter(Mandatory = $false)]
    [int]$PollSeconds = 10,

    [Parameter(Mandatory = $false)]
    [int]$TimeoutSeconds = 1800,

    [Parameter(Mandatory = $false)]
    [string]$OutDir,

    [Parameter(Mandatory = $false)]
    [string]$ApiBase = "https://api.github.com",

    [Parameter(Mandatory = $false)]
    [string]$Token
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Get-RepoRoot {
    $root_candidate = Resolve-Path (Join-Path $PSScriptRoot "..\..")
    return $root_candidate.Path
}

function Invoke-Git {
    param(
        [Parameter(Mandatory = $true)][string[]]$Args
    )

    $git = Get-Command git -ErrorAction SilentlyContinue
    if ($null -eq $git) {
        throw "git was not found on PATH."
    }

    $output = & git @Args
    if ($LASTEXITCODE -ne 0) {
        throw "git command failed: git $($Args -join ' ')"
    }

    return ($output | Out-String).Trim()
}

function Try-ParseOwnerRepoFromRemoteUrl {
    param(
        [Parameter(Mandatory = $true)][string]$RemoteUrl
    )

    $url = $RemoteUrl.Trim()

    # https://github.com/owner/repo.git
    $https_match = [regex]::Match($url, "^https?://[^/]+/(?<owner>[^/]+)/(?<repo>[^/]+?)(?:\.git)?/?$")
    if ($https_match.Success) {
        return @($https_match.Groups["owner"].Value, $https_match.Groups["repo"].Value)
    }

    # git@github.com:owner/repo.git
    $ssh_match = [regex]::Match($url, "^[^@]+@[^:]+:(?<owner>[^/]+)/(?<repo>[^/]+?)(?:\.git)?/?$")
    if ($ssh_match.Success) {
        return @($ssh_match.Groups["owner"].Value, $ssh_match.Groups["repo"].Value)
    }

    # ssh://git@github.com/owner/repo.git
    $ssh_url_match = [regex]::Match($url, "^ssh://[^/]+/(?<owner>[^/]+)/(?<repo>[^/]+?)(?:\.git)?/?$")
    if ($ssh_url_match.Success) {
        return @($ssh_url_match.Groups["owner"].Value, $ssh_url_match.Groups["repo"].Value)
    }

    return $null
}

function Get-GitHubToken {
    if (-not [string]::IsNullOrWhiteSpace($Token)) {
        return $Token
    }

    if (-not [string]::IsNullOrWhiteSpace($env:GH_TOKEN)) {
        return $env:GH_TOKEN
    }

    if (-not [string]::IsNullOrWhiteSpace($env:GITHUB_TOKEN)) {
        return $env:GITHUB_TOKEN
    }

    $gh = Get-Command gh -ErrorAction SilentlyContinue
    if ($null -ne $gh) {
        try {
            $cli_token = (& gh auth token -h github.com 2>$null | Out-String).Trim()
            if (-not [string]::IsNullOrWhiteSpace($cli_token)) {
                return $cli_token
            }
        } catch {
            # ignore and fall through
        }
    }

    throw "No token available. Set GH_TOKEN (preferred) or GITHUB_TOKEN, pass -Token, or run 'gh auth login -h github.com'."
}

function New-GitHubHeaders {
    param(
        [Parameter(Mandatory = $true)][string]$AuthToken
    )

    return @{
        "Authorization" = "Bearer $AuthToken"
        "Accept" = "application/vnd.github+json"
        "X-GitHub-Api-Version" = "2022-11-28"
        "User-Agent" = "orion-ci-feedback-loop"
    }
}

function Invoke-GitHubApiJson {
    param(
        [Parameter(Mandatory = $true)][string]$Method,
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $false)][hashtable]$Query
    )

    $auth_token = Get-GitHubToken
    $headers = New-GitHubHeaders -AuthToken $auth_token

    $uri_builder = New-Object System.UriBuilder ("$ApiBase$Path")
    if ($null -ne $Query) {
        $query_parts = New-Object System.Collections.Generic.List[string]
        foreach ($key in $Query.Keys) {
            $encoded_key = [System.Uri]::EscapeDataString([string]$key)
            $encoded_val = [System.Uri]::EscapeDataString([string]$Query[$key])
            $query_parts.Add("$encoded_key=$encoded_val")
        }
        $uri_builder.Query = ($query_parts -join "&")
    }

    return Invoke-RestMethod -Method $Method -Uri $uri_builder.Uri.AbsoluteUri -Headers $headers
}

function Get-RedirectLocation {
    param(
        [Parameter(Mandatory = $true)][string]$AbsoluteUri,
        [Parameter(Mandatory = $true)][hashtable]$Headers
    )

    $request = [System.Net.HttpWebRequest]::Create($AbsoluteUri)
    $request.Method = "GET"
    $request.AllowAutoRedirect = $false

    foreach ($key in $Headers.Keys) {
        if ($key -ieq "User-Agent") {
            $request.UserAgent = [string]$Headers[$key]
            continue
        }

        if ($key -ieq "Accept") {
            $request.Accept = [string]$Headers[$key]
            continue
        }

        if ($key -ieq "Authorization") {
            $request.Headers["Authorization"] = [string]$Headers[$key]
            continue
        }

        $request.Headers[$key] = [string]$Headers[$key]
    }

    try {
        $response = $request.GetResponse()
        try {
            $location = $response.Headers["Location"]
            return $location
        } finally {
            $response.Close()
        }
    } catch [System.Net.WebException] {
        $web_response = $_.Exception.Response
        if ($null -ne $web_response) {
            try {
                $location = $web_response.Headers["Location"]
                return $location
            } finally {
                $web_response.Close()
            }
        }
        throw
    }
}

function Download-RunLogsZip {
    param(
        [Parameter(Mandatory = $true)][string]$Owner,
        [Parameter(Mandatory = $true)][string]$Repo,
        [Parameter(Mandatory = $true)][long]$RunId,
        [Parameter(Mandatory = $true)][string]$ZipPath
    )

    $auth_token = Get-GitHubToken
    $headers = New-GitHubHeaders -AuthToken $auth_token

    $logs_uri = "$ApiBase/repos/$Owner/$Repo/actions/runs/$RunId/logs"
    $redirect = Get-RedirectLocation -AbsoluteUri $logs_uri -Headers $headers

    if ([string]::IsNullOrWhiteSpace($redirect)) {
        throw "Did not receive a redirect Location when requesting logs for run_id=$RunId."
    }

    Invoke-WebRequest -Uri $redirect -OutFile $ZipPath
}

$repo_root = Get-RepoRoot

if ([string]::IsNullOrWhiteSpace($OutDir)) {
    $OutDir = Join-Path $repo_root ".github\temp\ci-logs"
}

if ([string]::IsNullOrWhiteSpace($Sha)) {
    $Sha = Invoke-Git -Args @("rev-parse", "HEAD")
}

if ([string]::IsNullOrWhiteSpace($Owner) -or [string]::IsNullOrWhiteSpace($Repo)) {
    $remote_url = Invoke-Git -Args @("config", "--get", "remote.origin.url")
    $parsed = Try-ParseOwnerRepoFromRemoteUrl -RemoteUrl $remote_url
    if ($null -eq $parsed) {
        throw "Could not parse owner/repo from remote.origin.url: $remote_url"
    }

    if ([string]::IsNullOrWhiteSpace($Owner)) {
        $Owner = $parsed[0]
    }

    if ([string]::IsNullOrWhiteSpace($Repo)) {
        $Repo = $parsed[1]
    }
}

Write-Host "Owner/Repo: $Owner/$Repo"
Write-Host "SHA:        $Sha"

if ($DryRun) {
    Write-Host "DryRun: would query workflow runs for this SHA, poll for completion, download logs ZIP, and extract under: $OutDir"
    exit 0
}

$sha_dir = Join-Path $OutDir $Sha
New-Item -ItemType Directory -Path $sha_dir -Force | Out-Null

$runs = Invoke-GitHubApiJson -Method "GET" -Path "/repos/$Owner/$Repo/actions/runs" -Query @{
    head_sha = $Sha
    per_page = 50
}

if ($null -eq $runs.workflow_runs -or $runs.workflow_runs.Count -eq 0) {
    Write-Host "No workflow runs found for head_sha=$Sha"
    exit 2
}

$workflow_runs = @($runs.workflow_runs)
if (-not [string]::IsNullOrWhiteSpace($WorkflowName)) {
    $filtered = @()
    foreach ($run in $workflow_runs) {
        if ($run.name -like "*$WorkflowName*") {
            $filtered += $run
        }
    }
    $workflow_runs = $filtered
}

if ($workflow_runs.Count -eq 0) {
    Write-Host "No workflow runs matched filter WorkflowName='$WorkflowName'"
    exit 2
}

$deadline = (Get-Date).AddSeconds($TimeoutSeconds)

foreach ($run in $workflow_runs) {
    $run_id = [long]$run.id
    $run_name = [string]$run.name

    Write-Host ""
    Write-Host "Run: $run_name ($run_id)"
    Write-Host "URL: $($run.html_url)"

    $current = $run
    while ($current.status -ne "completed") {
        if ((Get-Date) -gt $deadline) {
            throw "Timeout waiting for run_id=$run_id to complete."
        }

        Start-Sleep -Seconds $PollSeconds
        $current = Invoke-GitHubApiJson -Method "GET" -Path "/repos/$Owner/$Repo/actions/runs/$run_id"
        Write-Host "  status=$($current.status) conclusion=$($current.conclusion)"
    }

    $conclusion = if ([string]::IsNullOrWhiteSpace([string]$current.conclusion)) { "unknown" } else { [string]$current.conclusion }

    $safe_name = ($run_name -replace "[^A-Za-z0-9._-]", "_")
    $run_dir_name = "${run_id}_${safe_name}_${conclusion}"
    $run_dir = Join-Path $sha_dir $run_dir_name
    New-Item -ItemType Directory -Path $run_dir -Force | Out-Null

    $meta_path = Join-Path $run_dir "run.json"
    $current | ConvertTo-Json -Depth 50 | Set-Content -Path $meta_path -Encoding UTF8

    $zip_path = Join-Path $run_dir "logs.zip"

    Write-Host "  downloading logs zip..."
    Download-RunLogsZip -Owner $Owner -Repo $Repo -RunId $run_id -ZipPath $zip_path

    Write-Host "  extracting logs..."
    Expand-Archive -Path $zip_path -DestinationPath $run_dir -Force

    Write-Host "  done: $run_dir"

    if ($conclusion -ne "success") {
        Write-Host "  NOTE: run conclusion=$conclusion"
    }
}

Write-Host ""
Write-Host "All matching workflow runs processed."
