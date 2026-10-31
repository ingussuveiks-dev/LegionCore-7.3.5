param([switch]$PrepareHost,[switch]$Baseline,[switch]$ScopeOnly,[switch]$VerifyReleaseBinary,[string]$ReleaseDirectory='',[string]$OutputDirectory='')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
function Block([string]$s,[string]$start){$a=$s.IndexOf($start);if($a -lt 0){throw "Missing $start"};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth -and $b -lt $s.Length){$depth+=([int]($s[$b] -eq '{'))-([int]($s[$b] -eq '}'));++$b};if($depth){throw "Unclosed $start"};$s.Substring($a,$b-$a)}
$path='src/server/scripts/Scenario/Artifacts/Shaman/Sharasdal/Sharasdal.cpp';$source=(Get-Content "$repo/$path" -Raw).Replace("`r",'')
$newUpper=$source.Contains('#include "sharasdal_erunak_upper.h"')
if($newUpper){
 & "$PSScriptRoot/Test-SharasdalErunakUpper.ps1" -ScopeOnly
 if(!$?){throw 'Actual upper introduction certificate failed'}
 # Freeze only this older lower-entry matrix after the strict new scope.
 # The upper suite compiles the full current actor/both member headers.
 $source=(& git -C $repo show "b8f080c1f:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned retained lower-entry actor missing'}
}
$old=(& git -C $repo show "bf9292b2f:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned pre-entry source missing'}
$npc=Block $source 'class npc_erunak_stonespeaker :';$oldNPC=Block $old 'class npc_erunak_stonespeaker :'
$los=Block $npc 'void MoveInLineOfSight(';$oldLOS=Block $oldNPC 'void MoveInLineOfSight('
$upper=Block $oldLOS 'if (me->GetPositionZ() >= 795.0f';if((Block $los 'if (me->GetPositionZ() >= 795.0f') -cne $upper){throw 'Separate upper intro changed'}
$expected="void MoveInLineOfSight(Unit* who) override { if (!who || who->GetTypeId() != TYPEID_PLAYER) return; if (me->GetDBTableGUIDLow() == 362063) { AcceptLowerEntry(who->ToPlayer()); return; } "+$upper+' }'
if(($los -replace '\s','') -cne ($expected -replace '\s','')){throw 'Lower entry dispatch/upper retained scope changed'}
$normal=$npc.Replace($los,$oldLOS).Replace('        #include "sharasdal_erunak_entry.h"','').Replace('            CompleteLowerEntries(diff);','')
if(($normal -replace '\s','') -cne ($oldNPC -replace '\s','')){throw 'Other Erunak fields/lifecycle/wounds/combat/waypoints changed'}
if(($source.Replace($npc,$oldNPC).Replace('#include <map>','').Replace('#include "Group.h"','').Replace('#include "QuestData.h"','') -replace '\s','') -cne ($old -replace '\s','')){throw 'Other actors/bosses/walls/loot/registrations changed'}
if($ScopeOnly){'PASS only exact lower Erunak entry/acknowledged per-player105805 and owned opening; upper intro/waypoints/wounds/combat/other actors retained';return}
if($Baseline -and $VerifyReleaseBinary){throw 'Counterfactual cannot certify Release'}
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/sharasdal-erunak-entry$(if($Baseline){'-baseline'})"};$out=[IO.Path]::GetFullPath($OutputDirectory)
& "$PSScriptRoot/Test-SharasdalErunakTimers.ps1" -PrepareHost -OutputDirectory $out
if(!$?){throw 'Native Erunak timer host preparation failed'}
[IO.File]::WriteAllText("$out/Actor.inc",$(if($Baseline){$oldNPC}else{$npc})+';')
Copy-Item "$repo/src/server/scripts/Scenario/Artifacts/Shaman/Sharasdal/sharasdal_erunak_entry.h" "$out/sharasdal_erunak_entry.h" -Force
$playerSource=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
[IO.File]::WriteAllText("$out/Quest.inc",(Block $playerSource 'void Player::AddQuestDelayedEvent('))
[IO.File]::WriteAllText("$out/Objective.inc",(Block $playerSource 'int32 Player::GetQuestObjectiveData(uint32 QuestID, uint32 ObjectID) const')+(Block $playerSource 'int32 Player::GetQuestObjectiveData(Quest const* quest, int8 storageIndex) const'))
# The actual native objective mutation branch is compiled; achievement/network,
# quest-log enumeration and unrelated special quest handlers are controlled.
$credit=Block $playerSource 'void Player::KilledMonsterCredit('
$start=$credit.IndexOf('        if (q_status->Status == QUEST_STATUS_INCOMPLETE &&');if($start -lt 0){throw 'Native quest credit admission missing'}
$body=Block $credit.Substring($start) 'if (q_status->Status == QUEST_STATUS_INCOMPLETE &&'
[IO.File]::WriteAllText("$out/Credit.inc",$body)
$fixture=Get-Content "$PSScriptRoot/SharasdalErunakTimersTest.cpp" -Raw
$prefix=$fixture.Substring(0,$fixture.IndexOf('#include "Recovery.inc"'))
$unit=@'
struct Unit{
 virtual ~Unit()=default;virtual uint32 GetTypeId()const{return TYPEID_UNIT;}virtual Player* ToPlayer(){return nullptr;}
 bool world=true,alive=true,phase=true;Map* map=&liveMap;uint32 instanceId=7;ObjectGuid guid;
 ObjectGuid GetGUID()const{return guid;}Map* GetMap()const{return map;}uint32 GetInstanceId()const{return instanceId;}
 virtual bool IsInWorld()const{return world;}virtual bool IsAlive()const{return alive;}
 bool InSamePhase(Unit* target)const{return phase&&target->phase;}
}
'@
$player=@'
struct Player:Unit{
 QuestStatusData quest;bool record=true,contact=true,gm=false,teleport=false,near=true,los=true,detect=true,rewarded=false,reject=false;
 uint32 classId=7,calls=0;Group groupValue;Group* group=nullptr;FunctionProcessor functions;std::vector<uint32> credits;std::function<void()> onCredit;
 uint32 GetTypeId()const override{return TYPEID_PLAYER;}Player* ToPlayer()override{return this;}void CastSpell(Unit*,uint32){}
 bool CanContact()const{return contact;}bool isGameMaster()const{return gm;}bool IsBeingTeleported()const{return teleport;}uint32 getClass()const{return classId;}
 bool canSeeOrDetect(Unit*)const{return detect;}bool IsWithinLOSInMap(Unit*)const{return los;}
 bool IsWithinDistInMap(Unit*,float range)const{assert(range==20||range==100);return near;}
 bool GetQuestRewardStatus(uint32 id)const{assert(id==40341);return rewarded;}uint32 GetQuestStatus(uint32 id)const{assert(id==40341);return record?quest.Status:0;}
 QuestStatusData* getQuestStatus(uint32 id){assert(id==40341);return record?&quest:nullptr;}
 void AddQuestDelayedEvent(uint32,uint64,std::function<void()>&&);void AddDelayedEvent(uint64 d,std::function<void()>&& f){functions.AddDelayedEvent(d,std::move(f));}
 int32 GetQuestObjectiveData(uint32,uint32)const;int32 GetQuestObjectiveData(Quest const*,int8)const;
 Group* GetGroup(){return group;}void KilledMonsterCredit(uint32);
 void SetQuestObjectiveData(Quest const*,QuestObjective const* obj,int32 count){if(!reject)quest.ObjectiveData[obj->StorageIndex]=count;}
 void SendQuestUpdateAddCredit(Quest const*,ObjectGuid,QuestObjective const&,uint32){}bool CanCompleteQuest(uint32){return false;}
 void CompleteQuest(uint32){assert(false);}void SetQuestStatus(uint32,uint32){}
}
'@
$prefix=$prefix.Replace((Block $prefix 'struct Unit{'),$unit.Replace("`r",'')).Replace((Block $prefix 'struct Player:Unit{'),$player.Replace("`r",''))
$prefix=$prefix.Replace('CreatureAI* AI(){return i_AI;}', 'uint32 entry=102826,mapId=1600;uint64 db=362063;uint32 GetEntry()const{return entry;}uint64 GetDBTableGUIDLow()const{return db;}uint32 GetMapId()const{return mapId;}CreatureAI* AI(){return i_AI;}')
[IO.File]::WriteAllText("$out/TimerHost.inc",$prefix)
if($PrepareHost){return}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$mode=if($Baseline){'/DBASELINE'}else{''}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi $mode /I`"$out`" `"$PSScriptRoot/SharasdalErunakEntryTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b 1`ncd /d `"$out`"`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Actual lower Erunak entry regression failed'}
if($newUpper){'Retained62 lower-entry matrix uses pinned b8f080c1f after strict upper scope; full current actor/both headers/native clock are compiled in the upper suite'}
if($VerifyReleaseBinary){if(!$ReleaseDirectory){$ReleaseDirectory="$repo/build-extractors/bin/Release"};$previous=$env:PYTHONPATH;try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_sharasdal_erunak_entry_release.py" $ReleaseDirectory;if($LASTEXITCODE){throw 'Actual lower Erunak entry Release missing'}}finally{$env:PYTHONPATH=$previous}}
