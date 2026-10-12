# Compares what a DOS run (run_dos.ps1) and a port run (run_port.ps1) of the same key script left behind: every .DMP screen
# dump (mode, palette, screen contents) and every data file that is not the same as in the shipped data folder.
#   usage: compare.ps1 -Dos <dos out dir> -Port <port out dir> [-TextPalette]   (text mode palettes differ by design, so they are
#          only compared with -TextPalette). -Mask 'x1,y1,x2,y2' (repeatable) leaves out screen regions that animate with the clock.
#          -FileMask 'EXT:recordsize:from-to,from-to' (repeatable) leaves out bytes of a data file that hold animation state at the moment of saving (offsets inside each record).
param([Parameter(Mandatory)][string]$Dos, [Parameter(Mandatory)][string]$Port, [switch]$TextPalette, [string[]]$Mask, [string[]]$FileMask)
$d = Join-Path $Dos 'run\GM'; $p = Join-Path $Port 'run\GM'
$root = Resolve-Path (Join-Path $PSScriptRoot '..\..\..')
$orig = Join-Path $root 'runtime\run\GM'
$bad = 0
function Rel($base, $f) { $f.FullName.Substring($base.Length).TrimStart('\') }

$dumps = Get-ChildItem $d -Filter *.DMP | Sort-Object Name
foreach ($f in $dumps) {
  $o = Join-Path $p $f.Name
  if (-not (Test-Path $o)) { "{0}: missing in the port run" -f $f.Name; $bad++; continue }
  $a = [IO.File]::ReadAllBytes($f.FullName); $b = [IO.File]::ReadAllBytes($o)
  if ($a.Length -ne $b.Length -or $a[0] -ne $b[0]) { "{0}: different mode/size ({1} vs {2} bytes, mode {3} vs {4})" -f $f.Name, $a.Length, $b.Length, $a[0], $b[0]; $bad++; continue }
  $gfx = $a[0] -eq 0x13
  $palDiff = 0; for ($i = 1; $i -lt 769; $i++) { if ($a[$i] -ne $b[$i]) { $palDiff++ } }
  $diff = New-Object System.Collections.Generic.List[int]
  $masks = @($Mask | Where-Object { $_ } | ForEach-Object { , ([int[]]($_ -split ',')) })
  for ($i = 769; $i -lt $a.Length; $i++) {
    if ($a[$i] -ne $b[$i]) {
      $o = $i - 769
      if ($gfx -and $masks.Count) { $px = $o % 320; $py = [Math]::Floor($o / 320); $skip = $false; foreach ($m in $masks) { if ($px -ge $m[0] -and $px -le $m[2] -and $py -ge $m[1] -and $py -le $m[3]) { $skip = $true; break } }; if ($skip) { continue } }
      $diff.Add($o)
    }
  }
  $msg = "{0}: {1}, {2} bytes of screen differ" -f $f.Name, $(if ($gfx) { 'graphics' } else { 'text' }), $diff.Count
  if ($gfx -or $TextPalette) { $msg += ", palette bytes differing: $palDiff"; if ($palDiff) { $bad++ } }
  if ($diff.Count) {
    $bad++
    if ($gfx) {
      $rows = $diff | ForEach-Object { [int][Math]::Floor($_ / 320) } | Select-Object -Unique
      $msg += " (rows {0}..{1}, first at x={2} y={3})" -f ($rows | Measure-Object -Minimum).Minimum, ($rows | Measure-Object -Maximum).Maximum, ($diff[0] % 320), [Math]::Floor($diff[0] / 320)
    } else {
      $cells = $diff | ForEach-Object { [int][Math]::Floor($_ / 2) } | Select-Object -Unique
      $msg += " (first at col {0} row {1})" -f ($cells[0] % 80), [Math]::Floor($cells[0] / 80)
    }
  }
  $msg
}

# data files: those that differ from the shipped folder in either run
$files = @{}
foreach ($base in $d, $p) {
  Get-ChildItem $base -Recurse -File | Where-Object { $_.Extension -notmatch '^\.(EXE|DMP|LOG|OUT)$' -and $_.Name -notmatch '^(SCRIPT\.TXT|KEYS\.|gm\.cfg)' } | ForEach-Object {
    $r = Rel $base $_
    $ofile = Join-Path $orig $r
    if (-not (Test-Path $ofile) -or (Get-FileHash $_.FullName).Hash -ne (Get-FileHash $ofile).Hash) { $files[$r.ToUpper()] = 1 }
  }
}
foreach ($r in ($files.Keys | Sort-Object)) {
  $fa = Get-ChildItem $d -Recurse -File | Where-Object { (Rel $d $_).ToUpper() -eq $r } | Select-Object -First 1
  $fb = Get-ChildItem $p -Recurse -File | Where-Object { (Rel $p $_).ToUpper() -eq $r } | Select-Object -First 1
  if (-not $fa) { "{0}: only in the port run ({1} bytes)" -f $r, $fb.Length; $bad++ }
  elseif (-not $fb) { "{0}: only in the DOS run ({1} bytes)" -f $r, $fa.Length; $bad++ }
  elseif ((Get-FileHash $fa.FullName).Hash -eq (Get-FileHash $fb.FullName).Hash) {
    $note = ''
    if ($r -match '^(.*)X(\.[A-Z]+)$') {            # the test scripts save as <name>X.<ext>: say how much it differs from <name>.<ext>
      $src = Join-Path $orig ($Matches[1] + $Matches[2])
      if (Test-Path $src) { $o2 = [IO.File]::ReadAllBytes($src); $x = [IO.File]::ReadAllBytes($fa.FullName); $n = 0; for ($i = 0; $i -lt [Math]::Min($o2.Length, $x.Length); $i++) { if ($o2[$i] -ne $x[$i]) { $n++ } }; $note = ", $n bytes differ from the shipped file" }
    }
    "{0}: identical ({1} bytes{2})" -f $r, $fa.Length, $note
  }
  else {
    $x = [IO.File]::ReadAllBytes($fa.FullName); $y = [IO.File]::ReadAllBytes($fb.FullName)
    $ext = [IO.Path]::GetExtension($r).TrimStart('.').ToUpper(); $ranges = @(); $rec = 0
    foreach ($fm in $FileMask) { $parts = $fm -split ':'; if ($parts[0].ToUpper() -eq $ext) { $rec = [int]$parts[1]; $ranges = @($parts[2] -split ',' | ForEach-Object { $ab = $_ -split '-'; , @([int]$ab[0], [int]$ab[$ab.Count - 1]) }) } }
    $n = 0; $first = -1; $ignored = 0
    if ($x.Length -eq $y.Length) {
      for ($i = 0; $i -lt $x.Length; $i++) {
        if ($x[$i] -ne $y[$i]) {
          $inside = $false
          if ($rec) { $o = $i % $rec; foreach ($rg in $ranges) { if ($o -ge $rg[0] -and $o -le $rg[1]) { $inside = $true; break } } }
          if ($inside) { $ignored++ } else { $n++; if ($first -lt 0) { $first = $i } }
        }
      }
      if ($n -eq 0) { "{0}: identical apart from {1} bytes of animation state" -f $r, $ignored; continue }
    } else { $n = [Math]::Abs($x.Length - $y.Length); $first = 0 }
    "{0}: DIFFERENT ({1} vs {2} bytes, {3} bytes differ, first at offset {4})" -f $r, $x.Length, $y.Length, $n, $first; $bad++
  }
}
if ($bad -eq 0) { "all identical" } else { "$bad difference(s)" }
