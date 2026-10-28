# Restore temporary root using the already-staged IonStack payload, over USB.
[CmdletBinding()]
param(
    [ValidatePattern('^[A-Za-z0-9]+$')]
    [string]$Serial = '2G0YC5ZF9Q01J0',
    [ValidateRange(1, 8)]
    [int]$Attempts = 5,
    [switch]$KeepWifiOff,
    [switch]$CheckOnly
)
$ErrorActionPreference = 'Stop'
$adbCommand = Get-Command adb -CommandType Application -ErrorAction SilentlyContinue | Select-Object -First 1
if ($adbCommand) {
    $adbExe = $adbCommand.Source
} else {
    $adbExe = Join-Path $env:LOCALAPPDATA 'Android-platform-tools\platform-tools\adb.exe'
    if (-not (Test-Path -LiteralPath $adbExe)) { throw 'Android platform-tools adb.exe was not found.' }
}
& $adbExe start-server
if ($LASTEXITCODE -ne 0) { throw 'ADB server could not start.' }
$pythonArguments = @('-B', (Join-Path $PSScriptRoot 'root_ionstack.py'), '--serial', $Serial, '--attempts', $Attempts)
if ($KeepWifiOff) { $pythonArguments += '--keep-wifi-off' }
if ($CheckOnly) { $pythonArguments += '--check-only' }
& python @pythonArguments
exit $LASTEXITCODE
