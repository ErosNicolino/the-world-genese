# Fase 15: a mesma vista do vale em oito horas e duas olhando para o sol
# (Saved/Screenshots/daynight_*.png). -Exposure, -SunGlare e -ExecCmds testam outros valores;
# -Extra passa outras opções (ex.: -VortexLifeRadius=0.3); -Only vida tira só as vistas com esse texto no nome (vida_*: floresta viva).
param([string]$Exposure = '', [string]$SunGlare = '', [string]$ExecCmds = '', [string]$Only = '', [string]$Extra = '', [int]$TimeoutSeconds = 300)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$project = Join-Path $root 'Vortex.uproject'
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$log = Join-Path $root 'Saved\Logs\DayNightShot.log'
$shots = Join-Path $root 'Saved\Screenshots'
Remove-Item -LiteralPath $log -ErrorAction SilentlyContinue
Get-ChildItem -LiteralPath $shots -Filter 'daynight_*.png' -ErrorAction SilentlyContinue | Remove-Item
$arguments = @("`"$project`"", '/Game/Vortex/Maps/L_TerrainProto', '-game', '-dx12', '-RenderOffScreen', '-VortexNoInitialFade',
    '-unattended', '-windowed', '-ResX=1280', '-ResY=720', '-VortexDayNightShot', '-nosound', '-NoSplash', "`"-ABSLOG=$log`"")
if ($Exposure) { $arguments += "-VortexExposure=$Exposure" }
if ($ExecCmds) { $arguments += "`"-ExecCmds=$ExecCmds`"" }
if ($Extra) { $arguments += $Extra.Split(' ') }
if ($Only) { $arguments += "-VortexShotOnly=$Only" }
if ($SunGlare) { $arguments += "-VortexSunGlare=$SunGlare" }
$process = Start-Process -FilePath $editor -ArgumentList $arguments -WindowStyle Hidden -PassThru
try {
    if (!$process.WaitForExit($TimeoutSeconds * 1000)) { throw 'Day/night shot timed out' }
    Select-String -LiteralPath $log -Pattern 'VORTEX_DAYNIGHT|VORTEX_FOREST|Fatal error' | ForEach-Object { $_.Line -replace '^.*LogTemp: (Display: )?', '' }
    if (!(Select-String -LiteralPath $log -Pattern 'VORTEX_DAYNIGHT_SHOT_DONE' -Quiet)) { throw 'Day/night shot failed' }
}
finally {
    if (!$process.HasExited) { Stop-Process -Id $process.Id -ErrorAction SilentlyContinue }
}
