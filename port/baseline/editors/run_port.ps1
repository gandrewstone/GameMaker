# Runs one of the port's editors (port\build\gm<program>.exe, headless) on a key script, in a fresh copy of the data, like
# run_dos.ps1 does for the shipped DOS editor. The files it leaves (saved work, DMP screen dumps) are in <Out>\run\GM.
#   usage: run_port.ps1 -Program palchos -Script <script.txt> -Out <dir> [-TimeoutSec 180]
param(
    [Parameter(Mandatory)][string]$Program,
    [Parameter(Mandatory)][string]$Script,
    [Parameter(Mandatory)][string]$Out,
    [int]$TimeoutSec = 180,
    [string]$Build = '',
    [string]$ProgramArgs = ''
)
$ErrorActionPreference = 'Stop'
$root = Resolve-Path (Join-Path $PSScriptRoot '..\..\..')
if (-not $Build) { $Build = Join-Path $root 'port\build' }
$exe = Join-Path $Build ("gm" + $Program.ToLower() + ".exe")
if (-not (Test-Path $exe)) { throw "missing $exe" }
if (Test-Path $Out) { throw "$Out exists" }
$run = New-Item -ItemType Directory -Force (Join-Path $Out 'run')
$gm = Join-Path $run 'GM'
Copy-Item -Recurse (Join-Path $root 'runtime\run\GM') $gm
Copy-Item $Script (Join-Path $gm 'SCRIPT.TXT')
$env:GM_NO_SETTINGS = '1'; $env:GM_HEADLESS = '1'; $env:GM_KEYSCRIPT = 'SCRIPT.TXT'
$romfont = Join-Path $env:TEMP 'gm_editors_oracle\ROMFONT.BIN'      # the DOS machine's glyphs (see getfont.ps1)
if (Test-Path $romfont) { $env:GM_ROMFONT = $romfont }
try {
    $sp = @{ FilePath = $exe; WorkingDirectory = $gm; PassThru = $true; WindowStyle = 'Hidden'; RedirectStandardError = (Join-Path $Out 'err.txt') }
    if ($ProgramArgs) { $sp.ArgumentList = $ProgramArgs }
    $p = Start-Process @sp
    if (-not $p.WaitForExit($TimeoutSec * 1000)) { $p.Kill(); Write-Warning "$Program timed out" }
} finally { $env:GM_KEYSCRIPT = $null; $env:GM_HEADLESS = $null; $env:GM_ROMFONT = $null }
"{0}: done, files in {1}" -f $Program, $gm
