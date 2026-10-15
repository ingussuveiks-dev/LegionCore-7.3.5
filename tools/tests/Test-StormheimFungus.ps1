$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$output="$repo/build-extractors/runtime-test/stormheim-fungus"
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
 @('HasQuestForItem','bool Player::HasQuestForItem(','bool Player::HasQuestForCreature('),
 @('ItemAddedQuestCheck','void Player::ItemAddedQuestCheck(','void Player::ItemRemovedQuestCheck('),
 @('ItemRemovedQuestCheck','void Player::ItemRemovedQuestCheck(','void Player::KilledMonster('),
 @('AdjustQuestReqItemCount','void Player::AdjustQuestReqItemCount(','uint16 Player::FindQuestSlot('),
 @('QuestObjectiveSatisfy','void Player::QuestObjectiveSatisfy(','void Player::SetQuestStatus(')
)){
 $a=$core.IndexOf($segment[1]);$b=$core.IndexOf($segment[2],$a)
 if($a -lt 0 -or $b -lt 0){throw "Missing core handler $($segment[0])"}
 [IO.File]::WriteAllText("$output/$($segment[0]).inc",$core.Substring($a,$b-$a))
}
$loader=Get-Content "$repo/src/server/game/Globals/QuestData.cpp" -Raw
$a=$loader.IndexOf('        if (qinfo->PrevQuestID)');$b=$loader.IndexOf('        if (qinfo->ExclusiveGroup)',$a)
[IO.File]::WriteAllText("$output/QuestPrevLoader.inc",$loader.Substring($a,$b-$a))
$fixture=Get-Content "$PSScriptRoot/StormheimHeartHandoffTest.cpp" -Raw
$fixture=$fixture.Substring(0,$fixture.IndexOf('int main()'))
$fixture=$fixture.Replace('uint32 id=0,special=0;', 'uint32 id=0,special=0,FlagsEx=0,ItemDrop[6]{},ItemDropQuantity[6]{};')
$fixture=$fixture.Replace('bool IsDFQuest()const', 'void SetSpecialFlag(uint32 f){special|=f;}bool IsDFQuest()const')
$fixture=$fixture.Replace('std::map<std::pair<uint32,uint32>,int32> data;', 'mutable std::map<std::pair<uint32,uint32>,int32> data;')
$fixture=$fixture.Replace('int32 GetQuestObjectiveData(uint32 q,uint32 e){', 'int32 GetQuestObjectiveData(uint32 q,uint32 e)const{')
$fixture=$fixture.Replace('int32 GetQuestObjectiveData(Quest const* q,int8 index){', 'int32 GetQuestObjectiveData(Quest const* q,int8 index)const{')
$fixture=$fixture.Replace('uint32 GetQuestSlotQuestId(uint8 index){', 'uint32 GetQuestSlotQuestId(uint8 index)const{')
$fixture=$fixture.Replace('int GetDifficultyID(){', 'int GetDifficultyID()const{')
$fixture=$fixture.Replace('Group* GetGroup(){', 'Group* GetGroup()const{')
$fixture=$fixture.Replace('std::map<uint32,uint32> bags; uint32 GetItemCount(uint32 id){return bags[id];}', 'mutable std::map<uint32,uint32> bags,bank;uint32 GetItemCount(uint32 id,bool includeBank=false)const{return bags[id]+(includeBank?bank[id]:0);}')
$fixture=$fixture.Replace('bool HasAura(uint32 a)', 'bool HasQuestForItem(uint32)const;void ItemAddedQuestCheck(uint32,uint32);void ItemRemovedQuestCheck(uint32,uint32);void AdjustQuestReqItemCount(Quest const*);bool InBattleground()const{return false;}void UpdateForQuestWorldObjects(){}void IncompleteQuest(uint32 q){quests[q].Status=QUEST_STATUS_INCOMPLETE;}bool HasAura(uint32 a)')
$fixture=$fixture.Replace('struct Quest {', 'constexpr int QUEST_ITEM_COUNT=6;struct ItemTemplate {int GetMaxCount()const{return 0;}uint32 GetMaxStackSize()const{return 10;}};struct ItemStore {ItemTemplate item;ItemTemplate const* GetItemTemplate(uint32){return &item;}} itemStore;auto sObjectMgr=&itemStore;struct Quest {')
[IO.File]::WriteAllText("$output/FungusFixture.inc",$fixture)
$go=Get-Content "$repo/src/server/game/Entities/GameObject/GameObjectData.cpp" -Raw
$a=$go.IndexOf('uint32 GameObjectTemplate::GetLootId() const');$b=$go.IndexOf('uint32 GameObjectTemplate::GetGossipMenuId() const',$a)
[IO.File]::WriteAllText("$output/GetLootId.inc",$go.Substring($a,$b-$a))
$reward=$core.IndexOf('void Player::RewardQuest(Quest const* quest,')
$a=$core.IndexOf('            case QUEST_OBJECTIVE_ITEM:', $reward);$b=$core.IndexOf('            case QUEST_OBJECTIVE_CURRENCY:', $a)
[IO.File]::WriteAllText("$output/RewardItemCase.inc",$core.Substring($a,$b-$a))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++20 /W4 /I`"$output`" `"$PSScriptRoot/StormheimFungusTest.cpp`" /Fe`"$output/StormheimFungus.exe`" /Fo`"$output/StormheimFungus.obj`"`r`nif not %errorlevel%==0 exit /b %errorlevel%`r`n`"$output/StormheimFungus.exe`"`r`n"
[IO.File]::WriteAllText("$output/run-StormheimFungus.cmd",$cmd)
& "$output/run-StormheimFungus.cmd"
if($LASTEXITCODE){throw 'Fungus production regression failed'}
