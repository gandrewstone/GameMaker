# Replays a game's shipped demo recording in the instrumented DOS playgame and writes baseline
# artifacts to port\baseline\<game>\ : states.txt (frame table) and a few PNG key frames.
# The full frame dump (~5 MB) stays in the work directory as <game>.bin (not checked in).
#   usage: run_baseline.ps1 -Games bcuda,nebula [-Work <dir>] [-DosBox <exe>]
param(
    [Parameter(Mandatory)][string[]]$Games,
    [string]$Work = (Join-Path $env:TEMP 'gm_oracle'),
    [int]$TimeoutSec = 300,
    [int]$MaxFrames = 5000,          # frame-dump cap; recordings normally end themselves long before
    [string]$Recs,                   # replay <Recs>\<game>.rec (see gen_recordings.ps1) instead of the game's shipped demo
    [string]$Tag = '',               # suffix for work files and the output folder, e.g. "-gen"
    [string]$DosBox = 'C:\Program Files (x86)\DOSBox-0.74-3\DOSBox.exe'
)
$ErrorActionPreference = 'Stop'
$root = Resolve-Path (Join-Path $PSScriptRoot '..\..')
$fdump = Join-Path $root 'port\build\fdump.exe'
$exe = Join-Path $Work 'PLAYGAME.EXE'
if (-not (Test-Path $exe)) { throw "missing $exe - run build_oracle.ps1 first" }
if (-not (Test-Path $fdump)) { throw "missing $fdump - run port\build.bat first" }

foreach ($g in $Games) {
    $src = Join-Path $root "cd\gameware\$g"
    $gam = Get-ChildItem $src -Filter '*.gam' | Select-Object -First 1
    $recName = $null
    if ($Recs) {
        $recFile = Join-Path $Recs "$g.rec"
        if (Test-Path $recFile) { $recName = 'GEN.REC' }
    } else {
        $rec = Get-ChildItem $src -Filter '*.rec' | Select-Object -First 1
        if ($rec) { $recName = $rec.Name }
    }
    if (-not $recName -or -not $gam) { Write-Warning "${g}: no .rec/.gam, skipped"; continue }

    $run = Join-Path $Work "run_$g$Tag"
    if (Test-Path $run) { Remove-Item -Recurse -Force $run }
    New-Item -ItemType Directory -Force (Join-Path $run 'run') | Out-Null
    Copy-Item -Recurse (Join-Path $root 'runtime\run\GM') (Join-Path $run 'run\GM')
    Copy-Item -Recurse $src (Join-Path $run "run\GM\$g")
    if ($Recs) { Copy-Item $recFile (Join-Path $run "run\GM\$g\GEN.REC") }
    Copy-Item $exe (Join-Path $run 'run\GM\PLAYGAME.EXE')

    @"
[sdl]
windowresolution=640x400
output=surface
[cpu]
cycles=max
[autoexec]
mount c run
c:
cd \gm
set FDUMP=c:\gm\frames.bin
set FTRACE=c:\gm\trace.txt
set FDUMPMAX=$MaxFrames
$(if ($env:FSCROLL) { 'set FSCROLL=1' })
playgame $g\$recName $g\$($gam.Name) > c:\gm\out.txt
exit
"@ | Set-Content (Join-Path $run 'replay.conf') -Encoding ascii

    $sw = [Diagnostics.Stopwatch]::StartNew()
    Push-Location $run
    try {
        $p = Start-Process $DosBox -ArgumentList '-conf', 'replay.conf', '-noconsole' -PassThru
        if (-not $p.WaitForExit($TimeoutSec * 1000)) { $p.Kill(); Write-Warning "${g}: timed out" }
    } finally { Pop-Location }

    $bin = Join-Path $run 'run\GM\frames.bin'
    if (-not (Test-Path $bin)) { Write-Warning "${g}: no frames produced"; continue }
    Copy-Item $bin (Join-Path $Work "$g$Tag.bin") -Force
    $tr = Join-Path $run 'run\GM\trace.txt'
    if (Test-Path $tr) { Copy-Item $tr (Join-Path $Work "$g$Tag.trace.txt") -Force }

    $out = Join-Path $PSScriptRoot "$g$Tag"
    New-Item -ItemType Directory -Force $out | Out-Null
    & $fdump list $bin | Set-Content (Join-Path $out 'states.txt') -Encoding ascii
    $n = (Get-Content (Join-Path $out 'states.txt')).Count
    foreach ($i in @(0, [int]($n / 2), ($n - 1)) | Select-Object -Unique) {
        & $fdump png $bin $i (Join-Path $out ("frame{0:D3}.png" -f $i)) 2>&1 | Out-Null
    }
    "{0}: {1} frames in {2:N0}s -> {3}" -f $g, $n, $sw.Elapsed.TotalSeconds, $out
}
