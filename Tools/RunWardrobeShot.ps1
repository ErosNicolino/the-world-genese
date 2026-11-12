# Fotos de frente do herói com as peças padrão e com as alternativas (Saved/Screenshots/wardrobe_*.png).
param([int]$TimeoutSeconds = 300)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$project = Join-Path $root 'Vortex.uproject'
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$log = Join-Path $root 'Saved\Logs\WardrobeShot.log'
Remove-Item -LiteralPath $log -ErrorAction SilentlyContinue
$arguments = @(
    "`"$project`"", '/Game/Vortex/Maps/L_TerrainProto', '-game', '-dx12', '-RenderOffScreen', '-windowed', '-ResX=1280', '-ResY=720',
    '-VortexWardrobeShot', '-unattended', '-nosound', '-NoSplash', "`"-ABSLOG=$log`""
)
$process = Start-Process -FilePath $editor -ArgumentList $arguments -WindowStyle Hidden -PassThru
$code = 1
try {
    if (!$process.WaitForExit($TimeoutSeconds * 1000)) { throw 'tempo esgotado' }
    Get-ChildItem (Join-Path $root 'Saved\Screenshots\wardrobe_*.png') | ForEach-Object { "$($_.Name) $($_.LastWriteTime)" }
    $code = 0
}
catch { Write-Output "WARDROBE_SHOT_FAIL $($_.Exception.Message)" }
finally { try { Stop-Process -Id $process.Id -Force -ErrorAction Stop } catch { } }
exit $code
