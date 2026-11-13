# Grava um teste que joga sozinho como filmagem do trailer: roda o teste com -VortexDumpFrames (VortexRoamBench.cpp), passo
# fixo de 30 quadros por segundo e 1920x1080, escondido; cada quadro (sem a HUD) vira Saved\Film\clips\<Name>.mp4.
# Ex.: Tools\RecordGameplay.ps1 -Name chefe -Flag VortexBossShots
param([Parameter(Mandatory)][string]$Name, [Parameter(Mandatory)][string]$Flag, [string]$Map = '/Game/Vortex/Maps/L_TerrainProto', [int]$TimeoutSeconds = 1800)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$film = Join-Path $root 'Saved\Film'
$frames = Join-Path $film $Name
$clips = Join-Path $film 'clips'
New-Item -ItemType Directory -Force $clips | Out-Null
if (Test-Path $frames) { Remove-Item -LiteralPath $frames -Recurse -Force }
$log = Join-Path $root "Saved\Logs\Gameplay_$Name.log"
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$args = @("`"$(Join-Path $root 'Vortex.uproject')`"", $Map, '-game', '-dx12', '-RenderOffScreen', '-windowed', '-ResX=1920', '-ResY=1080', '-ForceRes',
    '-BENCHMARK', '-FPS=30', '-unattended', "-$Flag", "-VortexDumpFrames=$Name", '-VortexNoInitialFade', '-nosound', '-NoSplash', "`"-ABSLOG=$log`"")
$game = Start-Process -FilePath $editor -ArgumentList $args -WindowStyle Hidden -PassThru
if (!$game.WaitForExit($TimeoutSeconds * 1000)) { Stop-Process -Id $game.Id -Force; Write-Warning 'O teste passou do tempo' }
$n = @(Get-ChildItem -LiteralPath $frames -Filter 'f_*.png' -ErrorAction SilentlyContinue).Count
if ($n -lt 10) { throw "Sem quadros (veja $log)" }
# um quadro que falhou deixa um buraco na numeração e o ffmpeg pararia nele: renumera em ordem
$i = 0; Get-ChildItem -LiteralPath $frames -Filter 'f_*.png' | Sort-Object Name | ForEach-Object { Rename-Item -LiteralPath $_.FullName ('g_{0:D5}.png' -f $i++) }
$out = Join-Path $clips "$Name.mp4"
& ffmpeg -hide_banner -loglevel error -y -framerate 30 -i (Join-Path $frames 'g_%05d.png') -c:v h264_nvenc -preset p5 -cq 18 -pix_fmt yuv420p $out
if ($LASTEXITCODE -eq 0) { Remove-Item -LiteralPath $frames -Recurse -Force }
"$Name`: $n quadros -> $out"