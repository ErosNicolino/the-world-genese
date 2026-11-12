# O elfo e o tecido (09/11): o painel do menu com cada pele e orelha (frontend_elfo_<n>.png) e o teste do vento e do custo dos vestidos
# (tecido_grupo.png e VORTEX_CLOTH_TEST no log).
param([int]$TimeoutSeconds = 240)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$project = Join-Path $root 'Vortex.uproject'
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'

function Run([string]$Name, [string]$Map, [string[]]$Extra, [string]$Pattern) {
    $log = Join-Path $root "Saved\Logs\$Name.log"
    Remove-Item -LiteralPath $log -ErrorAction SilentlyContinue
    $process = Start-Process -FilePath $editor -WindowStyle Hidden -PassThru -ArgumentList (@("`"$project`"", $Map, '-game', '-dx12', '-RenderOffScreen', '-windowed', '-ResX=1600', '-ResY=900',
        '-VortexNoInitialFade', '-unattended', '-nosound', '-NoSplash', "`"-ABSLOG=$log`"") + $Extra)
    try { if (!$process.WaitForExit($TimeoutSeconds * 1000)) { "$Name passou do tempo" } }
    finally { if (!$process.HasExited) { Stop-Process -Id $process.Id -ErrorAction SilentlyContinue } }
    Select-String -LiteralPath $log -Pattern "$Pattern|Fatal error" | ForEach-Object { $_.Line -replace '^.*?(VORTEX_)', '$1' }
}
Run 'ClothTest' '/Game/Vortex/Maps/L_MovementProbe' @('-VortexHeroShots', '-VortexClothTest', '-VortexHour=12') 'VORTEX_CLOTH_TEST'
Run 'ElfMenu' '/Game/Vortex/Maps/L_Menu' @('-VortexElfMenu') 'VORTEX_ELF_MENU'
