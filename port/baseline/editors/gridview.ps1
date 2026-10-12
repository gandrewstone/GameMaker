# Draws a mode 13h DMP dump 4x with a coordinate grid, to pick positions for mouse scripts:  gridview.ps1 <dump> <png> [-Step 20]
param([Parameter(Mandatory)][string]$Dump, [Parameter(Mandatory)][string]$Out, [int]$Step = 20)
Add-Type -AssemblyName System.Drawing
$b = [IO.File]::ReadAllBytes($Dump); $bmp = New-Object System.Drawing.Bitmap 320, 200
for ($y = 0; $y -lt 200; $y++) { for ($x = 0; $x -lt 320; $x++) { $c = $b[769 + $y * 320 + $x]; $bmp.SetPixel($x, $y, [System.Drawing.Color]::FromArgb([int]($b[1 + $c * 3] * 255 / 63), [int]($b[2 + $c * 3] * 255 / 63), [int]($b[3 + $c * 3] * 255 / 63))) } }
$big = New-Object System.Drawing.Bitmap 1280, 800; $g = [System.Drawing.Graphics]::FromImage($big); $g.InterpolationMode = 'NearestNeighbor'; $g.PixelOffsetMode = 'Half'; $g.DrawImage($bmp, 0, 0, 1280, 800)
$pen = New-Object System.Drawing.Pen ([System.Drawing.Color]::FromArgb(90, 255, 255, 255)); $f = New-Object System.Drawing.Font 'Arial', 9
for ($x = 0; $x -lt 320; $x += $Step) { $g.DrawLine($pen, $x * 4, 0, $x * 4, 800); $g.DrawString("$x", $f, [System.Drawing.Brushes]::Yellow, $x * 4 + 1, 1) }
for ($y = 0; $y -lt 200; $y += $Step) { $g.DrawLine($pen, 0, $y * 4, 1280, $y * 4); $g.DrawString("$y", $f, [System.Drawing.Brushes]::Yellow, 1, $y * 4 + 1) }
$big.Save($Out)
