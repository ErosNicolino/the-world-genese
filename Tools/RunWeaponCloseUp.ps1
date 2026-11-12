# Cada arma de perto: parada, no golpe e no especial (Saved/Screenshots/perto_<tipo>_<fase>.png). -Types espada_de_cinzas,machado
param([string]$Types = '', [string]$ExecCmds = '', [int]$TimeoutSeconds = 900, [switch]$BowFirstPerson)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$project = Join-Path $root 'Vortex.uproject'
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$log = Join-Path $root 'Saved\Logs\WeaponCloseUp.log'
$shots = Join-Path $root 'Saved\Screenshots'
Remove-Item -LiteralPath $log -ErrorAction SilentlyContinue
Get-ChildItem -LiteralPath $shots -Filter 'perto_*.png' -ErrorAction SilentlyContinue | Remove-Item
$arguments = @("`"$project`"", '/Game/Vortex/Maps/L_TerrainProto', '-game', '-dx12', '-RenderOffScreen', '-VortexNoInitialFade',
    '-unattended', '-windowed', '-ResX=960', '-ResY=720', '-VortexWeaponCloseUp', '-VortexNoCreatures', '-nosound', '-NoSplash', "`"-ABSLOG=$log`"")
if ($Types) { $arguments += "-VortexWeaponCloseUp=$Types" }
if ($BowFirstPerson) { $arguments += '-VortexBowFirstPerson' }
# -ExecCmds 'r.MotionBlurQuality 0': as fotos sem o borrão do movimento (para ver a arma)
if ($ExecCmds) { $arguments += "`"-ExecCmds=$ExecCmds`"" }
$process = Start-Process -FilePath $editor -ArgumentList $arguments -WindowStyle Hidden -PassThru
try {
    if (!$process.WaitForExit($TimeoutSeconds * 1000)) { throw 'Weapon close-up timed out' }
    if (!(Select-String -LiteralPath $log -Pattern 'VORTEX_CLOSEUP_DONE' -Quiet)) { throw 'Weapon close-up failed' }
    Select-String -LiteralPath $log -Pattern 'VORTEX_CLOSEUP_DONE' | ForEach-Object { $_.Line -replace '^.*LogTemp: (Display: )?', '' }
}
finally {
    if (!$process.HasExited) { Stop-Process -Id $process.Id -ErrorAction SilentlyContinue }
}