# Dumps the ROM 8x8 font that DOSBox shows to <Work>\ROMFONT.BIN. run_port.ps1 gives it to the port (GM_ROMFONT) so that screens
# drawn with the ROM font can be compared pixel for pixel; the port's own font table (Unscii) is not the IBM one.
param([string]$Work = (Join-Path $env:TEMP 'gm_editors_oracle'))
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'keyscript.ps1')
$t = Join-Path $Work 'font_script.txt'
(New-KeyScript -Steps 'delay:36', 'font:rom', 'quit') | Set-Content -Encoding ascii $t
$o = Join-Path $Work ('font_run_' + (Get-Date -Format HHmmss))
& (Join-Path $PSScriptRoot 'run_dos.ps1') -Program PALCHOS -Script $t -Out $o -TimeoutSec 60 | Out-Null
Copy-Item (Join-Path $o 'run\GM\ROM.BIN') (Join-Path $Work 'ROMFONT.BIN') -Force
"{0} bytes -> {1}" -f (Get-Item (Join-Path $Work 'ROMFONT.BIN')).Length, (Join-Path $Work 'ROMFONT.BIN')
