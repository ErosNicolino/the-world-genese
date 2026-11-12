# Fase 13: streaming com dois jogadores afastados. Padrão: servidor dedicado + 2 clientes.
# -Jumps: quantos saltos (padrão 8). -Listen: anfitrião (que também joga) + 1 cliente. O servidor confere chão nos dois pontos,
# níveis (visual só para quem desenha a tela), saltos rápidos e liberação; cada cliente avisa
# quando o chão e a malha do próprio pedaço estão prontos.
param([switch]$Listen, [int]$Jumps = 8, [int]$Port = 17791, [int]$TimeoutSeconds = 300)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$project = Join-Path $root 'Vortex.uproject'
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$mode = if ($Listen) { 'Listen' } else { 'Dedicated' }
$serverLog = Join-Path $root "Saved\Logs\Stream${mode}Server.log"
$clientLogs = @()
$clientCount = if ($Listen) { 1 } else { 2 }
for ($i = 1; $i -le $clientCount; $i++) { $clientLogs += Join-Path $root "Saved\Logs\Stream${mode}Client$i.log" }
Remove-Item -LiteralPath (@($serverLog) + $clientLogs) -ErrorAction SilentlyContinue
$processes = @()
$code = 1

function HasLine([string]$path, [string]$pattern) {
    return (Test-Path -LiteralPath $path) -and [bool](Select-String -LiteralPath $path -Pattern $pattern -Quiet)
}

try {
    if ($Listen) {
        $serverArgs = @("`"$project`"", '/Game/Vortex/Maps/L_TerrainProto?listen', '-game', "-port=$Port")
    } else {
        $serverArgs = @("`"$project`"", '/Game/Vortex/Maps/L_TerrainProto', '-server', "-port=$Port")
    }
    $serverArgs += @('-VortexStreamSmoke', "-VortexStreamJumps=$Jumps", '-unattended', '-nullrhi', '-nosound', '-NoSplash', "`"-ABSLOG=$serverLog`"")
    $server = Start-Process -FilePath $editor -ArgumentList $serverArgs -WindowStyle Hidden -PassThru
    $processes += $server
    $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
    while ((Get-Date) -lt $deadline -and !(HasLine $serverLog 'LogNet:.*(Listening|IpNetDriver|GameNetDriver)')) {
        if ($server.HasExited) { throw "Server exited early: $($server.ExitCode)" }
        Start-Sleep -Seconds 2
    }
    foreach ($log in $clientLogs) {
        $clientArgs = @("`"$project`"", "127.0.0.1:$Port", '-game', '-VortexStreamSmoke', '-unattended', '-nullrhi',
            '-nosound', '-NoSplash', "`"-ABSLOG=$log`"")
        $processes += Start-Process -FilePath $editor -ArgumentList $clientArgs -WindowStyle Hidden -PassThru
    }
    if (!$server.WaitForExit($TimeoutSeconds * 1000)) { throw 'Server did not finish' }
    Start-Sleep -Seconds 1
    Select-String -LiteralPath $serverLog -Pattern 'VORTEX_STREAM_SMOKE' | ForEach-Object { $_.Line -replace '^.*LogTemp: Display: ', 'servidor: ' }
    $i = 0
    foreach ($log in $clientLogs) {
        $i++
        if (Test-Path -LiteralPath $log) {
            Select-String -LiteralPath $log -Pattern 'VORTEX_STREAM_CLIENT_(READY|STATS)' | ForEach-Object { $_.Line -replace '^.*LogTemp: Display: ', "cliente ${i}: " }
        }
    }
    $clientAtB = $false
    foreach ($log in $clientLogs) { if (HasLine $log 'VORTEX_STREAM_CLIENT_READY at=\(0,0\)') { $clientAtB = $true } }
    if (!(HasLine $serverLog 'VORTEX_STREAM_SMOKE_PASS')) { throw 'server check failed' }
    if (!$clientAtB) { throw 'no client reported ground and mesh at the far point' }
    Write-Output "STREAM_NETWORK_SMOKE_PASS $mode"
    $code = 0
}
catch {
    Write-Output "STREAM_NETWORK_SMOKE_FAIL $mode $($_.Exception.Message)"
    foreach ($log in @($serverLog) + $clientLogs) {
        if (Test-Path -LiteralPath $log) {
            Select-String -LiteralPath $log -Pattern 'Fatal error|LogNet:.*Error' | Select-Object -Last 5 | ForEach-Object { $_.Line }
        }
    }
}
finally {
    foreach ($process in $processes) {
        if ($process) { try { Stop-Process -Id $process.Id -Force -ErrorAction Stop } catch { } }
    }
}
exit $code
