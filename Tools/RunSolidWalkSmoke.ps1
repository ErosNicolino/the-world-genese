param([int]$TimeoutSeconds = 200)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$project = Join-Path $root 'Vortex.uproject'
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$log = Join-Path $root 'Saved\Logs\SolidWalkSmoke.log'
Remove-Item -LiteralPath $log -ErrorAction SilentlyContinue
$arguments = @(
    "`"$project`"", '/Game/Vortex/Maps/L_TerrainProto', '-game',
    '-VortexSolidWalkSmoke', '-VortexNoCreatures', '-unattended', '-nullrhi', '-nosound', '-NoSplash',
    "`"-ABSLOG=$log`""
)
$process = Start-Process -FilePath $editor -ArgumentList $arguments -WindowStyle Hidden -PassThru
try {
    if (!$process.WaitForExit($TimeoutSeconds * 1000)) { throw 'tempo esgotado' }
    $lines = Select-String -LiteralPath $log -Pattern 'SOLID_WALK|SOLID_PROBE|SOLID_WALK_SMOKE_'
    $lines | ForEach-Object { $_.Line }
    if ($process.ExitCode -ne 0 -or !($lines.Line -match 'SOLID_WALK_SMOKE_PASS')) {
        throw "teste falhou (código $($process.ExitCode))"
    }
}
finally {
    if (!$process.HasExited) { Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue }
}
