param([switch]$Baseline,[switch]$ScopeOnly,[switch]$VerifyReleaseBinary,[switch]$RetainedGrash,[string]$OutputDirectory='')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth -and $last -lt $source.Length){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};if($depth){throw "Unclosed $start"};$source.Substring($first,$last-$first)}
$path='src/server/scripts/Scenario/Artifacts/Shaman/Sharasdal/Sharasdal.cpp'
$source=(Get-Content "$repo/$path" -Raw).Replace("`r",'')
$pinned=(& git -C $repo show "007b8732a:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned Grash wall baseline missing'}
$old=Block $pinned 'class npc_grash :';$current=Block $source 'class npc_grash :'
$normal=$current
$start=$normal.IndexOf('        ObjectGuid _wallRecipient;');$end=$normal.IndexOf('        uint32 GetData(',$start)
if($start -lt 0 -or $end -lt $start){throw 'Wall fields/method boundaries changed'};$normal=$normal.Remove($start,$end-$start)
$normal=$normal.Replace((Block $normal 'void SetGUID('),'')
$gossip=Block $current 'void sGossipSelect('
$added="            if (step == 2 && (!SharasdalWall::World(me) || !BindWallRecipient(player)))`n                return;`n"
if(!$gossip.Contains($added)){throw 'Wall recipient admission missing'}
if(($gossip.Replace($added,'') -replace '\s','') -cne ((Block $old 'void sGossipSelect(') -replace '\s','')){throw 'Unrelated raw gossip/menu/path/criteria changed'}
$life=Block $current 'void UpdateLifecycle('
if(($life.Replace('            CompleteWallCredit(diff);','') -replace '\s','') -cne ((Block $old 'void UpdateLifecycle(') -replace '\s','')){throw 'Wound/heal/reply lifecycle changed'}
$movement=Block $current 'void MovementInform(';$oldMovement=Block $old 'void MovementInform('
if(($movement.Replace((Block $movement 'if (type == WAYPOINT_MOTION_TYPE)'),(Block $oldMovement 'if (type == WAYPOINT_MOTION_TYPE)')) -replace '\s','') -cne ($oldMovement -replace '\s','')){throw 'Point boss movement/home/unrelated movement changed'}
foreach($method in @('void sGossipSelect(','void UpdateLifecycle(','void MovementInform(')){$normal=$normal.Replace((Block $normal $method),(Block $old $method))}
if(($normal -replace '\s','') -cne ($old -replace '\s','')){throw 'Other Grash fields/combat/heals/reset/actions/routes/registration changed'}
$whole=$source.Replace($current,$old).Replace((Block $source 'namespace SharasdalWall'),'').Replace('#include "WaypointMovementGenerator.h"','')
if(($whole -replace '\s','') -cne ($pinned -replace '\s','')){throw 'Other actors/instance/spells/wounds/rescue/boss/pickup changed'}
$instancePath='src/server/scripts/Scenario/Artifacts/Shaman/Sharasdal/instance_sharasdal.cpp'
$instanceSource=(Get-Content "$repo/$instancePath" -Raw).Replace("`r",'')
$oldInstance=(& git -C $repo show "007b8732a:$instancePath") -join "`n";if($LASTEXITCODE){throw 'Pinned instance re-entry context missing'}
$normalizedInstance=$instanceSource.Replace((Block $instanceSource 'void RearmGrashWall('),'').Replace('            RearmGrashWall(player);','')
if(($normalizedInstance -replace '\s','') -cne ($oldInstance -replace '\s','')){throw 'Other instance death/owner/criteria/presentation/entry changed'}
if($ScopeOnly){'PASS exact Grash issued-path/native-wall arrival and original-recipient49817 scope; prior actions/reset/heals/gossip receipts/point boss/other actors retained';return}
if($Baseline -and $VerifyReleaseBinary){throw 'Counterfactual cannot certify Release'}
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/sharasdal-wall"}
$out=[IO.Path]::GetFullPath($OutputDirectory);New-Item -ItemType Directory -Force $out|Out-Null
[IO.File]::WriteAllText("$out/Policy.inc",(Block $source 'namespace SharasdalCompanion')+(Block $source 'namespace SharasdalGrash')+(Block $source 'namespace SharasdalWall'))
[IO.File]::WriteAllText("$out/Actor.inc",$(if($Baseline){$old}else{$current})+';')
$core=Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw
foreach($entry in @(@('Owner','Player* Unit::GetCharmerOrOwnerPlayerOrPlayerItself() const'),@('ModifyHealth','int64 Unit::ModifyHealth('))){[IO.File]::WriteAllText("$out/$($entry[0]).inc",(Block $core $entry[1]))}
[IO.File]::WriteAllText("$out/OwnerGuid.inc",(Block (Get-Content "$repo/src/server/game/Entities/Unit/Unit.h" -Raw) 'ObjectGuid GetCharmerOrOwnerGUID() const'))
[IO.File]::WriteAllText("$out/Quest.inc",(Block (Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw) 'void Player::AddQuestDelayedEvent('))
$heal=Block $core 'int32 Unit::DealHeal(';$last=$heal.IndexOf('    Unit* unit = this;');if($last -lt 0){throw 'Native heal boundary missing'}
[IO.File]::WriteAllText("$out/DealHeal.inc",$heal.Substring(0,$last).Replace(', SpellInfo const* spellProto','')+"    return gain;`n}")
$creature=Get-Content "$repo/src/server/game/Entities/Creature/Creature.cpp" -Raw
[IO.File]::WriteAllText("$out/Notify.inc",(Block $creature 'void Creature::NotifyHealApplied('))
[IO.File]::WriteAllText("$out/Lifecycle.inc",(Block $creature 'if (i_AI || i_disabledAI)'))
$fh=Get-Content "$repo/src/common/Utilities/FunctionProcessor.h" -Raw;$fs=Get-Content "$repo/src/common/Utilities/FunctionProcessor.cpp" -Raw
[IO.File]::WriteAllText("$out/Functions.inc",'typedef std::multimap<uint64, std::function<void()>> FunctionList;'+(Block $fh 'class TC_COMMON_API FunctionProcessor').Replace('TC_COMMON_API ','')+';'+$fs.Substring($fs.IndexOf('FunctionProcessor::FunctionProcessor()')))
function Adapt([string]$value,[string]$before,[string]$after){if(!$value.Contains($before)){throw "Wall host boundary changed: $before"};$value.Replace($before,$after)}
$fixture=Get-Content "$PSScriptRoot/SharasdalGrashHealingTest.cpp" -Raw
$hostSource=$fixture.Substring(0,$fixture.IndexOf('#include "Policy.inc"'))
$hostSource=Adapt $hostSource 'enum{CLASS_SHAMAN=7,' 'enum{MOTION_SLOT_IDLE=0,GO_STATE_ACTIVE=0,CLASS_SHAMAN=7,'
$hostSource=Adapt $hostSource 'virtual uint32 GetData(uint32)const{return 0;}' 'virtual void SetGUID(ObjectGuid const&,int32=0){}virtual void LastWPReached(){}virtual uint32 GetData(uint32)const{return 0;}'
$hostSource=Adapt $hostSource 'struct Map{}liveMap,otherMap;' 'struct Map{Creature* lower=nullptr;Creature* GetCreature(ObjectGuid const&);}liveMap,otherMap;'
$hostSource=Adapt $hostSource 'void HandleGameObject(ObjectGuid,bool){}' 'inline static uint32 opens=0;inline static bool opensSucceed=true;void HandleGameObject(ObjectGuid,bool){++opens;}template<class GO>void HandleGameObject(ObjectGuid,bool open,GO* go){++opens;if(open&&opensSucceed)go->state=0;}'
$hostSource=Adapt $hostSource (Block $hostSource 'struct Motion{') @'
struct MovementGenerator{virtual ~MovementGenerator()=default;virtual uint32 GetMovementGeneratorType()const{return 0;}};
template<class T>struct WaypointMovementGenerator:MovementGenerator{
 uint32 _pathId,_currentNode=0;std::vector<uint32> nodes=std::vector<uint32>(13);std::vector<uint32>* _path=&nodes;
 explicit WaypointMovementGenerator(uint32 id):_pathId(id){}
 uint32 GetMovementGeneratorType()const override{return WAYPOINT_MOTION_TYPE;}
 uint32 GetPathId()const{return _pathId;}void MovementInform(Creature&);
};
struct Motion{
 std::vector<uint32> paths,points;std::unique_ptr<MovementGenerator> idle;
 MovementGenerator* GetMotionSlot(uint32 slot){assert(slot==MOTION_SLOT_IDLE);return idle.get();}
 void MovePath(uint32 id,bool,int=0,int=0){paths.push_back(id);idle=std::make_unique<WaypointMovementGenerator<Creature>>(id);}
 void MovePoint(uint32 id,float,float,float){points.push_back(id);}
}
'@
$hostSource=Adapt $hostSource 'bool preDelete=false,m_AI_locked=false;' 'CreatureAI* GetDisabledAI()const{return i_disabledAI;}uint64 dbGuid=362066;uint64 GetDBTableGUIDLow()const{return dbGuid;}bool preDelete=false,m_AI_locked=false;'
$hostSource=Adapt $hostSource 'struct GO{ObjectGuid GetGUID()const{return {};}};GO* FindNearestGameObject(uint32,float){return nullptr;}' @'
struct GO:Unit{uint64 dbGuid=134287;uint32 state=1;uint64 GetDBTableGUIDLow()const{return dbGuid;}uint32 GetGoState()const{return state;}};
GO* wall=nullptr;bool wallNear=true;
GO* FindNearestGameObject(uint32 entryId,float range){assert(entryId==248514&&range==5);return wallNear?wall:nullptr;}
'@
$hostSource=Adapt $hostSource 'using GameObject=Creature::GO;' @'
using GameObject=Creature::GO;
Creature* Map::GetCreature(ObjectGuid const& guid){return lower&&lower->guid==guid&&lower->world&&!lower->preDelete?lower:nullptr;}
#include "WaypointInform.inc"
'@
[IO.File]::WriteAllText("$out/Reentry.inc",(Block $instanceSource 'void RearmGrashWall('))
$movementSource=Get-Content "$repo/src/server/game/Movement/MovementGenerators/WaypointMovementGenerator.cpp" -Raw
[IO.File]::WriteAllText("$out/WaypointInform.inc",'template<> '+(Block $movementSource 'void WaypointMovementGenerator<Creature>::MovementInform('))
$nativePath=Block (Get-Content "$repo/src/server/game/Movement/MovementGenerators/WaypointMovementGenerator.h" -Raw) 'uint32 GetPathId() const'
if(($nativePath -replace '\s','') -cne 'uint32GetPathId()const{return_pathId;}'){throw 'Native waypoint identity changed'}
$oldCriteria=Block $hostSource 'void Player::UpdateAchievementCriteria('
$hostSource=$hostSource.Replace($oldCriteria,@'
void Player::UpdateAchievementCriteria(uint32 type,uint32 asset){
 assert(type==92);++credits[asset];uint32 leaf=asset==49817?47127:asset==49815?47121:asset==49900?47180:asset==50161?47303:asset==49819?47133:0;assert(leaf);
 // Controlled native-service boundary: only the current1066 stage2 wall leaf
 // accepts49817, with its real amount1 cap. No other root is completed here.
 if(!scenario.rejected.count(asset)&&(asset!=49817||(scenario.id==1066&&scenario.step==2)))scenario.achievements.progress[leaf].Counter=1;
 if(onCredit)onCredit(asset);
}
'@)
[IO.File]::WriteAllText("$out/NativeHost.inc",$hostSource)
$compileSource="$PSScriptRoot/SharasdalWallTest.cpp"
if($RetainedGrash){
 if($Baseline){throw 'Retained current Grash is not the wall counterfactual'}
 # Execute the existing Grash heal/gossip cases against this actual current full
 # actor and augmented native host, rather than the source-certified frozen host.
 $compileSource="$out/RetainedGrash.cpp"
 $retainedCases=$fixture.Substring($fixture.IndexOf('#include "Policy.inc"'))
 # These earlier heal/gossip cases start after the separate two-Brute encounter.
 # Its real2/2 producer is tested by Test-SharasdalBrutes, not invented here.
 $retainedCases=Adapt $retainedCases 'scenario=Scenario{};paths.missing.clear();' 'scenario=Scenario{};scenario.achievements.progress[47161].Counter=2;paths.missing.clear();'
 [IO.File]::WriteAllText($compileSource,"#include `"NativeHost.inc`"`n"+$retainedCases)
}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$mode=if($Baseline){'/DBASELINE'}else{''}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi $mode /I`"$out`" `"$compileSource`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`"`nif errorlevel 1 exit /b 1`ncd /d `"$out`"`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
& "$out/run.cmd";if($LASTEXITCODE){throw "Native Sharasdal wall regression failed: $LASTEXITCODE"}
if($VerifyReleaseBinary){
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_sharasdal_wall_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Canonical native wall implementation missing'}}finally{$env:PYTHONPATH=$previous}
}
