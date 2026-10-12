param([string]$OutputDirectory="$PSScriptRoot/../../build-extractors/runtime-test/highmountain-worm-support")
$ErrorActionPreference='Stop'
$output=[IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Force -Path $output | Out-Null
$source=Get-Content "$PSScriptRoot/../../src/server/scripts/Legion/highmountain_worm_support.cpp" -Raw
$source=[regex]::Replace($source,'(?m)^#include.*\r?\n','')
$source=$source.Substring(0,$source.IndexOf('void AddSC_highmountain_worm_support()'))
[IO.File]::WriteAllText("$output/HighmountainWormSupport.inc",$source)
$loader=Get-Content "$PSScriptRoot/../../src/server/scripts/Legion/legion_script_loader.cpp" -Raw
if($loader -notmatch 'void AddSC_highmountain_worm_support\(\);' -or $loader -notmatch '(?m)^\s+AddSC_highmountain_worm_support\(\);'){throw 'Missing script registration'}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if(!$vs){throw 'Visual C++ compiler not found'}
$cmd=@"
@echo off
call "$vs/VC/Auxiliary/Build/vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++17 /W4 /I"$output" "$PSScriptRoot/HighmountainWormSupportTest.cpp" /Fe"$output/HighmountainWormSupportTest.exe" /Fo"$output/HighmountainWormSupportTest.obj"
if errorlevel 1 exit /b 1
"$output/HighmountainWormSupportTest.exe"
exit /b %errorlevel%
"@
[IO.File]::WriteAllText("$output/run.cmd",$cmd)
& "$output/run.cmd"
if($LASTEXITCODE){throw 'Battle Worms support regression failed'}
