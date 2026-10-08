param([string]$OutputDirectory = "$PSScriptRoot/../../build-extractors/runtime-test/spell-visuals")
$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path "$PSScriptRoot/../..").Path
$output = [IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Force -Path $output | Out-Null
$source = [IO.File]::ReadAllText("$repo/src/server/game/Spells/SpellMgr.cpp")
$begin = $source.IndexOf('void SpellMgr::LoadSpellVisual()')
$end = $source.IndexOf('void SpellMgr::LoadSpellScene()', $begin)
if ($begin -lt 0 -or $end -le $begin) { throw 'Cannot locate production visual loader' }
[IO.File]::WriteAllText("$output/SpellVisualLoading.inc", $source.Substring($begin, $end - $begin))
$vswhere = "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe"
$vs = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (!$vs) { throw 'Visual C++ compiler not found' }
$commands = @"
@echo off
call "$vs/VC/Auxiliary/Build/vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++17 /W4 /I"$output" "$PSScriptRoot/SpellVisualLoadingTest.cpp" /Fe"$output/SpellVisualLoadingTest.exe" /Fo"$output/SpellVisualLoadingTest.obj"
if errorlevel 1 exit /b 1
"$output/SpellVisualLoadingTest.exe"
exit /b %errorlevel%
"@
[IO.File]::WriteAllText("$output/run.cmd", $commands)
& "$output/run.cmd"
if ($LASTEXITCODE -ne 0) { throw "Spell visual loading regression failed: $LASTEXITCODE" }
