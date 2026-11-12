param([int]$TimeoutSeconds = 180)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$project = Join-Path $root 'Vortex.uproject'
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$log = Join-Path $root 'Saved\Logs\CameraSequenceShot.log'
$names = @('turn','run','attack','aim','left')
$images = @($names | ForEach-Object { Join-Path $root ('Saved\Screenshots\camera_sequence_' + $_ + '.png') })
Remove-Item -LiteralPath $log -ErrorAction SilentlyContinue
foreach ($path in $images) { Remove-Item -LiteralPath $path -ErrorAction SilentlyContinue }
$arguments = @(
    "`"$project`"", '/Game/Vortex/Maps/L_TerrainProto', '-game', '-dx11', '-RenderOffScreen',
    '-unattended', '-windowed', '-ResX=1280', '-ResY=720', '-VortexCameraSequenceShot', '-nosound', '-NoSplash',
    "`"-ABSLOG=$log`""
)
$process = Start-Process -FilePath $editor -ArgumentList $arguments -WindowStyle Hidden -PassThru
try {
    if (!$process.WaitForExit($TimeoutSeconds * 1000)) { throw 'Camera sequence timed out' }
    if ($process.ExitCode -ne 0 -or !(Select-String -LiteralPath $log -Pattern 'VORTEX_CAMERA_SEQUENCE_DONE' -Quiet)) {
        throw "Camera sequence failed with exit code $($process.ExitCode)"
    }
    foreach ($path in $images) {
        if (!(Test-Path -LiteralPath $path)) { throw "Missing screenshot: $path" }
        Write-Output $path
    }
}
finally {
    if (!$process.HasExited) { Stop-Process -Id $process.Id -ErrorAction SilentlyContinue }
}
