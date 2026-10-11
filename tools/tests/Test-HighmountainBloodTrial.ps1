param([string]$OutputDirectory="$PSScriptRoot/../../build-extractors/runtime-test/highmountain-blood-trial")
$ErrorActionPreference='Stop'
$output=[IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Force -Path $output | Out-Null

$loader=Get-Content "$PSScriptRoot/../../src/server/scripts/Legion/legion_script_loader.cpp" -Raw
if($loader -notmatch 'void AddSC_highmountain_blood_trial\(\);' -or $loader -notmatch '(?m)^\s+AddSC_highmountain_blood_trial\(\);'){throw 'Missing Legion script registration'}
$source=Get-Content "$PSScriptRoot/../../src/server/scripts/Legion/highmountain_blood_trial.cpp" -Raw
$source=[regex]::Replace($source,'(?m)^#include.*\r?\n','')
$source=$source.Substring(0,$source.IndexOf('void AddSC_highmountain_blood_trial()'))
[IO.File]::WriteAllText("$output/HighmountainBloodTrial.inc",$source)
& "$PSScriptRoot/Export-QuestDelayedEvent.ps1" -OutputDirectory $output
$vswhere="${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe"
$vs=& $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if(!$vs){throw 'Visual C++ compiler not found'}
$cmd=@"
@echo off
call "$vs/VC/Auxiliary/Build/vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++17 /W4 /I"$output" "$PSScriptRoot/HighmountainBloodTrialTest.cpp" /Fe"$output/HighmountainBloodTrialTest.exe" /Fo"$output/HighmountainBloodTrialTest.obj"
if errorlevel 1 exit /b 1
"$output/HighmountainBloodTrialTest.exe"
exit /b %errorlevel%
"@
[IO.File]::WriteAllText("$output/run.cmd",$cmd)
& "$output/run.cmd"
if($LASTEXITCODE){throw 'Highmountain production regression failed'}
