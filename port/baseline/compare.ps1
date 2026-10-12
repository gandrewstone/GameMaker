# Replays each game's demo.rec in the ported engine (gmplay.exe) and compares the result with the
# DOS oracle produced by run_baseline.ps1 (<Work>\<game>.bin and <game>.trace.txt).
#
# For every game it reports:
#   - ticks:  per-tick engine trace (sequence, frame, position, scene, pending keys; the wall-clock
#             `timer=` field is ignored) -- must be identical
#   - logic:  frame-dump frames whose scene / character position / score differ -- must be 0
#   - pixels: frames with identical state AND scroll offset whose 64000 video bytes differ -- must be 0
#
#   usage: compare.ps1 -Games bcuda,nebula,... [-Work <dir>] [-Seconds 400]
param(
    [Parameter(Mandatory)][string[]]$Games,
    [string]$Work = (Join-Path $env:TEMP 'gm_oracle'),
    [int]$Seconds = 400,
    [int]$MaxFrames = 5000,          # must equal the cap used by run_baseline.ps1
    [string]$Recs,                   # replay <Recs>\<game>.rec instead of the shipped demo (same value as for run_baseline.ps1)
    [string]$Tag = ''                # same value as for run_baseline.ps1
)
$ErrorActionPreference = 'Stop'
$env:GM_NO_SETTINGS = '1'   # a person's saved window/volume settings must not change a test
$root = Resolve-Path (Join-Path $PSScriptRoot '..\..')
$exe = Join-Path $root 'port\build\gmplay.exe'
$fdump = Join-Path $root 'port\build\fdump.exe'

# start all replays first so they run side by side
$runs = foreach ($g in $Games) {
    $src = Join-Path $root "cd\gameware\$g"
    $rec = if ($Recs) { 'GEN.REC' } else { (Get-ChildItem $src -Filter *.rec | Select-Object -First 1).Name }
    $gam = (Get-ChildItem $src -Filter *.gam | Select-Object -First 1).Name
    $run = Join-Path $env:TEMP "gm_port_$g$Tag"
    if (Test-Path $run) { Remove-Item -Recurse -Force $run }
    New-Item -ItemType Directory $run | Out-Null
    Copy-Item -Recurse (Join-Path $root 'runtime\run\GM') (Join-Path $run 'GM')
    Copy-Item -Recurse $src (Join-Path $run "GM\$g")
    if ($Recs) { Copy-Item (Join-Path $Recs "$g.rec") (Join-Path $run "GM\$g\GEN.REC") }
    $env:FDUMP = Join-Path $run 'frames.bin'
    $env:FTRACE = Join-Path $run 'trace.txt'
    $env:FDUMPMAX = "$MaxFrames"
    $p = Start-Process $exe -ArgumentList "$g\$rec", "$g\$gam" -PassThru -WorkingDirectory (Join-Path $run 'GM') `
         -RedirectStandardError (Join-Path $run 'err.txt') -RedirectStandardOutput (Join-Path $run 'out.txt')
    [pscustomobject]@{ Game = $g; Run = $run; Proc = $p }
}

$deadline = (Get-Date).AddSeconds($Seconds)
foreach ($r in $runs) {
    $left = [int]($deadline - (Get-Date)).TotalMilliseconds
    if ($left -lt 1000 -or -not $r.Proc.WaitForExit($left)) { $r.Proc.Kill(); $r.Timeout = $true }
}

# tick lines only (debug env vars such as FMONSLOT add indented extra lines); the wall-clock timer is ignored
function Strip-Timer($path) { Get-Content $path | Where-Object { $_ -match '^\d' } | ForEach-Object { $_ -replace ' timer=\d+', '' } }

"{0,-10} {1,-22} {2,-28} {3,-26}" -f 'game', 'ticks (DOS vs port)', 'logic (frames differing)', 'pixels (same-pan frames)'
foreach ($r in $runs) {
    $g = $r.Game
    $bin = Join-Path $r.Run 'frames.bin'
    $dosBin = Join-Path $Work "$g$Tag.bin"
    $dosTr = Join-Path $Work "$g$Tag.trace.txt"
    if ($r.Timeout -or -not (Test-Path $bin)) { "{0,-10} NO RESULT (exit {1}) {2}" -f $g, $r.Proc.ExitCode, (Get-Content (Join-Path $r.Run 'err.txt') -TotalCount 3); continue }

    $a = @(Strip-Timer $dosTr); $b = @(Strip-Timer (Join-Path $r.Run 'trace.txt'))
    $n = [Math]::Min($a.Count, $b.Count); $first = -1; $bad = 0
    for ($i = 0; $i -lt $n; $i++) { if ($a[$i] -ne $b[$i]) { $bad++; if ($first -lt 0) { $first = $i } } }
    $ticks = if ($a.Count -eq $b.Count -and $bad -eq 0) { "identical ($($a.Count))" } else { "$bad differ, first tick $($first+1); $($a.Count) vs $($b.Count)" }

    $diff = & $fdump diff $dosBin $bin
    $logic = ($diff | Select-String 'of \d+ frames: (\d+) differ in logic').Matches.Groups[1].Value
    $total = ($diff | Select-String 'frames: (\d+) vs (\d+)').Matches.Groups
    $same = ($diff | Select-String 'identical state and pan \((\d+) frames\): (\d+) differ').Matches.Groups
    "{0,-10} {1,-22} {2,-28} {3,-26}" -f $g, $ticks, "$logic of $($total[1].Value) vs $($total[2].Value) frames", "$($same[2].Value) of $($same[1].Value)"
}
