param([string]$OutputDirectory = "$PSScriptRoot/../../build-extractors/runtime-test/valsharah-chain")
$ErrorActionPreference='Stop'
$output=[IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Force -Path $output | Out-Null
foreach($h in @('ScriptMgr.h','GameObject.h','Player.h')){[IO.File]::WriteAllText("$output/$h",'#pragma once')}
$source=Get-Content "$PSScriptRoot/../../src/server/scripts/Legion/DarkHeartThicket/boss_shade_of_xavius.cpp" -Raw
function Extract-Method([string]$text,[string]$needle){
 $start=$text.IndexOf($needle);if($start -lt 0){throw "Missing production method: $needle"}
 $open=$text.IndexOf('{',$start);$depth=1;$end=$open+1
 while($depth -gt 0 -and $end -lt $text.Length){if($text[$end] -eq '{'){$depth++};if($text[$end] -eq '}'){$depth--};$end++}
 if($depth){throw 'Unbalanced production method'}
 return $text.Substring($start,$end-$start).Replace(' override','')
}
[IO.File]::WriteAllText("$output/ValBossDeath.inc",(Extract-Method $source 'void JustDied('))
[IO.File]::WriteAllText("$output/ValMalfurion.inc",(Extract-Method $source 'void DoAction(')+"`n"+(Extract-Method $source 'void sGossipSelect('))
$inst=Get-Content "$PSScriptRoot/../../src/server/scripts/Legion/DarkHeartThicket/instance_darkheart_thicket.cpp" -Raw
$start=$inst.IndexOf('DoorData const doorData[]');$end=$inst.IndexOf('};',$start)+2
[IO.File]::WriteAllText("$output/ValDoors.inc",$inst.Substring($start,$end-$start))
$vswhere="${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe"
$vs=& $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if(!$vs){throw 'Visual C++ compiler not found'}
$cmd=@"
@echo off
call "$vs/VC/Auxiliary/Build/vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++17 /W4 /I"$output" "$PSScriptRoot/ValsharahChainTest.cpp" /Fe"$output/ValsharahChainTest.exe" /Fo"$output/ValsharahChainTest.obj"
if errorlevel 1 exit /b 1
"$output/ValsharahChainTest.exe"
exit /b %errorlevel%
"@
[IO.File]::WriteAllText("$output/run.cmd",$cmd)
& "$output/run.cmd"
if($LASTEXITCODE){throw 'Valsharah production regression failed'}
