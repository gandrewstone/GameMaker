# Starts each game in gmplay with scripted input and reports whether it crashed, hung, or reached moving graphics.
#
# Per game: type the game's path in the picker, press Enter through the intro screens and the menu (the highlighted
# item is "Play"), then repeat a pattern of arrow and space-bar presses. Screenshots are taken every few seconds.
#
#   usage: smoke.ps1 [-Games heart,nebula,...]   (default: every game under cd\)
#                    [-PlaySeconds 50] [-Parallel 3] [-Out <dir>]
# Results: a table on stdout, and per game a folder with err.txt, the screenshots and a contact sheet (sheet.png).
param(
    [string[]]$Games,
    [int]$PlaySeconds = 50,
    [int]$Parallel = 3,
    [string]$Out = (Join-Path $env:TEMP ("gm_smoke_" + (Get-Date -Format 'HHmmss')))
)
$env:GM_NO_SETTINGS = '1'   # a person's saved window/volume settings must not change a test
$root = Resolve-Path (Join-Path $PSScriptRoot '..')
$exe = Join-Path $PSScriptRoot 'build\gmplay.exe'
Add-Type -AssemblyName System.Drawing
New-Item -ItemType Directory $Out | Out-Null

# catalogue: game directory + .gam file name
# Only games in the 3.0 format (file starts with "GM") can be played: the 14 games in cd\sharware use an older format,
# ship with their own old engine executables, and are rejected by the 3.0 player (DOS as well as this port).
$catalogue = foreach ($d in Get-ChildItem (Join-Path $root 'cd\gameware'), (Join-Path $root 'cd\sharware') -Directory) {
    $gam = Get-ChildItem $d.FullName -Filter *.gam -File | Select-Object -First 1
    if (-not $gam) { continue }
    $head = [IO.File]::ReadAllBytes($gam.FullName)[0..1]
    if ($head[0] -ne 0x47 -or $head[1] -ne 0x4D) { Write-Output "skipping $($d.Name): older game format"; continue }
    [pscustomobject]@{ Name = $d.Name; Dir = $d.FullName; Gam = $gam.Name }
}
if ($Games) { $catalogue = $catalogue | Where-Object { $Games -contains $_.Name } }

$intro = 45                       # seconds spent getting through intros and the menu
$total = $intro + $PlaySeconds
$pattern = '\h20\r\h06 \h20\r\h12\u\h10\l\h06 \h12\d\h20\l\h06 \h10\u\h14\r'   # right, fire, right, up, left, fire, down, left, ...

function Start-Smoke($g) {
    $run = Join-Path $Out $g.Name
    New-Item -ItemType Directory $run | Out-Null
    $gm = Join-Path $run 'GM'
    New-Item -ItemType Directory $gm | Out-Null
    Copy-Item (Join-Path $root 'runtime\run\GM\GM.CFG'), (Join-Path $root 'runtime\run\GM\CONFIG.DAT') $gm
    Copy-Item -Recurse $g.Dir (Join-Path $gm $g.Name)
    $path = ("$($g.Name)\$($g.Gam)").ToLower() -replace '\\', '\\'
    $keys = '\b\b\b\b\b\b\b' + $path + '\n' + ('\w45\n' * [int]($intro / 3.6)) + ($pattern * 40)
    $env:GM_TYPE = $keys; $env:GM_TYPE_AT = '2'; $env:GM_WATCHDOG = '1'; $env:GM_SHOT_EVERY = '5'
    $env:GM_SHOT = (Join-Path $run 'shot_%02d.png'); $env:GM_SHOT_AFTER = "$total"
    $p = Start-Process $exe -PassThru -WorkingDirectory $gm -RedirectStandardError (Join-Path $run 'err.txt') -RedirectStandardOutput (Join-Path $run 'out.txt')
    [pscustomobject]@{ Game = $g; Run = $run; Proc = $p; Started = Get-Date }
}

function Summarize($job) {
    $run = $job.Run
    $shots = @(Get-ChildItem $run -Filter 'shot_*.png' | Sort-Object Name)
    $err = (Get-Content (Join-Path $run 'err.txt') -ErrorAction SilentlyContinue | Select-Object -First 3) -join ' | '
    $gfx = 0; $busy = 0; $hashes = @{}
    $late = $shots | Select-Object -Last ([Math]::Max(3, [int]($PlaySeconds / 5)))
    foreach ($s in $shots) {
        $bmp = [Drawing.Bitmap]::FromFile($s.FullName)
        $nonblack = 0; $n = 0; $h = 0
        for ($y = 0; $y -lt $bmp.Height; $y += 8) { for ($x = 0; $x -lt $bmp.Width; $x += 8) {
            $c = $bmp.GetPixel($x, $y); $n++
            if ($c.R + $c.G + $c.B -gt 30) { $nonblack++ }
            $h = ($h * 31 + $c.R + 3 * $c.G + 5 * $c.B) % 2147483647 } }
        $isGfx = $bmp.Width -eq 320
        $bmp.Dispose()
        if ($late.FullName -contains $s.FullName) {
            if ($isGfx -and $nonblack / $n -gt 0.15) { $busy++ }
            $hashes[$h] = 1
        }
        if ($isGfx) { $gfx++ }
    }
    # contact sheet
    if ($shots.Count) {
        $cols = 6; $rows = [Math]::Ceiling($shots.Count / $cols)
        $sheet = New-Object Drawing.Bitmap ($cols * 160), ($rows * 100); $gr = [Drawing.Graphics]::FromImage($sheet); $gr.Clear('Black')
        for ($i = 0; $i -lt $shots.Count; $i++) {
            $im = [Drawing.Image]::FromFile($shots[$i].FullName)
            $gr.DrawImage($im, ($i % $cols) * 160, [Math]::Floor($i / $cols) * 100, 160, 100); $im.Dispose() }
        $sheet.Save((Join-Path $run 'sheet.png')); $gr.Dispose(); $sheet.Dispose()
    }
    $code = if ($job.Proc.HasExited) { $job.Proc.ExitCode } else { 'running' }
    $verdict = if ($err -match 'crash|hang|invalid parameter') { 'CRASH/HANG' }
               elseif ($code -ne 0) { "EXIT $code" }
               elseif ($busy -lt 2) { 'never reached gameplay graphics' }
               elseif ($hashes.Count -lt 3) { 'screen frozen?' }
               else { 'ok' }
    [pscustomobject]@{ Game = $job.Game.Name; Verdict = $verdict; Shots = $shots.Count; GfxShots = $gfx; MovingLate = $hashes.Count; Error = $err }
}

$results = @(); $queue = [System.Collections.Queue]::new(@($catalogue)); $running = @()
while ($queue.Count -gt 0 -or $running.Count -gt 0) {
    while ($queue.Count -gt 0 -and $running.Count -lt $Parallel) { $running += Start-Smoke ($queue.Dequeue()) }
    Start-Sleep -Seconds 2
    $done = @($running | Where-Object { $_.Proc.HasExited -or ((Get-Date) - $_.Started).TotalSeconds -gt ($total + 40) })
    foreach ($j in $done) {
        if (-not $j.Proc.HasExited) { $j.Proc.Kill(); Start-Sleep 1 }
        $results += Summarize $j
        $running = @($running | Where-Object { $_ -ne $j })
    }
}
$results | Sort-Object Game | Format-Table -AutoSize -Wrap | Out-String -Width 200
"results in $Out"
$results
