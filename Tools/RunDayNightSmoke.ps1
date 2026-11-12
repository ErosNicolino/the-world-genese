# Fase 15: relógio do dia em servidor dedicado + cliente. O servidor confere conversões, virada
# do dia e hora no save; o cliente precisa calcular o mesmo relógio para T = 1000 s de servidor.
param([int]$Port = 17795, [int]$TimeoutSeconds = 180)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$project = Join-Path $root 'Vortex.uproject'
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$serverLog = Join-Path $root 'Saved\Logs\DayNightServer.log'
$clientLog = Join-Path $root 'Saved\Logs\DayNightClient.log'
Remove-Item -LiteralPath $serverLog,$clientLog -ErrorAction SilentlyContinue
$processes = @()
$code = 1
function HasLine([string]$path, [string]$pattern) {
    return (Test-Path -LiteralPath $path) -and [bool](Select-String -LiteralPath $path -Pattern $pattern -Quiet)
}
try {
    $server = Start-Process -FilePath $editor -ArgumentList @("`"$project`"", '/Game/Vortex/Maps/L_TerrainProto', '-server', "-port=$Port",
        '-VortexDayNightSmoke', '-unattended', '-nullrhi', '-nosound', '-NoSplash', "`"-ABSLOG=$serverLog`"") -WindowStyle Hidden -PassThru
    $processes += $server
    $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
    while ((Get-Date) -lt $deadline -and !(HasLine $serverLog 'LogNet:.*(Listening|IpNetDriver|GameNetDriver)')) {
        if ($server.HasExited) { throw "Server exited early: $($server.ExitCode)" }
        Start-Sleep -Seconds 2
    }
    $processes += Start-Process -FilePath $editor -ArgumentList @("`"$project`"", "127.0.0.1:$Port", '-game', '-VortexDayNightSmoke',
        '-unattended', '-nullrhi', '-nosound', '-NoSplash', "`"-ABSLOG=$clientLog`"") -WindowStyle Hidden -PassThru
    if (!$server.WaitForExit($TimeoutSeconds * 1000)) { throw 'Server did not finish' }
    $serverLine = (Select-String -LiteralPath $serverLog -Pattern 'VORTEX_DAYNIGHT_SMOKE_' | Select-Object -Last 1).Line
    $clientLine = (Select-String -LiteralPath $clientLog -Pattern 'VORTEX_DAYNIGHT_CLIENT' | Select-Object -Last 1).Line
    'servidor: ' + ($serverLine -replace '^.*LogTemp: Display: ', '')
    'cliente: ' + ($clientLine -replace '^.*LogTemp: Display: ', '')
    if ($serverLine -notmatch '_PASS') { throw 'server check failed' }
    $s = [double]::Parse(([regex]::Match($serverLine, 'relogio_T1000=([\d.]+)')).Groups[1].Value, [Globalization.CultureInfo]::InvariantCulture)
    $c = [double]::Parse(([regex]::Match($clientLine, 'relogio_T1000=([\d.]+)')).Groups[1].Value, [Globalization.CultureInfo]::InvariantCulture)
    if ([math]::Abs($s - $c) -gt 0.01) { throw "client clock differs: $c vs $s" }
    Write-Output 'DAYNIGHT_SMOKE_PASS'
    $code = 0
}
catch { Write-Output "DAYNIGHT_SMOKE_FAIL $($_.Exception.Message)" }
finally { foreach ($p in $processes) { try { Stop-Process -Id $p.Id -Force -ErrorAction Stop } catch { } } }
exit $code
