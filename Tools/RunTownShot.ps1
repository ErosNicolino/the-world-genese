# Cidade do Santuário: fotos do portão, praça, taverna, oficinas, de cima e à noite (Saved/Screenshots/cidade_*.png).
param([int]$TimeoutSeconds = 600, [string]$Extra = '')
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$project = Join-Path $root 'Vortex.uproject'
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$log = Join-Path $root 'Saved\Logs\TownShot.log'
$shots = Join-Path $root 'Saved\Screenshots'
Remove-Item -LiteralPath $log -ErrorAction SilentlyContinue
Get-ChildItem -LiteralPath $shots -Filter 'cidade_*.png' -ErrorAction SilentlyContinue | Remove-Item
$arguments = @("`"$project`"", '/Game/Vortex/Maps/L_TerrainProto', '-game', '-dx12', '-RenderOffScreen', '-VortexNoInitialFade',
    '-unattended', '-windowed', '-ResX=1280', '-ResY=720', '-VortexTownShot', '-nosound', '-NoSplash', "`"-ABSLOG=$log`"")
if ($Extra) { $arguments += $Extra.Split(' ') }
$process = Start-Process -FilePath $editor -ArgumentList $arguments -WindowStyle Hidden -PassThru
try {
    if (!$process.WaitForExit($TimeoutSeconds * 1000)) { throw 'Town shot timed out' }
    Select-String -LiteralPath $log -Pattern 'VORTEX_TOWN|VORTEX_FOUNTAIN|VORTEX_SANCTUARY|Fatal error' | ForEach-Object { $_.Line -replace '^.*LogTemp: (Display: )?', '' }
    if (!(Select-String -LiteralPath $log -Pattern 'VORTEX_TOWN_SHOT_DONE PASS' -Quiet)) { throw 'Town shot failed' }
}
finally {
    if (!$process.HasExited) { Stop-Process -Id $process.Id -ErrorAction SilentlyContinue }
}
