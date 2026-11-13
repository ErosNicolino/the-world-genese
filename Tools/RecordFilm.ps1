# Grava as tomadas de câmera de Data\Film\<Shots>.json (-VortexRoamBench -VortexFilm=<Shots>, em VortexRoamBench.cpp). O jogo
# roda escondido (não usa a tela) com passo fixo de 30 quadros por segundo e salva cada quadro, sem a HUD, em
# Saved\Film\<tomada>\; no fim cada tomada vira Saved\Film\clips\<tomada>.mp4 (1920x1080) e os PNG são apagados.
# -Only nome1,nome2: só essas tomadas (as outras ficam como estão). -Extra '-VortexQuietWorld -VortexNoWildlife': opções a mais do jogo.
param([string]$Shots = 'trailer', [int]$TimeoutSeconds = 3600, [string]$Only = '', [string]$Extra = '')
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$film = Join-Path $root 'Saved\Film'
$clips = Join-Path $film 'clips'
New-Item -ItemType Directory -Force $clips | Out-Null
$list = (Get-Content -Raw -Encoding UTF8 (Join-Path $root "Data\Film\$Shots.json") | ConvertFrom-Json).Shots
if ($Only) {
    # um JSON só com as escolhidas, ao lado do original
    $keep = $Only.Split(',')
    $list = @($list | Where-Object { $keep -contains $_.Name })
    $Shots = "$Shots`_parcial"
    @{ Shots = $list } | ConvertTo-Json -Depth 5 | Set-Content -Encoding UTF8 (Join-Path $root "Data\Film\$Shots.json")
}
foreach ($s in $list) { $d = Join-Path $film $s.Name; if (Test-Path $d) { Remove-Item -LiteralPath $d -Recurse -Force } }
$log = Join-Path $root 'Saved\Logs\Film.log'
Remove-Item -LiteralPath $log -ErrorAction SilentlyContinue
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$args = @("`"$(Join-Path $root 'Vortex.uproject')`"", '/Game/Vortex/Maps/L_TerrainProto', '-game', '-dx12', '-RenderOffScreen', '-windowed',
    '-ResX=1920', '-ResY=1080', '-ForceRes', '-BENCHMARK', '-FPS=30', '-unattended', '-VortexRoamBench', "-VortexFilm=$Shots",
    '-VortexNoInitialFade', '-nosound', '-NoSplash', "`"-ABSLOG=$log`"") + @($Extra.Split(' ') | Where-Object { $_ })
$game = Start-Process -FilePath $editor -ArgumentList $args -WindowStyle Hidden -PassThru
if (!$game.WaitForExit($TimeoutSeconds * 1000)) { Stop-Process -Id $game.Id -Force; Write-Warning 'O jogo passou do tempo' }
Select-String -LiteralPath $log -Pattern 'VORTEX_FILM' | ForEach-Object { $_.Line -replace '^.*LogTemp: (Display: |Error: )?', '' }
foreach ($s in $list) {
    $d = Join-Path $film $s.Name
    $n = @(Get-ChildItem -LiteralPath $d -Filter 'f_*.png' -ErrorAction SilentlyContinue).Count
    if ($n -lt 10) { "$($s.Name): sem quadros"; continue }
    # um quadro que falhou deixa um buraco na numeração e o ffmpeg pararia nele: renumera em ordem
    $i = 0; Get-ChildItem -LiteralPath $d -Filter 'f_*.png' | Sort-Object Name | ForEach-Object { Rename-Item -LiteralPath $_.FullName ('g_{0:D5}.png' -f $i++) }
    $out = Join-Path $clips "$($s.Name).mp4"
    & ffmpeg -hide_banner -loglevel error -y -framerate 30 -i (Join-Path $d 'g_%05d.png') -c:v h264_nvenc -preset p5 -cq 16 -pix_fmt yuv420p $out
    if ($LASTEXITCODE -eq 0) { Remove-Item -LiteralPath $d -Recurse -Force; "$($s.Name): $n quadros" }
}