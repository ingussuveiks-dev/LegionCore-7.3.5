param([string]$OutputDirectory="$PSScriptRoot/../../build-extractors/runtime-test/highmountain-pet-rocks")
$ErrorActionPreference='Stop'
$output=[IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Force -Path $output | Out-Null
$source=Get-Content "$PSScriptRoot/../../src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw
$start=$source.IndexOf('void SmartScript::ProcessEvent(')
$event=$source.Substring($start)
$code=''
foreach($pair in @(@('        case SMART_EVENT_ON_SPELLCLICK:', '        case SMART_EVENT_RECEIVE_EMOTE:'),@('        case SMART_EVENT_SPELLHIT_TARGET:', '        case SMART_EVENT_OOC_LOS:'))){
 $a=$event.IndexOf($pair[0]);$b=$event.IndexOf($pair[1],$a);if($a -lt 0 -or $b -lt 0){throw 'Production event missing'}
 $code+=$event.Substring($a,$b-$a)
}
[IO.File]::WriteAllText("$output/PetRocksEvents.inc",$code)
$a=$source.IndexOf('void SmartScript::RecalcTimer(');$b=$source.IndexOf('void SmartScript::UpdateTimer(',$a)
[IO.File]::WriteAllText("$output/PetRocksTimer.inc",$source.Substring($a,$b-$a))
$vswhere="${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe"
$vs=& $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if(!$vs){throw 'Visual C++ compiler not found'}
$cmd=@"
@echo off
call "$vs/VC/Auxiliary/Build/vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++17 /W4 /I"$output" "$PSScriptRoot/HighmountainPetRocksEventsTest.cpp" /Fe"$output/HighmountainPetRocksEventsTest.exe" /Fo"$output/HighmountainPetRocksEventsTest.obj"
if errorlevel 1 exit /b 1
"$output/HighmountainPetRocksEventsTest.exe"
exit /b %errorlevel%
"@
[IO.File]::WriteAllText("$output/run.cmd",$cmd)
& "$output/run.cmd"
if($LASTEXITCODE){throw 'Highmountain production regression failed'}
