# Lists the cells where two text-mode DMP dumps differ: showdiff.ps1 <dos.dmp> <port.dmp> [-Max 20]
param([Parameter(Mandatory)][string]$DosFile, [Parameter(Mandatory)][string]$PortFile, [int]$Max = 20)
$a = [IO.File]::ReadAllBytes($DosFile); $b = [IO.File]::ReadAllBytes($PortFile); $n = 0
for ($c = 0; $c -lt 2000; $c++) {
  $o = 769 + 2 * $c
  if ($a[$o] -ne $b[$o] -or $a[$o + 1] -ne $b[$o + 1]) {
    $n++
    if ($n -le $Max) { "col {0,2} row {1,2}: dos '{2}' attr {3:X2}   port '{4}' attr {5:X2}" -f ($c % 80), [Math]::Floor($c / 80), $(if ($a[$o] -ge 32 -and $a[$o] -lt 127) { [char]$a[$o] } else { '#' + $a[$o] }), $a[$o + 1], $(if ($b[$o] -ge 32 -and $b[$o] -lt 127) { [char]$b[$o] } else { '#' + $b[$o] }), $b[$o + 1] }
  }
}
"$n cells differ"
