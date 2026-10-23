param([switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/marksmanship-owned-replay"
New-Item -ItemType Directory -Force $out | Out-Null
$migration=Get-Content "$repo/sql/updates/world/2026_10_23_17_marksmanship_owned_replay.sql" -Raw
$rollback=$migration.Replace('COMMIT;','ROLLBACK;')
$restore=@"
DELETE FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=3 AND SourceEntry=248419 AND SourceId=1;
INSERT INTO conditions (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment) VALUES(22,3,248419,1,0,2,0,128826,1,0,0,0,'','Original owned-copy replay');
"@
$sql=@"
CREATE TEMPORARY TABLE saved_go LIKE gameobject_template;
INSERT INTO saved_go SELECT * FROM gameobject_template WHERE entry=248419;
CREATE TEMPORARY TABLE gameobject_template LIKE saved_go;
INSERT INTO gameobject_template SELECT * FROM saved_go;
CREATE TEMPORARY TABLE saved_smart LIKE smart_scripts;
INSERT INTO saved_smart SELECT * FROM smart_scripts WHERE entryorguid=248419 AND source_type=1;
CREATE TEMPORARY TABLE smart_scripts LIKE saved_smart;
INSERT INTO smart_scripts SELECT * FROM saved_smart;
CREATE TEMPORARY TABLE saved_conditions LIKE conditions;
INSERT INTO saved_conditions SELECT * FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry IN(248419,248834);
CREATE TEMPORARY TABLE conditions LIKE saved_conditions;
INSERT INTO conditions SELECT * FROM saved_conditions;
$restore
$migration
$migration
SELECT 'count',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=3 AND SourceEntry=248419 AND SourceId=1;
SELECT 'row',ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=3 AND SourceEntry=248419 AND SourceId=1 ORDER BY ElseGroup,ConditionTypeOrReference;
SELECT 'actions',COUNT(*) FROM smart_scripts current JOIN saved_smart old USING(entryorguid,source_type,id) WHERE current.link=old.link AND current.event_type=old.event_type AND current.action_type=old.action_type AND current.action_param1=old.action_param1 AND current.target_type=old.target_type AND current.event_flags=old.event_flags;
SELECT 'object',COUNT(*) FROM gameobject_template current JOIN saved_go old USING(entry) WHERE current.Data1=old.Data1 AND current.flags=old.flags AND current.Data0=old.Data0 AND current.AIName=old.AIName AND current.ScriptName=old.ScriptName;
$restore
INSERT INTO conditions (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment) VALUES(22,3,248419,1,0,22,0,1489,0,0,0,0,'','Custom admission');
$migration
SELECT 'custom-list',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=3 AND SourceEntry=248419 AND SourceId=1;
SELECT 'custom-list-old',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=3 AND SourceEntry=248419 AND SourceId=1 AND ConditionTypeOrReference=2 AND ConditionValue3=0;
$restore
UPDATE gameobject_template SET ScriptName='custom_pickup' WHERE entry=248419;
$migration
SELECT 'custom-script',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=3 AND SourceEntry=248419 AND SourceId=1 AND ConditionTypeOrReference=2 AND ConditionValue3=0;
UPDATE gameobject_template SET ScriptName='' WHERE entry=248419;
UPDATE smart_scripts SET action_param1=123 WHERE entryorguid=248419 AND source_type=1 AND id=2;
$migration
SELECT 'custom-action',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=3 AND SourceEntry=248419 AND SourceId=1 AND ConditionTypeOrReference=2 AND ConditionValue3=0;
UPDATE smart_scripts SET action_param1=200279 WHERE entryorguid=248419 AND source_type=1 AND id=2;
$restore
$rollback
SELECT 'rollback',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=3 AND SourceEntry=248419 AND SourceId=1;
SELECT 'rollback-original',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=3 AND SourceEntry=248419 AND SourceId=1 AND ConditionTypeOrReference=2 AND ConditionValue3=0;
"@
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try {
 $env:MYSQL_PWD=$connection[3]
 $rows=@($sql | & C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names)
 if($LASTEXITCODE){throw 'Marksmanship temporary replay fixture failed'}
}finally{$env:MYSQL_PWD=$previous}
foreach($expected in @("count`t24","actions`t4","object`t1","custom-list`t2","custom-list-old`t1","custom-script`t1","custom-action`t1","rollback`t1","rollback-original`t1")){if($rows -notcontains $expected){throw "Marksmanship replay mismatch: $expected"}}
[IO.File]::WriteAllLines("$out/conditions.tsv",[string[]]@($rows | Where-Object {$_ -like "row`t*"} | ForEach-Object {$_.Substring(4)}))
& "$PSScriptRoot/Test-TalonclawPickupAdmission.ps1" -MarksmanshipRowsPath "$out/conditions.tsv"
& "$PSScriptRoot/Test-SurvivalArtifactCatalog.ps1"
$native=Get-Content "$repo/build-extractors/runtime-test/artifact-acquisition/survival-native.tsv"
$steps=@($native | Where-Object {$_ -like "STEP`t972`t*"})
if($steps.Count -ne 8){throw 'Native Marksmanship stage count changed'}
foreach($expected in @("MARKSMANSHIP_CRITERION`t7`t46512`t29364`t92`t48210`t0","EFFECT`t200262`t0`t249`t0`t128826`t0`t0","EFFECT`t200279`t1`t6`t430`t1169`t0`t0")){if($native -notcontains $expected){throw "Native Marksmanship mismatch: $expected"}}
if($VerifyReleaseBinary){
 $previousPython=$env:PYTHONPATH
 try {
  $env:PYTHONPATH="$repo/.codex/pe-review-deps;$previousPython"
  & C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_marksmanship_replay_release.py" "$repo/build-extractors/bin/Release"
  if($LASTEXITCODE){throw 'Native Marksmanship replay Release check failed'}
 }finally{$env:PYTHONPATH=$previousPython}
}
'PASS: native owned/bank replay gates apply only at972/7 or8 with active40419, preserving all four actions, native scene/effects and custom conditions/scripts/actions'
