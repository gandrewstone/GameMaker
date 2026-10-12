# Runs one of the shipped DOS editors (runtime\run\GM\<PROGRAM>.EXE) in DOSBox, driven by a key script through KEYS.EXE,
# in a fresh copy of the data. The files it leaves (saved work, DMP screen dumps) are in <Out>.
#   usage: run_dos.ps1 -Program PALCHOS -Script <script.txt> -Out <dir> [-TimeoutSec 180]
#   -Shipped uses the program shipped in runtime\run\GM instead of the one built from the source (build_dos_editors.ps1).
param(
    [Parameter(Mandatory)][string]$Program,
    [Parameter(Mandatory)][string]$Script,
    [Parameter(Mandatory)][string]$Out,
    [int]$TimeoutSec = 180,
    [string]$Work = (Join-Path $env:TEMP 'gm_editors_oracle'),
    [string]$DosBox = 'C:\Program Files (x86)\DOSBox-0.74-3\DOSBox.exe',
    [string]$ProgramArgs = '',
    [switch]$Shipped
)
$ErrorActionPreference = 'Stop'
$root = Resolve-Path (Join-Path $PSScriptRoot '..\..\..')
$keys = Join-Path $Work 'KEYS.EXE'
if (-not (Test-Path $keys)) { throw "missing $keys - run build_keys.ps1 first" }
if (Test-Path $Out) { throw "$Out exists" }
$run = New-Item -ItemType Directory -Force (Join-Path $Out 'run')
Copy-Item -Recurse (Join-Path $root 'runtime\run\GM') (Join-Path $run 'GM')
if (-not $Shipped) {
    $built = Join-Path $Work ('dosbuild\' + $Program.ToUpper() + '.EXE')
    if (-not (Test-Path $built)) { throw "missing $built - run build_dos_editors.ps1 first (or use -Shipped)" }
    Copy-Item $built (Join-Path $run ('GM\' + $Program.ToUpper() + '.EXE')) -Force
}
Copy-Item $keys (Join-Path $run 'GM\KEYS.EXE')
Copy-Item $Script (Join-Path $run 'GM\SCRIPT.TXT')
@"
[sdl]
windowresolution=640x400
output=surface
[cpu]
cycles=max
[autoexec]
mount c "$($run.FullName)"
c:
cd \gm
cls
keys script.txt $Program.exe $ProgramArgs > c:\gm\keys.out
exit
"@ | Set-Content (Join-Path $Out 'dos.conf') -Encoding ascii
Push-Location $Out
try {
    $p = Start-Process $DosBox -ArgumentList '-conf', 'dos.conf', '-noconsole' -PassThru
    if (-not $p.WaitForExit($TimeoutSec * 1000)) { $p.Kill(); Write-Warning "$Program timed out" }
} finally { Pop-Location }
"{0}: done, files in {1}" -f $Program, (Join-Path $run 'GM')
