$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/stormheim-overlook"
& "$PSScriptRoot/Test-StormheimOverlookData.ps1"
& "$PSScriptRoot/Test-StormheimDilemma.ps1" *> "$out/upstream.log"
Get-ChildItem "$repo/build-extractors/runtime-test/stormheim-dilemma/*.inc" | Copy-Item -Destination $out
$f=Get-Content "$out/SpineFixture.inc" -Raw
$f=$f.Replace('struct WorldObject:Position {','struct WorldObject:Position {virtual bool IsPlayer(){return false;}')
$f=$f.Replace('uint32 GetQuestId()const{return id;}','bool IsAutoAccept()const;uint32 GetQuestId()const{return id;}')
$f=$f.Replace('struct PlayerMenu {','struct PlayerMenu {uint32 details=0;void SendQuestGiverQuestDetails(Quest const*,ObjectGuid,bool,bool){++details;}')
$f=$f.Replace('struct Vehicle {','struct Vehicle {uint32 GetCreatureEntry(){return 91470;}')
$f=$f.Replace('struct Aura {','struct Aura {template<class... T>static Aura* TryRefreshStackOrCreate(T...){return nullptr;}void ApplyForTargets(){}')
$f=$f.Replace('using Object=WorldObject;struct Unit:WorldObject {','struct SpellInfo;using Object=WorldObject;struct Unit:WorldObject {bool HandleSpellClick(Unit*,int8=-1);ObjectGuid GetOwnerGUID(){return {};}uint32 GetSpawnMode(){return 0;}bool IsInMap(Unit* p){return map==p->map;}int CastSpell(Unit*,SpellInfo const*,bool,void*,void*,ObjectGuid);void CastCustomSpell(uint32,int,int,Unit*,bool,void*,void*,ObjectGuid){}')
$f=$f.Replace('struct SpellInfo {','struct SpellInfo {uint32 EffectMask=1,ApplyAuraName=0;')
$f=$f.Replace('struct Player:Unit {','struct Player:Unit {OutdoorPvP* GetOutdoorPvP(){return nullptr;}bool storageFull=false,storeFails=false;uint32 newItems=0,storageErrors=0,popup=0;std::map<uint32,uint32> bank;void ItemAddedQuestCheck(uint32,uint32);void ItemRemovedQuestCheck(uint32,uint32);void IncompleteQuest(uint32 q){quests[q].Status=QUEST_STATUS_INCOMPLETE;}void UpdateForQuestWorldObjects(){}bool AddItem(uint32,uint32,uint32* =nullptr,ObjectGuid=ObjectGuid::Empty);InventoryResult CanStoreNewItem(int,int,ItemPosCountVec&,uint32,uint32,uint32*);Item* StoreNewItem(ItemPosCountVec const&,uint32,bool,int);void SendNewItem(Item*,uint32,bool,bool){++newItems;}uint32 GetLootSpecID(){return 0;}')
$f=$f.Replace('uint32 GetItemCount(uint32 id){return bags[id];}','uint32 GetItemCount(uint32 id,bool includeBank=false){return bags[id]+(includeBank?bank[id]:0);}')
$f=$f.Replace('struct SmartAI {','struct SmartAI {void OnSpellClick(Unit*);SmartScript* GetScript();')
$f=$f.Replace('bool CanRewardQuest(Quest const*,bool);','bool CanAddQuest(Quest const*,bool){return true;}void SetPopupQuestId(uint32 id){popup=id;}bool CanRewardQuest(Quest const*,bool);')
$f=$f.Replace('virtual void sQuestAccept(Player*,Quest const*)','void sQuestReward(Player*,Quest const*,uint32){}virtual void sQuestAccept(Player*,Quest const*)')
$f=$f.Replace('struct GameObjectAI {','struct GameObjectAI {void QuestReward(Player*,Quest const*,uint32){}')
[IO.File]::WriteAllText("$out/SpineFixture.inc",$f)
$f=Get-Content "$out/DilemmaFixture.inc" -Raw
$f=$f.Replace('GetEntry()==93603&&q==39153','GetEntry()==97250&&q==38317')
$f=$f.Replace('struct ScriptMgr{','struct ScriptMgr{template<class T>bool OnQuestReward(Player*,T*,Quest const*,uint32){return false;}')
$f=$f.Replace('struct Item:WorldObject{','struct Item:WorldObject{static int GenerateItemRandomPropertyId(uint32,uint32){return 0;}void SetGuidValue(int,ObjectGuid){}')
[IO.File]::WriteAllText("$out/OverlookFixture.inc",$f)
$s=Get-Content "$repo/src/server/game/Quests/QuestDef.cpp" -Raw
$a=$s.IndexOf('bool Quest::IsAutoAccept() const');$b=$s.IndexOf('bool Quest::IsAutoComplete() const',$a)
[IO.File]::WriteAllText("$out/NativeAutoAccept.inc",$s.Substring($a,$b-$a))
$s=Get-Content "$repo/src/server/game/Handlers/QuestHandler.cpp" -Raw
$a=$s.IndexOf('switch (object->GetTypeId())',$s.IndexOf('void WorldSession::HandleQuestGiverChooseReward('));$b=$s.IndexOf('// TODO: is this needed?',$a)
if($a -lt 0 -or $b -lt 0){throw 'Missing native post-reward next-quest dispatch'}
[IO.File]::WriteAllText("$out/NativeRewardHandoff.inc",$s.Substring($a,$b-$a))
$core=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
foreach($segment in @(
 @('ItemAddedQuestCheck','void Player::ItemAddedQuestCheck(','void Player::ItemRemovedQuestCheck('),
 @('ItemRemovedQuestCheck','void Player::ItemRemovedQuestCheck(','void Player::KilledMonster('),
 @('AddItem','bool Player::AddItem(','PetStable& Player::GetOrInitPetStable(')
)){$a=$core.IndexOf($segment[1]);$b=$core.IndexOf($segment[2],$a);if($a -lt 0 -or $b -lt 0){throw "Missing $($segment[0])"};[IO.File]::WriteAllText("$out/Native$($segment[0]).inc",$core.Substring($a,$b-$a))}
$s=Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw
$a=$s.IndexOf('bool Unit::HandleSpellClick(');$b=$s.IndexOf('void Unit::EnterVehicle(',$a);[IO.File]::WriteAllText("$out/NativeSpellClick.inc",$s.Substring($a,$b-$a))
$s=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartAI.cpp" -Raw
$a=$s.IndexOf('void SmartAI::OnSpellClick(');$b=$s.IndexOf('void SmartAI::OnApplyOrRemoveAura(',$a);[IO.File]::WriteAllText("$out/NativeOnSpellClick.inc",$s.Substring($a,$b-$a))
$s=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw
foreach($part in @(@('ClickEvent','case SMART_EVENT_ON_SPELLCLICK:','case SMART_EVENT_RECEIVE_EMOTE:'),@('AddItemAction','case SMART_ACTION_ADD_ITEM:','case SMART_ACTION_REMOVE_ITEM:'))){$a=$s.IndexOf($part[1]);$b=$s.IndexOf($part[2],$a);if($a -lt 0 -or $b -lt 0){throw 'Missing SmartAI branch'};[IO.File]::WriteAllText("$out/Native$($part[0]).inc",$s.Substring($a,$b-$a))}
$s=Get-Content "$repo/src/server/game/Movement/MovementGenerators/WaypointMovementGenerator.cpp" -Raw
$a=$s.IndexOf('void WaypointMovementGenerator<Creature>::LoadPath(');$b=$s.IndexOf('void WaypointMovementGenerator<Creature>::DoInitialize(',$a)
[IO.File]::WriteAllText("$out/NativePatrolLoad.inc",$s.Substring($a,$b-$a).Replace('Creature','PatrolCreature'))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++20 /I`"$out`" `"$PSScriptRoot/StormheimOverlookTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`r`nif errorlevel 1 exit /b 1`r`n`"$out/test.exe`" `"$out/world.tsv`"`r`n"
[IO.File]::WriteAllText("$out/run.cmd",$cmd);& "$out/run.cmd"
if($LASTEXITCODE){throw 'Overlook production regression failed'}
