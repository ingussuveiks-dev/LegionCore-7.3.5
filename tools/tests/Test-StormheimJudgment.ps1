param([switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$output="$repo/build-extractors/runtime-test/stormheim-judgment"
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
# The extracted shared dispatcher now references the separately regressed
# Skyhold return policy. A Warrior-only call is outside this Stormheim fixture;
# fail loudly if it is ever reached instead of silently allowing that credit.
if(!$fixture.Contains('#include "KilledMonsterCredit.inc"')){throw 'Shared kill dispatcher fixture boundary missing'}
if(!$fixture.Contains('namespace SkyholdArtifactReturn')){
 $fixture=$fixture.Replace('#include "KilledMonsterCredit.inc"',"namespace SkyholdArtifactReturn {inline bool CanCredit(Player*,Quest const*){assert(false && `"Warrior return outside Stormheim fixture`" );return false;}}`n#include `"KilledMonsterCredit.inc`"")
}
[IO.File]::WriteAllText("$output/JudgmentFixture.inc",$fixture)
$source=Get-Content "$repo/src/server/scripts/Legion/stormheim_judgment.cpp" -Raw
$source=[regex]::Replace($source,'(?m)^#include.*\r?\n','')
[IO.File]::WriteAllText("$output/Judgment.inc",$source.Substring(0,$source.IndexOf('void AddSC_stormheim_judgment()')))
$a=$core.IndexOf('bool Player::TalkedToCreature(');$b=$core.IndexOf('void Player::MoneyChanged(', $a)
[IO.File]::WriteAllText("$output/Talked.inc",$core.Substring($a,$b-$a))
$spell=Get-Content "$repo/src/server/game/Spells/Spell.cpp" -Raw
$a=$spell.IndexOf('SpellCastResult Spell::CanOpenLock(');$b=$spell.IndexOf('void Spell::SetSpellValue(',$a)
[IO.File]::WriteAllText("$output/CanOpenLock.inc",$spell.Substring($a,$b-$a))
$effects=Get-Content "$repo/src/server/game/Spells/SpellEffects.cpp" -Raw
$start=$effects.IndexOf('void Spell::EffectOpenLock(');$a=$effects.IndexOf('    SkillType skillId = SKILL_NONE;',$start);$b=$effects.IndexOf('    // not allow use skill grow',$a)
[IO.File]::WriteAllText("$output/OpenLockCompletion.inc",$effects.Substring($a,$b-$a))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++20 /W4 /I`"$output`" `"$PSScriptRoot/StormheimJudgmentTest.cpp`" /Fe`"$output/Judgment.exe`" /Fo`"$output/Judgment.obj`"`r`nif not %errorlevel%==0 exit /b %errorlevel%`r`n`"$output/Judgment.exe`"`r`n"
[IO.File]::WriteAllText("$output/run-Judgment.cmd",$cmd)
& "$output/run-Judgment.cmd"
if($LASTEXITCODE){throw 'Judgment production regression failed'}
if($VerifyReleaseBinary){
 $exe="$repo/build-extractors/bin/Release/worldserver.exe"
 $binary=[Text.Encoding]::ASCII.GetString([IO.File]::ReadAllBytes($exe))
 foreach($marker in @('go_stormheim_judgment','npc_stormheim_judgment_king','spell_stormheim_judgment_opening','Judgment Day: ignored generic kill credit','Judgment Day: ignored generic talk credit')){if(!$binary.Contains($marker)){throw "Canonical Release lacks $marker"}}
 [xml]$project=Get-Content "$repo/build-extractors/src/server/worldserver/worldserver.vcxproj"
 $release=$project.Project.ItemDefinitionGroup | Where-Object Condition -like '*Release|x64*' | Select-Object -First 1
 foreach($name in @('scripts','game')){
  $relative=$release.Link.AdditionalDependencies.Split(';') | Where-Object {$_ -match "$name\\Release\\$name.lib$"} | Select-Object -First 1
  if(!$relative){throw "Missing actual $name final-link dependency"}
  $library=[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $relative))
  if(!(Test-Path $library) -or (Get-Item $library).LastWriteTimeUtc -gt (Get-Item $exe).LastWriteTimeUtc){throw "Stale final $name link"}
 }
 'PASS: canonical Release contains Judgment GO/actor handlers and both native credit guards; actual scripts/game library inputs checked.'
}
