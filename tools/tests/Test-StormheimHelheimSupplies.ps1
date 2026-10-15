$ErrorActionPreference='Stop'
& "$PSScriptRoot/Test-StormheimHelheimSuppliesData.ps1"
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$output="$repo/build-extractors/runtime-test/stormheim-helheim-supplies"
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
$fixture=$fixture.Replace('struct Quest {', 'constexpr int QUEST_ITEM_COUNT=6;struct ItemTemplate {uint32 GetQuality()const{return 0;}int GetMaxCount()const{return 0;}uint32 GetMaxStackSize()const{return 10;}};struct ItemStore {ItemTemplate item;ItemTemplate const* GetItemTemplate(uint32){return &item;}} itemStore;auto sObjectMgr=&itemStore;struct Quest {')
$fixture=$fixture.Replace('uint32 GetMaxStackSize()const{return 10;}', 'uint32 GetMaxStackSize()const{return 10;}uint32 GetInventoryType()const{return 0;}')
$fixture=$fixture.Replace('struct Unit:WorldObject {','struct Unit:WorldObject {uint32 GetTypeId(){return ToPlayer()?4:0;}bool HasAura(uint32){return false;}')
$fixture=$fixture.Replace('uint32 GetTeam(){return team;}','uint32 GetSkillValue(int){return 0;}bool IsInArmyTraining(){return false;}struct Army {std::vector<int> currentUnits[1];} armyTrainingInfo;uint32 GetAuraCount(uint32){return 0;}uint32 GetTeam(){return team;}')
[IO.File]::WriteAllText("$output/HelheimSuppliesFixture.inc",$fixture)
$go=Get-Content "$repo/src/server/game/Entities/GameObject/GameObjectData.cpp" -Raw
$a=$go.IndexOf('uint32 GameObjectTemplate::GetLootId() const');$b=$go.IndexOf('uint32 GameObjectTemplate::GetGossipMenuId() const',$a)
[IO.File]::WriteAllText("$output/GetLootId.inc",$go.Substring($a,$b-$a))
$reward=$core.IndexOf('void Player::RewardQuest(Quest const* quest,')
$a=$core.IndexOf('            case QUEST_OBJECTIVE_ITEM:', $reward);$b=$core.IndexOf('            case QUEST_OBJECTIVE_CURRENCY:', $a)
[IO.File]::WriteAllText("$output/RewardItemCase.inc",$core.Substring($a,$b-$a))
$loot=Get-Content "$repo/src/server/game/Loot/LootMgr.cpp" -Raw
$a=$loot.IndexOf('bool LootStoreItem::Roll(');$b=$loot.IndexOf('CurrencyLoot::CurrencyLoot(',$a)
if($a -lt 0 -or $b -lt $a){throw 'Native loot admission/roll missing'}
[IO.File]::WriteAllText("$output/LootRollAdmission.inc",$loot.Substring($a,$b-$a))
$a=$loot.IndexOf('void LootTemplate::Process(Loot&');$b=$loot.IndexOf('void LootTemplate::ProcessBossLoot(',$a)
[IO.File]::WriteAllText("$output/LootProcess.inc",$loot.Substring($a,$b-$a))
$spells=Get-Content "$repo/src/server/game/Spells/Spell.cpp" -Raw
$a=$spells.IndexOf('SpellCastResult Spell::CanOpenLock(');$b=$spells.IndexOf('void Spell::SetSpellValue(',$a)
[IO.File]::WriteAllText("$output/CanOpenLock.inc",$spells.Substring($a,$b-$a))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++20 /W4 /I`"$output`" `"$PSScriptRoot/StormheimHelheimSuppliesTest.cpp`" /Fe`"$output/StormheimHelheimSupplies.exe`" /Fo`"$output/StormheimHelheimSupplies.obj`"`r`nif not %errorlevel%==0 exit /b %errorlevel%`r`n`"$output/StormheimHelheimSupplies.exe`" `"$output/loot-fixture.tsv`" `"$output/drops-fixture.tsv`"`r`n"
[IO.File]::WriteAllText("$output/run-StormheimHelheimSupplies.cmd",$cmd)
& "$output/run-StormheimHelheimSupplies.cmd"
if($LASTEXITCODE){throw 'HelheimSupplies production regression failed'}
