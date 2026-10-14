$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$output="$repo/build-extractors/runtime-test/stormheim-watcher"
New-Item -ItemType Directory -Force -Path $output | Out-Null
& "$PSScriptRoot/Export-QuestDelayedEvent.ps1" -OutputDirectory $output
$core=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
foreach($segment in @(
 @('HasQuestForItem','bool Player::HasQuestForItem(','bool Player::HasQuestForCreature('),
 @('AdjustQuestReqItemCount','void Player::AdjustQuestReqItemCount(','uint16 Player::FindQuestSlot('),
 @('CanRewardQuest','bool Player::CanRewardQuest(Quest const* quest, bool msg)','bool Player::CanRewardQuest(Quest const* quest, uint32 reward'),
 @('CanCompleteQuest','bool Player::CanCompleteQuest(','bool Player::CanCompleteRepeatableQuest('),
 @('PreviousQuest','bool Player::SatisfyQuestPreviousQuest(','bool Player::SatisfyQuestClass('),
 @('KilledMonsterCredit','void Player::KilledMonsterCredit(','void Player::KilledPlayerCredit('),
 @('ItemAddedQuestCheck','void Player::ItemAddedQuestCheck(','void Player::ItemRemovedQuestCheck('),
 @('ItemRemovedQuestCheck','void Player::ItemRemovedQuestCheck(','void Player::KilledMonster('),
 @('QuestObjectiveSatisfy','void Player::QuestObjectiveSatisfy(','void Player::SetQuestStatus(')
)){
 $a=$core.IndexOf($segment[1]);$b=$core.IndexOf($segment[2],$a)
 if($a -lt 0 -or $b -lt 0){throw "Missing core handler $($segment[0])"}
 [IO.File]::WriteAllText("$output/$($segment[0]).inc",$core.Substring($a,$b-$a))
}
$loot=Get-Content "$repo/src/server/game/Loot/LootMgr.cpp" -Raw
$a=$loot.IndexOf('void LootTemplate::Process(');$b=$loot.IndexOf('void LootTemplate::Process', $a+10)
[IO.File]::WriteAllText("$output/LootProcess.inc",$loot.Substring($a,$b-$a))
$go=Get-Content "$repo/src/server/game/Entities/GameObject/GameObjectData.cpp" -Raw
$a=$go.IndexOf('uint32 GameObjectTemplate::GetLootId()');$b=$go.IndexOf('uint32 GameObjectTemplate::GetGossipMenuId()', $a)
[IO.File]::WriteAllText("$output/GetLootId.inc",$go.Substring($a,$b-$a))
$a=$core.IndexOf('                    loot->clear();',$core.IndexOf('void Player::SendLoot('));$b=$core.IndexOf('                loot->shipmentBuildingType', $a)
$regular=$core.Substring($a,$b-$a);$regular=$regular.Substring(0,$regular.LastIndexOf('                }'))
[IO.File]::WriteAllText("$output/ChestFill.inc",$regular)
$a=$core.IndexOf('            case QUEST_OBJECTIVE_ITEM:',$core.IndexOf('void Player::RewardQuest(Quest const*'));$b=$core.IndexOf('            case QUEST_OBJECTIVE_CURRENCY:', $a)
[IO.File]::WriteAllText("$output/RewardItemCase.inc",$core.Substring($a,$b-$a))
$a=$core.IndexOf('    AddDelayedEvent(100, [this, quest_id]()',$core.IndexOf('void Player::RewardQuest(Quest const*'));$b=$core.IndexOf('    // update area quests', $a)
[IO.File]::WriteAllText("$output/RewardPhaseNotify.inc",$core.Substring($a,$b-$a))
$condition=Get-Content "$repo/src/server/game/Conditions/ConditionMgr.cpp" -Raw
$a=$condition.IndexOf('        case CONDITION_QUESTREWARDED:');$b=$condition.IndexOf('        case CONDITION_QUESTTAKEN:', $a)
[IO.File]::WriteAllText("$output/RewardCondition.inc",$condition.Substring($a,$b-$a))
$phase=Get-Content "$repo/src/server/game/Maps/PhaseMgr.cpp" -Raw
foreach($segment in @(
 @('QuestPhaseData','void PhaseUpdateData::AddQuestUpdate(','void PhaseUpdateData::AddScenarioUpdate('),
 @('PhaseRelated','bool PhaseUpdateData::IsConditionRelated(','bool PhaseMgr::IsConditionTypeSupported('),
 @('PhaseNotify','void PhaseMgr::NotifyConditionChanged(','//////////////////////////////////////////////////////////////////'),
 @('PhaseNeeds','bool PhaseMgr::NeedsPhaseUpdateWithData(','//////////////////////////////////////////////////////////////////'),
 @('PhaseCheck','inline bool PhaseMgr::CheckDefinition(','bool PhaseMgr::NeedsPhaseUpdateWithData('),
 @('PhaseRecalculate','void PhaseMgr::Recalculate()','inline bool PhaseMgr::CheckDefinition('),
 @('PhaseUpdate','void PhaseMgr::Update()','void PhaseMgr::RemoveUpdateFlag(')
)){$a=$phase.IndexOf($segment[1]);$b=$phase.IndexOf($segment[2],$a+1);if($a -lt 0 -or $b -lt 0){throw 'Missing phase handler'};[IO.File]::WriteAllText("$output/$($segment[0]).inc",$phase.Substring($a,$b-$a))}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++20 /W4 /I`"$output`" `"$PSScriptRoot/StormheimWatcherTest.cpp`" /Fe`"$output/test.exe`" /Fo`"$output/test.obj`"`r`nif not %errorlevel%==0 exit /b %errorlevel%`r`n`"$output/test.exe`"`r`n"
[IO.File]::WriteAllText("$output/run.cmd",$cmd)
& "$output/run.cmd"
if($LASTEXITCODE){throw 'Stormheim watcher production regression failed'}
