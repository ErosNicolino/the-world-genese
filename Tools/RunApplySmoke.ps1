# O "Aplicar" das opções no mundo: mudança pendente, tela de carregamento, o chão remonta parado e o DLSS.
# Escreve VORTEX_APPLY_SMOKE_PASS/FAIL.
param([string]$Extra = '', [int]$TimeoutSeconds = 600, [int]$ResX = 1280, [int]$ResY = 720, [string]$Console = '')
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$project = Join-Path $root 'Vortex.uproject'
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$log = Join-Path $root 'Saved\Logs\ApplySmoke.log'
Remove-Item -LiteralPath $log -ErrorAction SilentlyContinue
$cmds = 't.MaxFPS 0,r.VSync 0'
# -Console: comandos a mais (ex.: 'sg.ShadowQuality 3') para comparar qualidades
if ($Console) { $cmds += ',' + $Console }
$args = @("`"$project`"", '/Game/Vortex/Maps/L_TerrainProto', '-game', '-dx12', '-RenderOffScreen', '-unattended', '-windowed', "-ResX=$ResX", "-ResY=$ResY", '-ForceRes',
    '-VortexApplySmoke', '-VortexNoInitialFade', '-nosound', '-NoSplash', "`"-ExecCmds=$cmds`"", "`"-ABSLOG=$log`"")
if ($Extra) { $args += $Extra.Split(' ') }
$p = Start-Process -FilePath $editor -ArgumentList $args -WindowStyle Hidden -PassThru
if (!$p.WaitForExit($TimeoutSeconds * 1000)) { Stop-Process -Id $p.Id -Force; throw 'Apply smoke timed out' }
Select-String -LiteralPath $log -Pattern 'VORTEX_APPLY_SMOKE|Fatal error' | ForEach-Object { $_.Line -replace '^.*LogTemp: (Display: )?', '' }
