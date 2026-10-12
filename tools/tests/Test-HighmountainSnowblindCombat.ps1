param([string]$OutputDirectory="$PSScriptRoot/../../build-extractors/runtime-test/highmountain-snowblind-combat")
$ErrorActionPreference='Stop'
$output=[IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Force -Path $output | Out-Null
$source=Get-Content "$PSScriptRoot/../../src/server/scripts/Legion/highmountain_snowblind_combat.cpp" -Raw
$source=[regex]::Replace($source,'(?m)^#include.*\r?\n','')
$source=$source.Substring(0,$source.IndexOf('void AddSC_highmountain_snowblind_combat()'))
[IO.File]::WriteAllText("$output/HighmountainSnowblindCombat.inc",$source)
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if(!$vs){throw 'Visual C++ compiler not found'}
$cmd=@"
@echo off
call "$vs/VC/Auxiliary/Build/vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++17 /W4 /I"$output" "$PSScriptRoot/HighmountainSnowblindCombatTest.cpp" /Fe"$output/HighmountainSnowblindCombatTest.exe" /Fo"$output/HighmountainSnowblindCombatTest.obj"
if errorlevel 1 exit /b 1
"$output/HighmountainSnowblindCombatTest.exe"
exit /b %errorlevel%
"@
[IO.File]::WriteAllText("$output/run.cmd",$cmd)
& "$output/run.cmd"
if($LASTEXITCODE){throw 'Snowblind combat regression failed'}
