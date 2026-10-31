param([switch]$ScopeOnly,[switch]$PrepareHost,[switch]$Baseline,[switch]$VerifyReleaseBinary,[string]$ReleaseDirectory='',[string]$DataDirectory='',[string]$OutputDirectory='')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
function Block([string]$s,[string]$start){$a=$s.IndexOf($start);if($a -lt 0){throw "Missing $start"};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth -and $b -lt $s.Length){$depth+=([int]($s[$b] -eq '{'))-([int]($s[$b] -eq '}'));++$b};if($depth){throw "Unclosed $start"};$s.Substring($a,$b-$a)}
function Pinned([string]$path){$s=(& git -C $repo show "ccc0e2f74:$path") -join "`n";if($LASTEXITCODE){throw "Pinned lower-ascent context missing: $path"};$s}
function Same([string]$a,[string]$b,[string]$why){if(($a -replace '\s','') -cne ($b -replace '\s','')){throw $why}}
$path='src/server/scripts/Scenario/Artifacts/Shaman/Sharasdal/Sharasdal.cpp';$ipath='src/server/scripts/Scenario/Artifacts/Shaman/Sharasdal/instance_sharasdal.cpp'
$source=(Get-Content "$repo/$path" -Raw).Replace("`r",'');$instance=(Get-Content "$repo/$ipath" -Raw).Replace("`r",'');$old=Pinned $path;$oldInstance=Pinned $ipath
$erunak=Block $source 'class npc_erunak_stonespeaker :';$oldErunak=Block $old 'class npc_erunak_stonespeaker :'
$movement=Block $erunak 'void MovementInform(';$oldMovement=Block $oldErunak 'void MovementInform('
$arrival=@'
                if (id == 5 && me->GetEntry() == 102826 && me->GetDBTableGUIDLow() == 362063 &&
                    SharasdalWall::Path(me) == 10993810)
                    script->SetGuidData(50336, me->GetGUID()); // Retain actual arrival, including native step2.

'@
$expected=$oldMovement.Replace((Block $oldMovement 'else if (script->getScenarionStep() == 3)'), '').Replace('                if (script->getScenarionStep() <= 2)',$arrival+"`n"+'                if (script->getScenarionStep() <= 2)')
Same $movement $expected 'Other waypoint/home/lower speech or ascent dispatch changed'
Same ($erunak.Replace($movement,$oldMovement)) $oldErunak 'Other Erunak entry/upper/heal/combat/lifecycle changed'
$grash=Block $source 'class npc_grash :';$oldGrash=Block $old 'class npc_grash :'
$select=Block $grash 'void sGossipSelect(';$oldSelect=Block $oldGrash 'void sGossipSelect('
$bind=Block $select 'if (step == 2)'
$expectedBind='if (step == 2) { InstanceScript* script = me->GetInstanceScript(); if (!script) return; script->SetGuidData(208580, player->GetGUID()); if (script->GetGuidData(208580) != player->GetGUID()) return; }'
Same $bind $expectedBind 'Ascent original caller binding/acknowledgement changed'
Same ($select.Replace($bind,'')) $oldSelect 'Other gossip admission/credit/menu/route consumption changed'
$actions=Block $grash 'void DoAction(';$oldActions=Block $oldGrash 'void DoAction('
$expected=$oldActions
foreach($line in @(2,9)){$expected=$expected.Replace("                            erunak->AI()->Talk($line);","                            if (CreatureAI* content = erunak->GetDisabledAI() ? static_cast<CreatureAI*>(erunak->GetDisabledAI()) : erunak->AI())`n                                content->Talk($line);")}
$issue='                            if (InstanceScript* script = me->GetInstanceScript()) script->SetGuidData(10993810, erunak->GetGUID());'
Same ($actions.Replace($issue,'')) $expected 'Other routes/actions/timing or installed-path bridge changed'
Same ($grash.Replace($select,$oldSelect).Replace($actions,$oldActions)) $oldGrash 'Other Grash fields/heal/wall/combat/lifecycle changed'
Same ($source.Replace($erunak,$oldErunak).Replace($grash,$oldGrash)) $old 'Other scenario actors/spells/registration changed'
$normal=$instance.Replace('        #include "sharasdal_lower_ascent.h"','').Replace('            RearmAscent(player);','').Replace('            UpdateAscent(diff);','')
foreach($header in @('ObjectAccessor.h','WaypointManager.h','WaypointMovementGenerator.h','MoveSpline.h')){$normal=$normal.Replace("#include `"$header`"",'')}
Same $normal $oldInstance 'Other instance fields/deaths/credits/re-entry/creation changed'
foreach($name in @('sharasdal_erunak_entry.h','sharasdal_erunak_upper.h','sharasdal_upper_wall.h','sharasdal_gauntlet_arrival.h')){$relative='src/server/scripts/Scenario/Artifacts/Shaman/Sharasdal/'+$name;Same (Get-Content "$repo/$relative" -Raw) (Pinned $relative) "Other retained member policy changed: $name"}
if($ScopeOnly){'PASS only original lower step2 caller/acknowledged installed10993810/real final arrival and instance-owned native3 scene retry; both Grash route speech AI guards. Other actors/native deaths/credit/loot/upper/lower entry/walls and earlier field order retained';return}
if($Baseline){if($VerifyReleaseBinary){throw 'Counterfactual cannot certify Release'};& "$PSScriptRoot/Test-SharasdalAscentCounterfactual.ps1";if(!$?){throw 'Pinned ascent baseline failed'};return}
if(!$DataDirectory){$DataDirectory="$repo/build-extractors/runtime-test/sharasdal-lower-ascent-data"}
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/sharasdal-lower-ascent"};$out=[IO.Path]::GetFullPath($OutputDirectory)
& "$PSScriptRoot/Test-SharasdalErunakUpper.ps1" -PrepareHost -OutputDirectory $out
if(!$?){throw 'Native current Erunak/quest/AI/function host preparation failed'}
[IO.File]::WriteAllText("$out/Actor.inc",$erunak+';')
Copy-Item "$repo/src/server/scripts/Scenario/Artifacts/Shaman/Sharasdal/sharasdal_lower_ascent.h" "$out/sharasdal_lower_ascent.h" -Force
$upper=Get-Content "$PSScriptRoot/SharasdalErunakUpperTest.cpp" -Raw;$boundary=$upper.IndexOf('int main(){');if($boundary -lt 0){throw 'Native shared upper host boundary changed'}
$hostPrefix=$upper.Substring(0,$boundary)
$hostPrefix=$hostPrefix.Replace(' static Player* GetPlayer(',' static Player* FindPlayer(Map* map,ObjectGuid id){for(auto p:players)if(p->guid==id&&p->world&&p->GetMap()==map)return p;return nullptr;} static Player* GetPlayer(')
[IO.File]::WriteAllText("$out/AscentHost.inc",$hostPrefix)
function Adapt([string]$s,[string]$a,[string]$b){if(!$s.Contains($a)){throw "Lower-ascent service boundary changed: $a"};$s.Replace($a,$b)}
$prelude=Get-Content "$out/EntryPrelude.inc" -Raw
$prelude=Adapt $prelude 'struct ObjectGuid{' 'struct ObjectGuid{static ObjectGuid const Empty;'
$prelude=Adapt $prelude 'struct Map{std::vector<PlayerRef> players;' 'struct Map{uint32 id=1600;uint32 GetId()const{return id;}uint32 GetInstanceId()const{return 7;}std::function<void(uint32)> onUpdate;void Update(uint32 diff){if(onUpdate)onUpdate(diff);}template<class...Args>void ScriptsStart(Args...){ } std::vector<PlayerRef> players;'
$prelude+="`ninline ObjectGuid const ObjectGuid::Empty{0,0};using SpellCastResult=uint32;enum{SPELL_CAST_OK=0,SPELL_FAILED_ERROR=1,INTERACTION_DISTANCE=5};`n"
[IO.File]::WriteAllText("$out/EntryPrelude.inc",$prelude)
$hostSource=Get-Content "$out/TimerHost.inc" -Raw
$hostSource=Adapt $hostSource 'struct Player;struct Creature;struct CreatureAI;' 'struct Player;struct Creature;struct CreatureAI;struct InstanceScript;'
$hostSource=Adapt $hostSource 'virtual ~Unit()=default;' 'InstanceScript* GetZoneScript(){return nullptr;}InstanceScript* GetInstanceScript(){return nullptr;}virtual ~Unit()=default;'
$hostSource=Adapt $hostSource 'struct Conversation;struct Player:Unit{' 'struct Conversation;struct Player:Unit{std::set<uint32> auras;uint32 cameraCalls=0,cameraResult=0;bool makeCamera=true,close=true;std::function<void()> onCamera;std::vector<uint32> ascentEvents;bool HasAura(uint32 id)const{return auras.count(id);}SpellCastResult CastSpell(Unit*,uint32,bool);void UpdateAchievementCriteria(uint32,uint32,uint64);'
$hostSource=Adapt $hostSource 'assert(range==20||range==100);return near;' 'assert(range==5||range==20||range==100);return near&&(range!=5||close);'
$hostSource=Adapt $hostSource 'struct InstanceScript{Map* instance=&liveMap;' 'struct InstanceScript{virtual ~InstanceScript()=default;virtual void SetGuidData(uint32,ObjectGuid);virtual ObjectGuid GetGuidData(uint32)const;void ProcessEvent(Unit*,uint32){}Map* instance=&liveMap;'
$hostSource=Adapt $hostSource 'uint32 id=0;uint32 GetPathId()const{return id;}' 'uint32 id=0,_currentNode=0;uint32 GetCurrentNode()const{return _currentNode;}uint32 GetPathId()const{return id;}'
$hostSource=Adapt $hostSource 'assert(id==10993811&&!loop' 'assert((id==10993810||id==10993811)&&!loop'
$hostSource=Adapt $hostSource 'float z=245,hp=100;' 'float x=0,y=0,z=245,hp=100;'
$hostSource=Adapt $hostSource 'float GetPositionX()const{return 0;}float GetPositionY()const{return 0;}' 'float GetPositionX()const{return x;}float GetPositionY()const{return y;}'
$hostSource=Adapt $hostSource ' uint32 entry=102826,' ' bool finalized=true,cosmeticWorks=true;struct Spline{Creature* owner;bool Finalized()const{return owner->finalized;}} spline{this};Spline* movespline=&spline;std::set<uint32> auras;bool HasAura(uint32 id)const{return auras.count(id);}SpellCastResult CastSpell(Creature* target,uint32 id,bool triggered){assert(target==this&&id==83810&&!triggered);casts.push_back(id);if(cosmeticWorks)auras.insert(id);return cosmeticWorks?SPELL_CAST_OK:SPELL_FAILED_ERROR;}uint32 m_waypointID=0;uint32 GetCurrentWaypointID(){return m_waypointID;}void UpdateWaypointID(uint32 id){m_waypointID=id;} uint32 entry=102826,'
$hostSource=Adapt $hostSource 'virtual void MovementInform(uint32,uint32){}' 'virtual void LastWPReached(){}virtual void MovementInform(uint32,uint32){}'
[IO.File]::WriteAllText("$out/TimerHost.inc",$hostSource)
$wpHeader=Get-Content "$repo/src/server/game/Movement/Waypoints/WaypointManager.h" -Raw
[IO.File]::WriteAllText("$out/WaypointData.inc",(Block $wpHeader 'enum WaypointMoveType')+';'+(Block $wpHeader 'struct WaypointData')+';using WaypointPath=std::vector<WaypointData>;')
$wp=Get-Content "$repo/src/server/game/Movement/MovementGenerators/WaypointMovementGenerator.cpp" -Raw
foreach($method in @('OnArrived','MovementInform')){[IO.File]::WriteAllText("$out/Waypoint$method.inc",(Block $wp "void WaypointMovementGenerator<Creature>::$method(").Replace("void WaypointMovementGenerator<Creature>::$method(","void NativeWaypoint::$method("))}
$map=Get-Content "$repo/src/server/game/Maps/Map.cpp" -Raw
[IO.File]::WriteAllText("$out/InstanceTick.inc",(Block $map 'void InstanceMap::Update('))
$effects=Get-Content "$repo/src/server/game/Spells/SpellEffects.cpp" -Raw
[IO.File]::WriteAllText("$out/NativeAscentEvent.inc",(Block $effects 'void Spell::EffectSendEvent(').Replace('void Spell::EffectSendEvent(','void NativeExitEvent::EffectSendEvent('))
if($PrepareHost){return}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /I`"$out`" `"$PSScriptRoot/SharasdalLowerAscentTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b 1`ncd /d `"$out`"`n`"$out/test.exe`" `"$DataDirectory/path.tsv`"`nexit /b %errorlevel%`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Actual lower-ascent controller/native movement/quest regression failed'}
if($VerifyReleaseBinary){if(!$ReleaseDirectory){$ReleaseDirectory="$repo/build-extractors/bin/Release"};$prior=$env:PYTHONPATH;try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$prior";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_sharasdal_lower_ascent_release.py" $ReleaseDirectory;if($LASTEXITCODE){throw 'Actual selected lower-ascent implementation missing'}}finally{$env:PYTHONPATH=$prior}}
