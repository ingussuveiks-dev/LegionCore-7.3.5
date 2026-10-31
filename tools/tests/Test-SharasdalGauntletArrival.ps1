param([switch]$Baseline,[switch]$ScopeOnly,[switch]$VerifyReleaseBinary,[string]$ReleaseDirectory='',[string]$OutputDirectory='')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
function Block([string]$s,[string]$start){$a=$s.IndexOf($start);if($a -lt 0){throw "Missing $start"};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth -and $b -lt $s.Length){$depth+=([int]($s[$b] -eq '{'))-([int]($s[$b] -eq '}'));++$b};if($depth){throw "Unclosed $start"};$s.Substring($a,$b-$a)}
function Adapt([string]$s,[string]$a,[string]$b){if(!$s.Contains($a)){throw "Gauntlet boundary changed: $a"};$s.Replace($a,$b)}
$path='src/server/scripts/Scenario/Artifacts/Shaman/Sharasdal/Sharasdal.cpp';$ipath='src/server/scripts/Scenario/Artifacts/Shaman/Sharasdal/instance_sharasdal.cpp'
$source=(Get-Content "$repo/$path" -Raw).Replace("`r",'');$instance=(Get-Content "$repo/$ipath" -Raw).Replace("`r",'')
$comparison=$source
$comparisonInstance=$instance
if($source.Contains('#include "sharasdal_erunak_upper.h"')){
 & "$PSScriptRoot/Test-SharasdalErunakUpper.ps1" -ScopeOnly
 if(!$?){throw 'Current upper intro source certificate failed'}
 $comparisonInstance=(& git -C $repo show "b8f080c1f:$ipath") -join "`n";if($LASTEXITCODE){throw 'Pinned pre-upper comparison missing'}
 foreach($header in @('Conversation.h','ObjectMgr.h','SpellMgr.h')){$comparison=$comparison.Replace("#include `"$header`"",'')}
}
if($source.Contains('// AI-owned intro timers')){
 & "$PSScriptRoot/Test-SharasdalErunakTimers.ps1" -ScopeOnly
 if(!$?){throw 'Actual Erunak timer certificate failed'}
 $beforeTimers=(& git -C $repo show "c3dd68ad5:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned pre-timer comparison missing'}
 # Only the unrelated comparison is frozen; the new Erunak suite compiles its
 # full current actor and native lifecycle/queue/destruction implementation.
 $comparison=$comparison.Replace((Block $source 'class npc_erunak_stonespeaker :'),(Block $beforeTimers 'class npc_erunak_stonespeaker :'))
 if($source.Contains('#include "sharasdal_erunak_entry.h"')){$comparison=$comparison.Replace('#include <map>','').Replace('#include "Group.h"','').Replace('#include "QuestData.h"','')}
}
$old=(& git -C $repo show "ef0bbcbbf:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned pre-gauntlet actor missing'}
$oldInstance=(& git -C $repo show "ef0bbcbbf:$ipath") -join "`n";if($LASTEXITCODE){throw 'Pinned pre-gauntlet instance missing'}
$npc=Block $source 'class npc_lady_zitrin :';$oldNPC=Block $old 'class npc_lady_zitrin :'
$normal=$npc.Replace('        #include "sharasdal_gauntlet_arrival.h"','')
$ctor=Block $normal 'npc_lady_zitrinAI(Creature* creature)';$oldCtor=Block $oldNPC 'npc_lady_zitrinAI(Creature* creature)'
$oldBranch="            if (me->GetPositionX() <= -15.0f)`n                intro = true;`n            else`n                intro = false;"
if(((Adapt $ctor '            intro = IsGauntletIllusion();' $oldBranch) -replace '\s','') -cne ($oldCtor -replace '\s','')){throw 'Other constructor defaults changed'}
$normal=$normal.Replace($ctor,$oldCtor)
$los=Block $normal 'void MoveInLineOfSight('
if(($los -replace '\s','') -cne 'voidMoveInLineOfSight(Unit*who)override{AcceptGauntletArrival(who?who->ToPlayer():nullptr);}'){throw 'Actual player-only gauntlet admission changed'}
$normal=$normal.Replace($los,(Block $oldNPC 'void MoveInLineOfSight('))
$normal=Adapt $normal "            Scenario* gauntlet = GauntletWorld();`n            if (intro && gauntlet && gauntlet->GetCurrentStep() == 3)" '            if (intro)'
if(($normal -replace '\s','') -cne ($oldNPC -replace '\s','')){throw 'Other Lady death/combat/timers/wave spell/pose/summon fields changed'}
$normal=$comparison.Replace($npc,$oldNPC).Replace('#include "Map.h"','')
if(($normal -replace '\s','') -cne ($old -replace '\s','')){throw 'Other companions/Grash walls/Kraliss/acquisition/spells/registrations changed'}
$normal=$comparisonInstance.Replace('        ObjectGuid gauntletIllusion;','').Replace((Block $instance 'void RearmGauntlet('),'').Replace('            if (player) RearmGauntlet(player->GetGUID(), 40341);','').Replace('            if (creature->GetEntry() == 104856 && creature->GetDBTableGUIDLow() == 362091) gauntletIllusion = creature->GetGUID();','')
$added=@'
            if (creature->GetEntry() == 105298 && creature->GetDBTableGUIDLow() == 362092)
            {
                upperGrash = creature->GetGUID();
                RearmGauntlet(upperGrash, 105298);
            }
'@
$normal=Adapt $normal $added.Replace("`r",'') '            if (creature->GetEntry() == 105298 && creature->GetDBTableGUIDLow() == 362092) upperGrash = creature->GetGUID();'
if(($normal -replace '\s','') -cne ($oldInstance -replace '\s','')){throw 'Other instance fields/deaths/credit/entry changed'}
if($ScopeOnly){'PASS only exact illusion362091 gauntlet arrival/native receipt and upper362092 presentation/re-entry; other Lady combat/death/waves payload, Grash walls, native loot and acquisition retained';return}
if($Baseline -and $VerifyReleaseBinary){throw 'Counterfactual cannot certify repaired Release'}
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/sharasdal-gauntlet-arrival$(if($Baseline){'-baseline'})"};$out=[IO.Path]::GetFullPath($OutputDirectory)
& "$PSScriptRoot/Test-SharasdalLady.ps1" -PrepareHost -OutputDirectory $out
if(!$?){throw 'Actual retained Lady native host generation failed'}
[IO.File]::WriteAllText("$out/Actor.inc",$(if($Baseline){$oldNPC}else{$npc})+';')
[IO.File]::WriteAllText("$out/RearmGauntlet.inc",(Block $instance 'void RearmGauntlet('))
# The test's native services are augmented below; the production class and
# header above are compiled as a whole, including all retained combat methods.
$hostSource=Get-Content "$out/NativeHost.inc" -Raw
$hostSource=Adapt $hostSource 'virtual uint32 GetData(uint32)const{return 0;}' 'bool m_MoveInLineOfSight_locked=false;void MoveInLineOfSight_Safe(Unit*);std::vector<int32> actions;std::function<void(int32)> onAction;virtual uint32 GetData(uint32)const{return 0;}'
$hostSource=Adapt $hostSource 'virtual void DoAction(int32){}' 'virtual void DoAction(int32 action){actions.push_back(action);if(onAction)onAction(action);}'
$hostSource=Adapt $hostSource 'void CastSpell(Unit*,uint32){}' 'std::vector<uint32> casts;void CastSpell(Unit*,uint32 id){casts.push_back(id);}'
$hostSource=Adapt $hostSource 'float z=245;' 'float x=-21.1371f,z=807.444f;'
$hostSource=Adapt $hostSource 'float GetPositionX()const{return 0;}' 'float GetPositionX()const{return x;}'
$hostSource=Adapt $hostSource 'Creature* SummonCreature(uint32,float,float,float,float){return nullptr;}void DespawnOrUnsummon(uint32){}bool IsWithinDistInMap(Unit*,float)const{return true;}' 'Creature* wave=nullptr;uint32 summons=0,despawns=0,despawnDelay=0;bool arrivalNear=true;Creature* SummonCreature(uint32 id,float,float,float,float){assert(id==105251);++summons;return wave;}void DespawnOrUnsummon(uint32 delay){++despawns;despawnDelay=delay;}bool IsWithinDistInMap(Unit*,float range)const{assert(range==40);return arrivalNear;}'
$criteria=Block $hostSource 'void Player::UpdateAchievementCriteria('
$hostSource=$hostSource.Replace($criteria,@'
void Player::UpdateAchievementCriteria(uint32 type,uint32 asset){
 assert(type==92);++credits[asset];uint32 leaf=asset==50314?47378:asset==50397?47380:asset==49819?47133:0;assert(leaf);
 if(scenario.id==1066&&!scenario.rejected.count(asset)&&scenario.CanUpdateCriteria(leaf)){
  auto& count=scenario.achievements.progress[leaf].Counter;if(count<1)++count;
  if(scenario.step==3&&scenario.achievements.progress[47131].Counter&&scenario.achievements.progress[47378].Counter){scenario.step=4;scenario.currentTree=47132;}
 }
 if(onCredit)onCredit(asset);
}
'@)
$safe=Block (Get-Content "$repo/src/server/game/AI/CreatureAI.cpp" -Raw) 'void CreatureAI::MoveInLineOfSight_Safe('
[IO.File]::WriteAllText("$out/NativeHost.inc",$hostSource+"`n"+$safe)
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$mode=if($Baseline){'/DBASELINE'}else{''}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi $mode /I`"$out`" `"$PSScriptRoot/SharasdalGauntletArrivalTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Actual Sharasdal gauntlet arrival regression failed'}
if($VerifyReleaseBinary){if(!$ReleaseDirectory){$ReleaseDirectory="$repo/build-extractors/bin/Release"};$previous=$env:PYTHONPATH;try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_sharasdal_gauntlet_arrival_release.py" $ReleaseDirectory;if($LASTEXITCODE){throw 'Actual gauntlet arrival Release implementation missing'}}finally{$env:PYTHONPATH=$previous}}
