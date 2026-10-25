# Arguments are forwarded unchanged to the Python controller.
$ErrorActionPreference = 'Stop'
$version = '17.24.3'
$runtime = Join-Path $env:LOCALAPPDATA "quest3-ir\runtime\frida-$version"
$runtimePython = Join-Path $runtime 'Scripts\python.exe'
$server = Join-Path $runtime "frida-server-$version-android-arm64"
if (-not (Test-Path -LiteralPath $runtimePython) -or -not (Test-Path -LiteralPath $server)) {
    throw 'First run tools\update_frida_runtime.ps1 in normal PowerShell to prepare the isolated runtime.'
}
& $runtimePython -B (Join-Path $PSScriptRoot 'system_passthrough.py') --frida-server $server @args
exit $LASTEXITCODE
