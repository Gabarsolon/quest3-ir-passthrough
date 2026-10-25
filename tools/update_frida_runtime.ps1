# Run in normal PowerShell. Downloads only to the laptop; does not touch Quest.
[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$version = '17.24.3'
$runtime = Join-Path $env:LOCALAPPDATA "quest3-ir\runtime\frida-$version"
$runtimePython = Join-Path $runtime 'Scripts\python.exe'
$server = Join-Path $runtime "frida-server-$version-android-arm64"
$archive = "$server.xz"
$assetName = "frida-server-$version-android-arm64.xz"

if (-not (Test-Path -LiteralPath $runtimePython)) {
    & python -m venv --system-site-packages $runtime
    if ($LASTEXITCODE -ne 0) { throw 'Could not create the isolated Python runtime.' }
}
& $runtimePython -m pip install --disable-pip-version-check --no-deps "frida==$version"
if ($LASTEXITCODE -ne 0) { throw 'Frida Python installation failed.' }

$release = Invoke-RestMethod -Uri "https://api.github.com/repos/frida/frida/releases/tags/$version"
$asset = @($release.assets | Where-Object { $_.name -eq $assetName })
if ($asset.Count -ne 1 -or $asset[0].digest -notmatch '^sha256:([0-9a-fA-F]{64})$') {
    throw 'The official release did not supply a SHA-256 digest for the server asset.'
}
$expectedHash = $Matches[1]
Invoke-WebRequest -Uri $asset[0].browser_download_url -OutFile $archive
if ((Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash -ne $expectedHash) {
    throw 'Downloaded server archive failed SHA-256 verification.'
}
@'
import lzma, pathlib, sys
compressed, destination = map(pathlib.Path, sys.argv[1:])
data = lzma.decompress(compressed.read_bytes())
if len(data) < 20 or data[:5] != b'\x7fELF\x02' or data[18:20] != b'\xb7\x00':
    raise SystemExit('The downloaded server is not an AArch64 ELF executable.')
destination.write_bytes(data)
'@ | & $runtimePython -B - $archive $server
if ($LASTEXITCODE -ne 0) { throw 'Server decompression or architecture validation failed.' }
$actualVersion = & $runtimePython -c 'import frida; print(frida.__version__)'
if ($LASTEXITCODE -ne 0 -or $actualVersion -ne $version) { throw 'Isolated Frida version check failed.' }
Write-Host "Prepared isolated Frida $version. No headset files or global Python packages changed."
Write-Host 'Launch with: powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\run_ir.ps1 --display-refresh 120 --brightness 1.2'
