param([string]$OutputDirectory="$PSScriptRoot/../../build-extractors/runtime-test/highmountain-rocs-eagles")
$ErrorActionPreference='Stop'
$output=[IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Force -Path $output | Out-Null
$source=Get-Content "$PSScriptRoot/../../src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw
function Export-Section([string]$From,[string]$To,[string]$Name){
 $a=$source.IndexOf($From);$b=$source.IndexOf($To,$a)
 if($a -lt 0 -or $b -lt 0){throw "Missing production section: $Name"}
 [IO.File]::WriteAllText("$output/$Name",$source.Substring($a,$b-$a))
}
Export-Section 'void SmartScript::ProcessEvent(' '    switch (e.GetEventType())' 'EagleEventEntry.inc'
Export-Section '        case SMART_EVENT_SPELLHIT_TARGET:' '        case SMART_EVENT_OOC_LOS:' 'EagleSpellHit.inc'
Export-Section 'void SmartScript::OnReset()' 'void SmartScript::ResetBaseObject()' 'EagleReset.inc'
Export-Section 'void SmartScript::RecalcTimer(' 'void SmartScript::UpdateTimer(' 'EagleTimer.inc'
Export-Section '        case SMART_ACTION_MOVE_Z:' '        case SMART_ACTION_SET_KD:' 'EagleMovement.inc'
Export-Section 'void SmartScript::ProcessAction(' '    switch (e.GetActionType())' 'EagleActionEntry.inc'
$vswhere="${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe"
$vs=& $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if(!$vs){throw 'Visual C++ compiler not found'}
$cmd=@"
@echo off
call "$vs/VC/Auxiliary/Build/vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++17 /W4 /I"$output" "$PSScriptRoot/HighmountainRocsEaglesTest.cpp" /Fe"$output/HighmountainRocsEaglesTest.exe" /Fo"$output/HighmountainRocsEaglesTest.obj"
if errorlevel 1 exit /b 1
"$output/HighmountainRocsEaglesTest.exe"
exit /b %errorlevel%
"@
[IO.File]::WriteAllText("$output/run.cmd",$cmd)
& "$output/run.cmd"
if($LASTEXITCODE){throw 'Highmountain production regression failed'}
