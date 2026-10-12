# Produces replayable recordings for games that ship without a demo: plays each game in gmplay with a seeded,
# pseudo-random key script, then ends the session with Esc so the engine saves <game>.rec (it records every
# session from the menu's "Play"). The recordings are small (<= 3 KB) and are stored in baseline\recs\ so that
# DOS and the port can both replay them:
#
#   gen_recordings.ps1 -Games donut,glub ...        (default: every 3.0-format game)
#   run_baseline.ps1 -Games donut -Recs baseline\recs -Tag -gen     (DOS side)
#   compare.ps1      -Games donut -Recs baseline\recs -Tag -gen     (port side)
#
# Which keys get pressed: the scan codes that the game's character files bind to animation sequences
# (arrows, space, letters, digits and Enter; shift/ctrl/alt/function keys are not typed).
param(
    [string[]]$Games,
    [int]$PlaySeconds = 60,
    [int]$Seed = 1,
    [int]$Parallel = 3,
    [switch]$Shots,                  # also save a screenshot every 4 s into the work folder (to see why a game did not save its recording)
    [string]$Out = (Join-Path $PSScriptRoot 'recs'),
    [string]$Work = (Join-Path $env:TEMP ("gm_gen_" + (Get-Date -Format 'HHmmss')))
)
$env:GM_NO_SETTINGS = '1'   # a person's saved window/volume settings must not change a test
$root = Resolve-Path (Join-Path $PSScriptRoot '..\..')
$exe = Join-Path $root 'port\build\gmplay.exe'
New-Item -ItemType Directory -Force $Out | Out-Null
New-Item -ItemType Directory -Force $Work | Out-Null

# scan code -> GM_TYPE token
$tokens = @{ 0x48 = '\u'; 0x50 = '\d'; 0x4B = '\l'; 0x4D = '\r'; 0x39 = ' '; 0x1C = '\n' }
$rows = @{ 0x02 = '1234567890'; 0x10 = 'qwertyuiop'; 0x1E = 'asdfghjkl'; 0x2C = 'zxcvbnm' }
foreach ($start in $rows.Keys) { $i = 0; foreach ($ch in $rows[$start].ToCharArray()) { $tokens[$start + $i] = "$ch"; $i++ } }

function Get-GameKeys($dir) {
    # chrstruct: 20 sequences of 185 bytes; each sequence holds 10 frames of 17 bytes, then the activation key (int:
    # ascii in the low byte, scan code in the high byte)
    $keys = New-Object System.Collections.Generic.HashSet[int]
    foreach ($f in Get-ChildItem $dir -Filter *.chr -File) {
        $b = [IO.File]::ReadAllBytes($f.FullName)
        for ($chr = 0; $chr + 3784 -le $b.Length; $chr += 3784) {
            for ($s = 0; $s -lt 20; $s++) {
                $scan = $b[$chr + $s * 185 + 171]
                if ($scan -and $tokens.ContainsKey([int]$scan)) { [void]$keys.Add([int]$scan) }
            }
        }
    }
    @($keys)
}

$catalogue = foreach ($d in Get-ChildItem (Join-Path $root 'cd\gameware') -Directory) {
    $gam = Get-ChildItem $d.FullName -Filter *.gam -File | Select-Object -First 1
    if ($gam -and ([IO.File]::ReadAllBytes($gam.FullName)[0..1] -join ',') -eq '71,77') { [pscustomobject]@{ Name = $d.Name; Dir = $d.FullName; Gam = $gam } }
}
if ($Games) { $catalogue = $catalogue | Where-Object { $Games -contains $_.Name } }

$intro = 45
function Build-Script($g, $index) {
    $keys = Get-GameKeys $g.Dir
    if (-not $keys.Count) { $keys = 0x4B, 0x4D, 0x48, 0x39 }            # no usable bindings found: arrows and fire
    $rng = New-Object System.Random ($Seed * 1000 + $index)
    # weight the arrows and fire a little higher: they move the character
    $pool = @($keys) + @($keys | Where-Object { $_ -in 0x48, 0x4B, 0x4D, 0x39 })
    $sb = New-Object Text.StringBuilder
    [void]$sb.Append('\b\b\b\b\b\b\b' + (("$($g.Name)\$($g.Gam.Name)").ToLower() -replace '\\', '\\') + '\n')
    for ($i = 0; $i -lt [int]($intro / 3.6); $i++) { [void]$sb.Append('\w45\n') }   # intros and menu: Enter ("Play") every 3.6 s
    $t = 0.0; $n = 0
    while ($t -lt $PlaySeconds) {
        $k = $pool[$rng.Next($pool.Count)]
        $hold = $rng.Next(2, 26)
        $tok = $tokens[[int]$k]
        [void]$sb.Append(('\h{0:D2}' -f $hold) + $tok)
        $t += ($hold + 2) * 0.08
        if ($rng.Next(4) -eq 0) { $w = $rng.Next(2, 14); [void]$sb.Append(('\w{0:D2}' -f $w)); $t += $w * 0.08 }
        $n++
    }
    [void]$sb.Append('\w20\e' + ('\w15a\w05\n' * 60))              # leave the game, then dismiss the end screens, name prompt and high scores until back at the menu (that is when the .rec is saved)
    [pscustomobject]@{ Keys = $sb.ToString(); KeyCount = $keys.Count; Presses = $n }
}

$queue = [System.Collections.Queue]::new(@($catalogue)); $running = @(); $results = @(); $idx = 0
$total = 6 + $intro + $PlaySeconds + 110
while ($queue.Count -gt 0 -or $running.Count -gt 0) {
    while ($queue.Count -gt 0 -and $running.Count -lt $Parallel) {
        $g = $queue.Dequeue(); $idx++
        $run = Join-Path $Work $g.Name; $gm = Join-Path $run 'GM'
        New-Item -ItemType Directory -Force $gm | Out-Null
        Copy-Item (Join-Path $root 'runtime\run\GM\GM.CFG'), (Join-Path $root 'runtime\run\GM\CONFIG.DAT') $gm
        Copy-Item -Recurse $g.Dir (Join-Path $gm $g.Name)
        $script = Build-Script $g $idx
        $env:GM_TYPE = $script.Keys; $env:GM_TYPE_AT = '2'; $env:GM_WATCHDOG = '1'
        $env:GM_SHOT = (Join-Path $run $(if ($Shots) { 'shot_%02d.png' } else { 'final.png' })); $env:GM_SHOT_AFTER = "$($total + 4)"
        if ($Shots) { $env:GM_SHOT_EVERY = '4' } else { Remove-Item Env:GM_SHOT_EVERY -ErrorAction SilentlyContinue }
        $p = Start-Process $exe -PassThru -WorkingDirectory $gm -RedirectStandardError (Join-Path $run 'err.txt') -RedirectStandardOutput (Join-Path $run 'out.txt')
        $running += [pscustomobject]@{ G = $g; Run = $run; Proc = $p; Started = Get-Date; Script = $script }
    }
    Start-Sleep -Seconds 2
    foreach ($j in @($running | Where-Object { $_.Proc.HasExited -or ((Get-Date) - $_.Started).TotalSeconds -gt ($total + 60) })) {
        if (-not $j.Proc.HasExited) { $j.Proc.Kill(); Start-Sleep 1 }
        $recName = [IO.Path]::GetFileNameWithoutExtension($j.G.Gam.Name) + '.rec'
        $rec = Join-Path $j.Run "GM\$($j.G.Name)\$recName"
        $err = (Get-Content (Join-Path $j.Run 'err.txt') -ErrorAction SilentlyContinue | Select-Object -First 2) -join ' | '
        $entries = 0
        if (Test-Path $rec) { Copy-Item $rec (Join-Path $Out "$($j.G.Name).rec") -Force; $entries = (Get-Item $rec).Length / 3 }
        $results += [pscustomobject]@{ Game = $j.G.Name; Exit = $(if ($j.Proc.HasExited) { $j.Proc.ExitCode } else { '?' }); Bindings = $j.Script.KeyCount; Presses = $j.Script.Presses; RecEvents = $entries; Error = $err }
        $running = @($running | Where-Object { $_ -ne $j })
    }
}
$results | Sort-Object Game | Format-Table -AutoSize | Out-String -Width 200
"recordings in $Out; work files in $Work"
