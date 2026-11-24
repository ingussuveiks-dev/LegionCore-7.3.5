param([Parameter(Mandatory)][string]$OutputDirectory)

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path "$PSScriptRoot/../..").Path
$output = [IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Force -Path $output | Out-Null
$source = Get-Content "$root/src/server/game/Battlegrounds/Zones/BrawlBattlegroundSouthshoreVsTarrenMill.cpp" -Raw
$score = ([regex]::Match($source, '(?s)static constexpr uint8 RankCount.*?\n};')).Value
$handlers = ([regex]::Match($source, '(?s)void BrawlBattlegroundSouthshoreVsTarrenMill::ResetRank\(.*$')).Value
if (!$score -or !$handlers) { throw 'Southshore score handlers unavailable' }
[IO.File]::WriteAllText("$output/southshore-score.inc", "$score`n$handlers")

$vswhere = "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe"
$vs = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (!$vs) { throw 'Visual C++ compiler not found' }
$build = @"
@echo off
call "$vs/VC/Auxiliary/Build/vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++17 /I"$output" "$PSScriptRoot/SouthshoreBrawlScoreTest.cpp" /Fe"$output/southshore.exe" /Fo"$output/southshore.obj"
if errorlevel 1 exit /b 1
"$output/southshore.exe"
exit /b %errorlevel%
"@
[IO.File]::WriteAllText("$output/run.cmd", $build)
& "$output/run.cmd"
if ($LASTEXITCODE) { throw "Southshore regression failed ($LASTEXITCODE)" }
