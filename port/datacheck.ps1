# Looks for damaged or truncated game data under cd\ and runtime\ (the engine reads these files without checking).
#   GIF  must end with the 0x3B trailer          FLI  header size field must equal the file size
#   VOC  blocks must run to a terminator/the end  CMF  must end with the end-of-track event FF 2F 00
#   fixed-size types (.map .bbl .mbl .cbl .chr .mon .pal): reported when the size differs from the most common one
# usage: datacheck.ps1 [-Root <dir>] [-Detail]
param(
    [string]$Root = (Resolve-Path (Join-Path $PSScriptRoot '..')),
    [switch]$Detail
)
$files = Get-ChildItem (Join-Path $Root 'cd'), (Join-Path $Root 'runtime') -Recurse -File
$problems = New-Object System.Collections.Generic.List[object]
function Rel($f) { $f.FullName.Substring($Root.Length + 1) }
function Flag($f, $why) { $problems.Add([pscustomobject]@{ File = (Rel $f); Size = $f.Length; Problem = $why }) }

foreach ($f in $files) {
    switch ($f.Extension.ToLower()) {
        '.gif' {
            # walk the block structure; padding after the 0x3B trailer (zeros, ^Z) is normal and ignored
            $b = [IO.File]::ReadAllBytes($f.FullName)
            if ($b.Length -lt 14 -or [Text.Encoding]::ASCII.GetString($b, 0, 3) -ne 'GIF') { Flag $f 'not a GIF'; break }
            $p = 13
            if ($b[10] -band 0x80) { $p += 3 * (2 -shl ($b[10] -band 7)) }
            $state = 'truncated'
            while ($p -lt $b.Length) {
                $t = $b[$p]
                if ($t -eq 0x3B) { $state = 'ok'; break }
                if ($t -eq 0x21) { $p += 2 }
                elseif ($t -eq 0x2C) {
                    if ($p + 10 -gt $b.Length) { break }
                    $flags = $b[$p + 9]; $p += 10
                    if ($flags -band 0x80) { $p += 3 * (2 -shl ($flags -band 7)) }
                    $p += 1                                   # LZW minimum code size
                }
                else { $state = 'garbage'; break }
                while ($p -lt $b.Length -and $b[$p] -ne 0) { $p += 1 + $b[$p] }   # sub-blocks
                $p++
            }
            if ($state -eq 'truncated') { Flag $f 'GIF ends before its trailer (truncated)' }
            elseif ($state -eq 'garbage') { Flag $f 'GIF has an unknown block' }
        }
        '.fli' {
            $b = [IO.File]::ReadAllBytes($f.FullName)
            $hdr = [BitConverter]::ToUInt32($b, 0)
            if ([BitConverter]::ToUInt16($b, 4) -ne 0xAF11) { Flag $f 'not a FLI'; break }
            if ($hdr -ne $b.Length) { Flag $f "FLI header says $hdr bytes, file has $($b.Length)" }
        }
        '.voc' {
            $b = [IO.File]::ReadAllBytes($f.FullName)
            if ($b.Length -lt 26 -or [Text.Encoding]::ASCII.GetString($b, 0, 19) -ne 'Creative Voice File') { Flag $f 'not a VOC'; break }
            $p = [BitConverter]::ToUInt16($b, 20); $ok = $false
            while ($p -lt $b.Length) {
                if ($b[$p] -eq 0) { $ok = $true; break }          # terminator: a single zero byte, no length
                if ($p + 4 -gt $b.Length) { break }
                $len = $b[$p + 1] + 256 * $b[$p + 2] + 65536 * $b[$p + 3]
                if ($p + 4 + $len -gt $b.Length) { Flag $f "VOC block runs $($p + 4 + $len - $b.Length) bytes past the end"; $ok = $true; break }
                $p += 4 + $len
            }
            if (-not $ok -and $p -ne $b.Length) { Flag $f 'VOC ends mid-block' }
        }
        '.cmf' {
            $b = [IO.File]::ReadAllBytes($f.FullName)
            if ($b.Length -lt 44 -or [Text.Encoding]::ASCII.GetString($b, 0, 4) -ne 'CTMF') { Flag $f 'not a CMF'; break }
            if ([BitConverter]::ToUInt16($b, 8) -ge $b.Length) { Flag $f 'CMF music block starts past the end'; break }
            if (-not ($b[$b.Length - 3] -eq 0xFF -and $b[$b.Length - 2] -eq 0x2F -and $b[$b.Length - 1] -eq 0)) { Flag $f 'CMF does not end with FF 2F 00 (truncated?)' }
        }
    }
}

# fixed-size formats: flag sizes that differ from the dominant one
$fixed = '.map', '.bbl', '.mbl', '.cbl', '.chr', '.mon', '.pal'
foreach ($ext in $fixed) {
    $set = @($files | Where-Object { $_.Extension.ToLower() -eq $ext })
    if ($set.Count -lt 2) { continue }
    $dominant = ($set | Group-Object Length | Sort-Object Count -Descending | Select-Object -First 1).Name
    foreach ($f in $set | Where-Object { $_.Length -ne [int]$dominant }) { Flag $f "size differs from the usual $ext size ($dominant)" }
}

Write-Output ("{0} files checked, {1} problems" -f $files.Count, $problems.Count)
if ($problems.Count) {
    $problems | Group-Object { ($_.Problem -replace '\d+', 'N') } | Sort-Object Count -Descending |
        ForEach-Object { "{0,4}  {1}" -f $_.Count, $_.Name }
    if ($Detail) { $problems | Sort-Object File | Format-Table -AutoSize | Out-String -Width 200 }
}
$problems
