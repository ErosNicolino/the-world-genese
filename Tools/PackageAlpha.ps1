# Empacota o jogo (The World Gêneses, Alpha 1.0) e gera o instalador.
#   1. compila, cozinha o conteúdo e empacota em Build\Alpha\Windows (Shipping, sem console)
#   2. copia os dados que o jogo lê do disco (Data\*.json, *.bin, *.txt; os .glb/.png são fontes de importação)
#   3. gera o instalador em Build\Installer (o Portal, ou o antigo do Inno Setup 6 com -Installer Inno)
# Rodar com o editor fechado. -SkipBuild reaproveita o pacote já feito e refaz só o instalador.
# -Installer Portal (padrão) gera o instalador próprio de Tools\Installer\Portal; -Installer Inno, o antigo do Inno Setup.
# -Platform Linux empacota para Linux (Build\AlphaLinux) e gera o .run de Tools\Installer\Linux (precisa do Linux nas
# plataformas da UE 5.8 no Epic Launcher e do compilador da Epic em C:\UnrealToolchains).
param([string]$Config = 'Shipping', [switch]$Perf, [switch]$SkipBuild, [ValidateSet('Portal', 'Inno')][string]$Installer = 'Portal',
    [ValidateSet('Win64', 'Linux')][string]$Platform = 'Win64')
$ErrorActionPreference = 'Stop'
# prioridade abaixo do normal (herdada pela compilação e pelos shaders): o PC continua usável enquanto empacota
(Get-Process -Id $PID).PriorityClass = 'BelowNormal'
$root = Split-Path -Parent $PSScriptRoot
$project = Join-Path $root 'Vortex.uproject'
$uat = 'C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\RunUAT.bat'
# o Inno Setup pode estar instalado para todos (Program Files) ou só para o usuário (AppData\Local\Programs)
$iscc = @("$env:LOCALAPPDATA\Programs\Inno Setup 6\ISCC.exe", 'C:\Program Files (x86)\Inno Setup 6\ISCC.exe', 'C:\Program Files\Inno Setup 6\ISCC.exe') |
    Where-Object { Test-Path $_ } | Select-Object -First 1
if (!$iscc -and $Installer -eq 'Inno') { throw 'Inno Setup 6 não encontrado (ISCC.exe).' }
$linux = $Platform -eq 'Linux'
$archive = Join-Path $root $(if ($linux) { 'Build\AlphaLinux' } else { 'Build\Alpha' })
# 08/11: -Perf: o pacote de medição (Development, com os testes de desempenho) em Build\PerfTest, sem instalador; o do itch
# (Build\Alpha) fica como está. Depois: Tools\RunRoamBench.ps1 -Packaged
if ($Perf) { $Config = 'Development'; $archive = Join-Path $root 'Build\PerfTest' }
$game = Join-Path $archive $(if ($linux) { 'Linux' } else { 'Windows' })
$log = Join-Path $root "Saved\Logs\PackageAlpha_$Platform.log"
$version = '1.0.0-alpha'
$sourceIcon = Join-Path $root 'Build\Windows\Vortex.ico'
$applicationIcon = Join-Path $root 'Build\Windows\Application.ico'
if (!(Test-Path -LiteralPath $sourceIcon)) { throw "Ícone do jogo não encontrado: $sourceIcon" }
# O UBT usa exatamente este caminho para gravar o ícone dentro do Vortex.exe.
Copy-Item -LiteralPath $sourceIcon -Destination $applicationIcon -Force
if ($Installer -eq 'Inno') { & (Join-Path $PSScriptRoot 'Installer\Inno\BuildWizardArt.ps1') -ProjectRoot $root }

if (!$SkipBuild) {
    if (Get-Process UnrealEditor* -ErrorAction SilentlyContinue) { throw 'Feche o editor antes de empacotar.' }
    if (Test-Path $archive) { Remove-Item $archive -Recurse -Force }
    # os pré-requisitos (Visual C++) só existem no Windows
    # @(...) por fora: um if que devolve um item só vira texto, e o @ espalhava as letras ("-", "p", "r"...)
    $prereqs = @(if (!$linux) { '-prereqs' })
    & $uat BuildCookRun "-project=$project" -noP4 "-platform=$Platform" "-clientconfig=$Config" -build -cook -stage -pak -iostore `
        "-map=/Game/Vortex/Maps/L_Menu+/Game/Vortex/Maps/L_TerrainProto" @prereqs -archive "-archivedirectory=$archive" -nodebuginfo -utf8output -unattended *> $log
    if ($LASTEXITCODE -ne 0) {
        Select-String -LiteralPath $log -Pattern 'Error:|error C|ERROR' | Select-Object -Last 20 | ForEach-Object { $_.Line }
        throw "BuildCookRun falhou ($LASTEXITCODE). Log: $log"
    }
}
if (!(Test-Path (Join-Path $game $(if ($linux) { 'Vortex.sh' } else { 'Vortex.exe' })))) { throw "Pacote não encontrado em $game" }
if (!$linux) { Copy-Item -LiteralPath $applicationIcon -Destination (Join-Path $game 'Vortex.ico') -Force }

# dados lidos do disco pelo jogo (FPaths::ProjectDir()/Data)
$dataOut = Join-Path $game 'Vortex\Data'
if (Test-Path $dataOut) { Remove-Item $dataOut -Recurse -Force }
$dataIn = Join-Path $root 'Data'
Get-ChildItem $dataIn -Recurse -File -Include *.json, *.bin, *.txt | ForEach-Object {
    $target = Join-Path $dataOut $_.FullName.Substring($dataIn.Length + 1)
    New-Item -ItemType Directory -Force (Split-Path $target) | Out-Null
    Copy-Item $_.FullName $target
}
"Dados copiados: {0:N1} MB" -f ((Get-ChildItem $dataOut -Recurse -File | Measure-Object Length -Sum).Sum / 1MB)
"Jogo empacotado: {0:N2} GB" -f ((Get-ChildItem $game -Recurse -File | Measure-Object Length -Sum).Sum / 1GB)
if ($Perf) { return }

$out = Join-Path $root 'Build\Installer'
if ($linux) {
    python (Join-Path $PSScriptRoot 'Installer\Linux\BuildLinuxInstaller.py') $game (Join-Path $out 'TheWorldGeneses_Alpha_1.0_Linux.run') $version
    if ($LASTEXITCODE -ne 0) { throw "Instalador Linux falhou ($LASTEXITCODE)" }
    return
}
if ($Installer -eq 'Portal') {
    & (Join-Path $PSScriptRoot 'Installer\Portal\BuildPortal.ps1') -GameDir $game -OutputDir $out -Version $version
    return
}
& $iscc /Q "/DSourceDir=$game" "/DOutputDir=$out" "/DAppVersion=$version" (Join-Path $PSScriptRoot 'Installer\Inno\TheWorldGeneses.iss')
if ($LASTEXITCODE -ne 0) { throw "Inno Setup falhou ($LASTEXITCODE)" }
Get-ChildItem $out -Filter *.exe | ForEach-Object { "Instalador: {0} ({1:N0} MB)" -f $_.FullName, ($_.Length / 1MB) }
