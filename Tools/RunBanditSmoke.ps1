# Bandidos: os 4 tipos (espada, escudo, marreta, arco) vestidos e armados, ataque, flecha, escudo,
# XP ao vencer e um acampamento montado (Saved/Screenshots/bandido_*.png).
param([int]$TimeoutSeconds = 400)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$project = Join-Path $root 'Vortex.uproject'
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$log = Join-Path $root 'Saved\Logs\BanditSmoke.log'
Remove-Item -LiteralPath $log -ErrorAction SilentlyContinue
$arguments = @("`"$project`"", '/Game/Vortex/Maps/L_TerrainProto', '-game', '-dx12', '-RenderOffScreen', '-VortexNoInitialFade',
    '-unattended', '-windowed', '-ResX=1280', '-ResY=720', '-VortexBanditSmoke', '-nosound', '-NoSplash', "`"-ABSLOG=$log`"")
$process = Start-Process -FilePath $editor -ArgumentList $arguments -WindowStyle Hidden -PassThru
try {
    if (!$process.WaitForExit($TimeoutSeconds * 1000)) { throw 'Bandit smoke timed out' }
    Select-String -LiteralPath $log -Pattern 'VORTEX_BANDIT|Fatal error' | ForEach-Object { $_.Line -replace '^.*LogTemp: (Display: |Error: )?', '' }
    if (!(Select-String -LiteralPath $log -Pattern 'VORTEX_BANDIT_SMOKE_PASS' -Quiet)) { throw 'Bandit smoke failed' }
}
finally {
    if (!$process.HasExited) { Stop-Process -Id $process.Id -ErrorAction SilentlyContinue }
}
