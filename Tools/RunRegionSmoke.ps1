param([int]$TimeoutSeconds = 90)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$project = Join-Path $root 'Vortex.uproject'
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$log = Join-Path $root 'Saved\Logs\RegionSmoke.log'
Remove-Item -LiteralPath $log -ErrorAction SilentlyContinue
$arguments = @(
    "`"$project`"", '/Game/Vortex/Maps/L_TerrainProto', '-game', '-VortexRegionSmoke',
    '-unattended', '-nullrhi', '-nosound', '-NoSplash', "`"-ABSLOG=$log`""
)
$process = Start-Process -FilePath $editor -ArgumentList $arguments -WindowStyle Hidden -PassThru
try {
    if (!$process.WaitForExit($TimeoutSeconds * 1000)) { throw 'Region smoke timed out' }
    Select-String -LiteralPath $log -Pattern 'VORTEX_REGION_SMOKE_|Fatal error' | ForEach-Object { $_.Line }
    if ($process.ExitCode -ne 0 -or !(Select-String -LiteralPath $log -Pattern 'VORTEX_REGION_SMOKE_PASS' -Quiet)) {
        throw "Region smoke failed with exit code $($process.ExitCode)"
    }
}
finally {
    if (!$process.HasExited) { Stop-Process -Id $process.Id -ErrorAction SilentlyContinue }
}
