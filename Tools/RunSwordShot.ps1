param([int]$TimeoutSeconds = 180, [switch]$Probe, [switch]$Normal)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$project = Join-Path $root 'Vortex.uproject'
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$log = Join-Path $root 'Saved\Logs\SwordShot.log'
$prefix = if ($Normal) { 'sword_normal_' } else { 'sword_' }
$still = Join-Path $root ('Saved\Screenshots\' + $prefix + 'still.png')
$idle = Join-Path $root ('Saved\Screenshots\' + $prefix + 'idle.png')
$walking = Join-Path $root ('Saved\Screenshots\' + $prefix + 'walking.png')
Remove-Item -LiteralPath $log,$idle,$still,$walking -ErrorAction SilentlyContinue
$map = if ($Probe) { '/Game/Vortex/Maps/L_MovementProbe' } else { '/Game/Vortex/Maps/L_TerrainProto' }
$flag = if ($Normal) { '-VortexSwordNormalShot' } else { '-VortexSwordShot' }
$arguments = @(
    "`"$project`"", $map, '-game', '-dx11', '-RenderOffScreen',
    '-unattended', '-windowed', '-ResX=1280', '-ResY=720', $flag, '-nosound', '-NoSplash',
    "`"-ABSLOG=$log`""
)
$process = Start-Process -FilePath $editor -ArgumentList $arguments -WindowStyle Hidden -PassThru
try {
    if (!$process.WaitForExit($TimeoutSeconds * 1000)) { throw 'Sword shot timed out' }
    if ($process.ExitCode -ne 0 -or !(Test-Path -LiteralPath $idle) -or !(Test-Path -LiteralPath $still) -or !(Test-Path -LiteralPath $walking) -or
        !(Select-String -LiteralPath $log -Pattern 'VORTEX_SWORD_SHOT_DONE still=1 walking=1' -Quiet)) {
        Select-String -LiteralPath $log -Pattern 'VORTEX_SWORD_SHOT|Fatal error' | ForEach-Object { $_.Line }
        throw "Sword shot failed with exit code $($process.ExitCode)"
    }
    Write-Output $idle
    Write-Output $still
    Write-Output $walking
}
finally {
    if (!$process.HasExited) { Stop-Process -Id $process.Id -ErrorAction SilentlyContinue }
}
