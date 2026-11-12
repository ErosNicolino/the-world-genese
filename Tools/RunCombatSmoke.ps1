param([int]$TimeoutSeconds = 90)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$project = Join-Path $root 'Vortex.uproject'
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$log = Join-Path $root 'Saved\Logs\CombatSmoke.log'
Remove-Item -LiteralPath $log -ErrorAction SilentlyContinue
$arguments = @(
    "`"$project`"", '/Game/Vortex/Maps/L_MovementProbe', '-game', '-VortexCombatSmoke',
    '-unattended', '-nullrhi', '-nosound', '-NoSplash', "`"-ABSLOG=$log`""
)
$process = Start-Process -FilePath $editor -ArgumentList $arguments -WindowStyle Hidden -PassThru
try {
    $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
    while (!$process.HasExited -and (Get-Date) -lt $deadline) {
        Start-Sleep -Seconds 2
        $process.Refresh()
    }
    if (!$process.HasExited) { throw 'Combat smoke timed out' }
    $lines = @(Select-String -LiteralPath $log -Pattern 'VORTEX_COMBAT_SMOKE_|VORTEX_COMBAT_HIT|Fatal error' | ForEach-Object { $_.Line })
    $lines
    if ($process.ExitCode -ne 0 -or !(Select-String -LiteralPath $log -Pattern 'VORTEX_COMBAT_SMOKE_PASS' -Quiet)) {
        throw "Combat smoke failed with exit code $($process.ExitCode)"
    }
}
finally {
    if (!$process.HasExited) { Stop-Process -Id $process.Id -ErrorAction SilentlyContinue }
}
