param([int]$TimeoutSeconds = 120)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$project = Join-Path $root 'Vortex.uproject'
$log = Join-Path $root 'Saved\Logs\MenuFlow.log'
Remove-Item -LiteralPath $log -ErrorAction SilentlyContinue
$process = Start-Process -FilePath $editor -WindowStyle Hidden -PassThru -ArgumentList @(
    "`"$project`"", '-game', '-dx11', '-RenderOffScreen', '-unattended', '-nosound', '-NoSplash',
    '-VortexMenuFlowSmoke', '-VortexNoCreatures', "`"-ABSLOG=$log`"")
try {
    $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
    while ((Get-Date) -lt $deadline) {
        $process.Refresh()
        if ($process.HasExited) { break }
        Start-Sleep -Seconds 1
    }
    if (!$process.HasExited) { throw 'Menu flow timed out' }
    if ($process.ExitCode -ne 0 -or !(Select-String -LiteralPath $log -Pattern 'VORTEX_MENU_FLOW_SMOKE_PASS' -Quiet)) {
        Select-String -LiteralPath $log -Pattern 'VORTEX_MENU_FLOW_SMOKE_|Error:|host resumed' | Select-Object -Last 12
        throw "Menu flow failed with exit code $($process.ExitCode)"
    }
    Write-Output 'VORTEX_MENU_FLOW_SMOKE_PASS create -> save -> load -> resume position -> menu'
}
finally {
    if (!$process.HasExited) { Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue }
}
