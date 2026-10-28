#Requires -Version 5.1
<#
Push this checkout, make the repository public, and publish a prototype release.
Run in your normal PowerShell. -DryRun checks the local plan without network access.
#>
[CmdletBinding()]
param(
    [ValidatePattern('^v[0-9]+\.[0-9]+\.[0-9]+(?:-[0-9A-Za-z.-]+)?$')]
    [string]$Tag = 'v0.2.0-preview.1',
    [switch]$DryRun
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$Repository = 'Gabarsolon/quest3-ir-passthrough'
$RepoRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$NotesFile = Join-Path $RepoRoot "docs\releases\$Tag.md"
$GitExe = (Get-Command git -CommandType Application -ErrorAction Stop).Source
$GhCommand = Get-Command gh -CommandType Application -ErrorAction SilentlyContinue
if ($GhCommand) {
    $GhExe = $GhCommand.Source
} else {
    $GhExe = Join-Path $env:ProgramFiles 'GitHub CLI\gh.exe'
    if (-not (Test-Path -LiteralPath $GhExe)) { throw 'GitHub CLI is missing. Install gh first.' }
}

function Invoke-Native {
    param([string]$Exe, [string[]]$Arguments, [switch]$AllowFailure)
    # Windows PowerShell treats redirected native stderr as ErrorRecords.
    $ErrorActionPreference = 'Continue'
    $lines = @(& $Exe @Arguments 2>&1)
    $code = $LASTEXITCODE
    $output = ($lines | ForEach-Object { $_.ToString() }) -join [Environment]::NewLine
    if ($code -ne 0 -and -not $AllowFailure) {
        throw "$Exe failed (exit $code):`n$output"
    }
    [pscustomobject]@{ Code = $code; Text = $output }
}

function Invoke-RepoGit {
    param([string[]]$GitArguments)
    # Credentials apply to this command only; global Git configuration stays intact.
    $arguments = @('-c', "safe.directory=$RepoRoot", '-c', 'credential.helper=',
        '-c', 'credential.helper=!gh auth git-credential', '-C', $RepoRoot) + $GitArguments
    (Invoke-Native -Exe $GitExe -Arguments $arguments).Text
}

function Invoke-Gh {
    param([string[]]$GhArguments)
    (Invoke-Native -Exe $GhExe -Arguments $GhArguments).Text
}

function Assert-Clean {
    if (Invoke-RepoGit @('status', '--porcelain', '--untracked-files=normal')) {
        throw 'The checkout has uncommitted changes. Commit or stash them, then rerun.'
    }
}

$PreviousPath = $env:PATH
$PreviousPrompt = $env:GIT_TERMINAL_PROMPT
try {
    $env:PATH = [IO.Path]::GetDirectoryName($GhExe) + [IO.Path]::PathSeparator + $env:PATH
    $env:GIT_TERMINAL_PROMPT = '0'
    Assert-Clean
    if ((Invoke-RepoGit @('branch', '--show-current')) -ne 'main') { throw 'Switch to main before publishing.' }
    $origin = (Invoke-RepoGit @('remote', 'get-url', 'origin')).TrimEnd('/')
    if ($origin -notmatch '^https://github\.com/Gabarsolon/quest3-ir-passthrough(?:\.git)?$') {
        throw "Unexpected origin: $origin"
    }
    $Head = Invoke-RepoGit @('rev-parse', 'HEAD')
    if (-not (Test-Path -LiteralPath $NotesFile)) { throw "Missing release notes: $NotesFile" }
    Write-Host "Repository: https://github.com/$Repository"
    Write-Host "Commit:     $Head"
    Write-Host "Plan: push main, set public visibility, publish prerelease $Tag."
    if ($DryRun) { Write-Host 'Dry run complete. No network requests or publishing performed.'; return }

    $auth = Invoke-Native -Exe $GhExe -Arguments @('api', 'user') -AllowFailure
    if ($auth.Code -ne 0) {
        Write-Host 'GitHub login is needed. Complete the browser/device-code login.'
        # Keep login interactive so its device code appears immediately.
        & $GhExe auth login --hostname github.com --git-protocol https --web
        if ($LASTEXITCODE -ne 0) { throw 'GitHub login did not complete.' }
    }
    $info = Invoke-Gh @('api', "repos/$Repository") | ConvertFrom-Json
    if (-not $info.permissions.push) { throw 'This GitHub account cannot push to the repository.' }
    if ($info.private -and -not $info.permissions.admin) {
        throw 'Making this repository public requires an account with repository admin access.'
    }

    Write-Host 'Checking remote history and release tag...'
    $null = Invoke-RepoGit @('fetch', '--no-tags', 'origin', 'main')
    $RemoteHead = Invoke-RepoGit @('rev-parse', 'FETCH_HEAD')
    $ancestor = Invoke-Native -Exe $GitExe -Arguments @('-c', "safe.directory=$RepoRoot",
        '-C', $RepoRoot, 'merge-base', '--is-ancestor', $RemoteHead, $Head) -AllowFailure
    if ($ancestor.Code -ne 0) { throw 'Remote main contains commits missing locally. Reconcile them before publishing.' }

    $tagLines = Invoke-RepoGit @('ls-remote', '--tags', 'origin', "refs/tags/$Tag", "refs/tags/$Tag^{}")
    $tagTarget = ''
    foreach ($line in ($tagLines -split '\r?\n')) {
        if ($line) { $tagTarget = ($line -split '\s+')[0] }
    }
    if ($tagTarget -and $tagTarget -ne $Head) { throw "Tag $Tag already points to another commit. Choose another -Tag." }

    $pages = Invoke-Gh @('api', '--paginate', '--slurp', "repos/$Repository/releases") | ConvertFrom-Json
    $existing = @(foreach ($page in $pages) {
        foreach ($release in $page) { if ($release.tag_name -eq $Tag) { $release } }
    })
    if ($existing.Count -gt 0 -and ($existing[0].draft -or -not $tagTarget)) {
        throw "Release $Tag already exists as a draft or has no matching tag. Choose another -Tag."
    }
    Assert-Clean
    if ((Invoke-RepoGit @('rev-parse', 'HEAD')) -ne $Head) { throw 'HEAD changed during login. Rerun the script.' }

    if ($RemoteHead -ne $Head) {
        Write-Host 'Pushing the committed fixes...'
        Write-Host (Invoke-RepoGit @('push', 'origin', "${Head}:refs/heads/main"))
    }
    $published = Invoke-RepoGit @('ls-remote', 'origin', 'refs/heads/main')
    if (($published -split '\s+')[0] -ne $Head) { throw 'Remote main does not match the intended commit.' }

    if ($info.private) {
        Write-Host 'Making the repository public...'
        $null = Invoke-Gh @('repo', 'edit', $Repository, '--visibility', 'public',
            '--accept-visibility-change-consequences')
    }
    $visibility = Invoke-Gh @('api', "repos/$Repository") | ConvertFrom-Json
    if ($visibility.private) { throw 'The repository is still private.' }

    if ($existing.Count -eq 0) {
        Write-Host "Publishing $Tag..."
        Write-Host (Invoke-Gh @('release', 'create', $Tag, '--repo', $Repository,
            '--target', $Head, '--title', "$Tag - RAM-only stereo IR passthrough",
            '--notes-file', $NotesFile, '--prerelease'))
    } else {
        Write-Host 'The matching release is already published.'
    }
    $result = Invoke-Gh @('release', 'view', $Tag, '--repo', $Repository,
        '--json', 'url,isDraft') | ConvertFrom-Json
    if ($result.isDraft) { throw 'The release is still a draft.' }
    Write-Host "Published: $($result.url)" -ForegroundColor Green
} finally {
    $env:PATH = $PreviousPath
    $env:GIT_TERMINAL_PROMPT = $PreviousPrompt
}
