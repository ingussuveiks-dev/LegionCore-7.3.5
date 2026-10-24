param([switch]$BaselineSql,[switch]$VerifyReleaseBinary,[string]$BaselineRevision)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/talonclaw-dakarr"
New-Item -ItemType Directory -Force $out | Out-Null
$migration=if($BaselineSql){''}else{Get-Content "$repo/sql/updates/world/2026_10_23_15_talonclaw_dakarr_admission.sql" -Raw}
$sql=@"
CREATE TEMPORARY TABLE saved_actor LIKE creature_template;
INSERT INTO saved_actor SELECT * FROM creature_template WHERE entry IN(99480,100005);
CREATE TEMPORARY TABLE creature_template LIKE saved_actor;
INSERT INTO creature_template SELECT * FROM saved_actor;
CREATE TEMPORARY TABLE saved_smart LIKE smart_scripts;
INSERT INTO saved_smart SELECT * FROM smart_scripts WHERE entryorguid=99480 AND source_type=0;
CREATE TEMPORARY TABLE smart_scripts LIKE saved_smart;
INSERT INTO smart_scripts SELECT * FROM saved_smart;
CREATE TEMPORARY TABLE saved_conditions LIKE conditions;
INSERT INTO saved_conditions SELECT * FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=99480;
CREATE TEMPORARY TABLE conditions LIKE saved_conditions;
INSERT INTO conditions SELECT * FROM saved_conditions;
UPDATE creature_template SET ScriptName='' WHERE entry=99480;
DELETE FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=2 AND SourceEntry=99480 AND SourceId=0;
$migration
$migration
SELECT 'binding',COUNT(*) FROM creature_template WHERE entry=99480 AND AIName='SmartAI' AND ScriptName='npc_talonclaw_dakarr';
SELECT 'native-death',COUNT(*) FROM smart_scripts WHERE entryorguid=99480 AND source_type=0 AND id=1 AND event_type=6 AND link=2 AND action_type=205 AND action_param1=92 AND action_param2=47688 AND target_type=21 AND target_param1=50;
SELECT 'native-spawns',COUNT(*) FROM smart_scripts WHERE entryorguid=99480 AND source_type=0 AND ((id=2 AND link=3 AND action_type=12 AND action_param1=100310) OR (id=3 AND link=0 AND action_type=50 AND action_param1=248834));
SELECT 'preserved',COUNT(*) FROM creature_template actor JOIN saved_actor old USING(entry) WHERE actor.unit_flags=old.unit_flags AND actor.unit_flags2=old.unit_flags2 AND actor.faction=old.faction AND actor.AIName=old.AIName;
SELECT 'combat',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=1 AND SourceEntry=99480 AND SourceId=0 AND ConditionTypeOrReference=44 AND ConditionTarget=1 AND ConditionValue1=973 AND ConditionValue2=11;
SELECT 'condition',ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2 FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=2 AND SourceEntry=99480 AND SourceId=0;
DELETE FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=2 AND SourceEntry=99480 AND SourceId=0;
UPDATE creature_template SET ScriptName='custom_actor' WHERE entry=99480;
$migration
SELECT 'custom-actor',COUNT(*) FROM creature_template WHERE entry=99480 AND ScriptName='custom_actor';
SELECT 'custom-no-condition',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=2 AND SourceEntry=99480;
UPDATE creature_template SET ScriptName='',AIName='custom_ai' WHERE entry=99480;
$migration
SELECT 'custom-ai',COUNT(*) FROM creature_template WHERE entry=99480 AND AIName='custom_ai' AND ScriptName='';
UPDATE creature_template SET ScriptName='',AIName='SmartAI' WHERE entry=99480;
UPDATE smart_scripts SET action_param2=123456 WHERE entryorguid=99480 AND source_type=0 AND id=1;
$migration
SELECT 'custom-event',COUNT(*) FROM creature_template WHERE entry=99480 AND ScriptName='';
UPDATE smart_scripts SET action_param2=47688 WHERE entryorguid=99480 AND source_type=0 AND id=1;
INSERT INTO conditions (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment) VALUES(22,2,99480,0,0,22,1,1511,0,0,0,0,'','custom death gate');
$migration
SELECT 'custom-condition',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=2 AND SourceEntry=99480 AND SourceId=0;
SELECT 'custom-condition-kept',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=2 AND SourceEntry=99480 AND ConditionTypeOrReference=22;
"@
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try {
 $env:MYSQL_PWD=$connection[3]
 $rows=@($sql | & C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names)
 if($LASTEXITCODE){throw 'Dakarr temporary SQL fixture failed'}
}finally{$env:MYSQL_PWD=$previous}
foreach($expected in @("binding`t1","native-death`t1","native-spawns`t2","preserved`t2","combat`t1","custom-actor`t1","custom-no-condition`t0","custom-ai`t1","custom-event`t1","custom-condition`t1","custom-condition-kept`t1")){if($rows -notcontains $expected){throw "Dakarr fixture mismatch: $expected"}}
$condition=@($rows | Where-Object {$_ -like "condition`t*"})
if($condition.Count -ne 1 -or $condition[0] -ne "condition`t44`t1`t973`t11"){throw 'Dakarr native death admission missing'}
[IO.File]::WriteAllText("$out/condition.tsv",$condition[0].Substring(10))
function Section([string]$source,[string]$start,[string]$end){$first=$source.IndexOf($start);$last=$source.IndexOf($end,$first);if($first -lt 0 -or $last -lt $first){throw "Missing native section $start"};$source.Substring($first,$last-$first)}
$source=Get-Content "$repo/src/server/scripts/Legion/ClassHalls/artifact_acquisition.cpp" -Raw
if($BaselineRevision){$source=(& git -C $repo show "${BaselineRevision}:src/server/scripts/Legion/ClassHalls/artifact_acquisition.cpp") -join "`n";if($LASTEXITCODE){throw 'Cannot read baseline Dakarr source'}}
$end=if($source.Contains('namespace TalonclawAmbush')){'namespace TalonclawAmbush'}elseif($source.Contains('namespace TitanstrikePrustaga')){'namespace TitanstrikePrustaga'}else{'namespace ArtifactAcquisition'}
[IO.File]::WriteAllText("$out/Dakarr.inc",(Section $source 'namespace TalonclawDakarr' $end))
$source=Get-Content "$repo/src/server/game/Grids/Notifiers/GridNotifiers.cpp" -Raw
[IO.File]::WriteAllText("$out/NearestPlayer.inc",(Section $source 'bool NearestPlayerInObjectRangeCheck::operator()(Player* u)' 'NearestPlayerNotGMInObjectRangeCheck::NearestPlayerNotGMInObjectRangeCheck'))
$source=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
[IO.File]::WriteAllText("$out/PlayerScenario.inc",(Section $source '    // Update scenario/challenge criterias' '    // Update only individual achievement criteria here, otherwise we may get multiple updates'))
$source=Get-Content "$repo/src/server/game/Conditions/ConditionMgr.cpp" -Raw
[IO.File]::WriteAllText("$out/Condition.inc",(Section $source '        case CONDITION_SCENARION_STEP:' '        case CONDITION_REALM_ACHIEVEMENT:'))
$source=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw
$context='ConditionSourceInfo info = ConditionSourceInfo(unit ? unit : GetBaseObject(), GetBaseObject());'
if(!$source.Contains($context)){throw 'Native SmartAI base-actor condition context changed'}
[IO.File]::WriteAllText("$out/SourceInfo.inc",$context)
$source=Get-Content "$repo/src/server/game/Entities/Creature/Creature.cpp" -Raw
$entry=Section $source 'void Creature::AddToWorld()' 'void Creature::RemoveFromWorld()'
if(!$entry.Contains('AIM_Initialize();')){throw 'Fresh creature admission no longer initializes AI on world entry'}
$source=Get-Content "$repo/src/server/game/Maps/Map.cpp" -Raw
$entry=Section $source 'bool Map::AddToMap(T *obj)' 'bool Map::AddToMap(Transport* obj)'
if($entry.IndexOf('obj->AddToWorld();') -ge $entry.LastIndexOf('obj->UpdateObjectVisibility(true);')){throw 'Fresh map insertion exposes the creature before world-entry AI admission'}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/TalonclawDakarrTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`" `"$out/condition.tsv`"`n")
& "$out/run.cmd"
if($LASTEXITCODE){throw 'Native Dakarr admission regression failed'}
& "$PSScriptRoot/Test-SurvivalArtifactCatalog.ps1"
$native=Get-Content "$repo/build-extractors/runtime-test/artifact-acquisition/survival-native.tsv"
if($native -notcontains "CRITERION`t11`t46290`t29263`t92`t47688`t0"){throw 'Native final Dakarr lair stage mismatch'}
if($native -notcontains "DAKARR_RULE`t0`t0`t0`t0`t0`t0" -or $native -notcontains "DAKARR_ROOT_FLAGS`t0"){throw 'Native Dakarr alive/faction/extra-criteria rules changed'}
if($VerifyReleaseBinary){
 & "$PSScriptRoot/Test-TalonclawWorldPickup.ps1" -VerifyReleaseBinary
 $previousPython=$env:PYTHONPATH
 try {
  $env:PYTHONPATH="$repo/.codex/pe-review-deps;$previousPython"
  & C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_talonclaw_dakarr_release.py" "$repo/build-extractors/bin/Release"
  if($LASTEXITCODE){throw 'Deployed Dakarr admission verification failed'}
 }finally{$env:PYTHONPATH=$previousPython}
}
'PASS: repeated guarded binding/death-condition migration preserves native combat/spawn actions and custom actors/AI/events/conditions'
