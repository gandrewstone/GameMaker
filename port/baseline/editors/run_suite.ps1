# Runs the cases of suite.ps1 and prints one result line per case.
#   usage: run_suite.ps1 [-Only name,name] [-Work <dir>]
param([string[]]$Only, [string]$Work = (Join-Path $env:TEMP 'gm_editors_cases'))
. (Join-Path $PSScriptRoot 'suite.ps1')
$cases = Get-Cases | Where-Object { -not $Only -or $Only -contains $_.Name }
$summary = @()
foreach ($c in $cases) {
  $args = @{ Name = $c.Name; Program = $c.Program; Steps = $c.Steps; Work = $Work }
  if ($c.Args) { $args.ProgramArgs = $c.Args }
  if ($c.Mask) { $args.Mask = $c.Mask }
  if ($c.FileMask) { $args.FileMask = $c.FileMask }
  $out = & (Join-Path $PSScriptRoot 'runcase.ps1') @args 2>&1
  $text = ($out | Out-String).Trim()
  $last = ($out | Where-Object { $_ -match '^(all identical|\d+ difference)' } | Select-Object -Last 1)
  "{0,-10} {1}" -f $c.Name, $last
  ($out | Where-Object { $_ -notmatch '^(==|   dos:|   port:|all identical)' -and $_ -notmatch ': (text|graphics), 0 bytes of screen differ' -and $_ -notmatch ': identical' }) | ForEach-Object { "           $_" }
}
