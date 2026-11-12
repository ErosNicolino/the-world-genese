$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$project = Join-Path $projectRoot 'Vortex.uproject'
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$log = Join-Path $projectRoot 'Saved\Logs\SaveSmoke.log'
Remove-Item -LiteralPath $log -ErrorAction SilentlyContinue
$args = @(
    "`"$project`"", '/Game/Vortex/Maps/L_Framework', '-game', '-VortexSaveSmoke',
    '-unattended', '-nullrhi', '-nosound', '-NoSplash', "`"-ABSLOG=$log`""
)
$process = Start-Process -FilePath $editor -ArgumentList $args -WindowStyle Hidden -PassThru -Wait
if ($process.ExitCode -ne 0 -or !(Select-String -LiteralPath $log -Pattern 'SAVE_SMOKE_PASS' -Quiet)) {
    Get-Content -LiteralPath $log -Tail 30 | Select-String 'Vortex save|Error|Fatal'
    throw "Save smoke failed with exit code $($process.ExitCode)"
}
Write-Output 'SAVE_SMOKE_PASS world/player roundtrip and backup recovery'
