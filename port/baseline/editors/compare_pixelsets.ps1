# For animated screens: for every pixel, the set of colours it takes over a series of dumps (<Prefix>*.DMP) must be the same in the DOS and
# the port run. Reports the pixels where the sets differ.   usage: compare_pixelsets.ps1 -Dos <dir> -Port <dir> -Prefix A
param([Parameter(Mandatory)][string]$Dos, [Parameter(Mandatory)][string]$Port, [string]$Prefix = '')
function Sets($dir) {
  $flags = New-Object byte[] (64000 * 256)
  foreach ($f in Get-ChildItem (Join-Path $dir 'run\GM') -Filter "$Prefix*.DMP") {
    $b = [IO.File]::ReadAllBytes($f.FullName)
    for ($i = 0; $i -lt 64000; $i++) { $flags[$i * 256 + $b[769 + $i]] = 1 }
  }
  , $flags
}
$a = Sets $Dos; $b = Sets $Port
$diff = New-Object System.Collections.Generic.List[int]
for ($i = 0; $i -lt 64000; $i++) {
  $same = $true
  for ($c = 0; $c -lt 256; $c++) { if ($a[$i * 256 + $c] -ne $b[$i * 256 + $c]) { $same = $false; break } }
  if (-not $same) { $diff.Add($i) }
}
"pixels whose colour sets differ: {0} of 64000" -f $diff.Count
if ($diff.Count) { $ys = $diff | ForEach-Object { [int][Math]::Floor($_ / 320) }; "rows {0}..{1}; first at x={2} y={3}" -f ($ys | Measure-Object -Minimum).Minimum, ($ys | Measure-Object -Maximum).Maximum, ($diff[0] % 320), [Math]::Floor($diff[0] / 320) }
