# Runs the ported engine in a scratch copy of the installed GameMaker directory plus one sample game.
#   usage: run_port.ps1 [-Game bcuda] [-Replay] [-Seconds 60]
#   -Replay  plays the game's demo.rec non-interactively and writes frame dump <Game>_port.bin to the run dir
param(
    [string]$Game = 'bcuda',
    [switch]$Replay,
    [int]$Seconds = 60
)
$env:GM_NO_SETTINGS = '1'   # a person's saved window/volume settings must not change a test
$root = Resolve-Path (Join-Path $PSScriptRoot '..')
$run = Join-Path $env:TEMP 'gm_port_run'
if (Test-Path $run) { Remove-Item -Recurse -Force $run }
New-Item -ItemType Directory $run | Out-Null
Copy-Item -Recurse (Join-Path $root 'runtime\run\GM') (Join-Path $run 'GM')
Copy-Item -Recurse (Join-Path $root "cd\gameware\$Game") (Join-Path $run "GM\$Game")
$exe = Join-Path $PSScriptRoot 'build\gmplay.exe'
$gm = Join-Path $run 'GM'
$rec = (Get-ChildItem (Join-Path $gm $Game) -Filter *.rec | Select-Object -First 1).Name
$gam = (Get-ChildItem (Join-Path $gm $Game) -Filter *.gam | Select-Object -First 1).Name
$args = if ($Replay) { @("$Game\$rec", "$Game\$gam") } else { @() }
$env:FDUMP = Join-Path $run "${Game}_port.bin"
$p = Start-Process $exe -ArgumentList $args -PassThru -WorkingDirectory $gm `
     -RedirectStandardOutput (Join-Path $run 'out.txt') -RedirectStandardError (Join-Path $run 'err.txt')
if (-not $p.WaitForExit($Seconds * 1000)) { "still running after ${Seconds}s - killed"; $p.Kill() } else { "exit code $($p.ExitCode)" }
"--- stdout"; Get-Content (Join-Path $run 'out.txt') | Select-Object -First 20
"--- stderr"; Get-Content (Join-Path $run 'err.txt') | Select-Object -First 30
$bin = Join-Path $run "${Game}_port.bin"
if (Test-Path $bin) { "frame dump: $bin ($((Get-Item $bin).Length) bytes)" }
