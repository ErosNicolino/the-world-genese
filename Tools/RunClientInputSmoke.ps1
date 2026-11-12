param([int]$Port = 17779, [int]$TimeoutSeconds = 120, [int]$LagMs = 0, [int]$LossPct = 0)
# -LagMs/-LossPct: atraso (metade em cada lado) e perda de pacotes emulados; conta as correções
# do servidor (p.NetShowCorrections) nos dois registros.

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$project = Join-Path $projectRoot 'Vortex.uproject'
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$serverLog = Join-Path $projectRoot 'Saved\Logs\InputServer.log'
$clientLog = Join-Path $projectRoot 'Saved\Logs\InputClient.log'
Remove-Item -LiteralPath $serverLog, $clientLog -ErrorAction SilentlyContinue
$server = $null
$client = $null

$half = [int]($LagMs / 2)
function Emulation { return "NetEmulation.PktLag $half,NetEmulation.PktLoss $LossPct,p.NetShowCorrections 1" }

function HasLine([string]$path, [string]$pattern) {
    return (Test-Path -LiteralPath $path) -and [bool](Select-String -LiteralPath $path -Pattern $pattern -Quiet)
}

$code = 1
try {
    $serverArgs = @(
        "`"$project`"", '/Game/Vortex/Maps/L_Framework', '-server', "-port=$Port",
        '-VortexClientInputObserve', '-unattended', '-nullrhi', '-nosound', '-NoSplash',
        "`"-ABSLOG=$serverLog`"", "`"-ExecCmds=$(Emulation)`""
    )
    $server = Start-Process -FilePath $editor -ArgumentList $serverArgs -WindowStyle Hidden -PassThru
    $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
    while ((Get-Date) -lt $deadline -and !(HasLine $serverLog 'LogNet:.*Listening')) {
        if ($server.HasExited) { throw "Server exited early: $($server.ExitCode)" }
        Start-Sleep -Seconds 2
    }
    if (!(HasLine $serverLog 'LogNet:.*Listening')) { throw 'Server did not listen' }

    $clientArgs = @(
        "`"$project`"", "127.0.0.1:$Port", '-game', '-VortexClientInputSmoke',
        '-unattended', '-nullrhi', '-nosound', '-NoSplash', "`"-ABSLOG=$clientLog`"", "`"-ExecCmds=$(Emulation)`""
    )
    $client = Start-Process -FilePath $editor -ArgumentList $clientArgs -WindowStyle Hidden -PassThru
    while ((Get-Date) -lt $deadline) {
        if ((HasLine $serverLog 'SERVER_INPUT_SMOKE_PASS') -and
            (HasLine $clientLog 'CLIENT_INPUT_SMOKE_PASS')) {
            Start-Sleep -Seconds 1
            $serverFixes = @(Select-String -LiteralPath $serverLog -Pattern 'Server: Error for').Count
            $clientFixes = @(Select-String -LiteralPath $clientLog -Pattern 'Client: Error for').Count
            Write-Output "CLIENT_INPUT_SMOKE_PASS movement, sprint, jump and dodge reached server (lag=${LagMs}ms loss=${LossPct}% correcoes_servidor=$serverFixes correcoes_cliente=$clientFixes)"
            $code = 0
            break
        }
        if ($client.HasExited) { throw "Client exited early: $($client.ExitCode)" }
        Start-Sleep -Seconds 2
    }
    if ($code -ne 0) { throw 'Timed out waiting for client and server input results' }
}
catch {
    Write-Output "CLIENT_INPUT_SMOKE_FAIL $($_.Exception.Message)"
    if (Test-Path -LiteralPath $serverLog) {
        Select-String -LiteralPath $serverLog -Pattern 'SERVER_INPUT_SMOKE_|Error:' | Select-Object -Last 6
    }
    if (Test-Path -LiteralPath $clientLog) {
        Select-String -LiteralPath $clientLog -Pattern 'CLIENT_INPUT_SMOKE_|Error:' | Select-Object -Last 6
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
