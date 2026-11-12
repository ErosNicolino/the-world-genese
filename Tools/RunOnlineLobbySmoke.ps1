# Sala online pela Epic com dois jogos neste PC, como dois amigos em redes diferentes:
#   anfitrião: cria o mundo e o herói, abre a sala na Epic e grava o código em Saved/Logs/sala_online.txt;
#   convidado: entra pelo código SEM herói, cria o herói já dentro da sala, fica pronto e viaja junto.
# Cada jogo usa o seu cache da Epic, e o convidado roda com a pasta de dados do Windows (LOCALAPPDATA/
# APPDATA) apontada para Saved/OnlineSmokeGuest: assim a Epic dá a ele outro id de aparelho (outro jogador).
# A conexão vai pelo P2P/relay da Epic, o mesmo caminho do jogo instalado.
param([int]$TimeoutSeconds = 600)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$project = Join-Path $root 'Vortex.uproject'
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$logs = Join-Path $root 'Saved\Logs'
$hostLog = Join-Path $logs 'OnlineHost.log'
$guestLog = Join-Path $logs 'OnlineGuest.log'
$guestHome = Join-Path $root 'Saved\OnlineSmokeGuest'
$code = 1
$processes = @()
$savedLocal = $env:LOCALAPPDATA
$savedRoaming = $env:APPDATA
try {
    Remove-Item -LiteralPath $hostLog, $guestLog, (Join-Path $logs 'sala_online.txt') -ErrorAction SilentlyContinue
    $processes += Start-Process -FilePath $editor -WindowStyle Hidden -PassThru -ArgumentList @(
        "`"$project`"", '-game', '-dx12', '-RenderOffScreen', '-windowed', '-ResX=1280', '-ResY=720',
        '-unattended', '-nosound', '-NoSplash', '-VortexLobbySmoke', '-VortexLobbyOnline', '-VortexLobbyGuests=1',
        '-VortexEosCache=EOSCache_Host', "`"-ABSLOG=$hostLog`"")
    # o convidado só abre depois da sala existir
    $deadline = (Get-Date).AddSeconds(240)
    while ((Get-Date) -lt $deadline -and !(Test-Path (Join-Path $logs 'sala_online.txt'))) {
        if ($processes[0].HasExited) { throw 'O anfitrião fechou antes de abrir a sala' }
        Start-Sleep -Seconds 2
    }
    if (!(Test-Path (Join-Path $logs 'sala_online.txt'))) { throw 'A sala online não abriu' }
    New-Item -ItemType Directory -Force (Join-Path $guestHome 'Local'), (Join-Path $guestHome 'Roaming') | Out-Null
    $env:LOCALAPPDATA = Join-Path $guestHome 'Local'
    $env:APPDATA = Join-Path $guestHome 'Roaming'
    $processes += Start-Process -FilePath $editor -WindowStyle Hidden -PassThru -ArgumentList @(
        "`"$project`"", '-game', '-nullrhi', '-unattended', '-nosound', '-NoSplash', '-VortexLobbySmoke', '-VortexLobbyOnline',
        '-VortexLobbyJoin=online', '-VortexLobbyIndex=0', '-VortexLobbyGuests=1', '-VortexEosCache=EOSCache_Guest', '-VortexEosFreshDevice', "`"-ABSLOG=$guestLog`"")
    $env:LOCALAPPDATA = $savedLocal
    $env:APPDATA = $savedRoaming
    $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
    while ((Get-Date) -lt $deadline) {
        if (($processes | Where-Object { !$_.HasExited }).Count -eq 0) { break }
        Start-Sleep -Seconds 2
    }
    $pass = $true
    foreach ($log in @($hostLog, $guestLog)) {
        Write-Output "--- $(Split-Path -Leaf $log)"
        if (Test-Path $log) {
            Select-String -LiteralPath $log -Pattern 'VORTEX_LOBBY|VORTEX_REDE|VORTEX_HERO_LOADED|Fatal error|TravelFailure|NetworkFailure' |
                ForEach-Object { $_.Line -replace '^.*LogTemp: (Display: |Error: |Warning: )?', '' }
        }
        if (!(Test-Path $log) -or !(Select-String -LiteralPath $log -Pattern 'VORTEX_LOBBY_SMOKE_(HOST|GUEST) PASS' -Quiet)) { $pass = $false }
    }
    if ($pass) { Write-Output 'VORTEX_ONLINE_SMOKE_PASS'; $code = 0 } else { Write-Output 'VORTEX_ONLINE_SMOKE_FAIL' }
}
finally {
    $env:LOCALAPPDATA = $savedLocal
    $env:APPDATA = $savedRoaming
    foreach ($process in $processes) { if ($process -and !$process.HasExited) { try { Stop-Process -Id $process.Id -Force } catch { } } }
}
exit $code
