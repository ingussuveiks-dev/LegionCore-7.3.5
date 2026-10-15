param([switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
& "$PSScriptRoot/Test-StormheimHelheimDisguiseData.ps1"
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$output="$repo/build-extractors/runtime-test/stormheim-helheim-disguise"
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
$loader=Get-Content "$repo/src/server/game/Globals/QuestData.cpp" -Raw
$a=$loader.IndexOf('        if (qinfo->PrevQuestID)');$b=$loader.IndexOf('        if (qinfo->ExclusiveGroup)',$a)
[IO.File]::WriteAllText("$output/QuestPrevLoader.inc",$loader.Substring($a,$b-$a))
$fixture=Get-Content "$PSScriptRoot/StormheimHeartHandoffTest.cpp" -Raw
$fixture=$fixture.Substring(0,$fixture.IndexOf('int main()'))
$fixture=$fixture.Replace('uint32 id=0,special=0;', 'uint32 id=0,special=0,StartScript=0;')
$fixture=$fixture.Replace('static ObjectGuid const Empty;', 'void Clear(){value=0;}static ObjectGuid const Empty;')
$fixture=$fixture.Replace('struct WorldObject {', 'struct Position {float x=0,y=0,z=0,o=0;float GetPositionX()const{return x;}float GetPositionY()const{return y;}float GetPositionZ()const{return z;}float GetOrientation()const{return o;}float GetExactDist(Position const* p)const{return std::sqrt((x-p->x)*(x-p->x)+(y-p->y)*(y-p->y)+(z-p->z)*(z-p->z));}};struct WorldObject:Position {')
$fixture=[regex]::Replace($fixture,'float GetPositionX\(\)\{return 3442.69f;\}float GetPositionY\(\)\{return 2103.1f;\}float GetPositionZ\(\)\{return 277.243f;\}float GetOrientation\(\)\{return 1.57336f;\}','')
$fixture=$fixture.Replace('struct Creature:Unit {', 'struct Creature:Unit {void SetFacingTo(float angle){o=angle;}')
$fixture=$fixture.Replace('float GetExactDist(WorldObject* o){return o->distance;}', 'using Position::GetExactDist;float GetExactDist(WorldObject* object){return Position::GetExactDist(object);}')
$fixture=$fixture.Replace('return creatures.count(g)?creatures[g]:nullptr;', 'return creatures.count(g)&&creatures[g]->world?creatures[g]:nullptr;')
$fixture=$fixture.Replace('assert(n>0);if(q->id', 'assert(n>0||q->id==38778);if(q->id')
$fixture=$fixture.Replace('int CastSpell(Player*,uint32 a,bool){if(!castResult){auras.insert(a);if(castHook)castHook();}return castResult;}', 'int CastSpell(Player* target,uint32 id,bool triggered){return Unit::CastSpell(target,id,triggered);}')
$fixture=$fixture.Replace('struct Unit:WorldObject {', 'struct Unit:WorldObject {void RemoveAllMinionsByFilter(uint32,uint8){}void RemoveGameObject(uint32,bool){}')
$fixture=[regex]::Replace($fixture,'struct SpellInfo \{[^}]*\};','struct SpellInfo {uint32 Id=184579,TriggerSpell=0;SpellInfo* GetEffect(int,int)const{return const_cast<SpellInfo*>(this);}};')
$fixture=$fixture.Replace('uint32 GetTeam(){return team;}', 'bool isInFlight(){return false;}uint32 GetTeam(){return team;}')
$fixture=$fixture.Replace('bool TalkedToCreature(uint32,ObjectGuid){return false;}', 'bool TalkedToCreature(uint32,ObjectGuid);')
$fixture=$fixture.Replace('void Clear(){value=0;}', 'void Clear(){value=0;}explicit operator bool()const{return value!=0;}')
$fixture=$fixture.Replace('struct CreatureTemplate {', 'struct CreatureTemplate {uint32 ScriptID=2;')
$fixture=$fixture.Replace('struct WorldObject:Position {', 'struct WorldObject:Position {uint32 GetInstanceId(){return 0;}')
$fixture=$fixture.Replace('struct Unit:WorldObject {', 'struct Unit:WorldObject {uint32 GetTypeId(){return ToPlayer()?4:0;}bool HasAura(uint32){return false;}')
$fixture=$fixture.Replace('struct Creature:Unit {', 'struct Creature:Unit {bool IsAIEnabled=true;uint32 despawnDelay=0;std::vector<uint8> talkGroups;')
$fixture=$fixture.Replace('void DespawnOrUnsummon(){world=false;}', 'void DespawnOrUnsummon(uint32 delay=0){despawnDelay=delay;world=false;}')
$fixture=$fixture.Replace('struct GameObject:WorldObject {', 'struct GameObjectTemplate {uint32 type=10,ScriptId=1;struct {uint32 open=1691,questID=38811;} goober;};struct GameObject:WorldObject {uint64 dbGuid=0;uint64 GetDBTableGUIDLow(){return dbGuid;}uint32 GetScriptId(){return info.ScriptId;}GameObjectTemplate info;GameObjectTemplate* GetGOInfo(){return &info;}uint32 GetGoType(){return info.type;}float GetInteractionDistance(){return 5;}bool ShouldHideFor(ObjectGuid g){return hidden.count(g);}GameObjectAI* ai=nullptr;GameObjectAI* AI(){return ai;}')
# Forward-declare the GO AI before the object, matching the real engine headers.
$fixture=$fixture.Replace('struct Unit;struct Player;', 'struct GameObjectAI;struct Unit;struct Player;')
$fixture=$fixture.Replace('bool IsWithinDistInMap(WorldObject* o,float range){return o->distance<=range;}', 'bool IsWithinDistInMap(WorldObject* o,float range){return Position::GetExactDist(o)<=range;}')
$fixture=$fixture.Replace('Player* ToPlayer()override{return this;} std::function', 'std::function<Creature*(uint32,Position const&,ObjectGuid)> judgmentSummon;Creature* SummonCreature(uint32 e,Position const& point,int,uint32,int,ObjectGuid viewer){return judgmentSummon?judgmentSummon(e,point,viewer):nullptr;}Creature* SummonCreature(uint32 e,float x,float y,float z,float o,int t,uint32 d,ObjectGuid viewer){return SummonCreature(e,Position{x,y,z,o},t,d,0,viewer);}uint32 GetSkillValue(int){return 0;}bool IsInArmyTraining(){return false;}struct Army {std::vector<int> currentUnits[1];} armyTrainingInfo;uint32 GetAuraCount(uint32){return 0;}Player* ToPlayer()override{return this;} std::function')
$fixture=$fixture.Replace('void Talk(uint8,ObjectGuid){++me->talks;}', 'void Talk(uint8 group,ObjectGuid){++me->talks;me->talkGroups.push_back(group);}')
$fixture=$fixture.Replace('explicit SmartGameObjectAI(GameObject*){}', 'GameObject* go;explicit SmartGameObjectAI(GameObject* g):go(g){g->ai=this;}')
$fixture=$fixture.Replace('bool HasAura(uint32){return false;}', 'bool HasAura(uint32 id){for(auto const& pair:roots)if(pair.first.first==id)return true;return false;}')
$fixture=$fixture.Replace('bool isInFlight(){return false;}','bool flying=false;bool isInFlight(){return flying;}')
$fixture=$fixture.Replace('struct WorldObject:Position {','struct WorldObject:Position {std::set<uint32> phases;bool HasPhaseId(uint32 id)const{return phases.count(id);}')
$fixture=$fixture.Replace('struct FakeMap {','struct EventObject;std::map<ObjectGuid,EventObject*> eventObjects;struct FakeMap {EventObject* GetEventObject(ObjectGuid g){auto i=eventObjects.find(g);return i==eventObjects.end()?nullptr:i->second;}')
$fixture=$fixture.Replace('struct Unit:WorldObject {','struct Unit:WorldObject {struct Motion {uint32 follows=0;Unit* target=nullptr;void MoveFollow(Unit* p,float,float){target=p;++follows;}} motion;Motion* GetMotionMaster(){return &motion;}')
[IO.File]::WriteAllText("$output/HelheimDisguiseFixture.inc",$fixture)
$source=Get-Content "$repo/src/server/scripts/Legion/stormheim_helheim_disguise.cpp" -Raw
$source=[regex]::Replace($source,'(?m)^#include.*\r?\n','')
[IO.File]::WriteAllText("$output/HelheimDisguise.inc",$source.Substring(0,$source.IndexOf('void AddSC_stormheim_helheim_disguise()')))
$a=$core.IndexOf('bool Player::TalkedToCreature(');$b=$core.IndexOf('void Player::MoneyChanged(',$a)
[IO.File]::WriteAllText("$output/Talked.inc",$core.Substring($a,$b-$a))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++20 /W4 /I`"$output`" `"$PSScriptRoot/StormheimHelheimDisguiseTest.cpp`" /Fe`"$output/HelheimDisguise.exe`" /Fo`"$output/HelheimDisguise.obj`"`r`nif not %errorlevel%==0 exit /b %errorlevel%`r`n`"$output/HelheimDisguise.exe`"`r`n"
[IO.File]::WriteAllText("$output/run.cmd",$cmd)
& "$output/run.cmd"
if($LASTEXITCODE){throw 'Helheim disguise native regression failed'}
if($VerifyReleaseBinary){
 $exe="$repo/build-extractors/bin/Release/worldserver.exe";$bytes=[Text.Encoding]::ASCII.GetString([IO.File]::ReadAllBytes($exe))
 foreach($marker in @('npc_stormheim_helheim_ashildir','eventobject_stormheim_helheim_passage','player_stormheim_helheim_disguise','Stealth by Seaweed: ignored generic Guarm kill credit','Stealth by Seaweed: ignored generic Guarm talk credit')){if(!$bytes.Contains($marker)){throw "Canonical Release lacks $marker"}}
 [xml]$project=Get-Content "$repo/build-extractors/src/server/worldserver/worldserver.vcxproj"
 $release=$project.Project.ItemDefinitionGroup | Where-Object Condition -like '*Release|x64*' | Select-Object -First 1
 foreach($name in @('scripts','game')){
  $relative=$release.Link.AdditionalDependencies.Split(';') | Where-Object {$_ -match "$name\\Release\\$name.lib$"} | Select-Object -First 1
  if(!$relative){throw "Missing actual $name link input"}
  $lib=[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $relative))
  if(!(Test-Path $lib) -or (Get-Item $lib).LastWriteTimeUtc -gt (Get-Item $exe).LastWriteTimeUtc){throw "Canonical Release predates $name link input"}
 }
 'PASS: canonical Release disguise bindings/credit guards and actual scripts/game final-link inputs.'
}
