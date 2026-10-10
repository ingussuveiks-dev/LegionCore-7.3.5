param([string]$OutputDirectory="$PSScriptRoot/../../build-extractors/runtime-test/highmountain-lifespring")
$ErrorActionPreference='Stop'
$output=[IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Force -Path $output | Out-Null
$source=Get-Content "$PSScriptRoot/../../src/server/scripts/Legion/highmountain_lifespring.cpp" -Raw
$start=$source.IndexOf('namespace Lifespring')
if($start -lt 0){throw 'Production namespace missing'}
[IO.File]::WriteAllText("$output/LifespringHandlers.inc",$source.Substring($start))
$vswhere="${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe"
$vs=& $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if(!$vs){throw 'Visual C++ compiler not found'}
$cmd=@"
@echo off
call "$vs/VC/Auxiliary/Build/vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++17 /W4 /I"$output" "$PSScriptRoot/HighmountainLifespringTest.cpp" /Fe"$output/HighmountainLifespringTest.exe" /Fo"$output/HighmountainLifespringTest.obj"
if errorlevel 1 exit /b 1
"$output/HighmountainLifespringTest.exe"
exit /b %errorlevel%
"@
[IO.File]::WriteAllText("$output/run.cmd",$cmd)
& "$output/run.cmd"
if($LASTEXITCODE){throw 'Lifespring production regression failed'}
