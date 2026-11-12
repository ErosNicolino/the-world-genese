param([int]$Port = 17778, [int]$TimeoutSeconds = 120)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$project = Join-Path $projectRoot 'Vortex.uproject'
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$serverLog = Join-Path $projectRoot 'Saved\Logs\MovementServer.log'
$clientLog = Join-Path $projectRoot 'Saved\Logs\MovementClient.log'
Remove-Item -LiteralPath $serverLog, $clientLog -ErrorAction SilentlyContinue
$server = $null
$client = $null

function HasLine([string]$path, [string]$pattern) {
    return (Test-Path -LiteralPath $path) -and [bool](Select-String -LiteralPath $path -Pattern $pattern -Quiet)
}

$code = 1
try {
    $serverArgs = @(
        "`"$project`"", '/Game/Vortex/Maps/L_MovementProbe', '-server', "-port=$Port",
        '-VortexMovementSmoke', '-unattended', '-nullrhi', '-nosound', '-NoSplash',
        "`"-ABSLOG=$serverLog`""
    )
    $server = Start-Process -FilePath $editor -ArgumentList $serverArgs -WindowStyle Hidden -PassThru
    $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
    while ((Get-Date) -lt $deadline -and !(HasLine $serverLog 'LogNet:.*Listening')) {
        if ($server.HasExited) { throw "Server exited early: $($server.ExitCode)" }
        Start-Sleep -Seconds 2
    }
    if (!(HasLine $serverLog 'LogNet:.*Listening')) { throw 'Server did not listen' }

    $clientArgs = @(
        "`"$project`"", "127.0.0.1:$Port", '-game', '-VortexMovementObserve',
        '-unattended', '-nullrhi', '-nosound', '-NoSplash', "`"-ABSLOG=$clientLog`""
    )
    $client = Start-Process -FilePath $editor -ArgumentList $clientArgs -WindowStyle Hidden -PassThru
    while ((Get-Date) -lt $deadline) {
        if ((HasLine $serverLog 'MOVEMENT_SMOKE_PASS') -and
            (HasLine $clientLog 'Vortex movement: client observed replicated x=')) {
            Write-Output 'MOVEMENT_NETWORK_SMOKE_PASS host physics and client position'
            $code = 0
            break
        }
        if ($client.HasExited) { throw "Client exited early: $($client.ExitCode)" }
        Start-Sleep -Seconds 2
    }
    if ($code -ne 0) { throw 'Timed out waiting for server movement and client observation' }
}
catch {
    Write-Output "MOVEMENT_NETWORK_SMOKE_FAIL $($_.Exception.Message)"
    if (Test-Path -LiteralPath $serverLog) {
        Select-String -LiteralPath $serverLog -Pattern 'MOVEMENT_SMOKE_|Vortex framework: server PostLogin|Error:' | Select-Object -Last 8
    }
    if (Test-Path -LiteralPath $clientLog) {
        Select-String -LiteralPath $clientLog -Pattern 'Vortex movement:|Vortex framework: client received pawn|Error:' | Select-Object -Last 8
    }
    $code = 1
}
finally {
    # sempre encerra servidor e cliente (antes ficavam presos ocupando ~600 MB cada)
    foreach ($p in @($client, $server)) {
        if ($p) { try { Stop-Process -Id $p.Id -Force -ErrorAction Stop } catch { } }
    }
}
exit $code
