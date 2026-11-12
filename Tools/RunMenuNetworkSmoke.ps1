param([int]$Port = 18777, [int]$TimeoutSeconds = 120)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$project = Join-Path $root 'Vortex.uproject'
$serverLog = Join-Path $root 'Saved\Logs\MenuHost.log'
$clientLog = Join-Path $root 'Saved\Logs\MenuJoin.log'
Remove-Item -LiteralPath $serverLog, $clientLog -ErrorAction SilentlyContinue
$server = $null
$client = $null
$code = 1
try {
    $server = Start-Process -FilePath $editor -WindowStyle Hidden -PassThru -ArgumentList @(
        "`"$project`"", '/Game/Vortex/Maps/L_TerrainProto?listen', '-game', "-port=$Port",
        '-unattended', '-nullrhi', '-nosound', '-NoSplash', "`"-ABSLOG=$serverLog`"")
    $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
    while ((Get-Date) -lt $deadline) {
        $server.Refresh()
        if ($server.HasExited) { throw "Host exited: $($server.ExitCode)" }
        if ((Test-Path -LiteralPath $serverLog) -and
            (Select-String -LiteralPath $serverLog -Pattern 'LogNet:.*(Listening|IpNetDriver|GameNetDriver)' -Quiet)) { break }
        Start-Sleep -Seconds 2
    }
    if (!(Test-Path -LiteralPath $serverLog) -or
        !(Select-String -LiteralPath $serverLog -Pattern 'LogNet:.*(Listening|IpNetDriver|GameNetDriver)' -Quiet)) {
        throw 'Host did not start listening'
    }
    $client = Start-Process -FilePath $editor -WindowStyle Hidden -PassThru -ArgumentList @(
        "`"$project`"", '-game', '-unattended', '-nullrhi', '-nosound', '-NoSplash',
        "-VortexMenuJoinSmoke=127.0.0.1:$Port", "`"-ABSLOG=$clientLog`"")
    $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
    while ((Get-Date) -lt $deadline) {
        $client.Refresh()
        if ((Test-Path -LiteralPath $clientLog) -and
            (Select-String -LiteralPath $clientLog -Pattern 'VORTEX_MENU_JOIN_SMOKE_(PASS|FAIL)' -Quiet)) { break }
        if ($client.HasExited) { break }
        Start-Sleep -Seconds 2
    }
    if (!(Test-Path -LiteralPath $clientLog) -or
        !(Select-String -LiteralPath $clientLog -Pattern 'VORTEX_MENU_JOIN_SMOKE_PASS' -Quiet)) {
        throw 'Menu client did not join the host'
    }
    Write-Output 'VORTEX_MENU_JOIN_SMOKE_PASS menu -> direct LAN address -> replicated hosted world'
    $code = 0
}
catch {
    Write-Output "VORTEX_MENU_JOIN_SMOKE_FAIL $($_.Exception.Message)"
    if (Test-Path -LiteralPath $clientLog) {
        Select-String -LiteralPath $clientLog -Pattern 'VORTEX_MENU_JOIN|Error|LogNet:' | Select-Object -Last 12
    }
}
finally {
    foreach ($process in @($client, $server)) {
        if ($process) { try { Stop-Process -Id $process.Id -Force -ErrorAction Stop } catch { } }
    }
}
exit $code
