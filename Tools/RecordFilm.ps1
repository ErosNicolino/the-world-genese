# Grava o passeio de câmera pela vila (-VortexRoamBench -VortexFilm, em VortexRoamBench.cpp). O jogo roda escondido
# (não usa a tela do PC) com passo fixo de 30 quadros por segundo e salva cada quadro em Saved\Film\frames; o ffmpeg
# junta tudo em Saved\Film\bruto.mp4 (1920x1080, placa NVIDIA). Demora mais que os 40 s do vídeo: não importa.
param([int]$TimeoutSeconds = 1200)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$out = Join-Path $root 'Saved\Film'
$frames = Join-Path $out 'frames'
New-Item -ItemType Directory -Force $frames | Out-Null
Get-ChildItem -LiteralPath $frames -Filter 'f_*.png' | Remove-Item
$video = Join-Path $out 'bruto.mp4'
$log = Join-Path $root 'Saved\Logs\Film.log'
Remove-Item -LiteralPath $video, $log -ErrorAction SilentlyContinue
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$args = @("`"$(Join-Path $root 'Vortex.uproject')`"", '/Game/Vortex/Maps/L_TerrainProto', '-game', '-dx12', '-RenderOffScreen', '-windowed',
    '-ResX=1920', '-ResY=1080', '-ForceRes', '-BENCHMARK', '-FPS=30', '-unattended', '-VortexRoamBench', '-VortexFilm',
    '-VortexNoInitialFade', '-nosound', '-NoSplash', "`"-ABSLOG=$log`"")
$game = Start-Process -FilePath $editor -ArgumentList $args -WindowStyle Hidden -PassThru
if (!$game.WaitForExit($TimeoutSeconds * 1000)) { Stop-Process -Id $game.Id -Force; throw 'O jogo passou do tempo' }
Select-String -LiteralPath $log -Pattern 'VORTEX_FILM' | ForEach-Object { $_.Line -replace '^.*LogTemp: (Display: )?', '' }
$count = @(Get-ChildItem -LiteralPath $frames -Filter 'f_*.png').Count
if ($count -lt 30) { throw "Só $count quadros (veja Saved\Logs\Film.log)" }
& ffmpeg -hide_banner -loglevel error -y -framerate 30 -i (Join-Path $frames 'f_%05d.png') -c:v h264_nvenc -preset p5 -cq 17 -pix_fmt yuv420p $video
"Vídeo: $video ($count quadros)"