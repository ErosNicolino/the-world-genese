# HUD: fotos com a interface (Saved/Screenshots/hud_*.png): dia, combate e noite.
param([string]$Exposure = '', [string]$SunGlare = '', [string]$ExecCmds = '', [int]$TimeoutSeconds = 300, [string]$Extra = '')

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$project = Join-Path $root 'Vortex.uproject'
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$log = Join-Path $root 'Saved\Logs\HudShot.log'
$shots = Join-Path $root 'Saved\Screenshots'
Remove-Item -LiteralPath $log -ErrorAction SilentlyContinue
Get-ChildItem -LiteralPath $shots -Filter 'hud_*.png' -ErrorAction SilentlyContinue | Remove-Item
$arguments = @("`"$project`"", '/Game/Vortex/Maps/L_TerrainProto', '-game', '-dx12', '-RenderOffScreen', '-VortexNoInitialFade',
    '-unattended', '-windowed', '-ResX=1280', '-ResY=720', '-VortexHudShot', '-nosound', '-NoSplash', "`"-ABSLOG=$log`"")
if ($Exposure) { $arguments += "-VortexExposure=$Exposure" }
if ($ExecCmds) { $arguments += "`"-ExecCmds=$ExecCmds`"" }
if ($SunGlare) { $arguments += "-VortexSunGlare=$SunGlare" }
# -Extra: parâmetros a mais (ex.: '-VortexShaders=1' para as fotos com os shaders)
if ($Extra) { $arguments += $Extra.Split(' ') }
$process = Start-Process -FilePath $editor -ArgumentList $arguments -WindowStyle Hidden -PassThru
try {
    if (!$process.WaitForExit($TimeoutSeconds * 1000)) { throw 'HUD shot timed out' }
    Select-String -LiteralPath $log -Pattern 'VORTEX_HUD|VORTEX_SPECIAL|VORTEX_SLASH|VORTEX_RPG|VORTEX_SHEET|Fatal error' | ForEach-Object { $_.Line -replace '^.*LogTemp: (Display: )?', '' }
    if (!(Select-String -LiteralPath $log -Pattern 'VORTEX_HUD_SHOT_DONE' -Quiet)) { throw 'HUD shot failed' }
}
finally {
    if (!$process.HasExited) { Stop-Process -Id $process.Id -ErrorAction SilentlyContinue }
}
