param([switch]$Baseline,[switch]$AddressSanitizer,[switch]$VerifyNativeCatalog,[switch]$RouteBaseline)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..");$out="$repo/build-extractors/runtime-test/tuure-first-fight"
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};$source.Substring($first,$last-$first)}
function Replace-Fixture([string]$value,[string]$old,[string]$new){if(!$value.Contains($old)){throw "Fixture boundary missing: $old"};$value.Replace($old,$new)}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/$(if($Baseline){'baseline'}elseif($RouteBaseline){'route-baseline'}else{'current'})" -RuntimeDirectories @($repo,$out) -Test {
 $path='src/server/scripts/Scenario/Artifacts/Priest/Tuure/Tuure.cpp';$source=Get-Content "$repo/$path" -Raw
 $instancePath='src/server/scripts/Scenario/Artifacts/Priest/Tuure/instance_tuure.cpp';$selected=$source;$instanceSource=Get-Content "$repo/$instancePath" -Raw
 if($Baseline){$selected=(& git -C $repo show "8c946eb5:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned first-fight actor missing'};$instanceSource=(& git -C $repo show "8c946eb5:$instancePath") -join "`n";if($LASTEXITCODE){throw 'Pinned first-fight instance missing'}}
 [IO.File]::WriteAllText("$out/Instance.inc",(Block $instanceSource 'struct instance_tuure_InstanceMapScript :')+';')
 [IO.File]::WriteAllText("$out/Policy.inc",(Block $source 'namespace TuureFirstFight')+(Block $source 'namespace TuureIntroHeal')+(Block $source 'namespace TuureCompanion'))
 [IO.File]::WriteAllText("$out/Evade.inc",(Block (Block $selected 'struct npc_vindicator_borosAI :') 'void EnterEvadeMode('))
 $actor=Block $source 'struct npc_vindicator_borosAI :'
 [IO.File]::WriteAllText("$out/Methods.inc",((@('void HealReceived(','void UpdateLifecycle(','void Reset(','void CompleteIntroHeal(','void CancelIntroHeal(')|ForEach-Object {Block $actor $_}) -join "`n"))
 $fixture=Get-Content "$PSScriptRoot/TuureIntroHealTest.cpp" -Raw
 [IO.File]::WriteAllText("$out/Boros.inc",(Block $fixture 'struct Boros:CreatureAI')+';')
 $nativeFixture=$fixture.Substring(0,$fixture.IndexOf('#include "Policy.inc"'))
 $nativeFixture=Replace-Fixture $nativeFixture 'struct Map {};' @'
struct InstanceScript;
struct Reference{Player* player;Player* getSource()const{return player;}};
struct Map{InstanceScript* script=nullptr;std::vector<Reference> players;std::vector<Creature*> actors;uint32 GetInstanceId()const{return 7;}auto const& GetPlayers()const{return players;}Creature* GetCreature(ObjectGuid const&);};
using InstanceMap=Map;
struct WorldLocation{void WorldRelocate(uint32,float,float,float){}};
'@
 $nativeFixture=Replace-Fixture $nativeFixture 'struct InstanceScript {bool CheckDamageManager()const{return false;}void UpdateDamageManager(ObjectGuid,int32,bool){};};' @'
struct InstanceScript{
 InstanceMap* instance;uint32 creates=0;explicit InstanceScript(InstanceMap* map=nullptr):instance(map){}virtual ~InstanceScript()=default;
 virtual void OnCreatureCreate(Creature*){++creates;}virtual void CreatureDies(Creature*,Unit*){}virtual void CreatureDiesForScript(Creature*,Unit*){}virtual void OnUnitDeath(Unit*){}virtual void Update(uint32){}virtual WorldLocation* GetClosestGraveYard(float,float,float){return nullptr;}
 bool CheckDamageManager()const{return false;}void UpdateDamageManager(ObjectGuid,int32,bool){};
};
'@
 $nativeFixture=Replace-Fixture $nativeFixture 'virtual void MoveInLineOfSight(Unit*){};' 'virtual void MoveInLineOfSight(Unit*){};virtual void EnterEvadeMode(){};virtual void DoAction(int32){};'
 $nativeFixture=Replace-Fixture $nativeFixture 'InstanceScript* GetInstanceScript(){return nullptr;}' 'InstanceScript* GetInstanceScript(){return map?map->script:nullptr;}'
 $nativeFixture=Replace-Fixture $nativeFixture 'int32 DealHeal(Unit*,uint32);' 'int32 DealHeal(Unit*,uint32);void Kill(Creature*);Creature* FindNearestCreature(uint32,float,bool=true);'
 $nativeFixture=Replace-Fixture $nativeFixture 'Player* GetCharmerOrOwnerPlayerOrPlayerItself()const;' 'virtual bool IsPreDelete()const{return false;}virtual uint32 GetEntry()const{return 0;}virtual uint32 GetMapId()const{return 1489;}virtual uint32 GetInstanceId()const{return 7;}Player* GetCharmerOrOwnerPlayerOrPlayerItself()const;'
 $nativeFixture=Replace-Fixture $nativeFixture 'uint32 classId=5,credits=0;' 'uint32 classId=5,credits=0,firstCredits=0,healCredits=0,attempts=0;bool rejectCredit=false;'
 $nativeFixture=Replace-Fixture $nativeFixture 'void UpdateAchievementCriteria(uint32 type,uint32 asset){assert(type==92&&asset==50563);++credits;if(onCredit)onCredit();}' 'void UpdateAchievementCriteria(uint32 type,uint32 asset);'
 $nativeFixture=Replace-Fixture $nativeFixture 'bool preDelete=false,m_AI_locked=false;' 'uint64 dbGuid=0;uint32 states=0,flags=0x800f,flagRemovals=0,conversations=0,conversationAttempts=0;bool routeNear=true,failConversation=false;std::function<void()> onRemove;Player* nearest=nullptr;bool preDelete=false,m_AI_locked=false;'
 $nativeFixture=Replace-Fixture $nativeFixture 'Creature(){IsAIEnabled=true;}' 'Creature(){IsAIEnabled=true;}uint64 GetDBTableGUIDLow()const{return dbGuid;}bool HasUnitState(uint32 state)const{return (states&state)!=0;}Player* FindNearestPlayer(float distance){assert(distance==100);return nearest;}void CreateConversation(uint32 id){assert(id==2009);++conversationAttempts;if(!failConversation)++conversations;}void RemoveFlag(uint32 field,uint32 mask){assert(field==2&&mask==15);++flagRemovals;flags&=~mask;if(onRemove)onRemove();}'
 $nativeFixture=Replace-Fixture $nativeFixture 'assert(d==20);auto p=u?u->ToPlayer():nullptr;' 'if(d==40)return routeNear;assert(d==20);auto p=u?u->ToPlayer():nullptr;'
 $nativeFixture=Replace-Fixture $nativeFixture 'struct Scenario {uint32 id=1085,step=0;' 'struct Progress{uint64 Counter=0;};struct AchievementMgr{std::map<uint32,Progress> progress;auto const* GetCriteriaProgressMap()const{return &progress;}};struct Scenario {AchievementMgr achievement;AchievementMgr& GetAchievementMgr(){return achievement;}uint32 id=1085,step=0;'
 $nativeFixture=Replace-Fixture $nativeFixture 'struct Creature;struct Player;struct Unit;' @'
struct Creature;struct Player;struct Unit;
enum {CRITERIA_TYPE_KILL_CREATURE=0,MOTION_SLOT_IDLE=0,WAYPOINT_MOTION_TYPE=2,UNIT_STATE_ROAMING_MOVE=16};
constexpr float PET_FOLLOW_DIST=1.0f,PET_FOLLOW_ANGLE=2.0f;
struct WaypointData{uint32 delay_chance=0,delay=0,event_id=0,event_chance=0;};
inline bool roll_chance_i(uint32){return false;}inline uint32 urand(uint32 a,uint32){return a;}inline int sWaypointScripts=0;
#define TC_LOG_DEBUG(...) ((void)0)
struct MovementGenerator{virtual ~MovementGenerator()=default;virtual uint32 GetMovementGeneratorType()const{return WAYPOINT_MOTION_TYPE;}};
template<class T>struct WaypointMovementGenerator:MovementGenerator{uint32 _pathId=9859002,_currentNode=0;std::vector<WaypointData> nodes=std::vector<WaypointData>(13);std::vector<WaypointData>* _path=&nodes;struct Timer{void Reset(uint32){}} _nextMoveTime;
#include "PathId.inc"
 void MovementInform(Creature&);void OnArrived(Creature&);
};
struct WaypointMgr{bool missing=false;std::vector<int> path=std::vector<int>(13);auto const* GetPathScript(uint32 id){assert(id==9859002);return missing?nullptr:&path;}} waypointMgr;
auto sWaypointMgr=&waypointMgr;
struct TextMgr{uint32 calls=0;void SendChat(Creature*,uint8 id){assert(!id);++calls;}} textMgr;
auto sCreatureTextMgr=&textMgr;
'@
 $nativeFixture=Replace-Fixture $nativeFixture 'uint64 dbGuid=0;' 'uint32 waypoint=0,pathMoves=0,followMoves=0;bool motionPresent=false,failRoute=false;WaypointMovementGenerator<Creature> movement;std::function<void()> onConversation;uint64 dbGuid=0;'
 $nativeFixture=Replace-Fixture $nativeFixture 'assert(id==2009);++conversationAttempts;if(!failConversation)++conversations;' 'assert(id==2009||id==2007||id==2039);++conversationAttempts;if(!failConversation)++conversations;if(onConversation){auto callback=onConversation;callback();}'
 $nativeFixture=Replace-Fixture $nativeFixture 'Creature* GetMotionMaster(){return this;}' 'Creature* GetMotionMaster(){return this;}MovementGenerator* GetMotionSlot(uint32 slot){assert(!slot);return motionPresent?&movement:nullptr;}uint32 GetCurrentWaypointID()const{return waypoint;}void UpdateWaypointID(uint32 id){waypoint=id;}void MovePath(uint32 id,bool repeat){assert(id==9859002&&!repeat);++pathMoves;movement._pathId=id;motionPresent=!failRoute;}void MoveFollow(Creature*,float,float,uint32){++followMoves;}'
 $nativeFixture=Replace-Fixture $nativeFixture 'AchievementMgr achievement;AchievementMgr& GetAchievementMgr()' 'uint32 killDeliveries=0,routeDeliveries=0;bool rejectKill=false;AchievementMgr achievement;void UpdateAchievementCriteria(uint32,uint32,uint32,uint32,Unit*,Player*);AchievementMgr& GetAchievementMgr()'
 $nativeFixture=Replace-Fixture $nativeFixture 'virtual void DoAction(int32){};' 'virtual void DoAction(int32){};virtual void MovementInform(uint32,uint32){};virtual void LastWPReached(){};'
 $nativeFixture=Replace-Fixture $nativeFixture 'Creature* GetCreature(ObjectGuid const&);' 'Creature* GetCreature(ObjectGuid const&);template<class... Args>void ScriptsStart(Args...){}'
 $nativeFixture+="`n#include `"NativeArrival.inc`"`n"
 $waypoint=Get-Content "$repo/src/server/game/Movement/MovementGenerators/WaypointMovementGenerator.cpp" -Raw
 [IO.File]::WriteAllText("$out/NativeArrival.inc",((Block $waypoint 'void WaypointMovementGenerator<Creature>::MovementInform(')+(Block $waypoint 'void WaypointMovementGenerator<Creature>::OnArrived(')).Replace('void WaypointMovementGenerator<Creature>::','template<> void WaypointMovementGenerator<Creature>::'))
 $oldInstance=(& git -C $repo show "426827bf:$instancePath") -join "`n";if($LASTEXITCODE){throw 'Pinned route baseline missing'}
 [IO.File]::WriteAllText("$out/OldRouteDeath.inc",(Block $oldInstance 'void OnUnitDeath('))
 [IO.File]::WriteAllText("$out/PathId.inc",(Block (Get-Content "$repo/src/server/game/Movement/MovementGenerators/WaypointMovementGenerator.h" -Raw) 'uint32 GetPathId() const'))
 [IO.File]::WriteAllText("$out/NativeHost.inc",$nativeFixture)
 $core=Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw
 [IO.File]::WriteAllText("$out/NativeKillBridge.inc",(Block (Block $core 'void Unit::Kill(') 'if (InstanceScript* script = creature->GetInstanceScript())'))
 [IO.File]::WriteAllText("$out/Owner.inc",(Block $core 'Player* Unit::GetCharmerOrOwnerPlayerOrPlayerItself() const'))
 [IO.File]::WriteAllText("$out/OwnerGuid.inc",(Block (Get-Content "$repo/src/server/game/Entities/Unit/Unit.h" -Raw) 'ObjectGuid GetCharmerOrOwnerGUID() const'))
 [IO.File]::WriteAllText("$out/Quest.inc",(Block (Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw) 'void Player::AddQuestDelayedEvent('))
 $heal=Block $core 'int32 Unit::DealHeal(';$last=$heal.IndexOf('    Unit* unit = this;');if($last -lt 0){throw 'Native heal boundary missing'}
 [IO.File]::WriteAllText("$out/DealHeal.inc",$heal.Substring(0,$last).Replace(', SpellInfo const* spellProto','')+"    return gain;`n}")
 [IO.File]::WriteAllText("$out/ModifyHealth.inc",(Block $core 'int64 Unit::ModifyHealth('))
 [IO.File]::WriteAllText("$out/Lifecycle.inc",(Block (Get-Content "$repo/src/server/game/Entities/Creature/Creature.cpp" -Raw) 'if (i_AI || i_disabledAI)'))
 $nativeCriteria=Get-Content "$repo/src/server/game/Achievements/AchievementMgr.cpp" -Raw;$gate=$nativeCriteria.IndexOf('if (cachePtr->player->isGameMaster() || !cachePtr->player->CanContact())');$last=$nativeCriteria.IndexOf('return;',$gate)
 if($gate -lt 0 -or $last -lt $gate){throw 'Native criteria delivery gate missing'}
 [IO.File]::WriteAllText("$out/NativeCreditGate.inc",$nativeCriteria.Substring($gate,$last+7-$gate))
 $fh=Get-Content "$repo/src/common/Utilities/FunctionProcessor.h" -Raw;$fs=Get-Content "$repo/src/common/Utilities/FunctionProcessor.cpp" -Raw
 [IO.File]::WriteAllText("$out/Functions.inc",'typedef std::multimap<uint64, std::function<void()>> FunctionList;'+(Block $fh 'class TC_COMMON_API FunctionProcessor').Replace('TC_COMMON_API ','')+';'+$fs.Substring($fs.IndexOf('FunctionProcessor::FunctionProcessor()')))
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 $def=if($Baseline){'/DBASELINE'}elseif($RouteBaseline){'/DROUTE_BASELINE'}else{''};if($AddressSanitizer){$def+=' /fsanitize=address /Zi'}
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $def /I`"$out`" `"$PSScriptRoot/TuureFirstFightTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`"`nif errorlevel 1 exit /b 1`ncd /d `"$out`"`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
 & "$out/run.cmd";if($LASTEXITCODE){throw 'Tuure actual first-fight regression failed'}
 if($VerifyNativeCatalog){
  $savedCatalog=$env:LEGION_DIALOGUE_CONFIG
  try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile TuurePickupCatalog.cpp -ProbeName TuureFirstFightCatalog -OutputFile "$out/native.tsv" *> "$out/catalog.log";if(!$? -or $LASTEXITCODE){throw 'Native first-fight catalog failed'}}finally{$env:LEGION_DIALOGUE_CONFIG=$savedCatalog}
  $rows=Get-Content "$out/native.tsv"
  foreach($expected in @("TREE`t1`t47517`t0`t0`t4`t0`t0","CRITERION`t1`t47519`t47517`t0`t29833`t92`t50562","TREE`t2`t47881`t0`t0`t4`t0`t0","CRITERION`t2`t47882`t47881`t0`t30023`t92`t50831","CRITERION`t2`t47883`t47881`t0`t30011`t0`t106221","LFG`t1243`t1489`t12`t1`t1`t0`t0`t1")){if($rows -notcontains $expected){throw "Native first-fight/escape/solo contract differs: $expected"}}
  if($rows|Where-Object {$_ -like 'STAGE_EVENT*'}){throw 'Native stage send-event producer found; review to avoid duplicate/scripted credit'}
  $db=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
  $query=@'
SET SESSION TRANSACTION READ ONLY;
SELECT 'instance',map,script FROM instance_template WHERE map=1489;
SELECT 'spawn',guid,id,map,PhaseId,spawnMask,spawntimesecs FROM creature WHERE guid IN(361419,361428,361429) ORDER BY guid;
SELECT 'binding',entry,AIName,ScriptName FROM creature_template WHERE entry IN(106134,106201,106221) ORDER BY entry;
SELECT 'enemy-ai',id,event_type,action_type,action_param1,target_type FROM smart_scripts WHERE source_type=0 AND entryorguid=106201 ORDER BY id;
SELECT 'other205',COUNT(*) FROM smart_scripts WHERE action_type=205 AND action_param1=92 AND action_param2=50562;
SELECT 'scenario',ScenarioID,Class FROM scenario_data WHERE MapID=1489 ORDER BY ScenarioID;
SELECT 'path',id,COUNT(*) FROM waypoint_data_script WHERE id IN(9859002,9859003) GROUP BY id ORDER BY id;
'@
  $savedSqlPwd=$env:MYSQL_PWD
  try{$env:MYSQL_PWD=$db[3];$live=@($query|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($db[0]) --port=$($db[1]) --user=$($db[2]) --database=$($db[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Read-only first-fight data query failed'}}finally{$env:MYSQL_PWD=$savedSqlPwd}
  $expected=@("instance`t1489`tinstance_tuure","spawn`t361419`t106134`t1489`t6374`t4096`t7200","spawn`t361428`t106201`t1489`t6374`t4096`t7200","spawn`t361429`t106221`t1489`t6374`t4096`t7200","binding`t106134`t`tnpc_vindicator_boros","binding`t106201`tSmartAI`t","binding`t106221`tNullAI`t","enemy-ai`t0`t0`t11`t204082`t2","enemy-ai`t1`t0`t11`t204086`t2","other205`t0","scenario`t940`t0","scenario`t972`t3","scenario`t1085`t5","scenario`t1145`t12","scenario`t1146`t1","path`t9859002`t13","path`t9859003`t8")
  if(Compare-Object $expected $live){throw 'Native first-fight spawns/scripts/paths/shared-map data changed; preserve and review'}
  $live|Set-Content -Encoding utf8 "$out/live.tsv"
  'PASS matching real first-opponent/companion/next-actor spawns and scripts, sole producer, both native step2 criteria, solo admission and five-scenario shared-map/paths; no live data modified'
 }
}
