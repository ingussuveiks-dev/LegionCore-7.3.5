param([switch]$Baseline,[switch]$ScopeOnly,[switch]$VerifyReleaseBinary,[string]$OutputDirectory='')
$ErrorActionPreference='Stop'
if($VerifyReleaseBinary -and ($Baseline -or $ScopeOnly)){throw 'Only repaired native checks certify Release'}
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth -and $last -lt $source.Length){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};if($depth){throw "Unclosed $start"};$source.Substring($first,$last-$first)}
$path='src/server/scripts/Scenario/Artifacts/Paladin/SilverHand/SilverHand.cpp'
$source=(Get-Content "$repo/$path" -Raw).Replace("`r",'')
$pinned=(& git -C $repo show "b06f63842:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned procession recovery baseline unavailable'}
$original=Block $pinned 'class npc_travar_second : public CreatureScript'
$current=Block $source 'class npc_travar_second : public CreatureScript'
if(!$Baseline){
 $movement=(Block $original 'void MovementInform(').Replace('if (id == 14)'+"`n                    CompleteProcession();",'if (id == 14) { _processionState.Observe(me, _processionPlayerGuid); CompleteProcession(); }')
 if(($movement -replace '\s','') -cne ((Block $current 'void MovementInform(') -replace '\s','')){throw 'Unrelated procession movement changed'}
 $gossip=(Block $original 'void sGossipSelect(').Replace('me->RemoveFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);','if (!_processionPlayerGuid.IsEmpty() || _processionState.completed) return; me->RemoveFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);')
 if(($gossip -replace '\s','') -cne ((Block $current 'void sGossipSelect(') -replace '\s','')){throw 'Unrelated procession gossip changed'}
 $life=(Block $original 'void UpdateLifecycle(').Replace('UpdateLifecycle(uint32)','UpdateLifecycle(uint32 diff)').Replace('SilverHandCompanion::Recover(me, _pendingHealRecovery, wait_heal);','SilverHandCompanion::Recover(me, _pendingHealRecovery, wait_heal); _processionState.Update(diff); CompleteProcession();')
 if(($life -replace '\s','') -cne ((Block $current 'void UpdateLifecycle(') -replace '\s','')){throw 'Unrelated procession lifecycle changed'}
 $normalized=$current
 foreach($name in @('void MovementInform(','void sGossipSelect(','void UpdateLifecycle(','void CompleteProcession(')){$normalized=$normalized.Replace((Block $normalized $name),(Block $original $name))}
 $normalized=$normalized.Replace('SilverHandProcessionRecovery::State _processionState;','')
 if(($normalized -replace '\s','') -cne ($original -replace '\s','')){throw 'Unrelated second-Travard method/field changed'}
 $whole=$source.Replace($current,$original).Replace('#include "silver_hand_procession.h"','')
 if(($whole -replace '\s','') -cne ($pinned -replace '\s','')){throw 'Unrelated Silver Hand script changed'}
}
if($ScopeOnly){'PASS exact second-procession-recovery-only scope; first actor/other AIs/route/companion behavior retained';return}
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/silver-hand-procession-recovery"}
$out=[IO.Path]::GetFullPath($OutputDirectory);New-Item -ItemType Directory -Force $out|Out-Null
$actor=if($Baseline){$original}else{$current}
[IO.File]::WriteAllText("$out/ProcessionPolicy.inc",(Block $source 'namespace SilverHandProcession'))
[IO.File]::WriteAllText("$out/ProcessionGossip.inc",(Block $actor 'void sGossipSelect(').Replace(' override',''))
[IO.File]::WriteAllText("$out/ProcessionArrival.inc",(Block $actor 'void CompleteProcession('))
[IO.File]::WriteAllText("$out/ProcessionMovement.inc",(Block $actor 'void MovementInform(').Replace(' override',''))
[IO.File]::WriteAllText("$out/ProcessionLifecycle.inc",(Block $actor 'void UpdateLifecycle('))
[IO.File]::WriteAllText("$out/ProcessionReset.inc",(Block $actor 'void Reset('))
foreach($name in @('Scenario.h','ScenarioMgr.h')){[IO.File]::WriteAllText("$out/$name",'// Controlled native services are declared in the host before production policy.')}
$guid=Get-Content "$repo/src/server/game/Entities/Object/ObjectGuid.cpp" -Raw
[IO.File]::WriteAllText("$out/GuidLifecycle.inc",((@('ObjectGuid::ObjectGuid()','void ObjectGuid::Clear()','bool ObjectGuid::IsEmpty() const')|ForEach-Object{Block $guid $_}) -join "`n"))
$header=Get-Content "$repo/src/server/game/Globals/ObjectAccessor.h" -Raw
[IO.File]::WriteAllText("$out/WorldPlayer.inc",(Block $header 'static Player* GetObjectInWorld(ObjectGuid'))
[IO.File]::WriteAllText("$out/MapPlayer.inc",(Block $header 'template<class T> static T* GetObjectInMap('))
[IO.File]::WriteAllText("$out/AccessorPlayer.inc",(Block (Get-Content "$repo/src/server/game/Globals/ObjectAccessor.cpp" -Raw) 'Player* ObjectAccessor::GetPlayer('))
[IO.File]::WriteAllText("$out/NativeLifecycle.inc",(Block (Get-Content "$repo/src/server/game/Entities/Creature/Creature.cpp" -Raw) 'if (i_AI || i_disabledAI)'))
$waypoint=Get-Content "$repo/src/server/game/Movement/MovementGenerators/WaypointMovementGenerator.cpp" -Raw
[IO.File]::WriteAllText("$out/NativeWaypoint.inc",(Block $waypoint 'void WaypointMovementGenerator<Creature>::MovementInform('))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$definition=if($Baseline){'/DRECOVERY_BASELINE'}else{'/DRECOVERY'}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi $definition /I`"$out`" /I`"$repo/src/server/scripts/Scenario/Artifacts/Paladin/SilverHand`" `"$PSScriptRoot/SilverHandProcessionTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
& "$out/run.cmd";if($LASTEXITCODE){throw "Native procession recovery failed: $LASTEXITCODE"}
if($VerifyReleaseBinary){
 $previous=$env:LEGION_DIALOGUE_CONFIG
 try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile SilverHandAllyCatalog.cpp -ProbeName SilverHandAllyCatalog -OutputFile "$out/native.tsv" *> "$out/catalog.log";if(!$? -or $LASTEXITCODE){throw 'Native procession catalog failed'}}finally{$env:LEGION_DIALOGUE_CONFIG=$previous}
 foreach($row in @("STEP`t4`t2449`t47965`t0`tInto the Dark","CRITERION`t4`t30079`t92`t50897`t0","PROCESSION_META`t30079`t0`t0`t47966`t0`t0","STEP`t5`t2453`t47979`t0`tThe Maw of the Beast")){if((Get-Content "$out/native.tsv") -notcontains $row){throw "Native procession criterion changed: $row"}}
 $connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $query=@'
SET SESSION TRANSACTION READ ONLY;
SELECT 'binding',entry,gossip_menu_id,npcflag,npcflag2,ScriptName FROM creature_template WHERE entry=106639;
SELECT 'spawn',guid,id,map,PhaseId FROM creature WHERE id=106639;
SELECT 'path',id,COUNT(*),MIN(point),MAX(point) FROM waypoint_data_script WHERE id=10993806 GROUP BY id;
SELECT 'last-point',point,ROUND(position_x,2),ROUND(position_y,2),ROUND(position_z,2),delay FROM waypoint_data_script WHERE id=10993806 AND point=15;
SELECT 'phase',pd.phaseId,co.ConditionTypeOrReference,co.ConditionValue1,co.ConditionValue2,co.NegativeCondition FROM phase_definitions pd JOIN conditions co ON co.SourceGroup=pd.zoneId AND co.SourceEntry=pd.entry AND co.SourceTypeOrReferenceId=23 WHERE pd.zoneId=85 AND pd.entry=7;
SELECT 'additional',COUNT(*) FROM achievement_criteria_data WHERE criteria_id=30079;
SELECT 'complete','procession-recovery-readback';
'@
 [IO.File]::WriteAllText("$out/live-read.sql",$query)
 $previous=$env:MYSQL_PWD
 try{
  $env:MYSQL_PWD=$connection[3]
  $arguments=@("--host=$($connection[0])","--port=$($connection[1])","--user=$($connection[2])","--database=$($connection[4])",'--batch','--raw','--skip-column-names')
  $exitCode=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -Arguments $arguments -InputFile "$out/live-read.sql" -OutputFile "$out/live.tsv" -ErrorFile "$out/live.stderr"
  if($exitCode -or (Get-Item "$out/live.stderr").Length){throw 'Full-EOF procession readback failed'}
 }finally{$env:MYSQL_PWD=$previous}
 $expected=@("binding`t106639`t19599`t0`t0`tnpc_travar_second","spawn`t361991`t106639`t1539`t7000","path`t10993806`t15`t1`t15","last-point`t15`t1877.17`t2244.85`t36.91`t3000","phase`t7000`t44`t1092`t4`t0","additional`t0","complete`tprocession-recovery-readback")
 if(Compare-Object $expected @(Get-Content "$out/live.tsv")){throw 'Procession live binding/path/phase/additional condition changed'}
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_silver_hand_procession_recovery_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Canonical procession recovery PE failed'}}finally{$env:PYTHONPATH=$previous}
}
