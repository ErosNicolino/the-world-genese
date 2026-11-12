# Estádio do Despertar: acordar, pegar o arco, treinar, sair e pousar no vale (Saved/Screenshots/estadio_*.png).
param([int]$TimeoutSeconds = 600)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$project = Join-Path $root 'Vortex.uproject'
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$log = Join-Path $root 'Saved\Logs\StadiumShot.log'
$shots = Join-Path $root 'Saved\Screenshots'
Remove-Item -LiteralPath $log -ErrorAction SilentlyContinue
Get-ChildItem -LiteralPath $shots -Filter 'estadio_*.png' -ErrorAction SilentlyContinue | Remove-Item
$arguments = @("`"$project`"", '/Game/Vortex/Maps/L_TerrainProto', '-game', '-dx12', '-RenderOffScreen', '-VortexNoInitialFade',
    '-unattended', '-windowed', '-ResX=1280', '-ResY=720', '-VortexStadiumShot -VortexAwakening', '-nosound', '-NoSplash', "`"-ABSLOG=$log`"")
$process = Start-Process -FilePath $editor -ArgumentList $arguments -WindowStyle Hidden -PassThru
try {
    if (!$process.WaitForExit($TimeoutSeconds * 1000)) { throw 'Stadium shot timed out' }
    Select-String -LiteralPath $log -Pattern 'VORTEX_STADIUM|VORTEX_SHEET|VORTEX_PROJECTILE|Fatal error' | ForEach-Object { $_.Line -replace '^.*LogTemp: (Display: )?', '' }
    if (!(Select-String -LiteralPath $log -Pattern 'VORTEX_STADIUM_SHOT_DONE PASS' -Quiet)) { throw 'Stadium shot failed' }
}
finally {
    if (!$process.HasExited) { Stop-Process -Id $process.Id -ErrorAction SilentlyContinue }
}
