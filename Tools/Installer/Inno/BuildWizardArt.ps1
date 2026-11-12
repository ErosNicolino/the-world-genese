param([Parameter(Mandatory = $true)][string]$ProjectRoot)

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing

$source = Join-Path $ProjectRoot 'Docs\Images\abertura\10_heroi.png'
$emblem = Join-Path $PSScriptRoot '..\icon.png'
$wizardPath = Join-Path $PSScriptRoot 'wizard.bmp'
$smallPath = Join-Path $PSScriptRoot 'wizard_small.bmp'
if (!(Test-Path -LiteralPath $source)) { throw "Arte da abertura não encontrada: $source" }
if (!(Test-Path -LiteralPath $emblem)) { throw "Brasão não encontrado: $emblem" }

$canvas = [Drawing.Bitmap]::new(328, 628, [Drawing.Imaging.PixelFormat]::Format24bppRgb)
$g = [Drawing.Graphics]::FromImage($canvas)
$art = [Drawing.Image]::FromFile($source)
$mark = [Drawing.Image]::FromFile($emblem)
try {
    $g.SmoothingMode = [Drawing.Drawing2D.SmoothingMode]::AntiAlias
    $g.InterpolationMode = [Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
    $g.PixelOffsetMode = [Drawing.Drawing2D.PixelOffsetMode]::HighQuality
    $g.DrawImage($art, [Drawing.Rectangle]::new(0, 0, 328, 628), [Drawing.Rectangle]::new(425, 25, 248, 475), [Drawing.GraphicsUnit]::Pixel)

    $shade = [Drawing.SolidBrush]::new([Drawing.Color]::FromArgb(112, 9, 18, 26))
    $g.FillRectangle($shade, 0, 0, 328, 628)
    $shade.Dispose()
    $top = [Drawing.Drawing2D.LinearGradientBrush]::new([Drawing.Rectangle]::new(0, 0, 328, 270), [Drawing.Color]::FromArgb(210, 7, 15, 23), [Drawing.Color]::FromArgb(0, 7, 15, 23), 90)
    $g.FillRectangle($top, 0, 0, 328, 270)
    $top.Dispose()
    $bottom = [Drawing.Drawing2D.LinearGradientBrush]::new([Drawing.Rectangle]::new(0, 420, 328, 208), [Drawing.Color]::FromArgb(0, 7, 15, 23), [Drawing.Color]::FromArgb(225, 7, 15, 23), 90)
    $g.FillRectangle($bottom, 0, 420, 328, 208)
    $bottom.Dispose()

    $gold = [Drawing.Color]::FromArgb(238, 199, 121)
    $edge = [Drawing.Pen]::new($gold, 2)
    $g.DrawRectangle($edge, 12, 12, 304, 604)
    $edge.Dispose()

    $badge = [Drawing.SolidBrush]::new([Drawing.Color]::FromArgb(222, 12, 24, 29))
    $badgePen = [Drawing.Pen]::new([Drawing.Color]::FromArgb(235, 207, 150), 2)
    $g.FillEllipse($badge, 107, 35, 114, 114)
    $g.DrawEllipse($badgePen, 107, 35, 114, 114)
    $g.DrawImage($mark, [Drawing.Rectangle]::new(117, 43, 94, 98))
    $badge.Dispose(); $badgePen.Dispose()

    $center = [Drawing.StringFormat]::new()
    $center.Alignment = [Drawing.StringAlignment]::Center
    $center.LineAlignment = [Drawing.StringAlignment]::Center
    $cream = [Drawing.SolidBrush]::new([Drawing.Color]::FromArgb(255, 250, 239, 211))
    $goldText = [Drawing.SolidBrush]::new([Drawing.Color]::FromArgb(255, 236, 192, 112))
    $muted = [Drawing.SolidBrush]::new([Drawing.Color]::FromArgb(238, 226, 215, 188))
    $fontKicker = [Drawing.Font]::new('Segoe UI', 13, [Drawing.FontStyle]::Bold, [Drawing.GraphicsUnit]::Pixel)
    $fontTitle = [Drawing.Font]::new('Georgia', 31, [Drawing.FontStyle]::Bold, [Drawing.GraphicsUnit]::Pixel)
    $fontSubtitle = [Drawing.Font]::new('Georgia', 43, [Drawing.FontStyle]::Bold, [Drawing.GraphicsUnit]::Pixel)
    $fontTag = [Drawing.Font]::new('Segoe UI', 14, [Drawing.FontStyle]::Bold, [Drawing.GraphicsUnit]::Pixel)
    $fontSmall = [Drawing.Font]::new('Segoe UI', 11, [Drawing.FontStyle]::Regular, [Drawing.GraphicsUnit]::Pixel)

    $g.DrawString('UMA LENDA EM CONSTRUCAO', $fontKicker, $goldText, [Drawing.RectangleF]::new(20, 163, 288, 24), $center)
    $g.DrawString('THE WORLD', $fontTitle, $cream, [Drawing.RectangleF]::new(18, 187, 292, 44), $center)
    $g.DrawString(('G' + [char]202 + 'NESES'), $fontSubtitle, $cream, [Drawing.RectangleF]::new(16, 226, 296, 58), $center)

    $rule = [Drawing.Pen]::new([Drawing.Color]::FromArgb(220, 217, 170, 96), 2)
    $g.DrawLine($rule, 54, 542, 274, 542)
    $rule.Dispose()
    $fontFooter = [Drawing.Font]::new('Georgia', 21, [Drawing.FontStyle]::Bold, [Drawing.GraphicsUnit]::Pixel)
    $g.DrawString('SUA JORNADA', $fontFooter, $cream, [Drawing.RectangleF]::new(22, 550, 284, 32), $center)
    $g.DrawString(('COME' + [char]199 + 'A EM VALDORA'), $fontTag, $goldText, [Drawing.RectangleF]::new(24, 582, 280, 24), $center)
    $g.DrawString('AVENTURA - FANTASIA - COOPERACAO', $fontSmall, $muted, [Drawing.RectangleF]::new(20, 600, 288, 18), $center)

    foreach ($font in @($fontKicker, $fontTitle, $fontSubtitle, $fontTag, $fontSmall, $fontFooter)) { $font.Dispose() }
    $center.Dispose(); $cream.Dispose(); $goldText.Dispose(); $muted.Dispose()
    $canvas.Save($wizardPath, [Drawing.Imaging.ImageFormat]::Bmp)
}
finally {
    $mark.Dispose(); $art.Dispose(); $g.Dispose(); $canvas.Dispose()
}

$small = [Drawing.Bitmap]::new(110, 110, [Drawing.Imaging.PixelFormat]::Format24bppRgb)
$sg = [Drawing.Graphics]::FromImage($small)
$sg.SmoothingMode = [Drawing.Drawing2D.SmoothingMode]::AntiAlias
$sg.Clear([Drawing.Color]::FromArgb(14, 25, 31))
$smallPen = [Drawing.Pen]::new([Drawing.Color]::FromArgb(238, 199, 121), 3)
$sg.DrawEllipse($smallPen, 5, 5, 100, 100)
$smallMark = [Drawing.Image]::FromFile($emblem)
try { $sg.DrawImage($smallMark, [Drawing.Rectangle]::new(13, 12, 84, 86)); $small.Save($smallPath, [Drawing.Imaging.ImageFormat]::Bmp) }
finally { $smallMark.Dispose(); $smallPen.Dispose(); $sg.Dispose(); $small.Dispose() }

"Arte do instalador criada: $wizardPath"
