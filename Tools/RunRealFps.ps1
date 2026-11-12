# Mede o FPS no caminho de verdade do jogador (menu -> Continuar -> o último herói -> mundo), com o save dele.
# O tempo de cada parte da HUD vai para o log (VORTEX_PROBE a cada 10 s); o FPS sai do contador de quadros do log.
# Uso: Tools/RunRealFps.ps1 [-ResX 1920 -ResY 1080] [-Seconds 120] [-Extra "-VortexTownNoInner"] [-Tag nome]
param([int]$ResX = 1920, [int]$ResY = 1080, [int]$Seconds = 120, [string]$Extra = '', [string]$Tag = 'real')
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$project = Join-Path $root 'Vortex.uproject'
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$log = Join-Path $root "Saved\Logs\RealFps_$Tag.log"
Remove-Item -LiteralPath $log -ErrorAction SilentlyContinue
$arguments = @("`"$project`"", '-game', '-dx12', '-windowed', "-ResX=$ResX", "-ResY=$ResY", '-VortexAutoContinue', '-VortexProbeLog',
    '-nosound', '-NoSplash', "`"-ABSLOG=$log`"")
if ($Extra) { $arguments += $Extra.Split(' ') }
$process = Start-Process -FilePath $editor -ArgumentList $arguments -PassThru
try { $null = $process.WaitForExit($Seconds * 1000) }
finally { if (!$process.HasExited) { Stop-Process -Id $process.Id -ErrorAction SilentlyContinue } }
# FPS: quantas vezes a HUD desenhou em cada janela de 10 s
Select-String -LiteralPath $log -Pattern 'VORTEX_PROBE HUD +total' | ForEach-Object {
    if ($_.Line -match 'vezes=(\d+)') { '{0:N1} fps' -f ([int]$Matches[1] / 10.0) }
}
