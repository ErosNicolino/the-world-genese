# HQ de abertura em rede: um jogador hospeda (anfitrião com tela, ?listen) e outro entra pelo IP. Cada um abre a
# própria HQ em momentos diferentes; o anfitrião fecha a dele e a do cliente continua (cada máquina tem a sua, com o
# próprio relógio e o próprio som). Logs em Saved/Logs/IntroNetHost.log e IntroNetClient.log.
param([int]$Port = 17791, [int]$TimeoutSeconds = 420)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$project = Join-Path $root 'Vortex.uproject'
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$hostLog = Join-Path $root 'Saved\Logs\IntroNetHost.log'
$clientLog = Join-Path $root 'Saved\Logs\IntroNetClient.log'
Remove-Item -LiteralPath $hostLog, $clientLog -ErrorAction SilentlyContinue

function HasLine([string]$path, [string]$pattern) {
    return (Test-Path -LiteralPath $path) -and [bool](Select-String -LiteralPath $path -Pattern $pattern -Quiet)
}

$hostProc = $null
$clientProc = $null
$code = 1
try {
    $hostArgs = @("`"$project`"", '/Game/Vortex/Maps/L_TerrainProto?listen', '-game', "-port=$Port", '-dx12', '-RenderOffScreen',
        '-VortexNoInitialFade', '-unattended', '-windowed', '-ResX=960', '-ResY=540', '-VortexIntroNet=host', '-NoSplash', "`"-ABSLOG=$hostLog`"")
    $hostProc = Start-Process -FilePath $editor -ArgumentList $hostArgs -WindowStyle Hidden -PassThru
    $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
    while ((Get-Date) -lt $deadline -and !(HasLine $hostLog 'LogNet:.*Listening|GameNetDriver.*listening|IpNetDriver.*Listening')) {
        if ($hostProc.HasExited) { throw "Anfitrião fechou cedo: $($hostProc.ExitCode)" }
        Start-Sleep -Seconds 2
    }
    $clientArgs = @("`"$project`"", "127.0.0.1:$Port", '-game', '-dx12', '-RenderOffScreen', '-VortexNoInitialFade', '-unattended', '-windowed',
        '-ResX=960', '-ResY=540', '-VortexIntroNet=client', '-NoSplash', "`"-ABSLOG=$clientLog`"")
    $clientProc = Start-Process -FilePath $editor -ArgumentList $clientArgs -WindowStyle Hidden -PassThru
    while ((Get-Date) -lt $deadline) {
        if ((HasLine $hostLog 'VORTEX_INTRO_NET papel=host fim') -and (HasLine $clientLog 'VORTEX_INTRO_NET papel=client fim')) { break }
        if ($hostProc.HasExited -and $clientProc.HasExited) { break }
        Start-Sleep -Seconds 2
    }
    Select-String -LiteralPath $hostLog, $clientLog -Pattern 'VORTEX_INTRO_NET|VORTEX_INTRO (aberta|fechada)' | ForEach-Object { $_.Line -replace '^.*LogTemp: (Display: )?', '' }
    $ok = (HasLine $hostLog 'papel=host abriu aberta=1 jogadores=2') -and
          (HasLine $clientLog 'papel=client abriu aberta=1 jogadores=2') -and
          (HasLine $hostLog 'papel=host fechou aberta=0') -and
          (HasLine $clientLog 'papel=client depois-do-anfitriao-fechar aberta=1') -and
          (HasLine $clientLog 'papel=client fechou aberta=0')
    if ($ok) { Write-Output 'INTRO_NETWORK_PASS cada jogador abre, assiste e fecha a sua HQ, sem mexer na do outro'; $code = 0 }
    else { Write-Output 'INTRO_NETWORK_FAIL' }
}
finally {
    foreach ($p in @($clientProc, $hostProc)) { if ($p -and !$p.HasExited) { Stop-Process -Id $p.Id -ErrorAction SilentlyContinue } }
}
exit $code
