param([switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/marksmanship-pickup-admission"
New-Item -ItemType Directory -Force $out | Out-Null
$migration=Get-Content "$repo/sql/updates/world/2026_10_23_19_marksmanship_pickup_admission.sql" -Raw
$rollback=$migration.Replace('COMMIT;','ROLLBACK;')
$reset=@"
DELETE FROM conditions WHERE (SourceTypeOrReferenceId=4 AND SourceGroup=248419 AND SourceEntry=128826 AND SourceId=0) OR (SourceTypeOrReferenceId=22 AND SourceGroup=1 AND SourceEntry=248419 AND SourceId=1);
UPDATE gameobject_template SET ScriptName='' WHERE entry=248419;
"@
$sql=@"
CREATE TEMPORARY TABLE saved_go LIKE gameobject_template;
INSERT INTO saved_go SELECT * FROM gameobject_template WHERE entry=248419;
CREATE TEMPORARY TABLE gameobject_template LIKE saved_go;
INSERT INTO gameobject_template SELECT * FROM saved_go;
CREATE TEMPORARY TABLE saved_loot LIKE gameobject_loot_template;
INSERT INTO saved_loot SELECT * FROM gameobject_loot_template WHERE Entry=248419;
CREATE TEMPORARY TABLE gameobject_loot_template LIKE saved_loot;
INSERT INTO gameobject_loot_template SELECT * FROM saved_loot;
CREATE TEMPORARY TABLE saved_smart LIKE smart_scripts;
INSERT INTO saved_smart SELECT * FROM smart_scripts WHERE entryorguid=248419 AND source_type=1;
CREATE TEMPORARY TABLE smart_scripts LIKE saved_smart;
INSERT INTO smart_scripts SELECT * FROM saved_smart;
CREATE TEMPORARY TABLE saved_conditions LIKE conditions;
INSERT INTO saved_conditions SELECT * FROM conditions WHERE (SourceTypeOrReferenceId=4 AND SourceGroup IN(248419,248834)) OR (SourceTypeOrReferenceId=22 AND SourceEntry=248419 AND SourceId=1);
CREATE TEMPORARY TABLE conditions LIKE saved_conditions;
INSERT INTO conditions SELECT * FROM saved_conditions;
$reset
$migration
$migration
SELECT 'loot-count',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=4 AND SourceGroup=248419 AND SourceEntry=128826 AND SourceId=0;
SELECT 'root-count',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=1 AND SourceEntry=248419 AND SourceId=1;
SELECT 'loot-row',ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition FROM conditions WHERE SourceTypeOrReferenceId=4 AND SourceGroup=248419 AND SourceEntry=128826 AND SourceId=0 ORDER BY ElseGroup,ConditionTypeOrReference;
SELECT 'root-row',ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=1 AND SourceEntry=248419 AND SourceId=1 ORDER BY ElseGroup,ConditionTypeOrReference;
SELECT 'binding',COUNT(*) FROM gameobject_template WHERE entry=248419 AND ScriptName='go_artifact_acquisition';
SELECT 'object',COUNT(*) FROM gameobject_template current JOIN saved_go old USING(entry) WHERE current.Data0=old.Data0 AND current.Data1=old.Data1 AND current.flags=old.flags AND current.AIName=old.AIName;
SELECT 'loot',COUNT(*) FROM gameobject_loot_template current JOIN saved_loot old USING(Entry,Item) WHERE current.Chance=old.Chance AND current.MinCount=old.MinCount AND current.MaxCount=old.MaxCount AND current.Currency=old.Currency AND current.Reference=old.Reference;
SELECT 'actions',COUNT(*) FROM smart_scripts current JOIN saved_smart old USING(entryorguid,source_type,id) WHERE current.link=old.link AND current.event_type=old.event_type AND current.event_param1=old.event_param1 AND current.action_type=old.action_type AND current.action_param1=old.action_param1 AND current.action_param2=old.action_param2 AND current.target_type=old.target_type AND current.event_flags=old.event_flags;
SELECT 'other',COUNT(*) FROM conditions current JOIN saved_conditions old ON current.SourceTypeOrReferenceId=old.SourceTypeOrReferenceId AND current.SourceGroup=old.SourceGroup AND current.SourceEntry=old.SourceEntry AND current.SourceId=old.SourceId AND current.ElseGroup=old.ElseGroup AND current.ConditionTypeOrReference=old.ConditionTypeOrReference AND current.ConditionValue1=old.ConditionValue1 AND current.ConditionValue2=old.ConditionValue2 AND current.ConditionValue3=old.ConditionValue3 AND current.NegativeCondition=old.NegativeCondition WHERE (current.SourceTypeOrReferenceId=22 AND current.SourceGroup=3) OR (current.SourceTypeOrReferenceId=4 AND current.SourceGroup=248834);
SELECT 'saved-other',COUNT(*) FROM saved_conditions WHERE (SourceTypeOrReferenceId=22 AND SourceGroup=3) OR (SourceTypeOrReferenceId=4 AND SourceGroup=248834);
$reset
INSERT INTO conditions (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment) VALUES(4,248419,128826,0,0,4,0,1,0,0,0,0,'','Custom loot condition'),(22,1,248419,1,0,4,0,1,0,0,0,0,'','Custom root condition');
$migration
SELECT 'custom-lists',COUNT(*) FROM conditions WHERE ConditionTypeOrReference=4 AND ConditionValue1=1 AND ((SourceTypeOrReferenceId=4 AND SourceGroup=248419 AND SourceEntry=128826) OR (SourceTypeOrReferenceId=22 AND SourceGroup=1 AND SourceEntry=248419));
SELECT 'custom-count',COUNT(*) FROM conditions WHERE (SourceTypeOrReferenceId=4 AND SourceGroup=248419 AND SourceEntry=128826) OR (SourceTypeOrReferenceId=22 AND SourceGroup=1 AND SourceEntry=248419);
$reset
UPDATE gameobject_template SET ScriptName='custom_pickup' WHERE entry=248419;
$migration
SELECT 'custom-script',COUNT(*) FROM gameobject_template WHERE entry=248419 AND ScriptName='custom_pickup';
SELECT 'custom-script-rows',COUNT(*) FROM conditions WHERE (SourceTypeOrReferenceId=4 AND SourceGroup=248419 AND SourceEntry=128826) OR (SourceTypeOrReferenceId=22 AND SourceGroup=1 AND SourceEntry=248419);
$reset
UPDATE smart_scripts SET action_param1=123 WHERE entryorguid=248419 AND source_type=1 AND id=2;
$migration
SELECT 'custom-action-rows',COUNT(*) FROM conditions WHERE (SourceTypeOrReferenceId=4 AND SourceGroup=248419 AND SourceEntry=128826) OR (SourceTypeOrReferenceId=22 AND SourceGroup=1 AND SourceEntry=248419);
SELECT 'custom-action-binding',COUNT(*) FROM gameobject_template WHERE entry=248419 AND ScriptName='';
UPDATE smart_scripts SET action_param1=200279 WHERE entryorguid=248419 AND source_type=1 AND id=2;
UPDATE gameobject_loot_template SET Reference=123 WHERE Entry=248419 AND Item=128826;
$migration
SELECT 'custom-loot-rows',COUNT(*) FROM conditions WHERE (SourceTypeOrReferenceId=4 AND SourceGroup=248419 AND SourceEntry=128826) OR (SourceTypeOrReferenceId=22 AND SourceGroup=1 AND SourceEntry=248419);
UPDATE gameobject_loot_template SET Reference=0 WHERE Entry=248419 AND Item=128826;
$rollback
SELECT 'rollback-binding',COUNT(*) FROM gameobject_template WHERE entry=248419 AND ScriptName='';
SELECT 'rollback-rows',COUNT(*) FROM conditions WHERE (SourceTypeOrReferenceId=4 AND SourceGroup=248419 AND SourceEntry=128826) OR (SourceTypeOrReferenceId=22 AND SourceGroup=1 AND SourceEntry=248419);
"@
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try {
 $env:MYSQL_PWD=$connection[3]
 $rows=@($sql | & C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names)
 if($LASTEXITCODE){throw 'Marksmanship pickup temporary SQL fixture failed'}
}finally{$env:MYSQL_PWD=$previous}
foreach($expected in @("loot-count`t20","root-count`t24","binding`t1","object`t1","loot`t1","actions`t4","custom-lists`t2","custom-count`t2","custom-script`t1","custom-script-rows`t0","custom-action-rows`t0","custom-action-binding`t1","custom-loot-rows`t0","rollback-binding`t1","rollback-rows`t0")){if($rows -notcontains $expected){throw "Marksmanship pickup mismatch: $expected"}}
$other=($rows | Where-Object {$_ -like "other`t*"}) -replace '^other',''
$saved=($rows | Where-Object {$_ -like "saved-other`t*"}) -replace '^saved-other',''
if($other -ne $saved){throw 'Unrelated Survival loot or original owned-replay conditions changed'}
foreach($kind in @('loot','root')){
 [IO.File]::WriteAllLines("$out/$kind.tsv",[string[]]@($rows | Where-Object {$_ -like "$kind-row`t*"} | ForEach-Object {$_.Substring($kind.Length+5)}))
 & "$PSScriptRoot/Test-TalonclawPickupAdmission.ps1" -MarksmanshipRowsPath "$out/$kind.tsv" -MarksmanshipLoot:($kind -eq 'loot') -MarksmanshipRoot:($kind -eq 'root')
}
& "$PSScriptRoot/Test-MarksmanshipOwnedReplay.ps1"
$native=Get-Content "$repo/build-extractors/runtime-test/artifact-acquisition/survival-native.tsv"
if(@($native | Where-Object {$_ -like "MARKSMANSHIP_LOCK`t*"}).Count -ne 8 -or $native -notcontains "MARKSMANSHIP_LOCK`t1`t2`t5`t0`t0"){throw 'Native1691 ordinary OPEN lock definition changed'}
if($VerifyReleaseBinary){
 $previousPython=$env:PYTHONPATH
 try {
  $env:PYTHONPATH="$repo/.codex/pe-review-deps;$previousPython"
  & C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_marksmanship_pickup_admission_release.py" "$repo/build-extractors/bin/Release"
  if($LASTEXITCODE){throw 'Deployed Marksmanship pickup admission failed'}
 }finally{$env:PYTHONPATH=$previousPython}
}
'PASS: repeated atomic Marksmanship pickup migration preserves original actions, loot, owned-replay and custom definitions; real loot and post-claim root conditions enforce native final-stage context'
