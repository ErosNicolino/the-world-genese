# Fase 14: população do vale. Padrão: jogo local. -Network: servidor dedicado + 1 cliente, que
# precisa ver os 10 NPCs do protótipo aparecerem e sumirem pelo próprio chão. -Shot (local): foto da
# praça em Saved/Screenshots/npc_praca.png.
param([switch]$Network, [switch]$Shot, [int]$Port = 17793, [int]$TimeoutSeconds = 240)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$project = Join-Path $root 'Vortex.uproject'
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$mode = if ($Network) { 'Network' } else { 'Local' }
$serverLog = Join-Path $root "Saved\Logs\Population${mode}Server.log"
$clientLog = Join-Path $root "Saved\Logs\Population${mode}Client.log"
Remove-Item -LiteralPath $serverLog,$clientLog -ErrorAction SilentlyContinue
$processes = @()
$code = 1

function HasLine([string]$path, [string]$pattern) {
    return (Test-Path -LiteralPath $path) -and [bool](Select-String -LiteralPath $path -Pattern $pattern -Quiet)
}

try {
    if ($Network) {
        $serverArgs = @("`"$project`"", '/Game/Vortex/Maps/L_TerrainProto', '-server', "-port=$Port")
    } else {
        $serverArgs = @("`"$project`"", '/Game/Vortex/Maps/L_TerrainProto', '-game')
    }
    if ($Shot -and !$Network) { $serverArgs += @('-dx12', '-RenderOffScreen', '-windowed', '-ResX=1280', '-ResY=720', '-VortexPopulationShot') }
    else { $serverArgs += '-nullrhi' }
    $serverArgs += @('-VortexPopulationSmoke', '-unattended', '-nosound', '-NoSplash', "`"-ABSLOG=$serverLog`"")
    $server = Start-Process -FilePath $editor -ArgumentList $serverArgs -WindowStyle Hidden -PassThru
    $processes += $server
    if ($Network) {
        $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
        while ((Get-Date) -lt $deadline -and !(HasLine $serverLog 'LogNet:.*(Listening|IpNetDriver|GameNetDriver)')) {
            if ($server.HasExited) { throw "Server exited early: $($server.ExitCode)" }
            Start-Sleep -Seconds 2
        }
        $clientArgs = @("`"$project`"", "127.0.0.1:$Port", '-game', '-unattended', '-nullrhi', '-nosound', '-NoSplash', "`"-ABSLOG=$clientLog`"")
        $processes += Start-Process -FilePath $editor -ArgumentList $clientArgs -WindowStyle Hidden -PassThru
    }
    if (!$server.WaitForExit($TimeoutSeconds * 1000)) { throw 'Server did not finish' }
    Start-Sleep -Seconds 1
    Select-String -LiteralPath $serverLog -Pattern 'VORTEX_POPULATION_SMOKE|Vortex population' | ForEach-Object { $_.Line -replace '^.*LogTemp: (Display: )?', 'servidor: ' }
    if (!(HasLine $serverLog 'VORTEX_POPULATION_SMOKE_PASS')) { throw 'server check failed' }
    if ($Network) {
        $visible = @(Select-String -LiteralPath $clientLog -Pattern 'VORTEX_NPC_VISIBLE id=(\w+)' | ForEach-Object { $_.Matches[0].Groups[1].Value })
        $gone = @(Select-String -LiteralPath $clientLog -Pattern 'VORTEX_NPC_GONE id=(\w+) shown=1' | ForEach-Object { $_.Matches[0].Groups[1].Value })
        $distinct = @($visible | Sort-Object -Unique)
        "cliente: visiveis=$($visible.Count) ids_distintos=$($distinct.Count) sumicos=$($gone.Count) ids=$($distinct -join ',')"
        if ($distinct.Count -lt 10) { throw 'client did not see the 10 prototype NPCs' }
        if ($gone.Count -lt 10) { throw 'client did not see the NPCs leave' }
        foreach ($outside in 'sahil','amal','tavian','lyra') { if ($distinct -contains $outside) { throw "client saw $outside outside the prototype" } }
    }
    Write-Output "POPULATION_SMOKE_PASS $mode"
    $code = 0
}
catch {
    Write-Output "POPULATION_SMOKE_FAIL $mode $($_.Exception.Message)"
    foreach ($log in @($serverLog,$clientLog)) {
        if (Test-Path -LiteralPath $log) {
            Select-String -LiteralPath $log -Pattern 'Fatal error|Error:|VORTEX_POPULATION' | Select-Object -Last 8 | ForEach-Object { $_.Line }
        }
    }
}
finally {
    foreach ($process in $processes) {
        if ($process) { try { Stop-Process -Id $process.Id -Force -ErrorAction Stop } catch { } }
    }
}
exit $code
