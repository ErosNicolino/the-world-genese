# Envia o jogo para a página do itch.io com o butler (02/11). Login uma vez só: ..\butler\butler.exe login
#   powershell -ExecutionPolicy Bypass -File Tools\PushItch.ps1 -Target eros-nicolino/the-world-geneses
# Canais (03/11: o usuário quer só os executáveis na página, sem o .zip da pasta do jogo):
#   windows-instalador o instalador Portal (Build\Installer\TheWorldGeneses_Alpha_1.0_Portal.exe).
#   linux              o instalador do Linux (Build\Installer\TheWorldGeneses_Alpha_1.0_Linux.run).
#   windows            a pasta do jogo (Build\Alpha\Windows), só com -Folder. Na página ela aparece como um .zip; quem
#                      joga pelo app do itch recebe só o que mudou nas próximas versões.
# -Only linux (ou windows-instalador, windows) envia só um canal. -DryRun só mostra o que faria.
param([Parameter(Mandatory = $true)][string]$Target, [string]$Only = '', [switch]$Folder, [switch]$DryRun)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$butler = Join-Path (Split-Path -Parent $root) 'butler\butler.exe'
if (!(Test-Path $butler)) { throw "butler não encontrado em $butler" }
$version = '1.0.0-alpha+' + (Get-Date -Format 'yyyyMMdd.HHmm')
$jobs = @(
    @{ Channel = 'windows-instalador'; Path = Join-Path $root 'Build\Installer\TheWorldGeneses_Alpha_1.0_Portal.exe' },
    @{ Channel = 'linux'; Path = Join-Path $root 'Build\Installer\TheWorldGeneses_Alpha_1.0_Linux.run' }
)
if ($Folder) { $jobs += @{ Channel = 'windows'; Path = Join-Path $root 'Build\Alpha\Windows' } }
foreach ($job in $jobs) {
    if ($Only -and $job.Channel -ne $Only) { continue }
    if (!(Test-Path $job.Path)) { Write-Host "pulando $($job.Channel): falta $($job.Path)"; continue }
    $where = "$($Target):$($job.Channel)"
    Write-Host "== $where <- $($job.Path) (versão $version)"
    if ($DryRun) { continue }
    & $butler push $job.Path $where --userversion $version
    if ($LASTEXITCODE -ne 0) { throw "butler push falhou em $where" }
}
if (!$DryRun) { & $butler status $Target }
