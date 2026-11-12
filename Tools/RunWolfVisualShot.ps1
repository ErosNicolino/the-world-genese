param([int]$TimeoutSeconds = 180)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$project = Join-Path $root 'Vortex.uproject'
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$log = Join-Path $root 'Saved\Logs\WolfVisualShot.log'
$shots = @('front','right','back','left') | ForEach-Object { Join-Path $root "Saved\Screenshots\wolf_$_.png" }
Remove-Item -LiteralPath $log -ErrorAction SilentlyContinue
Remove-Item -LiteralPath $shots -ErrorAction SilentlyContinue
$args = @("`"$project`"", '/Game/Vortex/Maps/L_TerrainProto', '-game', '-dx11', '-RenderOffScreen',
    '-unattended', '-windowed', '-ResX=1280', '-ResY=720', '-VortexWolfVisualShot', '-nosound', '-NoSplash', "`"-ABSLOG=$log`"")
$process = Start-Process -FilePath $editor -ArgumentList $args -WindowStyle Hidden -PassThru
try {
    if (!$process.WaitForExit($TimeoutSeconds * 1000)) { throw 'Wolf visual shot timed out' }
    if ($process.ExitCode -ne 0 -or @($shots | Where-Object { !(Test-Path -LiteralPath $_) }).Count -gt 0 -or
        !(Select-String -LiteralPath $log -Pattern 'VORTEX_WOLF_VISUAL_SHOT' -Quiet)) {
        Select-String -LiteralPath $log -Pattern 'VORTEX_WOLF_VISUAL_|Fatal error' | ForEach-Object { $_.Line }
        throw "Wolf visual shot failed with exit code $($process.ExitCode)"
    }
    Write-Output $shots
}
finally {
    if (!$process.HasExited) { Stop-Process -Id $process.Id -ErrorAction SilentlyContinue }
}
