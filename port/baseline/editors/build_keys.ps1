# Builds KEYS.EXE (the DOS keyboard script driver, see KEYS.C) with the bundled Borland C++ inside DOSBox.
#   usage: build_keys.ps1 [-Work <dir>] [-DosBox <exe>]
param(
    [string]$Work = (Join-Path $env:TEMP 'gm_editors_oracle'),
    [string]$DosBox = 'C:\Program Files (x86)\DOSBox-0.74-3\DOSBox.exe'
)
$ErrorActionPreference = 'Stop'
$root = Resolve-Path (Join-Path $PSScriptRoot '..\..\..')
New-Item -ItemType Directory -Force $Work | Out-Null
$b = Join-Path $Work 'keysbuild'
if (Test-Path $b) { Remove-Item -Recurse -Force $b }
New-Item -ItemType Directory -Force $b | Out-Null
Copy-Item (Join-Path $PSScriptRoot 'KEYS.C') $b

@"
[sdl]
windowresolution=640x400
output=surface
[cpu]
cycles=max
[autoexec]
mount t "$(Join-Path $root 'tools')"
mount c "$b"
PATH=z:\;T:\BORLANDC\BIN
c:
bcc -ms -IT:\BORLANDC\INCLUDE -LT:\BORLANDC\LIB keys.c > build.log
exit
"@ | Set-Content (Join-Path $b 'build.conf') -Encoding ascii

Push-Location $b
try {
    $p = Start-Process $DosBox -ArgumentList '-conf', 'build.conf', '-noconsole' -PassThru
    if (-not $p.WaitForExit(120000)) { $p.Kill(); throw 'DOSBox build timed out' }
} finally { Pop-Location }
if (-not (Test-Path (Join-Path $b 'KEYS.EXE'))) { Get-Content (Join-Path $b 'build.log') -ErrorAction SilentlyContinue | Select-Object -Last 20; throw 'build failed' }
Copy-Item (Join-Path $b 'KEYS.EXE') (Join-Path $Work 'KEYS.EXE') -Force
"built $(Join-Path $Work 'KEYS.EXE') ($((Get-Item (Join-Path $b 'KEYS.EXE')).Length) bytes)"
