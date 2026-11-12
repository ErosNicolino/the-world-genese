# A rotina dos moradores da vila (08/11), com fotos (Saved/Screenshots/npc_*.png): 9h no serviço, 12h30 na taverna,
# 17h30 nos bancos, 23h dormindo; conta quem chegou, sentou, dormiu e cumprimentou o herói.
param([int]$TimeoutSeconds = 600)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$project = Join-Path $root 'Vortex.uproject'
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$log = Join-Path $root 'Saved\Logs\NpcShots.log'
$shots = Join-Path $root 'Saved\Screenshots'
Remove-Item -LiteralPath $log -ErrorAction SilentlyContinue
Get-ChildItem -LiteralPath $shots -Filter 'npc_*.png' -ErrorAction SilentlyContinue | Remove-Item
$arguments = @("`"$project`"", '/Game/Vortex/Maps/L_TerrainProto', '-game', '-dx12', '-RenderOffScreen', '-VortexNoInitialFade',
    '-unattended', '-windowed', '-ResX=1280', '-ResY=720', '-VortexNpcShots', '-nosound', '-NoSplash', "`"-ABSLOG=$log`"")
$process = Start-Process -FilePath $editor -ArgumentList $arguments -WindowStyle Hidden -PassThru
try {
    if (!$process.WaitForExit($TimeoutSeconds * 1000)) { throw 'Npc shots timed out' }
    Select-String -LiteralPath $log -Pattern 'VORTEX_NPC_SHOTS' | ForEach-Object { $_.Line -replace '^.*LogTemp: (Display: )?', '' }
    if (!(Select-String -LiteralPath $log -Pattern 'VORTEX_NPC_SHOTS_PASS' -Quiet)) { throw 'Npc shots failed' }
}
finally {
    if (!$process.HasExited) { Stop-Process -Id $process.Id -ErrorAction SilentlyContinue }
}
