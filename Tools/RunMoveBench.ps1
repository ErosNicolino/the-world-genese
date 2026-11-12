# Custo do movimento sem renderização: mede a thread do jogo com 0, 50 e 150 personagens andando
# (GameMode -VortexMoveBench=N, mapa L_MovementProbe).
param([int[]]$Counts = @(0, 50, 150), [int]$TimeoutSeconds = 180)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$project = Join-Path $projectRoot 'Vortex.uproject'
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$code = 0
foreach ($n in $Counts) {
    $log = Join-Path $projectRoot "Saved\Logs\MoveBench_$n.log"
    Remove-Item -LiteralPath $log -ErrorAction SilentlyContinue
    $args = @(
        "`"$project`"", '/Game/Vortex/Maps/L_MovementProbe', '-game', "-VortexMoveBench=$n",
        '-unattended', '-nullrhi', '-nosound', '-NoSplash', '-ExecCmds="t.MaxFPS 0"', "`"-ABSLOG=$log`""
    )
    $process = Start-Process -FilePath $editor -ArgumentList $args -WindowStyle Hidden -PassThru
    try {
        if (!$process.WaitForExit($TimeoutSeconds * 1000)) { throw "timeout ($n)" }
        $line = Select-String -LiteralPath $log -Pattern 'MOVE_BENCH' | Select-Object -Last 1
        if (!$line) { throw "sem resultado ($n)" }
        ($line.Line -split 'movement: ')[1]
    }
    catch { Write-Output "MOVE_BENCH_FAIL $($_.Exception.Message)"; $code = 1 }
    finally { try { Stop-Process -Id $process.Id -Force -ErrorAction Stop } catch { } }
}
exit $code
