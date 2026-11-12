# Grama: o mesmo prado em quatro vistas fixas (Saved/Screenshots/grass_*.png): rente ao chão,
# câmera do jogo, aberta e fim de tarde. -Exposure, -SunGlare e -ExecCmds como em RunDayNightShot.
param([string]$Exposure = '', [string]$SunGlare = '', [string]$ExecCmds = '', [int]$TimeoutSeconds = 300)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$project = Join-Path $root 'Vortex.uproject'
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$log = Join-Path $root 'Saved\Logs\GrassShot.log'
$shots = Join-Path $root 'Saved\Screenshots'
Remove-Item -LiteralPath $log -ErrorAction SilentlyContinue
Get-ChildItem -LiteralPath $shots -Filter 'grass_*.png' -ErrorAction SilentlyContinue | Remove-Item
$arguments = @("`"$project`"", '/Game/Vortex/Maps/L_TerrainProto', '-game', '-dx12', '-RenderOffScreen', '-VortexNoInitialFade',
    '-unattended', '-windowed', '-ResX=1280', '-ResY=720', '-VortexGrassShot', '-nosound', '-NoSplash', "`"-ABSLOG=$log`"")
if ($Exposure) { $arguments += "-VortexExposure=$Exposure" }
if ($ExecCmds) { $arguments += "`"-ExecCmds=$ExecCmds`"" }
if ($SunGlare) { $arguments += "-VortexSunGlare=$SunGlare" }
$process = Start-Process -FilePath $editor -ArgumentList $arguments -WindowStyle Hidden -PassThru
try {
    if (!$process.WaitForExit($TimeoutSeconds * 1000)) { throw 'Grass shot timed out' }
    Select-String -LiteralPath $log -Pattern 'VORTEX_GRASS|Fatal error' | ForEach-Object { $_.Line -replace '^.*LogTemp: (Display: )?', '' }
    if (!(Select-String -LiteralPath $log -Pattern 'VORTEX_GRASS_SHOT_DONE' -Quiet)) { throw 'Grass shot failed' }
}
finally {
    if (!$process.HasExited) { Stop-Process -Id $process.Id -ErrorAction SilentlyContinue }
}
