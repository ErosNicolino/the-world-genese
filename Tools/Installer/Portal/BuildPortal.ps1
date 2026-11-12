# Gera o instalador "Portal" do The World Gêneses (Portal.cs + Pack.cs):
#   1. recorta os 24 quadros da abertura (Data\UI\Abertura) e grava as legendas que passam durante a instalação
#   2. compila o instalador (WPF, .NET Framework 4.8, que já vem no Windows 10/11) com o csc do SDK do .NET
#   3. anexa o jogo empacotado ao fim do .exe
# -Test compila sem pedir administrador e sem anexar o jogo (para rodar com /demo).
# -Music <arquivo.mp3> troca a música do instalador (padrão: Musica_Instalador.mp3, nesta pasta; sem nenhuma, o instalador
# fica em silêncio e o botão de som some).
param([string]$GameDir, [string]$OutputDir, [string]$Version = '1.0.0-alpha', [string]$Music, [switch]$Test)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..')).Path
if (!$Music -and (Test-Path -LiteralPath (Join-Path $PSScriptRoot 'Musica_Instalador.mp3'))) { $Music = Join-Path $PSScriptRoot 'Musica_Instalador.mp3' }
if (!$GameDir) { $GameDir = Join-Path $root 'Build\Alpha\Windows' }
if (!$OutputDir) { $OutputDir = Join-Path $root 'Build\Installer' }
$work = Join-Path $root 'Intermediate\Portal'
$art = Join-Path $work 'Art'
New-Item -ItemType Directory -Force $art, $OutputDir | Out-Null

# --- 1. quadros e legendas
Add-Type -AssemblyName System.Drawing
$comic = Get-Content -Raw -Encoding UTF8 (Join-Path $root 'Data\UI\Abertura\abertura.json') | ConvertFrom-Json
$jpeg = [Drawing.Imaging.ImageCodecInfo]::GetImageEncoders() | Where-Object MimeType -eq 'image/jpeg'
$quality = [Drawing.Imaging.EncoderParameters]::new(1)
$quality.Param[0] = [Drawing.Imaging.EncoderParameter]::new([Drawing.Imaging.Encoder]::Quality, 90L)
$lore = [Collections.Generic.List[string]]::new()
$index = 0
for ($page = 0; $page -lt $comic.pages.Count; $page++) {
    $sheet = [Drawing.Image]::FromFile((Join-Path $root ('Data\UI\Abertura\folha_{0}.png' -f ($page + 1))))
    try {
        foreach ($panel in $comic.pages[$page].panels) {
            $r = $panel.rect
            # corta 6 px de cada lado: a moldura do quadro fica de fora
            $x = [int]($r[0] * $sheet.Width) + 6; $y = [int]($r[1] * $sheet.Height) + 6
            $w = [int](($r[2] - $r[0]) * $sheet.Width) - 12; $h = [int](($r[3] - $r[1]) * $sheet.Height) - 12
            $crop = [Drawing.Bitmap]::new($w, $h, [Drawing.Imaging.PixelFormat]::Format24bppRgb)
            $g = [Drawing.Graphics]::FromImage($crop)
            $g.InterpolationMode = [Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
            $g.DrawImage($sheet, [Drawing.Rectangle]::new(0, 0, $w, $h), [Drawing.Rectangle]::new($x, $y, $w, $h), [Drawing.GraphicsUnit]::Pixel)
            $g.Dispose()
            $crop.Save((Join-Path $art ('panel_{0:00}.jpg' -f $index)), $jpeg, $quality)
            $crop.Dispose()
            $lore.Add(($panel.text -replace "`r", '' -replace "`n", '|'))
            $index++
        }
    }
    finally { $sheet.Dispose() }
}
[IO.File]::WriteAllLines((Join-Path $art 'lore.txt'), $lore, [Text.UTF8Encoding]::new($false))
"Quadros da abertura: $index"

# a arte de capa (fundo da janela) e os emblemas dos nove Selos, na ordem das regiões do instalador
function Save-Resized([string]$From, [string]$To, [int]$Width, [string]$Kind) {
    $src = [Drawing.Image]::FromFile($From)
    try {
        $h = [int]($src.Height * $Width / $src.Width)
        $dst = [Drawing.Bitmap]::new($Width, $h, [Drawing.Imaging.PixelFormat]::Format24bppRgb)
        $g = [Drawing.Graphics]::FromImage($dst)
        $g.InterpolationMode = [Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
        $g.DrawImage($src, 0, 0, $Width, $h); $g.Dispose()
        if ($Kind -eq 'jpg') { $dst.Save($To, $jpeg, $quality) } else { $dst.Save($To, [Drawing.Imaging.ImageFormat]::Png) }
        $dst.Dispose()
    }
    finally { $src.Dispose() }
}
$cover = Join-Path $root 'Docs\Midia\Capa\Arte_de_capa.png'
if (Test-Path -LiteralPath $cover) { Save-Resized $cover (Join-Path $art 'capa.jpg') 1600 'jpg' }
$seals = 'Valdora', 'Brasafenda', 'Raikor', 'Nivaluz', 'Solcanion', 'Aurivale', 'Lodonegro', 'Morthuria', 'Verdalume'
for ($i = 0; $i -lt $seals.Count; $i++) {
    $seal = Join-Path $root "Data\UI\Seals\T_Seal_$($seals[$i]).png"
    if (Test-Path -LiteralPath $seal) { Save-Resized $seal (Join-Path $art "seal_$i.png") 96 'png' }
}

# --- 2. compilar
$csc = Get-ChildItem 'C:\Program Files\dotnet\sdk\*\Roslyn\bincore\csc.dll' | Sort-Object { [version]($_.FullName -replace '.*\\sdk\\([\d\.]+)\\.*', '$1') } | Select-Object -Last 1
if (!$csc) { throw 'SDK do .NET não encontrado (csc.dll).' }
$fw = "$env:WINDIR\Microsoft.NET\Framework64\v4.0.30319"
$refs = 'mscorlib.dll', 'System.dll', 'System.Core.dll', 'System.Xaml.dll', 'System.IO.Compression.dll', 'WPF\WindowsBase.dll', 'WPF\PresentationCore.dll', 'WPF\PresentationFramework.dll' |
    ForEach-Object { "/reference:$fw\$_" }
$manifest = Join-Path $PSScriptRoot 'app.manifest'
if ($Test) {
    $manifest = Join-Path $work 'app.test.manifest'
    (Get-Content -Raw (Join-Path $PSScriptRoot 'app.manifest')) -replace 'requireAdministrator', 'asInvoker' | Set-Content -Encoding UTF8 $manifest
}
$icon = Join-Path $root 'Build\Windows\Vortex.ico'
$resources = @(
    "/resource:$icon,icon.ico"
    "/resource:$(Join-Path $root 'Tools\Installer\icon.png'),emblem.png"
    "/resource:$(Join-Path $root 'SourceArt\Menu\Fonts\Cinzel-700.ttf'),Cinzel-700.ttf"
    "/resource:$(Join-Path $root 'SourceArt\Menu\Fonts\Cinzel-900.ttf'),Cinzel-900.ttf"
    "/resource:$(Join-Path $art 'lore.txt'),lore.txt"
) + (Get-ChildItem $art -Include 'panel_*.jpg', 'seal_*.png', 'capa.jpg' -File -Recurse | ForEach-Object { "/resource:$($_.FullName),$($_.Name)" })
if ($Music) {
    if (!(Test-Path -LiteralPath $Music)) { throw "Música não encontrada: $Music" }
    $resources += "/resource:$((Resolve-Path -LiteralPath $Music).Path),music.mp3"
}
$common = '/nologo', '/noconfig', '/nostdlib+', '/platform:x64', '/optimize+', '/langversion:latest', '/utf8output', '/warn:4'

$stub = Join-Path $work 'Portal.exe'
& dotnet $csc.FullName @common /target:winexe "/out:$stub" "/win32icon:$icon" "/win32manifest:$manifest" @refs @resources (Join-Path $PSScriptRoot 'Portal.cs')
if ($LASTEXITCODE -ne 0) { throw "Compilação do instalador falhou ($LASTEXITCODE)" }
if ($Test) { "Instalador de teste: $stub (rode com /demo)"; return }

$packer = Join-Path $work 'Pack.exe'
& dotnet $csc.FullName @common /target:exe "/out:$packer" @refs (Join-Path $PSScriptRoot 'Pack.cs')
if ($LASTEXITCODE -ne 0) { throw "Compilação do Pack falhou ($LASTEXITCODE)" }

# --- 3. anexar o jogo
if (!(Test-Path (Join-Path $GameDir 'Vortex.exe'))) { throw "Jogo empacotado não encontrado em $GameDir" }
$out = Join-Path $OutputDir 'TheWorldGeneses_Alpha_1.0_Portal.exe'
& $packer $stub $GameDir $Version $out | Select-Object -Last 3
if ($LASTEXITCODE -ne 0) { throw "Empacotamento falhou ($LASTEXITCODE)" }
"Instalador: {0} ({1:N0} MB)" -f $out, ((Get-Item $out).Length / 1MB)
