$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$output="$repo/build-extractors/runtime-test/stormheim-will"
New-Item -ItemType Directory -Force -Path $output | Out-Null
& "$PSScriptRoot/Export-QuestDelayedEvent.ps1" -OutputDirectory $output
$source=Get-Content "$repo/src/server/scripts/Legion/stormheim_will.cpp" -Raw
$source=[regex]::Replace($source,'(?m)^#include.*\r?\n','')
[IO.File]::WriteAllText("$output/Will.inc",$source.Substring(0,$source.IndexOf('void AddSC_stormheim_will()')))
$core=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
foreach($segment in @(
 @('AreaExplored','void Player::AreaExploredOrEventHappens(','void Player::GroupEventHappens('),
 @('KillCreditGO','void Player::KillCreditGO(','bool Player::TalkedToCreature('),
 @('CanRewardQuest','bool Player::CanRewardQuest(Quest const* quest, bool msg)','bool Player::CanRewardQuest(Quest const* quest, uint32 reward'),
 @('CanCompleteQuest','bool Player::CanCompleteQuest(','bool Player::CanCompleteRepeatableQuest('),
 @('PreviousQuest','bool Player::SatisfyQuestPreviousQuest(','bool Player::SatisfyQuestClass('),
 @('KilledMonsterCredit','void Player::KilledMonsterCredit(','void Player::KilledPlayerCredit('),
 @('GroupCredit','void Player::RewardPlayerAndGroupAtEvent(','bool Player::IsAtGroupRewardDistance('),
 @('QuestObjectiveSatisfy','void Player::QuestObjectiveSatisfy(','void Player::SetQuestStatus(')
)){
 $a=$core.IndexOf($segment[1]);$b=$core.IndexOf($segment[2],$a)
 if($a -lt 0 -or $b -lt 0){throw "Missing core handler $($segment[0])"}
 [IO.File]::WriteAllText("$output/$($segment[0]).inc",$core.Substring($a,$b-$a))
}
$map=Get-Content "$repo/src/server/game/Scripting/MapScripts.cpp" -Raw
$a=$map.IndexOf('            case SCRIPT_COMMAND_QUEST_EXPLORED:');$b=$map.IndexOf('            case SCRIPT_COMMAND_KILL_CREDIT:', $a)
if($a -lt 0 -or $b -lt 0){throw 'Missing native quest-explored action'}
[IO.File]::WriteAllText("$output/QuestStartAction.inc",$map.Substring($a,$b-$a))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++20 /W4 /I`"$output`" `"$PSScriptRoot/StormheimWillTest.cpp`" /Fe`"$output/test.exe`" /Fo`"$output/test.obj`"`r`nif not %errorlevel%==0 exit /b %errorlevel%`r`n`"$output/test.exe`"`r`n"
[IO.File]::WriteAllText("$output/run.cmd",$cmd)
& "$output/run.cmd"
if($LASTEXITCODE){throw 'Stormheim will production regression failed'}
