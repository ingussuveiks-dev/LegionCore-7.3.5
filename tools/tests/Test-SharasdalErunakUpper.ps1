param([switch]$Baseline,[switch]$ScopeOnly,[switch]$VerifyReleaseBinary,[string]$ReleaseDirectory='',[string]$OutputDirectory='')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
function Block([string]$s,[string]$start){$a=$s.IndexOf($start);if($a -lt 0){throw "Missing $start"};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth -and $b -lt $s.Length){$depth+=([int]($s[$b] -eq '{'))-([int]($s[$b] -eq '}'));++$b};if($depth){throw "Unclosed $start"};$s.Substring($a,$b-$a)}
$path='src/server/scripts/Scenario/Artifacts/Shaman/Sharasdal/Sharasdal.cpp';$ipath='src/server/scripts/Scenario/Artifacts/Shaman/Sharasdal/instance_sharasdal.cpp'
$source=(Get-Content "$repo/$path" -Raw).Replace("`r",'');$instance=(Get-Content "$repo/$ipath" -Raw).Replace("`r",'')
$old=(& git -C $repo show "b8f080c1f:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned pre-upper actor missing'}
$oldInstance=(& git -C $repo show "b8f080c1f:$ipath") -join "`n";if($LASTEXITCODE){throw 'Pinned pre-upper instance missing'}
$npc=Block $source 'class npc_erunak_stonespeaker :';$oldNPC=Block $old 'class npc_erunak_stonespeaker :'
$los=Block $npc 'void MoveInLineOfSight('
$expected='void MoveInLineOfSight(Unit* who) override { if (!who || who->GetTypeId() != TYPEID_PLAYER) return; if (me->GetDBTableGUIDLow() == 362063) { AcceptLowerEntry(who->ToPlayer()); return; } AcceptUpperIntro(who->ToPlayer()); }'
if(($los -replace '\s','') -cne ($expected -replace '\s','')){throw 'Upper/lower player dispatch changed'}
$normal=$npc.Replace($los,(Block $oldNPC 'void MoveInLineOfSight(')).Replace('        #include "sharasdal_erunak_upper.h"','').Replace('            CompleteUpperIntro(diff);','').Replace("            ProbeErunakArrival(); // New work starts after this tick's clocks.",'')
if(($normal -replace '\s','') -cne ($oldNPC -replace '\s','')){throw 'Other lower entry/wounds/combat/waypoints/timer fields changed'}
$normal=$source.Replace($npc,$oldNPC)
foreach($header in @('Conversation.h','ObjectMgr.h','SpellMgr.h')){$normal=$normal.Replace("#include `"$header`"",'')}
if(($normal -replace '\s','') -cne ($old -replace '\s','')){throw 'Other actors/bosses/walls/loot/registrations changed'}
$lowerPath='src/server/scripts/Scenario/Artifacts/Shaman/Sharasdal/sharasdal_erunak_entry.h';$lower=(Get-Content "$repo/$lowerPath" -Raw).Replace("`r",'');$oldLower=(& git -C $repo show "b8f080c1f:$lowerPath") -join "`n";if($LASTEXITCODE -or ($lower -replace '\s','') -cne ($oldLower -replace '\s','')){throw 'Actual lower entry member policy changed'}
$normal=$instance.Replace('        ObjectGuid upperErunak;','').Replace((Block $instance 'void RearmErunak('),'').Replace('            if (player) RearmErunak(player->GetGUID());','').Replace('            if (creature->GetEntry() == 102826 && creature->GetDBTableGUIDLow() == 362093) upperErunak = creature->GetGUID();','')
if(($normal -replace '\s','') -cne ($oldInstance -replace '\s','')){throw 'Other instance fields/deaths/criteria/entry/create changed'}
if($ScopeOnly){'PASS only exact upper Erunak original caller/native ascent/acknowledged approach+conversation and native original-player re-entry; lower entry/header/waypoints/wounds/other actors/deaths/progress retained';return}
if($Baseline -and $VerifyReleaseBinary){throw 'Counterfactual cannot certify Release'}
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/sharasdal-erunak-upper$(if($Baseline){'-baseline'})"};$out=[IO.Path]::GetFullPath($OutputDirectory)
& "$PSScriptRoot/Test-SharasdalErunakEntry.ps1" -PrepareHost -OutputDirectory $out
if(!$?){throw 'Native lower entry host preparation failed'}
[IO.File]::WriteAllText("$out/Actor.inc",$(if($Baseline){$oldNPC}else{$npc})+';')
Copy-Item "$repo/src/server/scripts/Scenario/Artifacts/Shaman/Sharasdal/sharasdal_erunak_upper.h" "$out/sharasdal_erunak_upper.h" -Force
[IO.File]::WriteAllText("$out/RearmUpper.inc",(Block $instance 'void RearmErunak(').Replace('void RearmErunak(','void InstanceScript::RearmErunak('))
[IO.File]::WriteAllText("$out/Receipt.inc",(Block (Block $source 'namespace SharasdalGrash') 'bool Receipt('))
[IO.File]::WriteAllText("$out/Path.inc",(Block (Block $source 'namespace SharasdalWall') 'uint32 Path('))
& "$PSScriptRoot/Test-SharasdalFinaleVoice.ps1" -PrepareHost -OutputDirectory $out
if(!$?){throw 'Native conversation host preparation failed'}
$voice=Get-Content "$PSScriptRoot/SharasdalFinaleVoiceTest.cpp" -Raw
[IO.File]::WriteAllText("$out/VoiceHost.inc",$voice.Substring(0,$voice.IndexOf('static void Load(')))
$entry=Get-Content "$PSScriptRoot/SharasdalErunakEntryTest.cpp" -Raw
$prelude=$entry.Substring(0,$entry.IndexOf('#include "TimerHost.inc"'))
$prelude=$prelude.Replace('bool operator<(ObjectGuid b)const', 'using LowType=uint64;uint64 GetLowPart()const{return low;}uint64 GetHighPart()const{return high;}void Clear(){high=low=0;}bool operator!=(ObjectGuid b)const{return !(*this==b);}bool operator<(ObjectGuid b)const')
$prelude=$prelude.Replace('struct Map{uint32 GetDifficultyID()const{return 0;}}','struct Player;struct PlayerRef{Player* value;Player* getSource()const{return value;}};struct Creature;struct Map{std::vector<PlayerRef> players;auto const& GetPlayers()const{return players;}Creature* GetCreature(ObjectGuid const&);uint32 GetDifficultyID()const{return 0;}}')
$prelude=$prelude.Replace('struct Scenario{','struct Progress{uint64 Counter=0;};struct Achievements{std::map<uint32,Progress> progress;auto GetCriteriaProgressMap()const{return &progress;}};struct Scenario{Achievements achievements;auto& GetAchievementMgr(){return achievements;}')
[IO.File]::WriteAllText("$out/EntryPrelude.inc",$prelude)
function Adapt([string]$s,[string]$a,[string]$b){if(!$s.Contains($a)){throw "Upper host boundary changed: $a"};$s.Replace($a,$b)}
$hostSource=(Get-Content "$out/TimerHost.inc" -Raw).Replace("`r",'')
$hostSource=Adapt $hostSource 'virtual Player* ToPlayer(){return nullptr;}' 'virtual Player* ToPlayer(){return nullptr;}bool IsPlayer()const{return GetTypeId()==TYPEID_PLAYER;}virtual float GetVisibilityRange(){return 100;}virtual Creature* FindNearestCreature(uint32,float,bool=true){return nullptr;}'
$hostSource=Adapt $hostSource 'struct Player:Unit{' 'struct Conversation;struct Player:Unit{uint32 binds=0;Creature* FindNearestCreature(uint32,float,bool=true)override;void BindQuestObject(uint32,Conversation*);'
$hostSource=Adapt $hostSource 'struct InstanceScript{uint32 step=0;' 'struct InstanceScript{Map* instance=&liveMap;ObjectGuid lowerErunak{0,0},upperErunak{0,0};void RearmErunak(ObjectGuid const&);uint32 step=0;'
$motion=@'
enum{MOTION_SLOT_IDLE=0,UNIT_STATE_ROOT=4};
struct MovementGenerator{uint32 type=WAYPOINT_MOTION_TYPE;virtual ~MovementGenerator()=default;uint32 GetMovementGeneratorType()const{return type;}};
template<class>struct WaypointMovementGenerator:MovementGenerator{uint32 id=0;uint32 GetPathId()const{return id;}};
struct Motion{uint32 paths=0;bool accept=true;WaypointMovementGenerator<Creature> path;MovementGenerator* GetMotionSlot(uint32 slot){assert(slot==MOTION_SLOT_IDLE);return path.id?&path:nullptr;}void MovePath(uint32 id,bool loop,int32 x=0,int32 y=0){assert(id==10993811&&!loop&&x>=-5&&x<=5&&y>=-5&&y<=5);++paths;if(accept){path.id=id;path.type=WAYPOINT_MOTION_TYPE;}}}
'@
$hostSource=$hostSource.Replace((Block $hostSource 'struct Motion{'),$motion.Replace("`r",''))
$hostSource=Adapt $hostSource 'CreatureAI* AI(){return i_AI;}' 'CreatureAI* GetDisabledAI(){return i_disabledAI;}CreatureAI* AI(){return i_AI;}'
$hostSource=Adapt $hostSource 'return id==105298?grash:' 'return id==102826?this:id==105298?grash:'
$hostSource=Adapt $hostSource 'void CreateConversation(uint32 id){conversations.push_back(id);if(onPresentation)onPresentation();}' 'void CreateConversation(uint32 id);'
$hostSource=Adapt $hostSource 'virtual void Reset(){}' 'uint32 guidCalls=0;virtual void SetGUID(ObjectGuid const&,int32=0){++guidCalls;}virtual void Reset(){}'
[IO.File]::WriteAllText("$out/TimerHost.inc",$hostSource)
# Host extensions and the full current policy are exercised by the native
# regression below. Actual movement/network/registry services stay controlled.
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$mode=if($Baseline){'/DBASELINE'}else{''}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi $mode /I`"$out`" `"$PSScriptRoot/SharasdalErunakUpperTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b 1`ncd /d `"$out`"`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Actual upper Erunak introduction regression failed'}
if($VerifyReleaseBinary){if(!$ReleaseDirectory){$ReleaseDirectory="$repo/build-extractors/bin/Release"};$prior=$env:PYTHONPATH;try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$prior";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_sharasdal_erunak_upper_release.py" $ReleaseDirectory;if($LASTEXITCODE){throw 'Actual upper Erunak implementation missing'}}finally{$env:PYTHONPATH=$prior}}
