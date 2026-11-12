param([ValidateSet(3, 5, 7)][int]$Count = 5, [int]$TimeoutSeconds = 90)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$project = Join-Path $root 'Vortex.uproject'
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$log = Join-Path $root 'Saved\Logs\PackSmoke.log'
Remove-Item -LiteralPath $log -ErrorAction SilentlyContinue
$arguments = @(
    "`"$project`"", '/Game/Vortex/Maps/L_MovementProbe', '-game', '-VortexPackSmoke', "-VortexPackCount=$Count",
    '-unattended', '-nullrhi', '-nosound', '-NoSplash', "`"-ABSLOG=$log`""
)
$process = Start-Process -FilePath $editor -ArgumentList $arguments -WindowStyle Hidden -PassThru
try {
    $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
    while (!$process.HasExited -and (Get-Date) -lt $deadline) {
        Start-Sleep -Seconds 2
        $process.Refresh()
    }
    if (!$process.HasExited) { throw 'Pack smoke timed out' }
    $lines = @(Select-String -LiteralPath $log -Pattern 'VORTEX_PACK_SMOKE_|VORTEX_WOLF_ATTACK|VORTEX_COMBAT_HIT|Fatal error' | ForEach-Object { $_.Line })
    $lines
    if ($process.ExitCode -ne 0 -or !(Select-String -LiteralPath $log -Pattern 'VORTEX_PACK_SMOKE_PASS' -Quiet)) {
        throw "Pack smoke failed with exit code $($process.ExitCode)"
    }
}
finally {
    if (!$process.HasExited) { Stop-Process -Id $process.Id -ErrorAction SilentlyContinue }
}
