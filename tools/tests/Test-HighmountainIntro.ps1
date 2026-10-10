param([string]$OutputDirectory="$PSScriptRoot/../../build-extractors/runtime-test/highmountain-intro")
$ErrorActionPreference='Stop'
$output=[IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Force -Path $output | Out-Null
& "$PSScriptRoot/Export-QuestDelayedEvent.ps1" -OutputDirectory $output
$loader=Get-Content "$PSScriptRoot/../../src/server/scripts/Legion/legion_script_loader.cpp" -Raw
if($loader -notmatch 'void AddSC_highmountain_intro\(\);' -or $loader -notmatch '(?m)^\s+AddSC_highmountain_intro\(\);'){throw 'Missing Legion script registration'}
$source=Get-Content "$PSScriptRoot/../../src/server/scripts/Legion/highmountain_intro.cpp" -Raw
$source=[regex]::Replace($source,'(?m)^#include.*\r?\n','')
$start=$source.IndexOf('class spell_highmountain_discover_thunder_totem');$end=$source.IndexOf('class player_highmountain_intro')
$source=$source.Remove($start,$end-$start)
$source=$source.Substring(0,$source.IndexOf('void AddSC_highmountain_intro()'))
[IO.File]::WriteAllText("$output/HighmountainIntro.inc",$source)
$vswhere="${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe"
$vs=& $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if(!$vs){throw 'Visual C++ compiler not found'}
$cmd=@"
@echo off
call "$vs/VC/Auxiliary/Build/vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++17 /W4 /I"$output" "$PSScriptRoot/HighmountainIntroTest.cpp" /Fe"$output/HighmountainIntroTest.exe" /Fo"$output/HighmountainIntroTest.obj"
if errorlevel 1 exit /b 1
"$output/HighmountainIntroTest.exe"
exit /b %errorlevel%
"@
[IO.File]::WriteAllText("$output/run.cmd",$cmd)
& "$output/run.cmd"
if($LASTEXITCODE){throw 'Highmountain production regression failed'}
