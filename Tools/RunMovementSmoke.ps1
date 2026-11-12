param([int]$TimeoutSeconds = 120, [switch]$StepProbe, [switch]$Matrix)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$project = Join-Path $projectRoot 'Vortex.uproject'
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$log = Join-Path $projectRoot $(if ($Matrix) { 'Saved\Logs\MatrixSmoke.log' } elseif ($StepProbe) { 'Saved\Logs\StepSmoke.log' } else { 'Saved\Logs\MovementSmoke.log' })
$flag = if ($Matrix) { '-VortexMatrixSmoke' } elseif ($StepProbe) { '-VortexStepSmoke' } else { '-VortexMovementSmoke' }
$marker = if ($Matrix) { 'MATRIX_SMOKE_PASS' } elseif ($StepProbe) { 'STEP_SMOKE_PASS' } else { 'MOVEMENT_SMOKE_PASS' }
Remove-Item -LiteralPath $log -ErrorAction SilentlyContinue
$args = @(
    "`"$project`"", '/Game/Vortex/Maps/L_MovementProbe', '-game', $flag,
    '-unattended', '-nullrhi', '-nosound', '-NoSplash', "`"-ABSLOG=$log`""
)
$process = Start-Process -FilePath $editor -ArgumentList $args -WindowStyle Hidden -PassThru
try {
    $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
    while (!$process.HasExited -and (Get-Date) -lt $deadline) {
        Start-Sleep -Seconds 2
        $process.Refresh()
    }
    if (!$process.HasExited) { throw 'Movement test timed out' }
    if ($process.ExitCode -ne 0 -or !(Select-String -LiteralPath $log -Pattern $marker -Quiet)) {
        Select-String -LiteralPath $log -Pattern 'MOVEMENT_SMOKE_|STEP_SMOKE_|MATRIX_|Fatal error' | Select-Object -Last 8
        throw "Movement smoke failed with exit code $($process.ExitCode)"
    }
    Select-String -LiteralPath $log -Pattern $(if ($Matrix) { 'MATRIX_' } else { $marker }) | ForEach-Object { $_.Line }
}
finally {
    if (!$process.HasExited) { Stop-Process -Id $process.Id -ErrorAction SilentlyContinue }
}
