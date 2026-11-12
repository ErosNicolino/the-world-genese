# Auditoria das habilidades: lança as 4 de todas as classes contra alvos novos, fotografa cada uma
# (Saved/Screenshots/auditoria_<classe>_<n>.png) e mede o efeito (Saved/Logs/SkillAudit.txt).
param([int]$TimeoutSeconds = 1800)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$project = Join-Path $root 'Vortex.uproject'
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$log = Join-Path $root 'Saved\Logs\SkillAudit.log'
$shots = Join-Path $root 'Saved\Screenshots'
Remove-Item -LiteralPath $log -ErrorAction SilentlyContinue
Get-ChildItem -LiteralPath $shots -Filter 'auditoria_*.png' -ErrorAction SilentlyContinue | Remove-Item
$arguments = @("`"$project`"", '/Game/Vortex/Maps/L_TerrainProto', '-game', '-dx12', '-RenderOffScreen', '-VortexNoInitialFade',
    '-unattended', '-windowed', '-ResX=1280', '-ResY=720', '-VortexClassShots', '-VortexSkillAudit', '-nosound', '-NoSplash', "`"-ABSLOG=$log`"")
$process = Start-Process -FilePath $editor -ArgumentList $arguments -WindowStyle Hidden -PassThru
try {
    if (!$process.WaitForExit($TimeoutSeconds * 1000)) { throw 'Skill audit timed out' }
    $lines = Select-String -LiteralPath $log -Pattern 'VORTEX_SKILL_AUDIT|VORTEX_SKILL_GESTURE|VORTEX_CLASS_SHOTS' | ForEach-Object { $_.Line -replace '^.*LogTemp: (Display: )?', '' }
    $lines | Set-Content -Encoding utf8 (Join-Path $root 'Saved\Logs\SkillAudit.txt')
    $lines | Where-Object { $_ -match 'SEM_EFEITO|FALHOU|auditoria:' }
    if (!(Select-String -LiteralPath $log -Pattern 'VORTEX_CLASS_SHOTS_PASS' -Quiet)) { throw 'Skill audit failed' }
}
finally {
    if (!$process.HasExited) { Stop-Process -Id $process.Id -ErrorAction SilentlyContinue }
}
