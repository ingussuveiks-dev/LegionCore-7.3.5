param([switch]$Baseline,[switch]$FinalBaseline,[switch]$FinalCastBaseline,[switch]$DiscoveryBaseline,[switch]$ActionBaseline,[switch]$ArrivalBaseline,[switch]$WaypointBaseline,[switch]$WaypointRetryBaseline,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
if(@($Baseline,$FinalBaseline,$FinalCastBaseline,$DiscoveryBaseline,$ActionBaseline,$ArrivalBaseline,$WaypointBaseline,$WaypointRetryBaseline|Where-Object {$_}).Count -gt 1 -or (($Baseline -or $FinalBaseline -or $FinalCastBaseline -or $DiscoveryBaseline -or $ActionBaseline -or $ArrivalBaseline -or $WaypointBaseline -or $WaypointRetryBaseline) -and $VerifyReleaseBinary)){throw 'Pinned baselines are mutually exclusive and cannot certify fixed Release'}
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/silver-hand-ritual-dispatch"
New-Item -ItemType Directory -Force $out|Out-Null
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};$source.Substring($first,$last-$first)}
$path='src/server/scripts/Scenario/Artifacts/Paladin/SilverHand/instance_silver_hand.cpp'
$source=Get-Content "$repo/$path" -Raw
$pinned=(& git -C $repo show "e01a2c64:$path") -join "`n"
if($LASTEXITCODE){throw 'Pinned instance baseline unavailable'}
if($FinalBaseline){$source=(& git -C $repo show "7680556f:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned final-stage baseline unavailable'}}
elseif($FinalCastBaseline){$source=(& git -C $repo show "8a7c56b5:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned final-cast baseline unavailable'}}
elseif($DiscoveryBaseline){$source=(& git -C $repo show "52cd603c:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned discovery baseline unavailable'}}
elseif($Baseline){$source=$pinned}else{
 $discoveryPinned=(& git -C $repo show "52cd603c:$path") -join "`n"
 if($LASTEXITCODE){throw 'Pinned discovery preservation source unavailable'}
 $discovery=Block $source 'void QueueFinaleDiscovery('
 $normalized=$source.Replace($discovery,'').Replace('bool _finaleDiscoveryPending = false;','').Replace('_finaleActorsStarted','_blindingLightStarted')
 $normalized=$normalized -replace '\{\s+QueueFinaleDiscovery\(\);\s+return;\s+\}','return;'
 $normalized=$normalized -replace 'if \(!travardFound \|\| !mordothFound\)\s+QueueFinaleDiscovery\(\);',''
 $speechPattern=[regex]::Escape('if (_blindingLightStarted.insert(travar->GetGUID()).second)')+'\s+travar->AI\(\)->Talk\(3\);'
 $normalized=$normalized -replace $speechPattern,'travar->AI()->Talk(3);'
 if(($normalized -replace '\s','') -cne ($discoveryPinned -replace '\s','')){throw 'Unrelated instance change beyond scoped finale discovery/speech admission'}
 $comparisonSource=$source
 $source=$normalized
 $helper=Block $source 'void CastBlindingLight('
 $castPinned=(& git -C $repo show "8a7c56b5:$path") -join "`n"
 if($LASTEXITCODE){throw 'Pinned final-cast preservation source unavailable'}
 $castPattern=[regex]::Escape('if (_blindingLightStarted.insert(boss->GetGUID()).second)')+'\s+CastBlindingLight\(boss\);'
 $oldCast="boss->CastSpell(boss, 215260); boss->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_IMMUNE_TO_PC| UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_NOT_ATTACKABLE_1);"
 $restored=$source.Replace($helper,'').Replace('GuidSet _blindingLightStarted;','').Replace('#include "Scenario.h"','').Replace('#include "ScenarioMgr.h"','') -replace $castPattern,$oldCast
 if(($restored -replace '\s','') -cne ($castPinned -replace '\s','')){throw 'Unrelated instance change beyond scoped final-cast recovery'}
 $original=Block $pinned 'void onScenarionNextStep('
 $current=Block $restored 'void onScenarionNextStep('
 $before='if (Creature* travar = player->FindNearestCreature(106676, 70.0f, true)) travar->AI()->DoAction(true); break;'
 $after='if (Creature* travar = player->FindNearestCreature(106676, 70.0f, true)) { travar->AI()->DoAction(true); break; }'
 $oldFinal=Block $original 'if (newStep == 10)'
 $newFinal=Block $current 'if (newStep == 10)'
 if(($current.Replace($newFinal,$oldFinal) -replace '\s','') -cne (($original -replace '\s','').Replace(($before -replace '\s',''),($after -replace '\s','')))){throw 'Unrelated step8 callback behavior changed'}
 if(($restored.Replace($current,$original) -replace '\s','') -cne ($pinned -replace '\s','')){throw 'Unrelated instance script behavior changed'}
 $source=$comparisonSource
}
[IO.File]::WriteAllText("$out/Dispatch.inc",(Block $source 'void onScenarionNextStep(').Replace(' override',''))
[IO.File]::WriteAllText("$out/FinalCast.inc",$(if($source.Contains('void CastBlindingLight(')){('GuidSet '+$(if($DiscoveryBaseline){'_blindingLightStarted'}else{'_finaleActorsStarted'})+';')+(Block $source 'void CastBlindingLight(')}else{''}))
[IO.File]::WriteAllText("$out/FinalDiscovery.inc",$(if($source.Contains('void QueueFinaleDiscovery(')){'bool _finaleDiscoveryPending = false;'+(Block $source 'void QueueFinaleDiscovery(')}else{''}))
$instanceCore=Get-Content "$repo/src/server/game/Instances/InstanceScript.cpp" -Raw
[IO.File]::WriteAllText("$out/InstanceDelay.inc",(Block $instanceCore 'void InstanceScript::AddDelayedEvent(').Replace('InstanceScript::','Instance::'))
if(!(Block $instanceCore 'void InstanceScript::UpdateForScript(').Contains('m_Functions.Update(diff);')){throw 'Native instance no longer dispatches its owned queue'}
$spellCore=Get-Content "$repo/src/server/game/Spells/Spell.cpp" -Raw
[IO.File]::WriteAllText("$out/NativeCastGate.inc",(Block $spellCore 'if (!(_triggeredCastFlags & TRIGGERED_IGNORE_CAST_IN_PROGRESS) && m_caster->IsNonMeleeSpellCast(false, true, true, m_spellInfo->Id == 75))'))
$definitions=Get-Content "$repo/src/server/game/Miscellaneous/SharedDefines.h" -Raw
if($definitions -notmatch 'SPELL_FAILED_SUCCESS\s*=\s*0,' -or $definitions -notmatch 'SPELL_FAILED_SPELL_IN_PROGRESS\s*=\s*123,' -or $definitions -notmatch 'SPELL_CAST_OK\s*=\s*SPELL_FAILED_SUCCESS'){throw 'Native cast result values changed'}
if((Get-Content "$repo/src/server/game/Entities/Unit/Unit.h" -Raw) -notmatch 'TRIGGERED_IGNORE_CAST_IN_PROGRESS\s*=\s*0x00000020,'){throw 'Native cast-in-progress trigger flag changed'}
$actorSource=Get-Content "$repo/src/server/scripts/Scenario/Artifacts/Paladin/SilverHand/SilverHand.cpp" -Raw
if($ActionBaseline){$actorSource=(& git -C $repo show '41ce375b:src/server/scripts/Scenario/Artifacts/Paladin/SilverHand/SilverHand.cpp') -join "`n";if($LASTEXITCODE){throw 'Pinned ritual action baseline unavailable'}}
if($ArrivalBaseline){$actorSource=(& git -C $repo show '14992461:src/server/scripts/Scenario/Artifacts/Paladin/SilverHand/SilverHand.cpp') -join "`n";if($LASTEXITCODE){throw 'Pinned ritual arrival baseline unavailable'}}
if($WaypointBaseline){$actorSource=(& git -C $repo show 'c9e683fa:src/server/scripts/Scenario/Artifacts/Paladin/SilverHand/SilverHand.cpp') -join "`n";if($LASTEXITCODE){throw 'Pinned waypoint recipient baseline unavailable'}}
if($WaypointRetryBaseline){$actorSource=(& git -C $repo show '60279f2b:src/server/scripts/Scenario/Artifacts/Paladin/SilverHand/SilverHand.cpp') -join "`n";if($LASTEXITCODE){throw 'Pinned waypoint retry baseline unavailable'}}
$actor=Block $actorSource 'struct npc_travar_thirdAI : ScriptedAI'
[IO.File]::WriteAllText("$out/WaypointComplete.inc",$(if($ActionBaseline -or $ArrivalBaseline -or $WaypointBaseline -or $WaypointRetryBaseline){''}else{Block $actor 'void CompletePrisonWaypoints('}))
[IO.File]::WriteAllText("$out/WaypointPolicy.inc",$(if($ActionBaseline -or $ArrivalBaseline -or $WaypointBaseline){''}else{Block $actorSource 'namespace SilverHandRitualWaypoint'}))
$comparisonActor=$actor
if(!$ActionBaseline -and !$ArrivalBaseline -and !$WaypointBaseline -and !$WaypointRetryBaseline){
 $retryPinned=(& git -C $repo show '60279f2b:src/server/scripts/Scenario/Artifacts/Paladin/SilverHand/SilverHand.cpp') -join "`n"
 if($LASTEXITCODE){throw 'Pinned retry preservation source missing'}
 $beforeRetry=Block $retryPinned 'struct npc_travar_thirdAI : ScriptedAI'
 $movement=Block $actor 'void MovementInform('
 $oldMovement=Block $beforeRetry 'void MovementInform('
 $restoredMovement=$movement
 foreach($pair in @(@('9','5','5','_mawWaypointArrived','_mawWaypointCredited'),@('19','6','6','_prisonWaypointArrived','_prisonWaypointCredited'))){
  $expected="if (id == $($pair[0])) { bool firstArrival = !$($pair[3]); $($pair[3]) = true; if (firstArrival) Talk($($pair[1])); CompletePrisonWaypoints(); if (firstArrival && !$($pair[4])) TC_LOG_WARN(`"scripts`", `"Silver Hand waypoint$($pair[0]) has no eligible stage$($pair[2]) participant; retaining observed arrival for credit retry`"); }"
  $branch=Block $movement "if (id == $($pair[0]))"
  if(($branch -replace '\s','') -cne ($expected -replace '\s','')){throw 'Waypoint observation/speech/diagnostic scope changed'}
  $restoredMovement=$restoredMovement.Replace($branch,(Block $oldMovement "if (id == $($pair[0]))"))
 }
 if(($restoredMovement -replace '\s','') -cne ($oldMovement -replace '\s','')){throw 'Unrelated third movement behavior changed'}
 $lifecycle=Block $actor 'void UpdateLifecycle('
 $oldLifecycle=Block $beforeRetry 'void UpdateLifecycle('
 if(($lifecycle.Replace('CompletePrisonWaypoints();','') -replace '\s','') -cne ($oldLifecycle -replace '\s','')){throw 'Unrelated lifecycle behavior changed'}
 $comparisonActor=$actor.Replace('bool _mawWaypointArrived = false;','').Replace('bool _prisonWaypointArrived = false;','').Replace((Block $actor 'void CompletePrisonWaypoints('),'').Replace($movement,$oldMovement).Replace($lifecycle,$oldLifecycle)
 if(($comparisonActor -replace '\s','') -cne ($beforeRetry -replace '\s','')){throw 'Third AI changed beyond two observations/completion/lifecycle retry'}
 if(((Block $actorSource 'namespace SilverHandRitualWaypoint') -replace '\s','') -cne ((Block $retryPinned 'namespace SilverHandRitualWaypoint') -replace '\s','')){throw 'Waypoint recipient policy changed during retry repair'}
}
if(!$ActionBaseline -and !$ArrivalBaseline -and !$WaypointBaseline){
 $waypointPinned=(& git -C $repo show 'c9e683fa:src/server/scripts/Scenario/Artifacts/Paladin/SilverHand/SilverHand.cpp') -join "`n"
 if($LASTEXITCODE){throw 'Pinned waypoint preservation source missing'}
 $oldActor=Block $waypointPinned 'struct npc_travar_thirdAI : ScriptedAI'
 $expectedMovement=Block $oldActor 'void MovementInform('
 foreach($pair in @(@('5','50877','_mawWaypointCredited'),@('6','50912','_prisonWaypointCredited'))){
  $before="if (Player* pl = me->FindNearestPlayer(100.0f)) pl->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, $($pair[1]));"
  $after="if (!$($pair[2])) if (Player* player = SilverHandRitualWaypoint::SelectPlayer(me, $($pair[0]))) { $($pair[2]) = true; player->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, $($pair[1])); }"
  $expectedMovement=($expectedMovement -replace '\s','').Replace(($before -replace '\s',''),($after -replace '\s',''))
 }
 $movement=Block $comparisonActor 'void MovementInform('
 if(($movement -replace '\s','') -cne $expectedMovement){throw 'Third movement changed beyond scoped two waypoint recipients/latches'}
 $normalized=$comparisonActor.Replace('bool _mawWaypointCredited = false;','').Replace('bool _prisonWaypointCredited = false;','').Replace($movement,(Block $oldActor 'void MovementInform('))
 if(($normalized -replace '\s','') -cne ($oldActor -replace '\s','')){throw 'Unrelated third AI behavior changed'}
}
[IO.File]::WriteAllText("$out/ActionPolicy.inc",$(if($ActionBaseline){''}else{Block $actorSource 'bool CanAdvance(Creature* actor)'}))
[IO.File]::WriteAllText("$out/ArrivalPolicy.inc",$(if($ActionBaseline -or $ArrivalBaseline){''}else{Block $actorSource 'namespace SilverHandRitualArrival'}))
[IO.File]::WriteAllText("$out/Arrival.inc",$(if($ActionBaseline -or $ArrivalBaseline){''}else{Block $actor 'void CompleteRitualArrival('}))
[IO.File]::WriteAllText("$out/Lifecycle.inc",$(if($ActionBaseline -or $ArrivalBaseline){''}else{(Block $actor 'void UpdateLifecycle(').Replace(' override','')}))
[IO.File]::WriteAllText("$out/Action.inc",(Block $actor 'void DoAction(').Replace(' override',''))
[IO.File]::WriteAllText("$out/Reset.inc",(Block $actor 'void Reset(').Replace(' override',''))
[IO.File]::WriteAllText("$out/Update.inc",(Block $actor 'void UpdateAI(').Replace(' override',''))
[IO.File]::WriteAllText("$out/Movement.inc",(Block $actor 'void MovementInform(').Replace(' override',''))
$waypointCore=Get-Content "$repo/src/server/game/Movement/MovementGenerators/WaypointMovementGenerator.cpp" -Raw
[IO.File]::WriteAllText("$out/WaypointBridge.inc",(Block $waypointCore 'void WaypointMovementGenerator<Creature>::MovementInform('))
[IO.File]::WriteAllText("$out/LifecycleBridge.inc",(Block (Get-Content "$repo/src/server/game/Entities/Creature/Creature.cpp" -Raw) 'if (i_AI || i_disabledAI)'))
[IO.File]::WriteAllText("$out/CriteriaTypes.inc",(Block (Get-Content "$repo/src/server/game/DataStores/DBCEnums.h" -Raw) 'enum CriteriaTypes')+';')
[IO.File]::WriteAllText("$out/GroupTypes.inc",(Block (Get-Content "$repo/src/server/game/Achievements/AchievementMgr.cpp" -Raw) 'bool AchievementGlobalMgr::IsGroupCriteriaType('))
$player=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
$first=$player.IndexOf('    if (!ignoreGroup && sAchievementMgr->IsGroupCriteriaType(type))',$player.IndexOf('void Player::UpdateAchievementCriteria('))
$last=$player.IndexOf('    // Update only individual achievement criteria here, otherwise we may get multiple updates',$first+1)
if($first -lt 0 -or $last -le $first){throw 'Native shared-scenario bridge missing'}
[IO.File]::WriteAllText("$out/ScenarioBridge.inc",$player.Substring($first,$last-$first))
$functions=Get-Content "$repo/src/common/Utilities/FunctionProcessor.cpp" -Raw
[IO.File]::WriteAllText("$out/FunctionClass.inc",(Block (Get-Content "$repo/src/common/Utilities/FunctionProcessor.h" -Raw) 'class TC_COMMON_API FunctionProcessor').Replace('TC_COMMON_API ','')+';')
$methods=@('FunctionProcessor::FunctionProcessor()','FunctionProcessor::~FunctionProcessor()','void FunctionProcessor::Update(','void FunctionProcessor::KillAllFunctions()','void FunctionProcessor::AddFunction(','void FunctionProcessor::AddFunctionsFromQueue()','uint64 FunctionProcessor::CalculateTime(','void FunctionProcessor::AddDelayedEvent(','uint32 FunctionProcessor::Size() const','uint32 FunctionProcessor::SizeQueue() const')
[IO.File]::WriteAllText("$out/FunctionMethods.inc",(($methods|ForEach-Object{Block $functions $_}) -join "`n"))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$definition=if($Baseline){'/DBASELINE'}elseif($FinalBaseline){'/DFINAL_BASELINE'}elseif($FinalCastBaseline){'/DFINAL_CAST_BASELINE'}elseif($DiscoveryBaseline){'/DDISCOVERY_BASELINE'}elseif($ActionBaseline){'/DACTION_BASELINE'}elseif($ArrivalBaseline){'/DARRIVAL_BASELINE'}elseif($WaypointBaseline){'/DWAYPOINT_BASELINE'}elseif($WaypointRetryBaseline){'/DWAYPOINT_RETRY_BASELINE'}else{''}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $definition /I`"$out`" `"$PSScriptRoot/SilverHandRitualDispatchTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native ritual dispatch regression failed'}
if($Baseline -or $FinalBaseline -or $FinalCastBaseline -or $DiscoveryBaseline -or $ActionBaseline -or $ArrivalBaseline -or $WaypointBaseline -or $WaypointRetryBaseline){return}
& "$PSScriptRoot/Test-SilverHandRitualEntry.ps1" -VerifyReleaseBinary:$VerifyReleaseBinary
if(!$?){throw 'Existing ritual admission/lifetime/data checks failed'}
$previous=$env:LEGION_DIALOGUE_CONFIG
try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile SilverHandFinalCatalog.cpp -ProbeName SilverHandFinalCatalog -OutputFile "$out/final-native.tsv" *> "$out/final-catalog.log";if(!$? -or $LASTEXITCODE){throw 'Native final spell metadata failed'}}finally{$env:LEGION_DIALOGUE_CONFIG=$previous}
$expected=@("SPELL`t215258`tBringing the Light...","SPELL`t215260`tBlinding Light","EFFECT`t215258`t0`t6`t226`t0`t1`t0","EFFECT`t215260`t0`t6`t60`t0`t22`t7","EFFECT`t215260`t1`t6`t5`t0`t22`t7","EFFECT`t215260`t2`t61`t0`t0`t1`t0","MISC`t215258`t25`t1","MISC`t215260`t21`t1","VISUAL`t215258`t55978`t1`t0`t0","VISUAL`t215260`t57625`t1`t0`t0")
if(Compare-Object $expected (Get-Content "$out/final-native.tsv")){throw 'Native final effects/visual links changed'}
if($VerifyReleaseBinary){
 $connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $previous=$env:MYSQL_PWD
 try{$env:MYSQL_PWD=$connection[3];$live=@("SET SESSION TRANSACTION READ ONLY;SELECT map,script FROM instance_template WHERE map=1539;SELECT point,delay FROM waypoint_data_script WHERE id=10993807 AND point IN (10,20) ORDER BY point;"|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Instance/path binding query failed'}}finally{$env:MYSQL_PWD=$previous}
 if(Compare-Object @("1539`tinstance_silver_hand","10`t7000","20`t7000") $live){throw 'Instance binding or native waypoint delays changed'}
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_silver_hand_final_discovery_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Installed finale discovery/cast/waypoint/arrival/action/dispatch repair missing'}}finally{$env:PYTHONPATH=$previous}
}
