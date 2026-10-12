param([string]$OutputDirectory="$PSScriptRoot/../../build-extractors/runtime-test/highmountain-witchqueen")
$ErrorActionPreference='Stop'
$output=[IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Force -Path $output | Out-Null
$root="$PSScriptRoot/../.."
$source=Get-Content "$root/src/server/scripts/Legion/zone_highmountain.cpp" -Raw
$a=$source.IndexOf('struct at_highmountain_shredding_cyclone');$b=$source.IndexOf('void AddSC_highmountain()', $a)
if($a -lt 0 -or $b -lt 0){throw 'Missing production cyclone script'}
[IO.File]::WriteAllText("$output/WitchqueenArea.inc",$source.Substring($a,$b-$a))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if(!$vs){throw 'Visual C++ compiler not found'}
$cmd=@"
@echo off
call "$vs/VC/Auxiliary/Build/vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++17 /W4 /I"$output" "$PSScriptRoot/HighmountainWitchqueenTest.cpp" /Fe"$output/HighmountainWitchqueenTest.exe" /Fo"$output/HighmountainWitchqueenTest.obj"
if errorlevel 1 exit /b 1
"$output/HighmountainWitchqueenTest.exe"
exit /b %errorlevel%
"@
[IO.File]::WriteAllText("$output/run.cmd",$cmd)
& "$output/run.cmd"
if($LASTEXITCODE){throw 'Witchqueen production regression failed'}
