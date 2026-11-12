# Jogador com jogador (grupo, duelo, troca, amizade): anfitrião que desenha a tela + um cliente.
# Fotos da HUD do anfitrião em Saved/Screenshots/social_*.png.
param([int]$Port = 17791, [int]$TimeoutSeconds = 240)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$project = Join-Path $root 'Vortex.uproject'
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$hostLog = Join-Path $root 'Saved\Logs\SocialHost.log'
$clientLog = Join-Path $root 'Saved\Logs\SocialClient.log'
Remove-Item -LiteralPath $hostLog,$clientLog -ErrorAction SilentlyContinue
Get-ChildItem -LiteralPath (Join-Path $root 'Saved\Screenshots') -Filter 'social_*.png' -ErrorAction SilentlyContinue | Remove-Item
$hostProc = $null; $clientProc = $null; $code = 1
function HasLine([string]$path, [string]$pattern) { return (Test-Path -LiteralPath $path) -and [bool](Select-String -LiteralPath $path -Pattern $pattern -Quiet) }
try {
    $hostArgs = @("`"$project`"", '/Game/Vortex/Maps/L_TerrainProto?listen', '-game', "-port=$Port", '-dx12', '-RenderOffScreen', '-VortexNoInitialFade',
        '-unattended', '-windowed', '-ResX=1280', '-ResY=720', '-VortexSocialSmoke', '-VortexNoCreatures', '-nosound', '-NoSplash', "`"-ABSLOG=$hostLog`"")
    $hostProc = Start-Process -FilePath $editor -ArgumentList $hostArgs -WindowStyle Hidden -PassThru
    $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
    while ((Get-Date) -lt $deadline -and !(HasLine $hostLog 'LogNet:.*(Listening|IpNetDriver|GameNetDriver)')) {
        if ($hostProc.HasExited) { throw "Host exited early: $($hostProc.ExitCode)" }
        Start-Sleep -Seconds 2
    }
    $clientArgs = @("`"$project`"", "127.0.0.1:$Port", '-game', '-unattended', '-nullrhi', '-nosound', '-NoSplash', "`"-ABSLOG=$clientLog`"")
    $clientProc = Start-Process -FilePath $editor -ArgumentList $clientArgs -WindowStyle Hidden -PassThru
    if (!$hostProc.WaitForExit($TimeoutSeconds * 1000)) { throw 'Social smoke timed out' }
    Select-String -LiteralPath $hostLog -Pattern 'VORTEX_SOCIAL_SMOKE|VORTEX_DUEL|VORTEX_TRADE' | ForEach-Object { $_.Line -replace '^.*LogTemp: (Display: )?', '' }
    if (HasLine $hostLog 'VORTEX_SOCIAL_SMOKE_PASS') { $code = 0 } else { throw 'Social smoke failed' }
}
catch { Write-Output "SOCIAL_SMOKE_FAIL $($_.Exception.Message)"; $code = 1 }
finally { foreach ($p in @($clientProc, $hostProc)) { if ($p -and !$p.HasExited) { try { Stop-Process -Id $p.Id -Force } catch { } } } }
exit $code
