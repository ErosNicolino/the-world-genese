param([int]$TimeoutSeconds = 120)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$project = Join-Path $root 'Vortex.uproject'
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$log = Join-Path $root 'Saved\Logs\DodgeDirectionSmoke.log'
Remove-Item -LiteralPath $log -ErrorAction SilentlyContinue
$arguments = @(
    "`"$project`"", '/Game/Vortex/Maps/L_MovementProbe', '-game',
    '-VortexDodgeDirectionSmoke', '-unattended', '-nullrhi', '-nosound', '-NoSplash',
    "`"-ABSLOG=$log`""
)
$process = Start-Process -FilePath $editor -ArgumentList $arguments -WindowStyle Hidden -PassThru
try {
    if (!$process.WaitForExit($TimeoutSeconds * 1000)) { throw 'tempo esgotado' }
    $lines = Select-String -LiteralPath $log -Pattern 'DODGE_.*_(PASS|FAIL)|DODGE_DIRECTION_SMOKE_'
    $lines | ForEach-Object { $_.Line }
    if ($process.ExitCode -ne 0 -or !($lines.Line -match 'DODGE_DIRECTION_SMOKE_PASS')) {
        throw "teste falhou (código $($process.ExitCode))"
    }
}
finally {
    if (!$process.HasExited) { Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue }
}
