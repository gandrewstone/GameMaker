# Renders mode 13h DMP dumps to a PNG: DOS | port | difference (red where the pixels differ), each 320x200 scaled x2.
#   usage: dumpview.ps1 -DosFile <dos.dmp> -PortFile <port.dmp> -Out <file.png> [-Crop x,y,w,h]
param([Parameter(Mandatory)][string]$DosFile, [Parameter(Mandatory)][string]$PortFile, [Parameter(Mandatory)][string]$Out, [int[]]$Crop = @(0, 0, 320, 200))
Add-Type -AssemblyName System.Drawing
function Frame($bytes) {
  $bmp = New-Object System.Drawing.Bitmap 320, 200
  for ($y = 0; $y -lt 200; $y++) { for ($x = 0; $x -lt 320; $x++) {
    $c = $bytes[769 + $y * 320 + $x]
    $r = [int]($bytes[1 + $c * 3] * 255 / 63); $g = [int]($bytes[2 + $c * 3] * 255 / 63); $b = [int]($bytes[3 + $c * 3] * 255 / 63)
    $bmp.SetPixel($x, $y, [System.Drawing.Color]::FromArgb($r, $g, $b))
  } }
  $bmp
}
$a = [IO.File]::ReadAllBytes($DosFile); $b = [IO.File]::ReadAllBytes($PortFile)
$fa = Frame $a; $fb = Frame $b
$fd = New-Object System.Drawing.Bitmap 320, 200
for ($y = 0; $y -lt 200; $y++) { for ($x = 0; $x -lt 320; $x++) {
  if ($a[769 + $y * 320 + $x] -ne $b[769 + $y * 320 + $x]) { $fd.SetPixel($x, $y, [System.Drawing.Color]::Red) } else { $fd.SetPixel($x, $y, [System.Drawing.Color]::FromArgb(30, 30, 30)) }
} }
$cx, $cy, $cw, $ch = $Crop
$scale = if ($cw -le 160) { 4 } else { 2 }
$sheet = New-Object System.Drawing.Bitmap ($cw * $scale * 3 + 20), ($ch * $scale)
$g = [System.Drawing.Graphics]::FromImage($sheet)
$g.InterpolationMode = 'NearestNeighbor'; $g.PixelOffsetMode = 'Half'
$i = 0
foreach ($f in $fa, $fb, $fd) {
  $g.DrawImage($f, [System.Drawing.Rectangle]::new($i * ($cw * $scale + 10), 0, $cw * $scale, $ch * $scale), [System.Drawing.Rectangle]::new($cx, $cy, $cw, $ch), 'Pixel')
  $i++
}
$sheet.Save($Out)
