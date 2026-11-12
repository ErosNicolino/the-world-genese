# HUD cartoon, minimapa e mapa do mundo: fotos num lago perto da cidade (Saved/Screenshots/hudmapa_*.png).
param([int]$TimeoutSeconds = 600)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$project = Join-Path $root 'Vortex.uproject'
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$log = Join-Path $root 'Saved\Logs\MapShots.log'
Remove-Item -LiteralPath $log -ErrorAction SilentlyContinue
$arguments = @("`"$project`"", '/Game/Vortex/Maps/L_TerrainProto', '-game', '-dx12', '-RenderOffScreen', '-VortexNoInitialFade',
    '-unattended', '-windowed', '-ResX=1280', '-ResY=720', '-VortexMapShots', '-VortexHour=10', '-nosound', '-NoSplash', "`"-ABSLOG=$log`"")
$process = Start-Process -FilePath $editor -ArgumentList $arguments -WindowStyle Hidden -PassThru
try {
    if (!$process.WaitForExit($TimeoutSeconds * 1000)) { throw 'Map shots timed out' }
    Select-String -LiteralPath $log -Pattern 'VORTEX_MAP_SHOTS|Fatal error' | ForEach-Object { $_.Line -replace '^.*LogTemp: (Display: |Error: )?', '' }
    if (!(Select-String -LiteralPath $log -Pattern 'VORTEX_MAP_SHOTS_PASS' -Quiet)) { throw 'Map shots failed' }
}
finally {
    if (!$process.HasExited) { Stop-Process -Id $process.Id -ErrorAction SilentlyContinue }
}
