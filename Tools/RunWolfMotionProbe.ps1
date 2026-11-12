# Lobo andando de lado por 1 s: deslocamento do osso da pata e dos vértices da malha, e três fotos
# do passo (Saved/Screenshots/wolf_walk_*.png). Osso que mexe com vértice parado = pele quebrada.
# -Fps N: sem render, quadros fixos (o freio entre decisões da IA aparece em FPS alto)
param([int]$Fps = 0, [int]$TimeoutSeconds = 180)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$project = Join-Path $root 'Vortex.uproject'
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$log = Join-Path $root 'Saved\Logs\WolfMotionProbe.log'
Remove-Item -LiteralPath $log -ErrorAction SilentlyContinue
$args = @("`"$project`"", '/Game/Vortex/Maps/L_TerrainProto', '-game', '-dx12', '-RenderOffScreen', '-VortexNoInitialFade',
    '-unattended', '-windowed', '-ResX=1280', '-ResY=720', '-VortexWolfMotionProbe', '-nosound', '-NoSplash', "`"-ABSLOG=$log`"")
if ($Fps -gt 0) {
    $args = $args | Where-Object { $_ -notin @('-dx12', '-RenderOffScreen') }
    $args += @('-nullrhi', "`"-ExecCmds=t.MaxFPS $Fps`"")
}
$process = Start-Process -FilePath $editor -ArgumentList $args -WindowStyle Hidden -PassThru
try {
    if (!$process.WaitForExit($TimeoutSeconds * 1000)) { throw 'Wolf motion probe timed out' }
    Select-String -LiteralPath $log -Pattern 'VORTEX_WOLF_MOTION|VORTEX_WOLF_CLIP|Fatal error' | ForEach-Object { $_.Line -replace '^.*LogTemp: (Display: )?', '' }
}
finally {
    if (!$process.HasExited) { Stop-Process -Id $process.Id -ErrorAction SilentlyContinue }
}
