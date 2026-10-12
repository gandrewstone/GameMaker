# Builds the instrumented DOS playgame (-DFRAMEDUMP) from the repo source, inside DOSBox with the
# bundled Borland toolchain. Works in a scratch copy so tracked .OBJ/.EXE files stay untouched.
#   usage: build_oracle.ps1 [-Work <dir>] [-DosBox <exe>]
param(
    [string]$Work = (Join-Path $env:TEMP 'gm_oracle'),
    [string]$DosBox = 'C:\Program Files (x86)\DOSBox-0.74-3\DOSBox.exe'
)
$ErrorActionPreference = 'Stop'
$root = Resolve-Path (Join-Path $PSScriptRoot '..\..')
$b = Join-Path $Work 'build'
if (Test-Path $b) { Remove-Item -Recurse -Force $b }
New-Item -ItemType Directory -Force $b | Out-Null
Copy-Item -Recurse (Join-Path $root 'code') (Join-Path $b 'code')
Copy-Item -Recurse (Join-Path $root 'tools') (Join-Path $b 'tools')

$gm = Join-Path $b 'code\GM'
# playgame needs the OLD mouse routines (see the #error in PLAYGAME.C), so everything is rebuilt with MOUSE.
(Get-Content (Join-Path $gm 'MOUSEFN.H')) -replace '^// #define MOUSE', '#define MOUSE' | Set-Content (Join-Path $gm 'MOUSEFN.H')
(Get-Content (Join-Path $gm 'PLAYGAME.MAK')) -replace '\$\(CFLAGS\) -I\.\.\\Image\\fli playgame\.c', '$(CFLAGS) -DFRAMEDUMP -I..\Image\fli playgame.c' | Set-Content (Join-Path $gm 'PLAYGAME.MAK')
if (-not (Select-String -Path (Join-Path $gm 'PLAYGAME.MAK') -Pattern 'FRAMEDUMP' -Quiet)) { throw 'failed to patch PLAYGAME.MAK' }
Get-ChildItem $gm -Filter *.OBJ | Remove-Item

@"
[sdl]
windowresolution=640x400
output=surface
[cpu]
cycles=max
[autoexec]
mount t tools
mount d code
PATH=z:\;d:\BIN;T:\BORLANDC\BIN
d:
cd gm
call create playgame
exit
"@ | Set-Content (Join-Path $b 'build.conf') -Encoding ascii

Push-Location $b
try {
    $p = Start-Process $DosBox -ArgumentList '-conf', 'build.conf', '-noconsole' -PassThru
    if (-not $p.WaitForExit(600000)) { $p.Kill(); throw 'DOSBox build timed out' }
} finally { Pop-Location }

$exe = Join-Path $gm 'PLAYGAME.EXE'
if (-not (Test-Path $exe)) { Get-Content (Join-Path $gm 'PLAYGAME.LOG') | Select-Object -Last 15; throw 'build failed' }
Copy-Item $exe (Join-Path $Work 'PLAYGAME.EXE') -Force
"built $(Join-Path $Work 'PLAYGAME.EXE') ($((Get-Item $exe).Length) bytes)"
