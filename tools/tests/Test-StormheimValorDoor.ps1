$ErrorActionPreference='Stop'
& "$PSScriptRoot/Test-StormheimValorDoorData.ps1"
& "$PSScriptRoot/Test-StormheimHelheimBargain.ps1" *> "$PSScriptRoot/../../.codex/valor-door-upstream.log"
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/stormheim-valor-door"
$upstream="$repo/build-extractors/runtime-test/stormheim-helheim-bargain"
Get-ChildItem "$upstream/*.inc" | Copy-Item -Destination $out
$f=Get-Content "$upstream/BargainFixture.inc" -Raw
$f=$f.Replace('QUEST_FLAGS_AUTOCOMPLETE=1,','QUEST_FLAGS_AUTOCOMPLETE=0x10000,')
$f=$f.Replace('struct WorldObject:Position {','struct WorldObject:Position {virtual uint32 GetTypeId(){return 0;}virtual GameObject* ToGameObject(){return nullptr;}bool hasInvolvedQuest(uint32);')
$f=$f.Replace('struct CreatureTemplate {','struct CreatureTemplate {uint32 TypeFlags[2]={};')
$f=$f.Replace('struct Creature:Unit {','struct Creature:Unit {ObjectGuid charmer;bool hostile=false,near=true;uint32 getFaction(){return 190;}ObjectGuid GetCharmerGUID(){return charmer;}bool IsHostileTo(Player*){return hostile;}bool IsWithinDistInMap(WorldObject* p,float range){return near&&map==p->map&&GetExactDist(p)<range+6.75f+1.5f;}')
$f=$f.Replace('struct GameObject:WorldObject {','struct GameObject:WorldObject {GameObject* ToGameObject()override{return this;}')
$f=$f.Replace('struct GameObjectAI {','struct GameObjectAI {virtual bool QuestAccept(Player*,Quest const*){return false;}')
$f=$f.Replace('uint32 GetTypeId(){return ToPlayer()?4:0;}', 'uint32 GetTypeId()override{return ToPlayer()?4:0;}')
$f=$f.Replace('struct PlayerMenu {','struct PlayerMenu {uint32 offers=0;void SendQuestGiverOfferReward(Quest const*,ObjectGuid,bool){++offers;}')
$f=$f.Replace('struct Player:Unit {','struct Player:Unit {Creature* GetNPCIfCanInteractWith(ObjectGuid,uint32,uint32=0);bool CanInteractWithQuestGiver(Object*);GameObject* GetGameObjectIfCanInteractWith(ObjectGuid,int){return nullptr;}void AddQuestAndCheckCompletion(Quest const*,Object*);void AddQuest(Quest const* q,Object*){Accept(q->id);}void DestroyItem(uint32,uint32,bool){}struct Reputation{int GetRank(void const*){return 0;}} reputation;Reputation& GetReputationMgr(){return reputation;}')
$f=$f.Replace('struct Unit:WorldObject {','using Object=WorldObject;struct Unit:WorldObject {')
$f=$f.Replace('bool CanTakeQuest(Quest const* q,bool b){return SatisfyQuestRace(q,b)&&SatisfyQuestPreviousQuest(q,b);}', 'bool CanTakeQuest(Quest const*,bool);bool SatisfyQuestStatus(Quest const*,bool);')
$f=$f.Replace('return s?s->Status:QUEST_STATUS_NONE;', 'return s?s->Status:(m_RewardedQuests.count(q)?QUEST_STATUS_REWARDED:QUEST_STATUS_NONE);')
foreach($name in @('ExclusiveGroup','Class','Level','Skill','Reputation','Timed','NextChain','PrevChain','Day','Week','Seasonal','Conditions')){$f=$f.Replace('struct Player:Unit {',"struct Player:Unit {bool SatisfyQuest$name(Quest const*,bool=false){return true;}")}
[IO.File]::WriteAllText("$out/ValorFixture.inc",$f)
$core=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
foreach($p in @(@('Accept','void Player::AddQuestAndCheckCompletion(','void Player::AddQuest('),@('CanTake','bool Player::CanTakeQuest(','bool Player::CanAddQuest('),@('Status','bool Player::SatisfyQuestStatus(','bool Player::SatisfyQuestConditions('),@('Interact','bool Player::CanInteractWithQuestGiver(','GameObject* Player::GetGameObjectIfCanInteractWith('))){$a=$core.IndexOf($p[1]);$b=$core.IndexOf($p[2],$a);if($a -lt 0 -or $b -lt 0){throw 'Native extraction anchor missing'};[IO.File]::WriteAllText("$out/$($p[0]).inc",$core.Substring($a,$b-$a))}
$handler=Get-Content "$repo/src/server/game/Handlers/QuestHandler.cpp" -Raw
$a=$handler.IndexOf('void WorldSession::HandleQuestGiverRequestReward(');$b=$handler.IndexOf('void WorldSession::HandleQuestLogRemoveQuest(',$a)
if($a -lt 0 -or $b -le $a){throw 'Native request-reward extraction anchor missing'}
[IO.File]::WriteAllText("$out/RequestReward.inc",$handler.Substring($a,$b-$a))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++20 /W4 /I`"$out`" `"$PSScriptRoot/StormheimValorDoorTest.cpp`" /Fe`"$out/ValorDoor.exe`" /Fo`"$out/ValorDoor.obj`"`r`nif not %errorlevel%==0 exit /b %errorlevel%`r`n`"$out/ValorDoor.exe`" `"$out/world.tsv`"`r`n"
[IO.File]::WriteAllText("$out/run.cmd",$cmd);& "$out/run.cmd"
if($LASTEXITCODE){throw 'Valor Door native quest/interaction regression failed'}
