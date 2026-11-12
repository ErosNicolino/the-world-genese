param(
    [int]$Port = 17777,
    [int]$TimeoutSeconds = 150
)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$project = Join-Path $projectRoot 'Vortex.uproject'
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$logs = Join-Path $projectRoot 'Saved\Logs'
New-Item -ItemType Directory -Path $logs -Force | Out-Null
$serverLog = Join-Path $logs 'FrameworkServer.log'
$clientLog = Join-Path $logs 'FrameworkClient.log'
Remove-Item -LiteralPath $serverLog, $clientLog -ErrorAction SilentlyContinue
$server = $null
$client = $null

function HasLine([string]$path, [string]$pattern) {
    return (Test-Path -LiteralPath $path) -and [bool](Select-String -LiteralPath $path -Pattern $pattern -Quiet)
}

$code = 1
try {
    $serverArgs = @(
        "`"$project`"", '/Game/Vortex/Maps/L_Framework', '-server', "-port=$Port",
        '-unattended', '-nullrhi', '-nosound', '-NoSplash', "`"-ABSLOG=$serverLog`""
    )
    $server = Start-Process -FilePath $editor -ArgumentList $serverArgs -WindowStyle Hidden -PassThru
    $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
    while ((Get-Date) -lt $deadline) {
        if ($server.HasExited) { throw "Server exited early: $($server.ExitCode)" }
        if (HasLine $serverLog 'LogNet:.*(Listening|IpNetDriver|GameNetDriver)') { break }
        Start-Sleep -Seconds 2
    }
    if (!(HasLine $serverLog 'LogNet:.*(Listening|IpNetDriver|GameNetDriver)')) {
        throw 'Server did not initialize networking within timeout'
    }

    $clientArgs = @(
        "`"$project`"", "127.0.0.1:$Port", '-game', '-unattended', '-nullrhi',
        '-nosound', '-NoSplash', "`"-ABSLOG=$clientLog`""
    )
    $client = Start-Process -FilePath $editor -ArgumentList $clientArgs -WindowStyle Hidden -PassThru
    $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
    while ((Get-Date) -lt $deadline) {
        if ($client.HasExited) { throw "Client exited early: $($client.ExitCode)" }
        if ((HasLine $serverLog 'Vortex framework: server PostLogin, players=1') -and
            (HasLine $clientLog 'Vortex framework: client received connected players=1') -and
            (HasLine $clientLog 'Vortex framework: client received session slot=0') -and
            (HasLine $clientLog 'Vortex world: client received descriptor id=')) {
            Write-Output 'NETWORK_SMOKE_PASS host login, game state, player state and world descriptor replicated'
            $code = 0
            break
        }
        Start-Sleep -Seconds 2
    }
    if ($code -ne 0) { throw 'Client did not receive expected replicated state within timeout' }
}
catch {
    Write-Output "NETWORK_SMOKE_FAIL $($_.Exception.Message)"
    if (Test-Path -LiteralPath $serverLog) {
        Get-Content -LiteralPath $serverLog -Tail 35 | Select-String 'Error|Warning|LogNet|Vortex framework'
    }
    if (Test-Path -LiteralPath $clientLog) {
        Get-Content -LiteralPath $clientLog -Tail 35 | Select-String 'Error|Warning|LogNet|Vortex framework'
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
