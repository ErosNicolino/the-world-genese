# Criaturas do Godot e missões: cada espécie nasce, é fotografada (Saved/Screenshots/criatura_*.png), ataca,
# morre e dá XP; depois o começo da linha principal (O Despertado e Recruta do Vale). -Species Slime,Golem,...
param([string]$Species = '', [int]$TimeoutSeconds = 600, [switch]$SlimeFace)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$project = Join-Path $root 'Vortex.uproject'
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$log = Join-Path $root 'Saved\Logs\CreatureSmoke.log'
$shots = Join-Path $root 'Saved\Screenshots'
Remove-Item -LiteralPath $log -ErrorAction SilentlyContinue
Get-ChildItem -LiteralPath $shots -Filter 'criatura_*.png' -ErrorAction SilentlyContinue | Remove-Item
$arguments = @("`"$project`"", '/Game/Vortex/Maps/L_TerrainProto', '-game', '-dx12', '-RenderOffScreen', '-VortexNoInitialFade',
    '-unattended', '-windowed', '-ResX=1280', '-ResY=720', '-VortexCreatureSmoke', '-nosound', '-NoSplash', "`"-ABSLOG=$log`"")
if ($Species) { $arguments += "-VortexCreatureSmoke=$Species" }
if ($SlimeFace) { $arguments += '-VortexSlimeFace' }
$process = Start-Process -FilePath $editor -ArgumentList $arguments -WindowStyle Hidden -PassThru
try {
    if (!$process.WaitForExit($TimeoutSeconds * 1000)) { throw 'Creature smoke timed out' }
    Select-String -LiteralPath $log -Pattern 'VORTEX_CREATURE_SMOKE|VORTEX_QUEST|VORTEX_CREATURES|VORTEX_MONSTER|Fatal error' | ForEach-Object { $_.Line -replace '^.*LogTemp: (Display: |Error: )?', '' }
    if (!(Select-String -LiteralPath $log -Pattern 'VORTEX_CREATURE_SMOKE_PASS' -Quiet)) { throw 'Creature smoke failed' }
}
finally {
    if (!$process.HasExited) { Stop-Process -Id $process.Id -ErrorAction SilentlyContinue }
}
