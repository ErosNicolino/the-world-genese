# O continente, com fotos (Saved/Screenshots/mundo_*.png): a emenda do vale com o continente,
# o centro de cada uma das nove regiões (de pé, sem cair) e o mapa (M).
# -Only lugar_ : só as fotos que começam assim (07/11: os lugares marcantes).
param([int]$TimeoutSeconds = 600, [string]$Only = '', [string]$Extra = '')   # -Extra '-VortexWeather=Rain': a mesma foto com outro tempo

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$project = Join-Path $root 'Vortex.uproject'
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$log = Join-Path $root 'Saved\Logs\WorldShots.log'
$shots = Join-Path $root 'Saved\Screenshots'
Remove-Item -LiteralPath $log -ErrorAction SilentlyContinue
Get-ChildItem -LiteralPath $shots -Filter 'mundo_*.png' -ErrorAction SilentlyContinue | Remove-Item
$arguments = @("`"$project`"", '/Game/Vortex/Maps/L_TerrainProto', '-game', '-dx12', '-RenderOffScreen', '-VortexNoInitialFade',
    '-unattended', '-windowed', '-ResX=1280', '-ResY=720', '-VortexWorldShots', '-nosound', '-NoSplash', "`"-ABSLOG=$log`"")
if ($Only) { $arguments += "-VortexShotsOnly=$Only" }
if ($Extra) { $arguments += $Extra.Split(' ') }
$process = Start-Process -FilePath $editor -ArgumentList $arguments -WindowStyle Hidden -PassThru
try {
    if (!$process.WaitForExit($TimeoutSeconds * 1000)) { throw 'World shots timed out' }
    Select-String -LiteralPath $log -Pattern 'VORTEX_WORLD_SHOTS' | ForEach-Object { $_.Line -replace '^.*LogTemp: (Display: )?', '' }
    if (!(Select-String -LiteralPath $log -Pattern 'VORTEX_WORLD_SHOTS_PASS' -Quiet)) { throw 'World shots failed' }
}
finally {
    if (!$process.HasExited) { Stop-Process -Id $process.Id -ErrorAction SilentlyContinue }
}
