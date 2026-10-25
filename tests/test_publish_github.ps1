# Offline command mocks. No network requests, Git writes, or account changes.
$ErrorActionPreference = 'Stop'
$PublishScript = Join-Path $PSScriptRoot '..\tools\publish_github.ps1'
$global:PublishMock = $null
function Get-Command {
    [CmdletBinding()]
    param([string]$Name, [string]$CommandType)
    if ($Name -eq 'git') { return [pscustomobject]@{ Source = 'MockGit' } }
    if ($Name -eq 'gh') { return [pscustomobject]@{ Source = 'MockGh' } }
    throw "Unexpected executable: $Name"
}
function MockGit {
    $global:LASTEXITCODE = 0
    $a = @($args); $i = 0
    while ($a[$i] -eq '-c' -or $a[$i] -eq '-C') { $i += 2 }
    switch ($a[$i]) {
        'status' { if ($global:PublishMock.Dirty) { ' M README.md' } }
        'branch' { 'main' }
        'remote' { 'https://github.com/Gabarsolon/quest3-ir-passthrough.git' }
        'rev-parse' { if ($a[-1] -eq 'FETCH_HEAD') { $global:PublishMock.Remote } else { '1' * 40 } }
        'fetch' { }
        'merge-base' { if ($global:PublishMock.Diverged) { $global:LASTEXITCODE = 1 } }
        'ls-remote' {
            if ($a -contains '--tags') {
                if ($global:PublishMock.TagHead) { "$($global:PublishMock.TagHead)`trefs/tags/v0.1.0-preview.1" }
            } else { "$($global:PublishMock.Remote)`trefs/heads/main" }
        }
        'push' { $global:PublishMock.Remote = '1' * 40; $global:PublishMock.Mutations++ }
        default { throw "Unexpected git call: $a" }
    }
}
function MockGh {
    $global:LASTEXITCODE = 0
    $a = @($args)
    switch ($a[0]) {
        'api' {
            if ($a -contains 'user') {
                if ($global:PublishMock.Auth) { '{"login":"Gabarsolon"}' }
                else { $global:LASTEXITCODE = 1; 'Not authenticated' }
            } elseif ($a[-1] -like '*/releases') {
                if ($global:PublishMock.Released) {
                    '[[{"tag_name":"v0.1.0-preview.1","draft":false}]]'
                } else { '[[]]' }
            } else {
                @{ private = $global:PublishMock.Private; permissions = @{push=$true;admin=$true} } | ConvertTo-Json
            }
        }
        'auth' { $global:PublishMock.Auth = $true }
        'repo' { $global:PublishMock.Private = $false; $global:PublishMock.Mutations++ }
        'release' {
            if ($a[1] -eq 'create') {
                $targetIndex = [array]::IndexOf($a, '--target')
                if ($targetIndex -lt 0 -or $a[$targetIndex + 1] -ne ('1' * 40)) { throw 'Incorrect release target.' }
                $global:PublishMock.Released = $true; $global:PublishMock.TagHead = '1' * 40
                $global:PublishMock.Mutations++
            } else { '{"url":"https://github.com/Gabarsolon/quest3-ir-passthrough/releases/tag/v0.1.0-preview.1","isDraft":false}' }
        }
        default { throw "Unexpected gh call: $a" }
    }
}
function Reset-Mock {
    $global:PublishMock = @{Dirty=$false;Diverged=$false;Remote=('0'*40);TagHead='';
        Auth=$true;Private=$true;Released=$false;Mutations=0}
}
function Assert-Rejected {
    $caught = $false
    try { & $PublishScript *> $null } catch { $caught = $true }
    if (-not $caught -or $global:PublishMock.Mutations -ne 0) { throw 'Expected a failure before publishing.' }
}
Reset-Mock
& $PublishScript -DryRun *> $null
if ($global:PublishMock.Mutations) { throw 'Dry run mutated remote state.' }
& $PublishScript *> $null
if ($global:PublishMock.Mutations -ne 3 -or -not $global:PublishMock.Released) { throw 'Publish did not complete.' }
& $PublishScript *> $null
if ($global:PublishMock.Mutations -ne 3) { throw 'Rerun repeated publishing mutations.' }
Reset-Mock; $global:PublishMock.Auth = $false
& $PublishScript *> $null
if (-not $global:PublishMock.Released) { throw 'Login fallback did not complete.' }
Reset-Mock; $global:PublishMock.Dirty = $true; Assert-Rejected
Reset-Mock; $global:PublishMock.Diverged = $true; Assert-Rejected
Reset-Mock; $global:PublishMock.TagHead = '2' * 40; Assert-Rejected
'Passed: dry run, publish, rerun, login fallback, dirty checkout, divergence, tag conflict.'
