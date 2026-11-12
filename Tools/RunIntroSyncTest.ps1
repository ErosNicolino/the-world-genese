# HQ de abertura com som: a imagem e a legenda seguem a canção, mesmo depois de o jogo travar 3 s no meio dela.
param([int]$TimeoutSeconds = 600)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$project = Join-Path $root 'Vortex.uproject'
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$log = Join-Path $root 'Saved\Logs\IntroSync.log'
Remove-Item -LiteralPath $log -ErrorAction SilentlyContinue
# sem -nosound: o teste precisa da música tocando de verdade
$arguments = @("`"$project`"", '/Game/Vortex/Maps/L_TerrainProto', '-game', '-dx12', '-RenderOffScreen', '-VortexNoInitialFade',
    '-unattended', '-windowed', '-ResX=1280', '-ResY=720', '-VortexIntroShots', '-VortexIntroSync', '-VortexHour=10', '-NoSplash', "`"-ABSLOG=$log`"")
$process = Start-Process -FilePath $editor -ArgumentList $arguments -WindowStyle Hidden -PassThru
try {
    if (!$process.WaitForExit($TimeoutSeconds * 1000)) { throw 'Intro sync timed out' }
    Select-String -LiteralPath $log -Pattern 'VORTEX_INTRO_SYNC|VORTEX_INTRO_SHOTS|Fatal error' | ForEach-Object { $_.Line -replace '^.*LogTemp: (Display: |Error: )?', '' }
    if (!(Select-String -LiteralPath $log -Pattern 'VORTEX_INTRO_SYNC_PASS' -Quiet)) { throw 'Intro sync failed' }
}
finally {
    if (!$process.HasExited) { Stop-Process -Id $process.Id -ErrorAction SilentlyContinue }
}
