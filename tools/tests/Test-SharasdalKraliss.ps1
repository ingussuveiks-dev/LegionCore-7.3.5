param([switch]$Baseline,[switch]$ScopeOnly,[switch]$VerifyReleaseBinary,[string]$OutputDirectory='')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth -and $last -lt $source.Length){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};if($depth){throw "Unclosed $start"};$source.Substring($first,$last-$first)}
$path='src/server/scripts/Scenario/Artifacts/Shaman/Sharasdal/instance_sharasdal.cpp';$actorPath='src/server/scripts/Scenario/Artifacts/Shaman/Sharasdal/Sharasdal.cpp'
$source=(Get-Content "$repo/$path" -Raw).Replace("`r",'');$actor=(Get-Content "$repo/$actorPath" -Raw).Replace("`r",'')
$scopeSource=$source;$scopeActor=$actor
if($source.Contains('namespace SharasdalLady')){
 & "$PSScriptRoot/Test-SharasdalLady.ps1" -ScopeOnly
 if(!$?){throw 'Current Lady source certificate failed'}
 $scopeSource=(& git -C $repo show "d7610f9d1:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned pre-Lady instance missing'}
 $scopeActor=(& git -C $repo show "d7610f9d1:$actorPath") -join "`n";if($LASTEXITCODE){throw 'Pinned pre-Lady actor missing'}
}
$old=(& git -C $repo show "553a4c55a:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned instance before Kraliss ownership missing'}
$oldActor=(& git -C $repo show "553a4c55a:$actorPath") -join "`n";if($LASTEXITCODE){throw 'Pinned Kraliss actor missing'}
$normal=$scopeSource.Replace((Block $scopeSource 'namespace SharasdalKraliss'),'')
$start=$normal.IndexOf('        ObjectGuid kralissRecipient;');$end=$normal.IndexOf('        void BeginBrutes(',$start)
if($start -lt 0 -or $end -lt $start){throw 'Kraliss member/method boundaries changed'};$normal=$normal.Remove($start,$end-$start)
$deathPrefix=@'
            if (creature && creature->GetEntry() == 102839)
            {
                RecordKralissDeath(creature, killer);
                return;
            }
'@
$entryPrefix=@'
            if (kralissPending && player && player->GetGUID() == kralissRecipient)
            {
                kralissExhausted = false;
                kralissRetry = kralissTries = 0;
            }
'@
foreach($prefix in @($deathPrefix,$entryPrefix)){
 $prefix=$prefix.Replace("`r",'');if(!$normal.Contains($prefix)){throw 'Kraliss native death/entry prefix missing'};$normal=$normal.Replace($prefix,'')
}
$normal=$normal.Replace('            UpdateKraliss(diff);','')
if(($normal -replace '\s','') -cne ($old -replace '\s','')){throw 'Other native instance guard/Brute/wall/fields/entry/registration changed'}
$currentNPC=Block $actor 'class npc_kraliss :';$oldNPC=Block $oldActor 'class npc_kraliss :'
$normalNPC=$currentNPC
if($currentNPC.Contains('events.RescheduleEvent(EVENT_3, 1000);')){
 & "$PSScriptRoot/Test-SharasdalKralissRoar.ps1" -ScopeOnly
 if(!$?){throw 'Current roar source certificate failed'}
 $roarBase=(& git -C $repo show "267d273dc:$actorPath") -join "`n";if($LASTEXITCODE){throw 'Pinned pre-roar actor missing'}
 $normalNPC=Block $roarBase 'class npc_kraliss :'
}
$normalActor=$scopeActor.Replace($currentNPC,$normalNPC.Replace((Block $normalNPC 'void JustDied('),(Block $oldNPC 'void JustDied(')))
if(($normalActor -replace '\s','') -cne ($oldActor -replace '\s','')){throw 'Other Kraliss combat/cast/Reset/actor or Lady/outro/pickup changed'}
if($ScopeOnly){'PASS exact real Kraliss instance death/owner/quest/native credit/retry and removal of proximity credit; prior wall/Brute/guard/actors/boss combat/Lady/outro/pickup retained';return}
if($Baseline -and $VerifyReleaseBinary){throw 'Counterfactual cannot certify Release'}
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/sharasdal-kraliss"}
$out=[IO.Path]::GetFullPath($OutputDirectory);New-Item -ItemType Directory -Force $out|Out-Null
$policy=(Block $source 'namespace SharasdalGuard')+(Block $source 'namespace SharasdalBrutes')+(Block $source 'namespace SharasdalKraliss')
if($source.Contains('namespace SharasdalLady')){$policy+=(Block $source 'namespace SharasdalLady')}
[IO.File]::WriteAllText("$out/Policy.inc",$policy)
[IO.File]::WriteAllText("$out/Actor.inc",$(if($Baseline){$oldNPC}else{$currentNPC})+';')
[IO.File]::WriteAllText("$out/Instance.inc",(Block $(if($Baseline){$old}else{$source}) 'struct instance_sharasdal_InstanceMapScript :')+';')
$core=Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw
foreach($entry in @(@('Owner','Player* Unit::GetCharmerOrOwnerPlayerOrPlayerItself() const'),@('ModifyHealth','int64 Unit::ModifyHealth('),@('CreateConversation','void Unit::CreateConversation('))){[IO.File]::WriteAllText("$out/$($entry[0]).inc",(Block $core $entry[1]))}
[IO.File]::WriteAllText("$out/OwnerGuid.inc",(Block (Get-Content "$repo/src/server/game/Entities/Unit/Unit.h" -Raw) 'ObjectGuid GetCharmerOrOwnerGUID() const'))
[IO.File]::WriteAllText("$out/Quest.inc",(Block (Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw) 'void Player::AddQuestDelayedEvent('))
$heal=Block $core 'int32 Unit::DealHeal(';$last=$heal.IndexOf('    Unit* unit = this;');if($last -lt 0){throw 'Native heal boundary missing'}
[IO.File]::WriteAllText("$out/DealHeal.inc",$heal.Substring(0,$last).Replace(', SpellInfo const* spellProto','')+"    return gain;`n}")
$creatureSource=Get-Content "$repo/src/server/game/Entities/Creature/Creature.cpp" -Raw
[IO.File]::WriteAllText("$out/Notify.inc",(Block $creatureSource 'void Creature::NotifyHealApplied('))
[IO.File]::WriteAllText("$out/Lifecycle.inc",(Block $creatureSource 'if (i_AI || i_disabledAI)'))
$fh=Get-Content "$repo/src/common/Utilities/FunctionProcessor.h" -Raw;$fs=Get-Content "$repo/src/common/Utilities/FunctionProcessor.cpp" -Raw
[IO.File]::WriteAllText("$out/Functions.inc",'typedef std::multimap<uint64, std::function<void()>> FunctionList;'+(Block $fh 'class TC_COMMON_API FunctionProcessor').Replace('TC_COMMON_API ','')+';'+$fs.Substring($fs.IndexOf('FunctionProcessor::FunctionProcessor()')))
$kill=Block $core 'void Unit::Kill(';[IO.File]::WriteAllText("$out/Kill.inc",'void Unit::NativeKillDispatch(Creature* creature){'+(Block $kill 'if (InstanceScript* script = creature->GetInstanceScript())')+'}')
[IO.File]::WriteAllText("$out/ScenarioGate.inc",(Block (Get-Content "$repo/src/server/game/Scenarios/Scenario.cpp" -Raw) 'bool Scenario::CanUpdateCriteria('))
function Adapt([string]$value,[string]$before,[string]$after){if(!$value.Contains($before)){throw "Native host boundary changed: $before"};$value.Replace($before,$after)}
$fixture=Get-Content "$PSScriptRoot/SharasdalGrashHealingTest.cpp" -Raw
$hostSource=$fixture.Substring(0,$fixture.IndexOf('#include "Policy.inc"'))
$hostSource=Adapt $hostSource 'EVENT_2=2};' 'EVENT_2=2,EVENT_3=3};'
$hostSource=Adapt $hostSource '#include <algorithm>' "#include <algorithm>`n#include <array>"
$hostSource=Adapt $hostSource 'enum{CLASS_SHAMAN=7,' 'enum{UNIT_FIELD_FLAGS=2,CLASS_SHAMAN=7,'
$hostSource=Adapt $hostSource 'struct ObjectGuid{' 'struct ObjectGuid{static ObjectGuid const Empty;'
$hostSource=Adapt $hostSource 'struct Map{}liveMap,otherMap;' @'
inline ObjectGuid const ObjectGuid::Empty{};
struct PlayerRef{Player* player;Player* getSource()const{return player;}};
struct PlayerList:std::vector<PlayerRef>{using std::vector<PlayerRef>::operator=;bool isEmpty()const{return empty();}};
struct Map{PlayerList players;auto const& GetPlayers()const{return players;}uint32 GetInstanceId()const{return 7;}Creature* GetCreature(ObjectGuid const&); }liveMap,otherMap;
using InstanceMap=Map;struct WorldLocation{void WorldRelocate(uint32,float,float,float){}};
'@
$hostSource=Adapt $hostSource 'virtual ~CreatureAI()=default;' 'virtual ~CreatureAI()=default;virtual void EnterEvadeMode(){}'
$oldInstance=Block $hostSource 'struct InstanceScript{'
$hostSource=$hostSource.Replace($oldInstance,@'
struct InstanceScript{
 Map* instance;explicit InstanceScript(Map* map=&liveMap):instance(map){}virtual ~InstanceScript()=default;
 virtual void CreatureDies(Creature*,Unit*){}virtual void CreatureDiesForScript(Creature*,Unit*){++scriptDeaths;}uint32 scriptDeaths=0;
 virtual void OnPlayerEnter(Player*){}virtual void OnUnitDeath(Unit*){}virtual void OnCreatureCreate(Creature*){}virtual void Update(uint32){}virtual WorldLocation* GetClosestGraveYard(float,float,float){return nullptr;}
 bool CheckDamageManager()const{return false;}void UpdateDamageManager(ObjectGuid,int32,bool){}void HandleGameObject(ObjectGuid,bool){}uint32 getScenarionStep()const;
}
'@)
$hostSource=Adapt $hostSource 'namespace WorldPackets::Combat{' 'inline InstanceScript* activeInstance=nullptr;namespace WorldPackets::Combat{'
$hostSource=Adapt $hostSource 'virtual Creature* ToCreature(){return nullptr;}' 'virtual uint32 GetEntry()const{return 0;}virtual Player* FindNearestPlayer(float){return nullptr;}virtual Creature* FindNearestCreature(uint32,float,bool=true);void NativeKillDispatch(Creature*);void CreateConversation(uint32);virtual Creature* ToCreature(){return nullptr;}'
$hostSource=Adapt $hostSource 'InstanceScript* GetInstanceScript(){static InstanceScript instance;return &instance;}' 'InstanceScript* GetInstanceScript(){static InstanceScript fallback;return activeInstance?activeInstance:&fallback;}'
$hostSource=Adapt $hostSource 'QuestStatusData quest;bool record=' 'uint32 group=0;bool IsInSameGroupWith(Player const* other)const{return this==other||(group&&group==other->group);}QuestStatusData quest;bool record='
$hostSource=Adapt $hostSource 'bool preDelete=false,m_AI_locked=false;' 'uint64 dbGuid=362066;uint64 GetDBTableGUIDLow()const{return dbGuid;}CreatureAI* GetDisabledAI()const{return i_disabledAI;}void SetFlag(uint32 field,uint32 value){if(field==UNIT_FIELD_NPC_FLAGS)flags|=value;}void SetVisible(bool){}bool preDelete=false,m_AI_locked=false;'
$hostSource=Adapt $hostSource 'Creature* FindNearestCreature(uint32 id,float,bool=true){return id==102826?erunak:id==104853?adel:nullptr;}' 'Creature* FindNearestCreature(uint32 id,float distance,bool living=true)override{return id==102826?erunak:id==104853?adel:Unit::FindNearestCreature(id,distance,living);}'
$hostSource=Adapt $hostSource 'struct ObjectAccessor{inline static std::vector<Player*> players;' 'struct ObjectAccessor{inline static std::vector<Creature*> creatures;inline static std::vector<Player*> players;'
$hostSource=Adapt $hostSource '#include "Owner.inc"' @'
Creature* Map::GetCreature(ObjectGuid const& guid){for(auto actor:ObjectAccessor::creatures)if(actor->GetGUID()==guid&&actor->map==this&&actor->world&&!actor->preDelete)return actor;return nullptr;}
Creature* Unit::FindNearestCreature(uint32 id,float,bool living){for(auto actor:ObjectAccessor::creatures)if(actor->entry==id&&actor->map==map&&actor->world&&(!living||actor->alive))return actor;return nullptr;}
#include "Owner.inc"
#include "Kill.inc"
inline uint32 conversationAttempts=0,conversationCreates=0,conversationDeletes=0;inline bool conversationFail=false;inline int32 conversationDuration=17600;inline ObjectGuid conversationCaster;
enum class HighGuid{Conversation};struct Generator{uint64 Generate(){return 1;}};
struct ObjectMgr{template<HighGuid>Generator* GetGenerator(){static Generator generator;return &generator;}} objectMgr;auto sObjectMgr=&objectMgr;
struct Conversation;inline std::vector<std::unique_ptr<Conversation>> mapConversations;
struct Conversation{
 ~Conversation(){++conversationDeletes;}
 bool CreateConversation(uint64,uint32 id,Unit* caster,void*,Unit const&){assert(id==1693);++conversationAttempts;if(conversationFail)return false;++conversationCreates;conversationCaster=caster->GetGUID();mapConversations.emplace_back(this);return true;}
 int32 GetDuration()const{return conversationDuration;}
};
#include "CreateConversation.inc"
'@
$oldScenario=Block $hostSource 'struct Scenario{'
$hostSource=$hostSource.Replace($oldScenario,$oldScenario.Replace('uint32 id=1066,step=0;','uint32 currentTree=47120,_criteriaTreeForUpdate=0;bool CanUpdateCriteria(uint32,uint32=0)const;uint32 id=1066,step=0;'))
$hostSource=Adapt $hostSource 'struct CriteriaProgress{' @'
struct CriteriaTreeEntry{uint32 ID,CriteriaID;};
struct DB2Manager{std::map<uint32,std::vector<CriteriaTreeEntry const*>> rows;auto GetCriteriaTreeList(uint32 root){auto found=rows.find(root);return found==rows.end()?nullptr:&found->second;}} db2;auto& sDB2Manager=db2;
struct CriteriaProgress{
'@
$hostSource=Adapt $hostSource 'uint32 InstanceScript::getScenarionStep()const{return scenario.step;}' "uint32 InstanceScript::getScenarionStep()const{return scenario.step;}`n#include `"ScenarioGate.inc`""
$oldCriteria=Block $hostSource 'void Player::UpdateAchievementCriteria('
$hostSource=$hostSource.Replace($oldCriteria,@'
void Player::UpdateAchievementCriteria(uint32 type,uint32 asset){
 assert(type==92);++credits[asset];
 uint32 leaf=asset==49815?47121:asset==49900?47180:asset==50161?47303:asset==49861?47161:asset==49957?47197:asset==49816?47123:asset==49819?47133:asset==49817?47127:0;assert(leaf);
 if(scenario.id==1066&&!scenario.rejected.count(asset)&&scenario.CanUpdateCriteria(leaf)){
  ++scenario.achievements.progress[leaf].Counter;
  auto count=[&](uint32 tree){return scenario.achievements.progress[tree].Counter;};
  if(scenario.step==0&&count(47121)&&count(47180)&&count(47303)&&count(47161)>=2){scenario.step=1;scenario.currentTree=47122;}
  if(scenario.step==1&&count(47123)&&count(47197)){scenario.step=2;scenario.currentTree=47126;}
 }
 if(onCredit)onCredit(asset);
}
'@)
$hostSource=Adapt $hostSource 'struct Unit;struct Player;struct Creature;' 'struct SpellInfo{uint32 Id=0;};struct Unit;struct Player;struct Creature;'
$hostSource=Adapt $hostSource 'virtual ~CreatureAI()=default;' 'virtual void SetGUID(ObjectGuid const&,int32=0){}virtual void JustDied(Unit*){}virtual void SpellHitTarget(Unit*,SpellInfo const*){}virtual ~CreatureAI()=default;'
$hostSource=Adapt $hostSource 'enum{UNIT_FIELD_FLAGS=2,' 'enum{SELECT_TARGET_RANDOM=0,UNIT_FIELD_FLAGS=2,'
$hostSource=Adapt $hostSource 'virtual uint32 GetEntry()const{return 0;}' 'void CastSpell(Unit*,uint32){}virtual uint32 GetEntry()const{return 0;}'
$hostSource=Adapt $hostSource 'bool UpdateVictim(){return false;}' 'Unit* SelectTarget(uint32,uint32){return nullptr;}bool UpdateVictim(){return false;}'
$hostSource=Adapt $hostSource 'bool preDelete=false,m_AI_locked=false;' 'ObjectGuid m_lootRecipient;Player* GetLootRecipient()const;bool preDelete=false,m_AI_locked=false;'
$hostSource=Adapt $hostSource 'struct ObjectAccessor{' 'struct ObjectAccessor{static Player* FindPlayer(ObjectGuid id){for(auto player:players)if(player->guid==id)return player;return nullptr;}'
$hostSource=Adapt $hostSource '#include "Owner.inc"' "#include `"LootRecipient.inc`"`n#include `"Owner.inc`""
[IO.File]::WriteAllText("$out/LootRecipient.inc",(Block $creatureSource 'Player* Creature::GetLootRecipient() const'))
$criteria=Block $hostSource 'void Player::UpdateAchievementCriteria('
$hostSource=$hostSource.Replace($criteria,@'
void Player::UpdateAchievementCriteria(uint32 type,uint32 asset){
 assert(type==92);++credits[asset];uint32 leaf=asset==50240?47330:asset==49817?47127:asset==49957?47197:asset==49816?47123:asset==49861?47161:0;assert(leaf);
 if(scenario.id==1066&&!scenario.rejected.count(asset)&&scenario.CanUpdateCriteria(leaf)){
  auto& count=scenario.achievements.progress[leaf].Counter;if(count<1)++count;
  if(scenario.step==2&&scenario.achievements.progress[47127].Counter&&scenario.achievements.progress[47330].Counter){scenario.step=3;scenario.currentTree=47130;}
 }
 if(onCredit)onCredit(asset);
}
'@)
[IO.File]::WriteAllText("$out/NativeHost.inc",$hostSource)
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$mode=if($Baseline){'/DBASELINE'}else{''}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi $mode /I`"$out`" `"$PSScriptRoot/SharasdalKralissTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`"`nif errorlevel 1 exit /b 1`ncd /d `"$out`"`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
& "$out/run.cmd";if($LASTEXITCODE){throw "Native Kraliss regression failed: $LASTEXITCODE"}
if($VerifyReleaseBinary){
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_sharasdal_kraliss_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Canonical native Kraliss implementation missing'}}finally{$env:PYTHONPATH=$previous}
}
