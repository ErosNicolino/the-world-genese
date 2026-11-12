param([switch]$Before, [int]$TimeoutSeconds = 180)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$project = Join-Path $root 'Vortex.uproject'
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$prefix = if ($Before) { 'camera_before_' } else { 'camera_after_' }
$flag = if ($Before) { '-VortexCameraBeforeShot' } else { '-VortexCameraShot' }
$log = Join-Path $root ('Saved\Logs\' + $prefix + 'shot.log')
$idle = Join-Path $root ('Saved\Screenshots\' + $prefix + 'idle.png')
$run = Join-Path $root ('Saved\Screenshots\' + $prefix + 'run.png')
Remove-Item -LiteralPath $log,$idle,$run -ErrorAction SilentlyContinue
$arguments = @(
    "`"$project`"", '/Game/Vortex/Maps/L_TerrainProto', '-game', '-dx11', '-RenderOffScreen',
    '-unattended', '-windowed', '-ResX=1280', '-ResY=720', $flag, '-nosound', '-NoSplash',
    "`"-ABSLOG=$log`""
)
$process = Start-Process -FilePath $editor -ArgumentList $arguments -WindowStyle Hidden -PassThru
try {
    if (!$process.WaitForExit($TimeoutSeconds * 1000)) { throw 'Camera shot timed out' }
    if ($process.ExitCode -ne 0 -or !(Test-Path -LiteralPath $idle) -or !(Test-Path -LiteralPath $run) -or
        !(Select-String -LiteralPath $log -Pattern 'VORTEX_CAMERA_SHOT_DONE' -Quiet)) {
        Select-String -LiteralPath $log -Pattern 'VORTEX_CAMERA_SHOT|Fatal error' | ForEach-Object { $_.Line }
        throw "Camera shot failed with exit code $($process.ExitCode)"
    }
    Write-Output $idle
    Write-Output $run
}
finally {
    if (!$process.HasExited) { Stop-Process -Id $process.Id -ErrorAction SilentlyContinue }
}
