# Menu: fotos do menu e da criação de herói (Saved/Screenshots/frontend_*.png).
param([string]$Exposure = '', [string]$SunGlare = '', [string]$ExecCmds = '', [int]$TimeoutSeconds = 300)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$project = Join-Path $root 'Vortex.uproject'
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$log = Join-Path $root 'Saved\Logs\FrontEndShot.log'
$shots = Join-Path $root 'Saved\Screenshots'
Remove-Item -LiteralPath $log -ErrorAction SilentlyContinue
Get-ChildItem -LiteralPath $shots -Filter 'frontend_*.png' -ErrorAction SilentlyContinue | Remove-Item
$arguments = @("`"$project`"", '/Game/Vortex/Maps/L_Menu', '-game', '-dx12', '-RenderOffScreen', '-VortexNoInitialFade',
    '-unattended', '-windowed', '-ResX=1920', '-ResY=1080', '-ForceRes', '-VortexFrontEndShot', '-nosound', '-NoSplash', "`"-ABSLOG=$log`"")
if ($Exposure) { $arguments += "-VortexExposure=$Exposure" }
if ($ExecCmds) { $arguments += "`"-ExecCmds=$ExecCmds`"" }
if ($SunGlare) { $arguments += "-VortexSunGlare=$SunGlare" }
$process = Start-Process -FilePath $editor -ArgumentList $arguments -WindowStyle Hidden -PassThru
try {
    if (!$process.WaitForExit($TimeoutSeconds * 1000)) { throw 'Front end shot timed out' }
    Select-String -LiteralPath $log -Pattern 'VORTEX_FRONTEND|VORTEX_LOOK|Fatal error' | ForEach-Object { $_.Line -replace '^.*LogTemp: (Display: )?', '' }
    if (!(Select-String -LiteralPath $log -Pattern 'VORTEX_FRONTEND_SHOT_DONE' -Quiet)) { throw 'Front end shot failed' }
}
finally {
    if (!$process.HasExited) { Stop-Process -Id $process.Id -ErrorAction SilentlyContinue }
}
