# Fase 12: três vistas longas do L_TerrainProto, cada uma com e sem névoa (Saved/Screenshots/fog_*).
# -Density/-Start/-Opacity/-Falloff testam outra névoa sem regravar o mapa.
param([int]$TimeoutSeconds = 300, [string]$Density = '', [string]$Start = '', [string]$Opacity = '', [string]$Falloff = '')

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$project = Join-Path $root 'Vortex.uproject'
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$log = Join-Path $root 'Saved\Logs\FogShot.log'
$shots = Join-Path $root 'Saved\Screenshots'
Remove-Item -LiteralPath $log -ErrorAction SilentlyContinue
Get-ChildItem -LiteralPath $shots -Filter 'fog_*.png' -ErrorAction SilentlyContinue | Remove-Item
$arguments = @(
    "`"$project`"", '/Game/Vortex/Maps/L_TerrainProto', '-game', '-dx12', '-RenderOffScreen',
    '-unattended', '-windowed', '-ResX=1280', '-ResY=720', '-VortexFogShot', '-nosound', '-NoSplash',
    "`"-ABSLOG=$log`""
)
if ($Density) { $arguments += "-VortexFogDensity=$Density" }
if ($Start) { $arguments += "-VortexFogStart=$Start" }
if ($Opacity) { $arguments += "-VortexFogOpacity=$Opacity" }
if ($Falloff) { $arguments += "-VortexFogFalloff=$Falloff" }
$process = Start-Process -FilePath $editor -ArgumentList $arguments -WindowStyle Hidden -PassThru
try {
    if (!$process.WaitForExit($TimeoutSeconds * 1000)) { throw 'Fog shot timed out' }
    Select-String -LiteralPath $log -Pattern 'VORTEX_FOG_SHOT|Fatal error' | ForEach-Object { $_.Line }
    if ($process.ExitCode -ne 0 -or !(Select-String -LiteralPath $log -Pattern 'VORTEX_FOG_SHOT_DONE' -Quiet)) {
        throw "Fog shot failed with exit code $($process.ExitCode)"
    }
    Get-ChildItem -LiteralPath $shots -Filter 'fog_*.png' | ForEach-Object { $_.FullName }
}
finally {
    if (!$process.HasExited) { Stop-Process -Id $process.Id -ErrorAction SilentlyContinue }
}
