param([string]$OutputDirectory = "$PSScriptRoot/../../build-extractors/runtime-test/warlock-regression")
$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path "$PSScriptRoot/../..").Path
$output = [IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Force -Path $output | Out-Null
$source = [IO.File]::ReadAllText("$repo/src/server/scripts/Spells/spell_warlock.cpp")
$start = $source.IndexOf('class spell_warl_burning_rush :')
$end = $source.IndexOf('// Banish -', $start)
if ($start -lt 0 -or $end -le $start) { throw 'Cannot locate Burning Rush script' }
$script = $source.Substring($start, $end - $start)
foreach ($entry in @(@('SpellCastResult CheckCast()', 'BurningRushCheckCast.inc'), @('void OnTick(', 'BurningRushTick.inc'))) {
    $begin = $script.IndexOf($entry[0])
    if ($begin -lt 0) { throw "Missing production function: $($entry[0])" }
    $open = $script.IndexOf('{', $begin)
    $depth = 1
    $close = $open + 1
    while ($depth -gt 0 -and $close -lt $script.Length) {
        if ($script[$close] -eq '{') { ++$depth }
        if ($script[$close] -eq '}') { --$depth }
        ++$close
    }
    if ($depth -ne 0) { throw 'Unbalanced production function' }
    [IO.File]::WriteAllText("$output/$($entry[1])", $script.Substring($begin, $close - $begin))
}
$movement = [IO.File]::ReadAllText("$repo/src/server/game/Handlers/MovementHandler.cpp")
$begin = $movement.IndexOf('void WorldSession::HandleSetActiveMover(')
$end = $movement.IndexOf('void WorldSession::HandleMoveTimeSkipped(', $begin)
if ($begin -lt 0 -or $end -le $begin) { throw 'Cannot locate mover handler' }
[IO.File]::WriteAllText("$output/MovementHandoff.inc", $movement.Substring($begin, $end - $begin))
$vswhere = "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe"
$vs = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (!$vs) { throw 'Visual C++ compiler not found' }
$commands = @"
@echo off
call "$vs/VC/Auxiliary/Build/vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++17 /W4 /I"$output" "$PSScriptRoot/WarlockRegressionTest.cpp" /Fe"$output/WarlockRegressionTest.exe" /Fo"$output/WarlockRegressionTest.obj"
if errorlevel 1 exit /b 1
"$output/WarlockRegressionTest.exe"
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++17 /W4 /I"$output" "$PSScriptRoot/MovementHandoffTest.cpp" /Fe"$output/MovementHandoffTest.exe" /Fo"$output/MovementHandoffTest.obj"
if errorlevel 1 exit /b 1
"$output/MovementHandoffTest.exe"
exit /b %errorlevel%
"@
[IO.File]::WriteAllText("$output/run.cmd", $commands)
& "$output/run.cmd"
if ($LASTEXITCODE -ne 0) { throw "Warlock regression failed: $LASTEXITCODE" }
