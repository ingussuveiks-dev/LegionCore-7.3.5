$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
& "$PSScriptRoot/Test-StormheimJournalsData.ps1"
& "$PSScriptRoot/Test-StormheimBuiltScale.ps1" *> "$repo/.codex/journals-upstream.log"
$out="$repo/build-extractors/runtime-test/stormheim-journals"
Get-ChildItem "$repo/build-extractors/runtime-test/stormheim-built-scale/*.inc" | Copy-Item -Destination $out
$f=Get-Content "$out/HelheimSuppliesFixture.inc" -Raw
$f=$f.Replace('QUEST_FLAGS_AUTOCOMPLETE=1,','QUEST_FLAGS_AUTOCOMPLETE=0x10000,').Replace('uint32 GetEntry(){return entry;}','uint32 GetEntry()const{return entry;}')
$f=$f.Replace('struct Quest {','struct Quest {uint32 SourceItemId=0,SourceItemIdCount=0;')
$f=$f.Replace('QuestRelations starters;','QuestRelations starters,enders;QuestRelationBounds GetGOQuestInvolvedRelationBounds(uint32 id){return enders.equal_range(id);}QuestRelationBounds GetCreatureQuestInvolvedRelationBounds(uint32 id){return enders.equal_range(id);}')
$f=$f.Replace('struct WorldObject {','struct WorldObject {bool hasInvolvedQuest(uint32);')
$f=$f.Replace('struct GameObject:WorldObject {','struct GameObject:WorldObject {bool hasQuest(uint32)const;bool hasInvolvedQuest(uint32)const;')
$menu='struct QuestMenu {std::map<uint32,uint32> items;void ClearMenu(){items.clear();}void AddMenuItem(uint32 q,uint32 icon){items[q]=icon;}};'
$f=$f.Replace('struct PlayerMenu {',$menu+'struct PlayerMenu {QuestMenu quests;QuestMenu& GetQuestMenu(){return quests;}std::vector<uint32> offers;void SendQuestGiverOfferReward(Quest const* q,ObjectGuid,bool){offers.push_back(q->id);}')
$f=$f.Replace('struct StarterItem;struct Player:Unit {','struct Item;struct StarterItem;struct Player:Unit {void SendDirectMessage(int){}void PrepareQuestMenu(ObjectGuid);bool GiveQuestSourceItem(Quest const*);uint32 sourcePackets=0,storeCount=0;InventoryResult sourceResult=0;InventoryResult CanStoreNewItem(int,int,ItemPosCountVec&,uint32,uint32 n){storeCount=n;return sourceResult;}Item* StoreNewItem(ItemPosCountVec const&,uint32,bool);void SendNewItem(Item*,uint32,bool,bool){++sourcePackets;}bool CanInteractWithQuestGiver(WorldObject*){return world&&alive&&phase&&visible;}')
[IO.File]::WriteAllText("$out/HelheimSuppliesFixture.inc",$f)
$prefix=Get-Content "$PSScriptRoot/StormheimBuiltScaleTest.cpp" -Raw
$prefix=$prefix.Substring(0,$prefix.IndexOf('int main('))
$prefix="using InventoryResult=int;using ItemPosCountVec=int;constexpr int NULL_BAG=0,NULL_SLOT=0,EQUIP_ERR_OK=0,EQUIP_ERR_ITEM_MAX_COUNT=2;`n"+$prefix
[IO.File]::WriteAllText("$out/JournalsFixture.inc",$prefix)
$player=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
foreach($part in @(@('QuestMenu','void Player::PrepareQuestMenu(','void Player::SendPreparedQuest('),@('QuestSource','bool Player::GiveQuestSourceItem(','bool Player::TakeQuestSourceItem('))){$a=$player.IndexOf($part[1]);$b=$player.IndexOf($part[2],$a);if($a -lt 0 -or $b -le $a){throw 'Missing journal native player method'};[IO.File]::WriteAllText("$out/$($part[0]).inc",$player.Substring($a,$b-$a))}
$go=Get-Content "$repo/src/server/game/Entities/GameObject/GameObject.cpp" -Raw
$a=$go.IndexOf('bool GameObject::hasQuest(');$b=$go.IndexOf('bool GameObject::IsTransport(',$a)
[IO.File]::WriteAllText("$out/JournalRelations.inc",$go.Substring($a,$b-$a))
$handler=Get-Content "$repo/src/server/game/Handlers/QuestHandler.cpp" -Raw
$a=$handler.IndexOf('void WorldSession::HandleQuestGiverRequestReward(');$b=$handler.IndexOf('void WorldSession::HandleQuestLogRemoveQuest(',$a)
[IO.File]::WriteAllText("$out/JournalRequest.inc",$handler.Substring($a,$b-$a))
$condition=Get-Content "$repo/src/server/game/Conditions/ConditionMgr.cpp" -Raw
$a=$condition.IndexOf('bool ConditionMgr::IsPlayerMeetingCondition(Unit* unit, int32 conditionID, bool send)');$b=$condition.IndexOf('bool ConditionMgr::IsPlayerMeetingCondition(Unit* unit, PlayerConditionEntry const* condition)',$a)
[IO.File]::WriteAllText("$out/JournalConditionLookup.inc",$condition.Substring($a,$b-$a))
$a=$condition.IndexOf('        case CONDITION_QUEST_OBJECTIVE_DONE:')
# Keep exactly this switch case, without depending on the next condition name.
$part=$condition.Substring($a);$b=$part.IndexOf('        case ',$part.IndexOf('            break;'))
[IO.File]::WriteAllText("$out/JournalCondition.inc",$part.Substring(0,$b))
$smart=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw
$a=$smart.IndexOf('        case SMART_ACTION_CALL_KILLEDMONSTER:');$b=$smart.IndexOf('        case SMART_ACTION_SET_INST_DATA:',$a)
[IO.File]::WriteAllText("$out/JournalSmartCredit.inc",$smart.Substring($a,$b-$a))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++20 /I`"$out`" `"$PSScriptRoot/StormheimJournalsTest.cpp`" /Fe`"$out/Journals.exe`" /Fo`"$out/Journals.obj`"`r`nif not %errorlevel%==0 exit /b %errorlevel%`r`n`"$out/Journals.exe`" `"$out/world.tsv`"`r`n"
[IO.File]::WriteAllText("$out/run.cmd",$cmd);& "$out/run.cmd"
if($LASTEXITCODE){throw 'Journal native progression test failed'}
