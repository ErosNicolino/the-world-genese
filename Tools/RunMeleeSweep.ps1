# Golpe de perto de cada arma contra criaturas paradas em volta do herói (VortexMeleeSweep.cpp).
param([int]$TimeoutSeconds = 600)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$project = Join-Path $root 'Vortex.uproject'
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$log = Join-Path $root 'Saved\Logs\MeleeSweep.log'
Remove-Item -LiteralPath $log -ErrorAction SilentlyContinue
$process = Start-Process -FilePath $editor -WindowStyle Hidden -PassThru -ArgumentList @(
    "`"$project`"", '/Game/Vortex/Maps/L_MovementProbe', '-game', '-VortexMeleeSweep',
    '-unattended', '-nullrhi', '-nosound', '-NoSplash', "`"-ABSLOG=$log`"")
try {
    $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
    while (!$process.HasExited -and (Get-Date) -lt $deadline) { Start-Sleep -Seconds 2; $process.Refresh() }
    if (!$process.HasExited) { throw 'Melee sweep timed out' }
    Select-String -LiteralPath $log -Pattern 'VORTEX_MELEE_|Fatal error' | ForEach-Object { $_.Line -replace '^.*?(VORTEX_)', '$1' }
    if (!(Select-String -LiteralPath $log -Pattern 'VORTEX_MELEE_SWEEP_PASS' -Quiet)) { throw 'Melee sweep failed' }
}
finally { if (!$process.HasExited) { Stop-Process -Id $process.Id -ErrorAction SilentlyContinue } }
