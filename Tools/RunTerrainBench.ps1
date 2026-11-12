# Passeio na mata do protótipo de terreno (mesmo trajeto do --bench-wild do Godot), com janela de
# 1280x720 e sem limite de FPS. -Profile alto usa o padrão do projeto; lumen verifica que o
# padrão é Lumen; leve desliga GI/reflexos/sombras virtuais. -Shot grava uma foto no meio.
# -Hour: hora do mundo no passeio (Fase 15; sem ela, o mundo começa às 8h e o relógio corre).
# -Hud: mede com a HUD na tela (sem ela a HUD fica escondida na medição).
param([ValidateSet('alto', 'leve', 'lumen')][string]$Profile = 'alto', [switch]$Shot, [string]$Hour = '', [switch]$Hud, [string]$Extra = '', [int]$ResX = 1280, [int]$ResY = 720, [int]$TimeoutSeconds = 900)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$project = Join-Path $projectRoot 'Vortex.uproject'
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$detailedProfile = $Profile -eq 'alto' -and [bool]$env:VORTEX_BENCH_PROFILE
$logSuffix = if ($detailedProfile) { '_profile' } else { '' }
$log = Join-Path $projectRoot "Saved\Logs\TerrainBench_$Profile$logSuffix.log"
Remove-Item -LiteralPath $log -ErrorAction SilentlyContinue
$cmds = 't.MaxFPS 0,r.VSync 0'
# -Extra 'sg.GlobalIlluminationQuality 2,r.ScreenPercentage 75': compara qualidades
if ($Extra) { $cmds += ',' + $Extra }
if ($Profile -eq 'leve') {
    $cmds += ',r.DynamicGlobalIlluminationMethod 0,r.ReflectionMethod 2,r.Shadow.Virtual.Enable 0,r.Lumen.DiffuseIndirect.Allow 0'
}
$args = @(
    "`"$project`"", '/Game/Vortex/Maps/L_TerrainProto', '-game', '-dx12', '-RenderOffScreen', '-unattended', '-windowed', "-ResX=$ResX", "-ResY=$ResY", '-ForceRes',
    '-VortexTerrainBench', '-nosound', '-NoSplash', "`"-ExecCmds=$cmds`"", "`"-ABSLOG=$log`""
)
if ($Shot) { $args += '-VortexTerrainShot' }
if ($Hud) { $args += '-VortexBenchHud' }
if ($env:VORTEX_BENCH_ARGS) { $args += $env:VORTEX_BENCH_ARGS.Split(' ') }
if ($detailedProfile) { $args += '-VortexBenchProfile' }
if ($Hour) { $args += @("-VortexHour=$Hour", '-VortexDayScale=0') }
# Lumen não desliga por comando com o jogo rodando: no perfil leve a troca vai na inicialização
if ($Profile -eq 'leve') {
    $args += '"-ini:Engine:[/Script/Engine.RendererSettings]:r.DynamicGlobalIlluminationMethod=0,[/Script/Engine.RendererSettings]:r.ReflectionMethod=2,[/Script/Engine.RendererSettings]:r.Shadow.Virtual.Enable=0,[/Script/Engine.RendererSettings]:r.GenerateMeshDistanceFields=False"'
}
$process = Start-Process -FilePath $editor -ArgumentList $args -WindowStyle Hidden -PassThru
$code = 1
try {
    # memória do processo (privada e conjunto de trabalho), amostrada a cada 0,5 s
    $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
    [long]$peakPrivate = 0; [long]$peakWorking = 0; [long]$lastPrivate = 0
    while (!$process.HasExited) {
        if ((Get-Date) -gt $deadline) { throw 'tempo esgotado' }
        try {
            $process.Refresh()
            $lastPrivate = $process.PrivateMemorySize64
            if ($lastPrivate -gt $peakPrivate) { $peakPrivate = $lastPrivate }
            if ($process.WorkingSet64 -gt $peakWorking) { $peakWorking = $process.WorkingSet64 }
        } catch { }
        Start-Sleep -Milliseconds 500
    }
    $line = Select-String -LiteralPath $log -Pattern 'TERRAIN_BENCH fps' | Select-Object -Last 1
    if (!$line) { throw 'sem resultado' }
    "$Profile :: " + ($line.Line -split 'terrain: ')[1]
    if ($detailedProfile) { 'PERFIL DETALHADO: os contadores aumentam o tempo por quadro; nao compare estes FPS com o passeio normal.' }
    $render = (Select-String -LiteralPath $log -Pattern 'TERRAIN_BENCH_RENDER' | Select-Object -Last 1).Line
    $render -split 'terrain: ' | Select-Object -Last 1
    $chunk = (Select-String -LiteralPath $log -Pattern 'TERRAIN_BENCH_CHUNK' | Select-Object -Last 1).Line
    if ($chunk) { $chunk -split 'terrain: ' | Select-Object -Last 1 }
    'memoria_processo privada_pico_mb={0:N0} privada_final_mb={1:N0} conjunto_pico_mb={2:N0}' -f ($peakPrivate / 1MB), ($lastPrivate / 1MB), ($peakWorking / 1MB)
    if ($Profile -eq 'lumen' -and $render -notmatch 'gi=1 reflexos=1') { throw 'Lumen não ficou ativo no render' }
    $code = 0
}
catch { Write-Output "TERRAIN_BENCH_FAIL $($_.Exception.Message)" }
finally { try { Stop-Process -Id $process.Id -Force -ErrorAction Stop } catch { } }
exit $code

