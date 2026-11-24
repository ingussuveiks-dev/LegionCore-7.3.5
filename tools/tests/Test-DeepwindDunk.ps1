param([Parameter(Mandatory)][string]$OutputDirectory)

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path "$PSScriptRoot/../..").Path
$output = [IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Force -Path $output | Out-Null
$source = Get-Content "$root/src/server/game/Battlegrounds/Zones/BattlegroundDeepwindGorge.cpp" -Raw
$objective = ([regex]::Match($source, '(?s)void BattlegroundDeepwindDunk::EventPlayerClickedOnFlag\(.*?(?=void BattlegroundDeepwindDunk::DropBall\()')).Value
$scoring = ([regex]::Match($source, '(?s)void BattlegroundDeepwindDunk::AddPoints\(.*?(?=void BattlegroundDeepwindDunk::EndBattleground\()')).Value
if (!$objective -or !$scoring) { throw 'Deepwind production handlers unavailable' }
[IO.File]::WriteAllText("$output/deepwind-objectives.inc", "$objective`n$scoring")

$vswhere = "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe"
$vs = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (!$vs) { throw 'Visual C++ compiler not found' }
$build = @"
@echo off
call "$vs/VC/Auxiliary/Build/vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++17 /I"$output" "$PSScriptRoot/DeepwindDunkTest.cpp" /Fe"$output/deepwind.exe" /Fo"$output/deepwind.obj"
if errorlevel 1 exit /b 1
"$output/deepwind.exe"
exit /b %errorlevel%
"@
[IO.File]::WriteAllText("$output/run.cmd", $build)
& "$output/run.cmd"
if ($LASTEXITCODE) { throw "Deepwind Dunk regression failed ($LASTEXITCODE)" }
