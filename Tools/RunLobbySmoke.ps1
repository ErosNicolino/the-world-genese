# Sala de espera na rede local: um anfitrião e N convidados (heróis viajantes) entram, ficam prontos e
# viajam juntos ao mundo. Foto em Saved/Screenshots/sala_espera.png.
param([int]$Guests = 3, [int]$Port = 18787, [int]$TimeoutSeconds = 420)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$project = Join-Path $root 'Vortex.uproject'
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$hostLog = Join-Path $root 'Saved\Logs\LobbyHost.log'
Remove-Item -LiteralPath (Join-Path $root 'Saved\Screenshots\sala_espera.png') -ErrorAction SilentlyContinue
$processes = @()
$code = 1
try {
    Remove-Item -LiteralPath $hostLog -ErrorAction SilentlyContinue
    $processes += Start-Process -FilePath $editor -WindowStyle Hidden -PassThru -ArgumentList @(
        "`"$project`"", '-game', '-dx12', '-RenderOffScreen', '-windowed', '-ResX=1280', '-ResY=720', "-port=$Port",
        '-unattended', '-nosound', '-NoSplash', '-VortexLobbySmoke', "-VortexLobbyGuests=$Guests", "`"-ABSLOG=$hostLog`"")
    $deadline = (Get-Date).AddSeconds(180)
    while ((Get-Date) -lt $deadline) {
        if ((Test-Path -LiteralPath $hostLog) -and (Select-String -LiteralPath $hostLog -Pattern 'VORTEX_LOBBY no mapa da sala' -Quiet)) { break }
        if ($processes[0].HasExited) { throw 'Host exited early' }
        Start-Sleep -Seconds 2
    }
    Start-Sleep -Seconds 3
    for ($i = 0; $i -lt $Guests; $i++) {
        $log = Join-Path $root "Saved\Logs\LobbyGuest$i.log"
        Remove-Item -LiteralPath $log -ErrorAction SilentlyContinue
        $processes += Start-Process -FilePath $editor -WindowStyle Hidden -PassThru -ArgumentList @(
            "`"$project`"", '-game', '-nullrhi', '-unattended', '-nosound', '-NoSplash', '-VortexLobbySmoke',
            "-VortexLobbyJoin=127.0.0.1:$Port", "-VortexLobbyIndex=$i", "-VortexLobbyGuests=$Guests", "`"-ABSLOG=$log`"")
        Start-Sleep -Seconds 4
    }
    $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
    while ((Get-Date) -lt $deadline) {
        if (($processes | Where-Object { !$_.HasExited }).Count -eq 0) { break }
        Start-Sleep -Seconds 2
    }
    $pass = $true
    foreach ($log in @($hostLog) + (0..($Guests - 1) | ForEach-Object { Join-Path $root "Saved\Logs\LobbyGuest$_.log" })) {
        Write-Output "--- $(Split-Path -Leaf $log)"
        Select-String -LiteralPath $log -Pattern 'VORTEX_LOBBY|VORTEX_HERO_LOADED|Fatal error' | ForEach-Object { $_.Line -replace '^.*LogTemp: (Display: |Error: )?', '' }
        if (!(Select-String -LiteralPath $log -Pattern 'VORTEX_LOBBY_SMOKE_(HOST|GUEST) PASS' -Quiet)) { $pass = $false }
    }
    if ($pass) { Write-Output 'VORTEX_LOBBY_SMOKE_PASS'; $code = 0 } else { Write-Output 'VORTEX_LOBBY_SMOKE_FAIL' }
}
finally {
    foreach ($process in $processes) { if ($process -and !$process.HasExited) { try { Stop-Process -Id $process.Id -Force } catch { } } }
}
exit $code
