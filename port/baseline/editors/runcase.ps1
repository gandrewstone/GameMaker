# Runs one key script in the DOS editor (built from the source) and in the port and compares what they leave behind: the
# screen dumps and the data files (see README.md).
#   usage: runcase.ps1 -Name pal-edit -Program palchos -Steps 'key:Enter','text:sample.pal',... [-Delay 36] [-ProgramArgs 'a b']
#          [-Work <dir>] [-TextPalette] [-Shipped]
param(
    [Parameter(Mandatory)][string]$Name,
    [Parameter(Mandatory)][string]$Program,
    [Parameter(Mandatory)][string[]]$Steps,
    [int]$Delay = 36,
    [string]$Work = (Join-Path $env:TEMP 'gm_editors_cases'),
    [int]$TimeoutSec = 300,
    [string]$ProgramArgs = '',
    [string[]]$Mask,
    [string[]]$FileMask,
    [switch]$TextPalette,
    [switch]$Shipped
)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'keyscript.ps1')
New-Item -ItemType Directory -Force $Work | Out-Null
$stamp = Get-Date -Format 'HHmmss'
$script = Join-Path $Work "$Name.txt"
$all = @("delay:$Delay") + $Steps
if ($Steps -notcontains 'quit') { $all += 'delay:18'; $all += 'quit' }
(New-KeyScript -Steps $all -Delay $Delay) | Set-Content -Encoding ascii $script
$dos = Join-Path $Work "$Name-dos-$stamp"; $port = Join-Path $Work "$Name-port-$stamp"
& (Join-Path $PSScriptRoot 'run_dos.ps1') -Program $Program.ToUpper() -Script $script -Out $dos -TimeoutSec $TimeoutSec -ProgramArgs $ProgramArgs -Shipped:$Shipped | Out-Null
& (Join-Path $PSScriptRoot 'run_port.ps1') -Program $Program.ToLower() -Script $script -Out $port -TimeoutSec $TimeoutSec -ProgramArgs $ProgramArgs | Out-Null
"== $Name ($Program)"
$res = & (Join-Path $PSScriptRoot 'compare.ps1') -Dos $dos -Port $port -TextPalette:$TextPalette -Mask $Mask -FileMask $FileMask
$res
"   dos: $dos"
"   port: $port"
