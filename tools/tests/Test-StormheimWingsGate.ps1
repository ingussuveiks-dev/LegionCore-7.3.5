$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
& "$PSScriptRoot/Test-StormheimWingsGateData.ps1"
& "$PSScriptRoot/Test-StormheimBuiltScale.ps1" *> "$repo/.codex/wings-gate-upstream.log"
$out="$repo/build-extractors/runtime-test/stormheim-wings"
Get-ChildItem "$repo/build-extractors/runtime-test/stormheim-built-scale/*.inc" | Copy-Item -Destination $out
$fixture=Get-Content "$out/HelheimSuppliesFixture.inc" -Raw
$fixture=$fixture.Replace('bool SatisfyQuestConditions(Quest const*,bool){return true;}','bool SatisfyQuestConditions(Quest const*,bool);')
[IO.File]::WriteAllText("$out/HelheimSuppliesFixture.inc",$fixture)
$prefix=Get-Content "$PSScriptRoot/StormheimBuiltScaleTest.cpp" -Raw
[IO.File]::WriteAllText("$out/WingsGateFixture.inc",$prefix.Substring(0,$prefix.IndexOf('int main(')))
$core=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
$a=$core.IndexOf('bool Player::SatisfyQuestConditions(');$b=$core.IndexOf('bool Player::SatisfyQuestTimed(',$a)
[IO.File]::WriteAllText("$out/QuestConditionsGate.inc",$core.Substring($a,$b-$a))
$conditions=Get-Content "$repo/src/server/game/Conditions/ConditionMgr.cpp" -Raw
$a=$conditions.IndexOf('bool ConditionMgr::IsObjectMeetToConditionList(');$b=$conditions.IndexOf('bool ConditionMgr::CanHaveSourceGroupSet(',$a)
[IO.File]::WriteAllText("$out/ConditionGroups.inc",$conditions.Substring($a,$b-$a))
$a=$conditions.IndexOf('        case CONDITION_QUESTREWARDED:');$b=$conditions.IndexOf('        case CONDITION_QUESTTAKEN:',$a)
[IO.File]::WriteAllText("$out/RewardedCondition.inc",$conditions.Substring($a,$b-$a))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++20 /I`"$out`" `"$PSScriptRoot/StormheimWingsGateTest.cpp`" /Fe`"$out/WingsGate.exe`" /Fo`"$out/WingsGate.obj`"`r`nif not %errorlevel%==0 exit /b %errorlevel%`r`n`"$out/WingsGate.exe`" `"$out/gate.tsv`"`r`n"
[IO.File]::WriteAllText("$out/gate.cmd",$cmd);& "$out/gate.cmd"
if($LASTEXITCODE){throw 'Wings native faction gate regression failed'}
