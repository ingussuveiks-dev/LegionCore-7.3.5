param([switch]$ScopeOnly,[switch]$ExportOnly,[switch]$VerifyNativeCatalog,[string]$OutputDirectory='')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/monk-chen-rescue"};$out=[IO.Path]::GetFullPath($OutputDirectory)
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence/$(Get-Date -Format yyyyMMdd-HHmmss-fff)" -RuntimeDirectories @($repo,$out,"$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
 function Block([string]$s,[string]$start){$a=$s.IndexOf($start);if($a -lt 0){throw "Missing $start"};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth -and $b -lt $s.Length){$depth+=([int]($s[$b] -eq '{'))-([int]($s[$b] -eq '}'));++$b};if($depth){throw "Unclosed $start"};$s.Substring($a,$b-$a)}
 function Shape([string]$s){(($s -replace '(?s)/\*.*?\*/','') -replace '//[^\n]*','') -replace '\s',''}
 $path='src/server/scripts/Scenario/Artifacts/Monk/Intro/monk_intro_scenario.cpp';$source=(Get-Content "$repo/$path" -Raw).Replace("`r",'');$old=(& git -C $repo show "6b6fae36c:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned pre-rescue actor source missing'}
 $currentActor=Block $source 'class npc_monk_chen_porter :'
 $hasArrival=$source.Contains('bool _arrivalStarted = false;')
 if($hasArrival){
  & "$PSScriptRoot/Test-MonkChenArrival.ps1" -ScopeOnly
  $preArrival=(& git -C $repo show "b015d4e28:$path") -join "`n";if($LASTEXITCODE){throw 'Certified pre-arrival actor missing'}
  $source=$source.Replace($currentActor,(Block $preArrival 'class npc_monk_chen_porter :'))
 }
 $actor=Block $source 'class npc_monk_chen_porter :';$previous=Block $old 'class npc_monk_chen_porter :'
 $guard=Block $actor 'if (MonkChenRescue::Source(me))'
 $expectedGuard=@'
if (MonkChenRescue::Source(me))
                {
                    InstanceScript* script = me->GetInstanceScript();
                    if (!script || script->GetGuidData(MonkChenRescue::ClaimData) != me->GetGUID())
                    {
                        if (script) script->SetGuidData(MonkChenRescue::RequestData, me->GetGUID());
                        return;
                    }
                    if (_rescueStarted) return;
                    _rescueStarted = true;
                }
'@
 if((Shape $guard) -cne (Shape $expectedGuard)){throw 'Native source claim/consume-before-payload bridge changed'}
 $normal=$actor.Replace('bool _rescueStarted = false;','').Replace((Block $actor 'uint32 GetData('),'').Replace($guard,'').Replace('if (!MonkChenRescue::Source(me))','')
 if((Shape $normal) -cne (Shape $previous)){throw 'Other Chen callbacks/movement/text/combat/arrival/credit changed'}
 if((Shape $source.Replace($actor,$previous).Replace('#include "InstanceScript.h"','').Replace('#include "chen_rescue.h"','')) -cne (Shape $old)){throw 'Other Monk actors changed'}
 $ip='src/server/scripts/Scenario/Artifacts/Rogue/Dreadblades/instance_class_art_scenarios.cpp';$actual=(Get-Content "$repo/$ip" -Raw).Replace("`r",'');$prior=(& git -C $repo show "6b6fae36c:$ip") -join "`n";if($LASTEXITCODE){throw 'Pinned pre-rescue instances missing'}
 $currentInstance=Block $actual 'struct instance_monk_intro :'
 if($hasArrival){$preArrivalInstance=(& git -C $repo show "b015d4e28:$ip") -join "`n";if($LASTEXITCODE){throw 'Certified pre-arrival instance missing'};$actual=$actual.Replace($currentInstance,(Block $preArrivalInstance 'struct instance_monk_intro :'))}
 $instance=Block $actual 'struct instance_monk_intro :';$oldInstance=Block $prior 'struct instance_monk_intro :'
 if((Shape $actual.Replace($instance,$oldInstance).Replace('#include "../../Monk/Intro/chen_rescue.h"','')) -cne (Shape $prior)){throw 'Other shared Rogue/Mage/Priest instances/brewer changed'}
 if(!$instance.Contains('InstanceScript::OnCreatureCreate(actor);') -or !$instance.Contains('InstanceScript::GetGuidData(type)')){throw 'Native base create/GUID behavior must remain'}
 if($ScopeOnly){'PASS exact Chen rescue source/status/claim bridge only; all original nine timers/arrival/text/combat and other Monk/shared instance bodies retained';return}
 [IO.File]::WriteAllText("$out/Actor.inc",$currentActor+';');[IO.File]::WriteAllText("$out/BaselineActor.inc",$previous.Replace('npc_monk_chen_porter','npc_monk_chen_porter_baseline')+';');[IO.File]::WriteAllText("$out/Instance.inc",$currentInstance+';')
 Copy-Item "$repo/src/server/scripts/Scenario/Artifacts/Monk/Intro/chen_rescue.h" "$out/Rescue.inc"
 Copy-Item "$repo/src/server/scripts/Scenario/Artifacts/Monk/Intro/chen_arrival.h" "$out/chen_arrival.h"
 Copy-Item "$repo/src/server/scripts/Scenario/Artifacts/Monk/Intro/chen_routes.h" "$out/chen_routes.h"
 foreach($stub in @('WaypointMovementGenerator.h','MoveSpline.h','Timer.h','CreatureTextMgr.h')){[IO.File]::WriteAllText("$out/$stub","#pragma once`n// Controlled services declared explicitly in MonkChenRescueTest.cpp.`n")}
 $waypoints=Get-Content "$repo/src/server/game/Movement/Waypoints/WaypointManager.h" -Raw
 [IO.File]::WriteAllText("$out/WaypointFields.inc",(Block $waypoints 'enum WaypointMoveType')+';'+(Block $waypoints 'struct WaypointData')+';')
 [IO.File]::WriteAllText("$out/PathBase.inc",(Block (Get-Content "$repo/src/server/game/Movement/MovementGenerators/PathMovementBase.h" -Raw) 'template<class Entity, class BasePath>')+';')
 $wp=Get-Content "$repo/src/server/game/Movement/MovementGenerators/WaypointMovementGenerator.cpp" -Raw
 [IO.File]::WriteAllText("$out/WaypointArrival.inc",(Block $wp 'void WaypointMovementGenerator<Creature>::OnArrived(')+(Block $wp 'void WaypointMovementGenerator<Creature>::MovementInform('))
 [IO.File]::WriteAllText("$out/PathId.inc",(Block (Get-Content "$repo/src/server/game/Movement/MovementGenerators/WaypointMovementGenerator.h" -Raw) 'uint32 GetPathId() const'))
 $timer=Get-Content "$repo/src/common/Utilities/Timer.h" -Raw
 [IO.File]::WriteAllText("$out/Elapsed.inc",(Block $timer 'inline uint32 getMSTimeDiff(')+(Block $timer 'inline uint32 GetMSTimeDiffToNow('))
 $fh=Get-Content "$repo/src/common/Utilities/FunctionProcessor.h" -Raw;$fs=Get-Content "$repo/src/common/Utilities/FunctionProcessor.cpp" -Raw
 [IO.File]::WriteAllText("$out/Functions.inc",'typedef std::multimap<uint64, std::function<void()>> FunctionList;'+(Block $fh 'class TC_COMMON_API FunctionProcessor').Replace('TC_COMMON_API ','')+';'+$fs.Substring($fs.IndexOf('FunctionProcessor::FunctionProcessor()')))
 $unit=Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw
 [IO.File]::WriteAllText("$out/Owner.inc",(Block $unit 'Player* Unit::GetCharmerOrOwnerPlayerOrPlayerItself() const'))
 $kill=$unit.Substring($unit.IndexOf('// Call creature just died function'))
 [IO.File]::WriteAllText("$out/NativeDeath.inc",(Block $kill 'if (creature->IsAIEnabled)')+"`n"+(Block $kill 'if (InstanceScript* script = creature->GetInstanceScript())'))
 [IO.File]::WriteAllText("$out/QuestDelay.inc",(Block (Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw) 'void Player::AddQuestDelayedEvent('))
 [IO.File]::WriteAllText("$out/TalkForward.inc",(Block (Get-Content "$repo/src/server/game/AI/CreatureAI.cpp" -Raw) 'void CreatureAI::Talk(uint8 id, ObjectGuid WhisperGuid)'))
 [IO.File]::WriteAllText("$out/InstanceUpdate.inc",(Block (Get-Content "$repo/src/server/game/Maps/Map.cpp" -Raw) 'void InstanceMap::Update('))
 [IO.File]::WriteAllText("$out/CanCriteria.inc",(Block (Get-Content "$repo/src/server/game/Scenarios/Scenario.cpp" -Raw) 'bool Scenario::CanUpdateCriteria(').Replace('uint32 recursTree /*=0*/','uint32 recursTree'))
 [IO.File]::WriteAllText("$out/DeathState.inc",(Block (Get-Content "$repo/src/server/game/Entities/Unit/Unit.h" -Raw) 'enum DeathState')+';')
 $cs=Get-Content "$repo/src/server/game/Entities/Creature/Creature.cpp" -Raw
 foreach($pair in @(@('Lifecycle','if (i_AI || i_disabledAI)'),@('AIDestroy','bool Creature::AIM_Destroy('))){[IO.File]::WriteAllText("$out/$($pair[0]).inc",(Block $cs $pair[1]))}
 if($ExportOnly){return}
 if($VerifyNativeCatalog){
  $prior=$env:LEGION_DIALOGUE_CONFIG
  Push-Location $out
  try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile MonkIntroCatalog.cpp -ProbeName MonkIntroCatalog -OutputFile "$out/native.tsv" *> "$out/catalog.log";if(!$? -or $LASTEXITCODE){throw 'Fresh native943 catalog failed'}}finally{$env:LEGION_DIALOGUE_CONFIG=$prior;Pop-Location}
  $native=Get-Content "$out/native.tsv"
  $contracts=@(@(1887,45900,45901,29068,46868),@(1896,45841,45842,29046,46734),@(1897,45902,45903,29069,46902),@(1898,45904,45905,29047,46956),@(1899,45906,45907,29070,46947),@(1900,45908,45909,29071,46951),@(1901,45910,45911,29072,46952))
  foreach($c in $contracts){foreach($expected in @("TREE`t$($c[0])`t$($c[2])`t$($c[1])`t0`t$(if($c[0] -le 1896){0}else{1})`t$($c[3])","CRITERION`t$($c[0])`t$($c[3])`t92`t$($c[4])`t0`t0")){if($native -notcontains $expected){throw 'Matching native943 tree/leaf/event changed'}}}
  if($native -notcontains "MAP_DIFFICULTY`t3448`t12`t1"){throw 'Private native943 admission changed'}
 }
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /I`"$out`" `"$PSScriptRoot/MonkChenRescueTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b 1`ncd /d `"$out`"`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
 & "$out/run.cmd";if($LASTEXITCODE){throw 'Actual Chen rescue ASAN regression failed'}
}
