param([string]$OutputDirectory="$PSScriptRoot/../../build-extractors/runtime-test/valsharah-ritual")
$ErrorActionPreference='Stop'
$output=[IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Force -Path $output | Out-Null
$source=Get-Content "$PSScriptRoot/../../src/server/scripts/Legion/valsharah_rituals.cpp" -Raw
$source=[regex]::Replace($source,'(?m)^#include.*\r?\n','')
$start=$source.IndexOf('class spell_valsharah_summon_ysera_ritual');$end=$source.IndexOf('struct npc_valsharah_path_tyrande')
$source=$source.Remove($start,$end-$start)
$source=$source.Substring(0,$source.IndexOf('void AddSC_valsharah_rituals()'))
[IO.File]::WriteAllText("$output/ValRitual.inc",$source)
$vswhere="${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe"
$vs=& $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if(!$vs){throw 'Visual C++ compiler not found'}
$cmd=@"
@echo off
call "$vs/VC/Auxiliary/Build/vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++17 /W4 /I"$output" "$PSScriptRoot/ValsharahRitualTest.cpp" /Fe"$output/ValsharahRitualTest.exe" /Fo"$output/ValsharahRitualTest.obj"
if errorlevel 1 exit /b 1
"$output/ValsharahRitualTest.exe"
exit /b %errorlevel%
"@
[IO.File]::WriteAllText("$output/run.cmd",$cmd)
& "$output/run.cmd"
if($LASTEXITCODE){throw 'Valsharah ritual regression failed'}
