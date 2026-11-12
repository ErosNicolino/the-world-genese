# Roda todos os testes automáticos do Vortex em sequência e mostra um resumo no fim.
# Feche o editor antes (os testes abrem o jogo sozinhos).
#
#   powershell -ExecutionPolicy Bypass -File Tools\RunAllTests.ps1            # rápido (~20 min): testes de passa/falha
#   powershell -ExecutionPolicy Bypass -File Tools\RunAllTests.ps1 -Completo  # + rede pesada, fotos e desempenho (~1 h)
#   ... -Compilar   compila o projeto antes
#   ... -Parar      para no primeiro teste que falhar
#
# As fotos ficam em Saved\Screenshots e o resumo em Saved\Logs\RunAllTests.txt.
param([switch]$Completo, [switch]$Compilar, [switch]$Parar)

$ErrorActionPreference = 'Continue'
$root = Split-Path -Parent $PSScriptRoot
$tools = $PSScriptRoot
$summary = Join-Path $root 'Saved\Logs\RunAllTests.txt'

if (Get-Process UnrealEditor -ErrorAction SilentlyContinue | Where-Object { $_.MainWindowTitle }) {
    Write-Host 'Feche o editor da Unreal antes de rodar os testes.' -ForegroundColor Yellow
    exit 1
}

if ($Compilar) {
    Write-Host '== Compilando VortexEditor...' -ForegroundColor Cyan
    & 'C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat' VortexEditor Win64 Development "-Project=$root\Vortex.uproject" -WaitMutex -NoHotReloadFromIDE -NoUBA -MaxParallelActions=1 |
        Select-String -Pattern 'error|Result:' | ForEach-Object { $_.Line }
    if ($LASTEXITCODE -ne 0) { Write-Host 'A compilação falhou.' -ForegroundColor Red; exit 1 }
}

# nome que aparece, script, argumentos
$tests = @(
    @('Save e backup', 'RunSaveSmoke.ps1', @()),
    @('Heróis por mundo', 'RunHeroSmoke.ps1', @()),
    @('Menu: criar, salvar, voltar', 'RunMenuFlowSmoke.ps1', @()),
    @('Movimento', 'RunMovementSmoke.ps1', @()),
    @('Movimento: rampas, escada, piscina', 'RunMovementSmoke.ps1', @('-Matrix')),
    @('Esquiva nas 4 direções', 'RunDodgeDirectionSmoke.ps1', @()),
    @('Colisão com árvores e pedras', 'RunSolidWalkSmoke.ps1', @()),
    @('Câmera', 'RunCameraSmoke.ps1', @()),
    @('Combate', 'RunCombatSmoke.ps1', @()),
    @('Espada: combo, defesa, aparo', 'RunSwordSmoke.ps1', @()),
    @('Lobo', 'RunWolfSmoke.ps1', @()),
    @('Matilha (5 lobos)', 'RunPackSmoke.ps1', @('-Count', '5')),
    @('Criaturas do Godot e missões', 'RunCreatureSmoke.ps1', @()),
    @('Bandidos: bando, flecha, escudo, acampamento e baú guardado', 'RunBanditSmoke.ps1', @()),
    @('Pontos de interesse, portal, aranha, esqueletos e teias', 'RunPoiSmoke.ps1', @()),
    @('Fauna e flora: pássaros, peixes, plantas da água, esporos e dormideira', 'RunWildlifeSmoke.ps1', @()),
    @('HUD cartoon, minimapa e mapa: lago, zooms, detalhe por bloco e marcador', 'RunMapShots.ps1', @()),
    @('HQ de abertura: folhas pintando, legenda, clarão, folha virando e o Acorde', 'RunIntroShots.ps1', @()),
    @('HQ de abertura em rede: cada jogador vê e fecha a sua', 'RunIntroNetworkSmoke.ps1', @()),
    @('Golpe de perto de cada arma: colado, de lado, atrás e com outro preso no foco', 'RunMeleeSweep.ps1', @()),
    @('Heróis fixos por raça: modelo de cada raça, parado e golpeando (fotos)', 'RunHeroShots.ps1', @()),
    @('Lojas: comprar, vender, estoque do dia', 'RunShopSmoke.ps1', @()),
    @('Golpe de perto, Soco Sério, primeira pessoa, nado', 'RunFeatureShots.ps1', @()),
    @('Chefe da Caldeira: fases, lava, céu', 'RunBossShots.ps1', @()),
    @('Classes: Despertar e habilidades', 'RunClassShots.ps1', @()),
    @('Coletáveis e cavalo: colher, domar, montar, assobio', 'RunMountShots.ps1', @()),
    @('Continente: emenda com o vale e as nove regiões', 'RunWorldShots.ps1', @()),
    @('Rede: grupo, duelo, troca e amizade', 'RunSocialSmoke.ps1', @()),
    @('Região do vale', 'RunRegionSmoke.ps1', @()),
    @('Rede: sessão', 'RunFrameworkNetworkSmoke.ps1', @()),
    @('Rede: movimento', 'RunMovementNetworkSmoke.ps1', @()),
    @('Rede: entrada do cliente', 'RunClientInputSmoke.ps1', @()),
    @('Rede: lobo', 'RunWolfNetworkSmoke.ps1', @()),
    @('Rede: dia e noite', 'RunDayNightSmoke.ps1', @()),
    @('Rede: menu LAN', 'RunMenuNetworkSmoke.ps1', @())
)
if ($Completo) {
    $tests += @(
        @('Rede: entrada com atraso e perda', 'RunClientInputSmoke.ps1', @('-LagMs', '150', '-LossPct', '3')),
        @('Rede: streaming do terreno', 'RunStreamNetworkSmoke.ps1', @()),
        @('Rede: NPCs', 'RunPopulationSmoke.ps1', @('-Network')),
        @('NPCs', 'RunPopulationSmoke.ps1', @()),
        @('Fotos: menu e criação', 'RunFrontEndShot.ps1', @()),
        @('Fotos: HUD e especial', 'RunHudShot.ps1', @()),
        @('Fotos: armas de perto', 'RunWeaponCloseUp.ps1', @()),
        @('Fotos: dia e noite', 'RunDayNightShot.ps1', @()),
        @('Fotos: grama', 'RunGrassShot.ps1', @()),
        @('Desempenho (meio-dia)', 'RunTerrainBench.ps1', @('-Profile', 'alto', '-Hour', '12'))
    )
}

$results = @()
$start = Get-Date
$i = 0
foreach ($test in $tests) {
    $i++
    $name, $script, $testArgs = $test
    Write-Host ("[{0}/{1}] {2}..." -f $i, $tests.Count, $name) -ForegroundColor Cyan -NoNewline
    $t0 = Get-Date
    $output = & powershell -NoProfile -ExecutionPolicy Bypass -File (Join-Path $tools $script) @testArgs 2>&1 | Out-String
    $code = $LASTEXITCODE
    $seconds = [int]((Get-Date) - $t0).TotalSeconds
    # passa quando o script termina sem erro (cada um já confere o próprio PASS no log) e nada indica falha
    $passed = $code -eq 0 -and $output -notmatch '_FAIL\b|Fatal error|timed out|tempo esgotado|teste falhou|failed with exit code'
    $line = ($output -split "`r?`n" | Where-Object { $_ -match 'PASS|FAIL|DONE|fps=|throw|Exception|timed out' } | Select-Object -Last 1)
    if ($line) { $line = ($line -replace '^.*LogTemp: (Display: )?', '').Trim() }
    $status = if ($passed) { 'PASSOU' } else { 'FALHOU' }
    Write-Host (" {0} ({1}s)" -f $status, $seconds) -ForegroundColor $(if ($passed) { 'Green' } else { 'Red' })
    if (-not $passed -and $line) { Write-Host "    $line" -ForegroundColor DarkYellow }
    $results += [pscustomobject]@{ Teste = $name; Resultado = $status; Segundos = $seconds; Detalhe = $line }
    if ($Parar -and -not $passed) { break }
}

$total = [int]((Get-Date) - $start).TotalMinutes
$ok = ($results | Where-Object { $_.Resultado -eq 'PASSOU' }).Count
Write-Host ''
Write-Host ("== {0} de {1} passaram em {2} min" -f $ok, $results.Count, $total) -ForegroundColor $(if ($ok -eq $results.Count) { 'Green' } else { 'Yellow' })
$results | Format-Table -AutoSize | Out-String | Tee-Object -FilePath $summary | Write-Host
Write-Host "Resumo salvo em $summary"
if ($ok -ne $results.Count) { exit 1 }
