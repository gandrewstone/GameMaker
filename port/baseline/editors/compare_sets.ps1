# For animated screens: every screen dump of the DOS run must also occur, byte for byte, among the dumps of the port run
# (the frames follow the clock, so the two runs show them at different moments).  usage: compare_sets.ps1 -Dos <dir> -Port <dir> [-Prefix A]
param([Parameter(Mandatory)][string]$Dos, [Parameter(Mandatory)][string]$Port, [string]$Prefix = '')
function Hashes($dir) {
  $h = @{}
  Get-ChildItem (Join-Path $dir 'run\GM') -Filter "$Prefix*.DMP" | ForEach-Object { $h[(Get-FileHash $_.FullName).Hash] = $_.Name }
  $h
}
$d = Hashes $Dos; $p = Hashes $Port
$missing = @($d.Keys | Where-Object { -not $p.ContainsKey($_) })
"DOS: {0} distinct frames, port: {1} distinct frames, DOS frames not seen in the port: {2}" -f $d.Count, $p.Count, $missing.Count
foreach ($m in $missing) { "   not in port: " + $d[$m] }
$onlyp = @($p.Keys | Where-Object { -not $d.ContainsKey($_) })
"port frames not seen in DOS: {0}" -f $onlyp.Count
