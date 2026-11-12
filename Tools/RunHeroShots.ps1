# Os heróis novos de frente, parados e golpeando (VortexHeroShots.cpp): Saved/Screenshots/heroi_<raça>[_golpe].png.
param([int]$TimeoutSeconds = 300, [string]$ExecCmds = '', [string]$Extra = '-VortexNoExtra', [string]$Map = '/Game/Vortex/Maps/L_MovementProbe')   # -Extra -VortexFairyShot: as fotos da fada
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$project = Join-Path $root 'Vortex.uproject'
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$log = Join-Path $root 'Saved\Logs\HeroShots.log'
Remove-Item -LiteralPath $log -ErrorAction SilentlyContinue
$process = Start-Process -FilePath $editor -WindowStyle Hidden -PassThru -ArgumentList @(
    "`"$project`"", $Map, '-game', '-dx12', '-RenderOffScreen', '-windowed', '-ResX=1280', '-ResY=720',
    '-VortexNoInitialFade', $Extra, '-unattended', '-nosound', '-NoSplash', '-VortexHeroShots', '-VortexHour=12', "`"-ABSLOG=$log`"",
    # -ExecCmds 'r.MotionBlurQuality 0': o golpe sem o borrão do movimento
    $(if ($ExecCmds) { "`"-ExecCmds=$ExecCmds`"" } else { '-VortexNoExec' }))
try {
    if (!$process.WaitForExit($TimeoutSeconds * 1000)) { throw 'Hero shots timed out' }
    Select-String -LiteralPath $log -Pattern 'VORTEX_HERO_SHOTS|Fatal error' | ForEach-Object { $_.Line -replace '^.*?(VORTEX_)', '$1' }
    if (!(Select-String -LiteralPath $log -Pattern 'VORTEX_HERO_SHOTS_PASS' -Quiet)) { throw 'Hero shots failed' }
}
finally { if (!$process.HasExited) { Stop-Process -Id $process.Id -ErrorAction SilentlyContinue } }
