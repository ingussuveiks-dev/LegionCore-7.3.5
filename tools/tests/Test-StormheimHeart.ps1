$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$output="$repo/build-extractors/runtime-test/stormheim-heart"
New-Item -ItemType Directory -Force -Path $output | Out-Null
& "$PSScriptRoot/Export-QuestDelayedEvent.ps1" -OutputDirectory $output
$core=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
foreach($segment in @(
 @('NextQuest','Quest const* Player::GetNextQuest(','bool Player::CanSeeStartQuest('),
 @('Race','bool Player::SatisfyQuestRace(','bool Player::SatisfyQuestReputation('),
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
$source=Get-Content "$repo/src/server/scripts/Legion/stormheim_heart.cpp" -Raw
$source=[regex]::Replace($source,'(?m)^#include.*\r?\n','')
[IO.File]::WriteAllText("$output/Heart.inc",$source.Substring(0,$source.IndexOf('void AddSC_stormheim_heart()')))
$event=Get-Content "$repo/src/server/game/Entities/EventObject/EventObject.cpp" -Raw
$a=$event.IndexOf('void EventObject::Update(');$b=$event.IndexOf('void EventObject::MoveInLineOfSight(',$a)
[IO.File]::WriteAllText("$output/EventUpdate.inc",$event.Substring($a,$b-$a))
$a=$event.IndexOf('bool EventObject::ActivatedForPlayer(')
[IO.File]::WriteAllText("$output/Activated.inc",$event.Substring($a))
$a=$core.IndexOf('void Player::KilledMonster(');$b=$core.IndexOf('void Player::KilledMonsterCredit(',$a)
[IO.File]::WriteAllText("$output/KilledMonster.inc",$core.Substring($a,$b-$a))
$reward=Get-Content "$repo/src/server/game/Entities/Player/KillRewarder.cpp" -Raw
$a=$reward.IndexOf('inline void KillRewarder::_RewardKillCredit(');$b=$reward.IndexOf('void KillRewarder::_RewardPlayer(',$a)
[IO.File]::WriteAllText("$output/RewardKillCredit.inc",$reward.Substring($a,$b-$a))
$unit=Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw
$a=$unit.IndexOf('Player* Unit::GetCharmerOrOwnerPlayerOrPlayerItself(');$b=$unit.IndexOf('Player* Unit::GetAffectingPlayer(',$a)
[IO.File]::WriteAllText("$output/OwnerPlayer.inc",$unit.Substring($a,$b-$a))
$smart=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartAI.cpp" -Raw
$a=$smart.IndexOf('void SmartAI::SetData(');$b=$smart.IndexOf('void SmartAI::', $a+10)
[IO.File]::WriteAllText("$output/SmartSetData.inc",$smart.Substring($a,$b-$a))
$actions=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw
$a=$actions.IndexOf('        case SMART_ACTION_TALK:')+'        case SMART_ACTION_TALK:'.Length;$b=$actions.IndexOf('        case SMART_ACTION_SIMPLE_TALK:',$a)
[IO.File]::WriteAllText("$output/TalkAction.inc",$actions.Substring($a,$b-$a))
$start=$actions.IndexOf('void SmartScript::GetScript()');$a=$actions.IndexOf('    SmartAIEventList e;',$start);$b=$actions.IndexOf('    else if (go)',$a)
[IO.File]::WriteAllText("$output/CreatureScriptSelection.inc",$actions.Substring($a,$b-$a))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++20 /W4 /I`"$output`" `"$PSScriptRoot/StormheimHeartTest.cpp`" /Fe`"$output/StormheimHeart.exe`" /Fo`"$output/StormheimHeart.obj`"`r`nif not %errorlevel%==0 exit /b %errorlevel%`r`n`"$output/StormheimHeart.exe`"`r`n"
[IO.File]::WriteAllText("$output/run-StormheimHeart.cmd",$cmd)
& "$output/run-StormheimHeart.cmd"
if($LASTEXITCODE){throw 'Heart production regression failed'}
