param([string]$OutputDirectory="$PSScriptRoot/../../build-extractors/runtime-test/highmountain-cavern")
$ErrorActionPreference='Stop'
$output=[IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Force -Path $output | Out-Null
$source=Get-Content "$PSScriptRoot/../../src/server/game/Entities/Player/Player.cpp" -Raw
$start=$source.IndexOf('bool Player::SatisfyQuestPreviousQuest(');$end=$source.IndexOf('bool Player::SatisfyQuestClass(',$start)
$code=$source.Substring($start,$end-$start)
$start=$source.IndexOf('void Player::KilledMonster(');$end=$source.IndexOf('void Player::KilledMonsterCredit(',$start)
$code+=$source.Substring($start,$end-$start)
[IO.File]::WriteAllText("$output/CavernQuestHandlers.inc",$code)
$source=Get-Content "$PSScriptRoot/../../src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw
$start=$source.IndexOf('case SMART_EVENT_HEALTH_PCT:')+'case SMART_EVENT_HEALTH_PCT:'.Length;$end=$source.IndexOf('case SMART_EVENT_TARGET_HEALTH_PCT:',$start)
[IO.File]::WriteAllText("$output/CavernHealthHandler.inc",$source.Substring($start,$end-$start))
$vswhere="${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe"
$vs=& $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if(!$vs){throw 'Visual C++ compiler not found'}
$cmd=@"
@echo off
call "$vs/VC/Auxiliary/Build/vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++17 /W4 /I"$output" "$PSScriptRoot/HighmountainCavernTest.cpp" /Fe"$output/HighmountainCavernTest.exe" /Fo"$output/HighmountainCavernTest.obj"
if errorlevel 1 exit /b 1
"$output/HighmountainCavernTest.exe"
exit /b %errorlevel%
"@
[IO.File]::WriteAllText("$output/run.cmd",$cmd)
& "$output/run.cmd"
if($LASTEXITCODE){throw 'Cavern production regression failed'}
