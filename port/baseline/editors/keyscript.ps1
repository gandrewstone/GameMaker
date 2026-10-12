# Key script generator for the editors oracle (format: see KEYS.C). Dot-source it.
#
#   $s = New-KeyScript -Steps 'key:Enter','text:sample.pal','key:Enter','dump:menu1','quit' [-Delay 36]
#
# Step kinds: key:<Enter|Esc|Up|Down|Left|Right|Backspace|Tab|Home|End|F1..F10|PgUp|PgDn|Space|Delete|Insert>, text:<chars>,
#             shift:<on|off>, mouse:<dx>,<dy>,<buttons> (move by cursor units, buttons 1 left 2 right 4 middle), dump:<name up to 8 chars>, quit, and "delay:<ticks>" which sets the delay of the following steps.
# Delays are in BIOS ticks (18.2 per second) from the moment the previous step happened.

$script:SpecialKeys = @{
  'Enter' = 0x1C0D; 'Esc' = 0x011B; 'Up' = 0x4800; 'Down' = 0x5000; 'Left' = 0x4B00; 'Right' = 0x4D00; 'Backspace' = 0x0E08;
  'Tab' = 0x0F09; 'Home' = 0x4700; 'End' = 0x4F00; 'PgUp' = 0x4900; 'PgDn' = 0x5100; 'Space' = 0x3920; 'Delete' = 0x5300; 'Insert' = 0x5200;
  'F1' = 0x3B00; 'F2' = 0x3C00; 'F3' = 0x3D00; 'F4' = 0x3E00; 'F5' = 0x3F00; 'F6' = 0x4000; 'F7' = 0x4100; 'F8' = 0x4200; 'F9' = 0x4300; 'F10' = 0x4400
}
$script:CharScan = @{}
'qwertyuiop'.ToCharArray() | ForEach-Object -Begin { $i = 0x10 } -Process { $script:CharScan[[string]$_] = $i++ }
'asdfghjkl'.ToCharArray()  | ForEach-Object -Begin { $i = 0x1E } -Process { $script:CharScan[[string]$_] = $i++ }
'zxcvbnm'.ToCharArray()    | ForEach-Object -Begin { $i = 0x2C } -Process { $script:CharScan[[string]$_] = $i++ }
'1234567890'.ToCharArray() | ForEach-Object -Begin { $i = 0x02 } -Process { $script:CharScan[[string]$_] = $i++ }
$script:CharScan['.'] = 0x34; $script:CharScan['\'] = 0x2B; $script:CharScan['-'] = 0x0C; $script:CharScan[' '] = 0x39; $script:CharScan['/'] = 0x35

function New-KeyScript([string[]]$Steps, [int]$Delay = 36) {
  $lines = New-Object System.Collections.Generic.List[string]
  foreach ($st in $Steps) {
    $kind, $arg = $st -split ':', 2
    switch ($kind) {
      'delay' { $Delay = [int]$arg }
      'key' {
        if (-not $script:SpecialKeys.ContainsKey($arg)) { throw "unknown key $arg" }
        $lines.Add(('K {0} {1:X4}' -f $Delay, $script:SpecialKeys[$arg]))
      }
      'text' {
        $first = $true
        foreach ($ch in $arg.ToCharArray()) {
          $lower = [string][char]::ToLower($ch)
          if (-not $script:CharScan.ContainsKey($lower)) { throw "no scan code for '$ch'" }
          $d = if ($first) { $Delay } else { 4 }      # typing is quick once the prompt is up
          $lines.Add(('K {0} {1:X2}{2:X2}' -f $d, $script:CharScan[$lower], [int]$ch))
          $first = $false
        }
      }
      'shift' { $lines.Add(('S {0} {1:X2}' -f $Delay, $(if ($arg -eq 'on') { 3 } else { 0 }))) }
      'dump' { $lines.Add(('D {0} {1}.DMP' -f $Delay, $arg.ToUpper())) }
      'mouse' { $m = $arg -split ','; $lines.Add(('M {0} {1} {2} {3}' -f $Delay, [int]$m[0], [int]$m[1], [int]$m[2])) }
      'font' { $lines.Add(('F {0} {1}.BIN' -f $Delay, $arg.ToUpper())) }
      'quit' { $lines.Add(('Q {0}' -f $Delay)) }
      default { throw "unknown step $st" }
    }
  }
  return ($lines -join "`r`n") + "`r`n"
}
