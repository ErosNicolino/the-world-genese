# Passeio de desempenho de verdade: da cidade ao continente a galope, com criaturas e HUD (1280x720, sem
# limite de FPS). Escreve o FPS, o pior 1%, os quadros longos e o tempo de cada sistema.
# -Packaged (08/11): o jogo empacotado de Tools\PackageAlpha.ps1 -Perf (o número de verdade: sem o custo do editor)
param([string]$Extra = '', [int]$TimeoutSeconds = 600, [int]$ResX = 1280, [int]$ResY = 720, [string]$Console = '', [switch]$Packaged)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$project = Join-Path $root 'Vortex.uproject'
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$log = Join-Path $root 'Saved\Logs\RoamBench.log'
Remove-Item -LiteralPath $log -ErrorAction SilentlyContinue
$cmds = 't.MaxFPS 0,r.VSync 0'
# -Console: comandos a mais (ex.: 'sg.ShadowQuality 3') para comparar qualidades
if ($Console) { $cmds += ',' + $Console }
if ($Packaged) { $editor = Join-Path $root 'Build\PerfTest\Windows\Vortex.exe'; $log = Join-Path $root 'Saved\Logs\RoamBenchPackaged.log'; Remove-Item -LiteralPath $log -ErrorAction SilentlyContinue }
$args = @($(if ($Packaged) { '' } else { "`"$project`"" }), '/Game/Vortex/Maps/L_TerrainProto', $(if ($Packaged) { '' } else { '-game' }), '-dx12', '-RenderOffScreen', '-unattended', '-windowed', "-ResX=$ResX", "-ResY=$ResY", '-ForceRes',
    '-VortexRoamBench', '-VortexNoInitialFade', '-nosound', '-NoSplash', "`"-ExecCmds=$cmds`"", "`"-ABSLOG=$log`"")
$args = @($args | Where-Object { $_ -ne '' })
if ($Extra) { $args += $Extra.Split(' ') }
$p = Start-Process -FilePath $editor -ArgumentList $args -WindowStyle Hidden -PassThru
if (!$p.WaitForExit($TimeoutSeconds * 1000)) { Stop-Process -Id $p.Id -Force; throw 'Roam bench timed out' }
Select-String -LiteralPath $log -Pattern 'VORTEX_ROAM|VORTEX_HITCH|VORTEX_SLOW' | ForEach-Object { $_.Line -replace '^.*LogTemp: (Display: )?', '' }
