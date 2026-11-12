# Armas do Godot: para cada tipo, pose, golpe, carga e especial (Saved/Screenshots/arma_*.png). -Types arco_de_betula,besta,...
param([string]$Types = '', [int]$TimeoutSeconds = 600)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$project = Join-Path $root 'Vortex.uproject'
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$log = Join-Path $root 'Saved\Logs\WeaponShot.log'
$shots = Join-Path $root 'Saved\Screenshots'
Remove-Item -LiteralPath $log -ErrorAction SilentlyContinue
Get-ChildItem -LiteralPath $shots -Filter 'arma_*.png' -ErrorAction SilentlyContinue | Remove-Item
$arguments = @("`"$project`"", '/Game/Vortex/Maps/L_TerrainProto', '-game', '-dx12', '-RenderOffScreen', '-VortexNoInitialFade',
    '-unattended', '-windowed', '-ResX=1280', '-ResY=720', '-VortexWeaponShot', '-nosound', '-NoSplash', "`"-ABSLOG=$log`"")
if ($Types) { $arguments += "-VortexWeaponShot=$Types" }
$process = Start-Process -FilePath $editor -ArgumentList $arguments -WindowStyle Hidden -PassThru
try {
    if (!$process.WaitForExit($TimeoutSeconds * 1000)) { throw 'Weapon shot timed out' }
    Select-String -LiteralPath $log -Pattern 'VORTEX_WEAPON|VORTEX_CHARGE|VORTEX_PROJECTILE|Fatal error' | ForEach-Object { $_.Line -replace '^.*LogTemp: (Display: )?', '' }
    if (!(Select-String -LiteralPath $log -Pattern 'VORTEX_WEAPON_SHOT_DONE PASS' -Quiet)) { throw 'Weapon shot failed' }
}
finally {
    if (!$process.HasExited) { Stop-Process -Id $process.Id -ErrorAction SilentlyContinue }
}
