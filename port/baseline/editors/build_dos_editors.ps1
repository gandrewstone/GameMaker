# Rebuilds the DOS editors from the repository source with the bundled Borland C++ inside DOSBox, in a scratch copy:
# the oracle for the port, which is built from the same source. CRIPPLEWARE (the demo restrictions of GMGEN.H) is switched
# off in the copy, as the port does under GM_PORT. The programs are left in <Work>\dosbuild\ (run_dos.ps1 uses them).
#   usage: build_dos_editors.ps1 [-Work <dir>] [-Programs palchos,blocedit,...] [-DosBox <exe>]
param(
    [string]$Work = (Join-Path $env:TEMP 'gm_editors_oracle'),
    [string[]]$Programs = @('palchos', 'blocedit', 'mapmaker', 'monedit', 'charedit', 'image', 'grator', 'sndedit', 'utility', 'menu'),
    [string]$DosBox = 'C:\Program Files (x86)\DOSBox-0.74-3\DOSBox.exe'
)
$ErrorActionPreference = 'Stop'
$root = Resolve-Path (Join-Path $PSScriptRoot '..\..\..')
New-Item -ItemType Directory -Force $Work | Out-Null
$b = Join-Path $Work 'dosbuild_src'
if (Test-Path $b) { Remove-Item -Recurse -Force $b }
New-Item -ItemType Directory -Force $b | Out-Null
Copy-Item -Recurse (Join-Path $root 'code') (Join-Path $b 'code')
Copy-Item -Recurse (Join-Path $root 'tools') (Join-Path $b 'tools')
$gm = Join-Path $b 'code\GM'
# no demo restrictions (the shipped programs in runtime\run\GM are the full version)
$h = [IO.File]::ReadAllText((Join-Path $gm 'GMGEN.H'), [Text.Encoding]::GetEncoding(437))
$h2 = $h -replace '(?m)^#ifndef GM_PORT[^\r\n]*\r\n#define CRIPPLEWARE\r\n#endif', '// CRIPPLEWARE off for the oracle'
if ($h2 -eq $h) { throw 'could not switch CRIPPLEWARE off in GMGEN.H' }
[IO.File]::WriteAllText((Join-Path $gm 'GMGEN.H'), $h2, [Text.Encoding]::GetEncoding(437))
# KEYS dumps the screen from the timer interrupt when a program only waits for the mouse; the software cursor is erased and drawn
# again from a timer callback, and an interrupt in the middle of that would catch the screen without a cursor. In this copy the
# callbacks run with interrupts off (the port runs them on its main thread, so they are atomic there anyway).
$tm = Join-Path $gm 'TRANMOUS.HPP'
$tt = [IO.File]::ReadAllText($tm, [Text.Encoding]::GetEncoding(437))
$tt2 = $tt.Replace('    Time.Traverse(CallEm);', '    disable(); Time.Traverse(CallEm); enable();')
if ($tt2 -eq $tt) { throw 'could not patch TRANMOUS.HPP' }
[IO.File]::WriteAllText($tm, $tt2, [Text.Encoding]::GetEncoding(437))
Get-ChildItem $gm -Filter *.OBJ | Remove-Item

$calls = ($Programs | ForEach-Object { "call create $_" }) -join "`r`n"
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
$calls
exit
"@ | Set-Content (Join-Path $b 'build.conf') -Encoding ascii

Push-Location $b
try {
    $p = Start-Process $DosBox -ArgumentList '-conf', 'build.conf', '-noconsole' -PassThru
    if (-not $p.WaitForExit(1800000)) { $p.Kill(); throw 'DOSBox build timed out' }
} finally { Pop-Location }

$out = Join-Path $Work 'dosbuild'
if (Test-Path $out) { Remove-Item -Recurse -Force $out }
New-Item -ItemType Directory -Force $out | Out-Null
foreach ($pr in $Programs) {
    $exe = Join-Path $gm ($pr.ToUpper() + '.EXE')
    if (Test-Path $exe) { Copy-Item $exe $out; "built {0} ({1} bytes)" -f $pr.ToUpper(), (Get-Item $exe).Length }
    else { Write-Warning "$pr failed:"; Get-Content (Join-Path $gm ($pr.ToUpper() + '.LOG')) -ErrorAction SilentlyContinue | Select-Object -Last 12 }
}
