param([int]$Port = 17789, [int]$TimeoutSeconds = 150)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$project = Join-Path $root 'Vortex.uproject'
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$serverLog = Join-Path $root 'Saved\Logs\WolfServer.log'
$clientLog = Join-Path $root 'Saved\Logs\WolfClient.log'
Remove-Item -LiteralPath $serverLog,$clientLog -ErrorAction SilentlyContinue
$server = $null
$client = $null
$code = 1

function HasLine([string]$path, [string]$pattern) {
    return (Test-Path -LiteralPath $path) -and [bool](Select-String -LiteralPath $path -Pattern $pattern -Quiet)
}

try {
    $serverArgs = @("`"$project`"", '/Game/Vortex/Maps/L_TerrainProto', '-server', "-port=$Port",
        '-unattended', '-nullrhi', '-nosound', '-NoSplash', "`"-ABSLOG=$serverLog`"")
    $server = Start-Process -FilePath $editor -ArgumentList $serverArgs -WindowStyle Hidden -PassThru
    $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
    while ((Get-Date) -lt $deadline -and !(HasLine $serverLog 'LogNet:.*(Listening|IpNetDriver|GameNetDriver)')) {
        if ($server.HasExited) { throw "Server exited early: $($server.ExitCode)" }
        Start-Sleep -Seconds 2
    }
    if (!(HasLine $serverLog 'LogNet:.*(Listening|IpNetDriver|GameNetDriver)')) { throw 'Server did not start' }
    $clientArgs = @("`"$project`"", "127.0.0.1:$Port", '-game', '-unattended', '-nullrhi',
        '-nosound', '-NoSplash', "`"-ABSLOG=$clientLog`"")
    $client = Start-Process -FilePath $editor -ArgumentList $clientArgs -WindowStyle Hidden -PassThru
    $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
    while ((Get-Date) -lt $deadline) {
        if ($client.HasExited) { throw "Client exited early: $($client.ExitCode)" }
        if ((HasLine $serverLog 'VORTEX_WOLF_GROUND_READY') -and
            (HasLine $clientLog 'VORTEX_WOLF_VISIBLE net=3')) {
            Write-Output 'WOLF_NETWORK_SMOKE_PASS server ground and client visible after local terrain collision'
            Select-String -LiteralPath $serverLog,$clientLog -Pattern 'VORTEX_WOLF_GROUND_READY|VORTEX_WOLF_VISIBLE' |
                ForEach-Object { $_.Line }
            $code = 0
            break
        }
        Start-Sleep -Seconds 2
    }
    if ($code -ne 0) { throw 'Wolf did not become ready on server and client' }
}
catch {
    Write-Output "WOLF_NETWORK_SMOKE_FAIL $($_.Exception.Message)"
    foreach ($log in @($serverLog,$clientLog)) {
        if (Test-Path -LiteralPath $log) {
            Select-String -LiteralPath $log -Pattern 'VORTEX_WOLF_|Fatal error|LogNet:.*(Error|Warning)' |
                Select-Object -Last 15 | ForEach-Object { $_.Line }
        }
    }
    $code = 1
}
finally {
    foreach ($process in @($client,$server)) {
        if ($process) { try { Stop-Process -Id $process.Id -Force -ErrorAction Stop } catch { } }
    }
}
exit $code
