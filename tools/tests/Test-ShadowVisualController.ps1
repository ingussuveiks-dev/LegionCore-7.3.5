param([string]$OutputDirectory = "$PSScriptRoot/../../build-extractors/runtime-test/shadow-visual-controller")
$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path "$PSScriptRoot/../..").Path
$output = [IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Force -Path $output | Out-Null
$source = [IO.File]::ReadAllText("$repo/src/server/scripts/Spells/spell_priest.cpp")
$start = $source.IndexOf('class spell_pri_shadowform :')
if ($start -lt 0) { throw 'Shadowform production controller missing' }
$end = $source.IndexOf("`n};", $start)
if ($end -lt 0) { throw 'Shadowform production controller incomplete' }
[IO.File]::WriteAllText("$output/Shadowform.inc", $source.Substring($start, $end + 3 - $start))
$vswhere = "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe"
$vs = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (!$vs) { throw 'Visual C++ compiler not found' }
$commands = @"
@echo off
call "$vs/VC/Auxiliary/Build/vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++17 /W4 /I"$output" "$PSScriptRoot/ShadowVisualControllerTest.cpp" /Fe"$output/ShadowVisualControllerTest.exe" /Fo"$output/ShadowVisualControllerTest.obj"
if errorlevel 1 exit /b 1
"$output/ShadowVisualControllerTest.exe"
exit /b %errorlevel%
"@
[IO.File]::WriteAllText("$output/run.cmd", $commands)
& "$output/run.cmd"
if ($LASTEXITCODE -ne 0) { throw "Shadow visual controller regression failed: $LASTEXITCODE" }
