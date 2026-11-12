# Epic (EOS) neste PC: entra sem conta, cria uma sala e acha ela pelo código. Precisa de internet e de
# Data/Online/eos_credentials.json (Tools/CopyEosCredentials.py). O P2P entre dois jogadores precisa de dois PCs.
param([int]$TimeoutSeconds = 120)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$log = Join-Path $root 'Saved\Logs\OnlineProbe.log'
Remove-Item -LiteralPath $log -ErrorAction SilentlyContinue
$arguments = @("`"$(Join-Path $root 'Vortex.uproject')`"", '/Game/Vortex/Maps/L_Menu', '-game', '-RenderOffScreen', '-unattended', '-windowed',
    '-ResX=640', '-ResY=360', '-VortexOnlineProbe', '-nosound', '-NoSplash', "`"-ABSLOG=$log`"")
$process = Start-Process -FilePath 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe' -ArgumentList $arguments -WindowStyle Hidden -PassThru
$deadline = (Get-Date).AddSeconds($TimeoutSeconds)
while ((Get-Date) -lt $deadline -and !((Test-Path $log) -and (Select-String -LiteralPath $log -Pattern 'VORTEX_ONLINE_PROBE' -Quiet))) { Start-Sleep -Seconds 2 }
Stop-Process -Id $process.Id -ErrorAction SilentlyContinue
Select-String -LiteralPath $log -Pattern 'VORTEX_ONLINE|LogEOS.*(Error|Warning)' | Select-Object -First 20 | ForEach-Object { $_.Line -replace '^.*(LogTemp|LogEOS\w*): (Display: |Warning: |Error: )?', '' }
if (!(Select-String -LiteralPath $log -Pattern 'VORTEX_ONLINE_PROBE PASS' -Quiet)) { throw 'Online probe failed' }
