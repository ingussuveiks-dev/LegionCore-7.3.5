param([switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/marksmanship-farewell-door"
New-Item -ItemType Directory -Force $out | Out-Null
& "$PSScriptRoot/Test-MarksmanshipOrestesFarewell.ps1" *> "$out/upstream.log"
$migration=Get-Content "$repo/sql/updates/world/2026_10_24_06_marksmanship_farewell_door.sql" -Raw
$reset='DELETE FROM smart_scripts; INSERT INTO smart_scripts SELECT * FROM baseline_smart; DELETE FROM conditions; INSERT INTO conditions SELECT * FROM baseline_conditions; UPDATE gameobject_template SET flags=16,ScriptName='''';'
$sql=@"
CREATE TEMPORARY TABLE saved_smart LIKE smart_scripts;
INSERT INTO saved_smart SELECT * FROM smart_scripts WHERE entryorguid IN(100398,100749);
CREATE TEMPORARY TABLE smart_scripts LIKE saved_smart;
INSERT INTO smart_scripts SELECT * FROM saved_smart;
SET @restore_door := NOT EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=100749 AND source_type=0 AND id=3);
INSERT INTO smart_scripts(entryorguid,source_type,id,link,event_type,event_chance,action_type,target_type,target_param1,target_param2,comment)
SELECT 100749,0,3,4,61,100,9,15,249740,50,'Link - Activate Gameobject' WHERE @restore_door;
DELETE FROM smart_scripts WHERE entryorguid=100398 AND source_type=0 AND id=7;
UPDATE smart_scripts SET link=3 WHERE entryorguid=100749 AND source_type=0 AND id=2;
UPDATE smart_scripts SET link=0 WHERE entryorguid=100398 AND source_type=0 AND id=6;
UPDATE smart_scripts SET event_param1=50500,event_param2=50500 WHERE entryorguid=100398 AND source_type=9 AND id=0;
CREATE TEMPORARY TABLE baseline_smart LIKE smart_scripts;
INSERT INTO baseline_smart SELECT * FROM smart_scripts;
CREATE TEMPORARY TABLE saved_conditions LIKE conditions;
INSERT INTO saved_conditions SELECT * FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry IN(100398,100749);
CREATE TEMPORARY TABLE conditions LIKE saved_conditions;
INSERT INTO conditions SELECT * FROM saved_conditions WHERE NOT (SourceEntry=100398 AND SourceId=0 AND SourceGroup=6 AND ConditionTypeOrReference=30);
CREATE TEMPORARY TABLE baseline_conditions LIKE conditions;
INSERT INTO baseline_conditions SELECT * FROM conditions;
CREATE TEMPORARY TABLE saved_door LIKE gameobject_template;
INSERT INTO saved_door SELECT * FROM gameobject_template WHERE entry=249740;
CREATE TEMPORARY TABLE gameobject_template LIKE saved_door;
INSERT INTO gameobject_template SELECT * FROM saved_door;
INSERT INTO smart_scripts(entryorguid,source_type,id,link,event_type,event_chance,action_type,target_type,target_param1,target_param2,comment)
VALUES(100398,0,7,0,61,100,9,15,249740,50,'Orestes - after eligible farewell credit and death - open native Fel Spike Door');
UPDATE smart_scripts SET link=4 WHERE entryorguid=100749 AND source_type=0 AND id=2;
DELETE FROM smart_scripts WHERE entryorguid=100749 AND source_type=0 AND id=3;
UPDATE smart_scripts SET link=7 WHERE entryorguid=100398 AND source_type=0 AND id=6;
SELECT 'before',smart_scripts.* FROM smart_scripts ORDER BY entryorguid,source_type,id;
$reset
$migration
$migration
SELECT 'after',smart_scripts.* FROM smart_scripts ORDER BY entryorguid,source_type,id;
SELECT 'door-count',COUNT(*) FROM smart_scripts WHERE action_type=9 AND target_type=15 AND target_param1=249740;
SELECT 'torvis-next',link FROM smart_scripts WHERE entryorguid=100749 AND source_type=0 AND id=2;
SELECT 'gate-count',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=100398 AND SourceId=0 AND SourceGroup=6;
SELECT 'row',source_type,id,link,event_type,event_phase_mask,event_flags,event_param1,event_param2,action_type,action_param1,action_param2,action_param3,action_param4,target_type,target_param1,target_param2 FROM smart_scripts WHERE entryorguid=100398 AND ((source_type=0 AND id IN(5,6,7)) OR source_type=9) ORDER BY source_type,id;
SELECT 'gate',ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=100398 AND SourceId=0 AND SourceGroup=6 ORDER BY ElseGroup,ConditionTypeOrReference;
"@
$variants=@{
 'custom-slot'="INSERT INTO smart_scripts(entryorguid,source_type,id,event_type,action_type,target_type,comment) VALUES(100398,0,7,61,37,1,'Custom');"
 'custom-caster'='UPDATE smart_scripts SET action_param2=0 WHERE entryorguid=100749 AND source_type=0 AND id=2;'
 'custom-range'='UPDATE smart_scripts SET target_param2=75 WHERE entryorguid=100749 AND source_type=0 AND id=3;'
 'custom-flags'='UPDATE gameobject_template SET flags=0;'
 'custom-door-script'="UPDATE gameobject_template SET ScriptName='custom_door';"
 'custom-death-link'='UPDATE smart_scripts SET link=9 WHERE entryorguid=100398 AND source_type=0 AND id=6;'
 'custom-condition'='UPDATE conditions SET ConditionValue1=8 WHERE SourceEntry=100398 AND SourceGroup=6 AND ConditionTypeOrReference=32;'
 'other-door-link'="INSERT INTO smart_scripts(entryorguid,source_type,id,link,event_type,action_type,target_type,comment) VALUES(100749,0,99,3,61,37,1,'Custom');"
}
foreach($name in $variants.Keys){$sql+="$reset`n$($variants[$name])`n$migration`nSELECT '$name',link FROM smart_scripts WHERE entryorguid=100749 AND source_type=0 AND id=2;`n"}
$sql+="$reset`nCREATE TEMPORARY TABLE saved_actor LIKE creature_template; INSERT INTO saved_actor SELECT * FROM creature_template WHERE entry IN(100398,100749); CREATE TEMPORARY TABLE creature_template LIKE saved_actor; INSERT INTO creature_template SELECT * FROM saved_actor; UPDATE creature_template SET ScriptName='custom_actor' WHERE entry=100398;`n$migration`nSELECT 'custom-actor',link FROM smart_scripts WHERE entryorguid=100749 AND source_type=0 AND id=2;"
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$connection[3];$rows=@($sql | & C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Farewell door temporary SQL fixture failed'}}finally{$env:MYSQL_PWD=$previous}
foreach($expected in @("door-count`t1","torvis-next`t4","gate-count`t10","custom-actor`t3")+@($variants.Keys | ForEach-Object {"$_`t3"})){if($rows -notcontains $expected){throw "Farewell door mismatch: $expected"}}
$before=@($rows | Where-Object {$_ -like "before`t*"} | ForEach-Object {$_.Substring(7)})
$after=@($rows | Where-Object {$_ -like "after`t*"} | ForEach-Object {$_.Substring(6)})
if(Compare-Object $before $after){throw 'Unrelated Orestes/Torvis fields changed'}
[IO.File]::WriteAllLines("$out/rows.tsv",[string[]]@($rows | Where-Object {$_ -like "row`t*"} | ForEach-Object {$_.Substring(4)}))
[IO.File]::WriteAllLines("$out/conditions.tsv",[string[]]@($rows | Where-Object {$_ -like "gate`t*"} | ForEach-Object {$_.Substring(5)}))
& "$repo/build-extractors/runtime-test/marksmanship-orestes-farewell/test.exe" "$out/rows.tsv" "$out/conditions.tsv";if($LASTEXITCODE){throw 'Native deferred-door lifecycle failed'}
if($VerifyReleaseBinary){
 $previousPython=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previousPython";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_orestes_farewell_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Canonical farewell door PE verification failed'}}finally{$env:PYTHONPATH=$previousPython}
}
'PASS: native door action moved from Torvis death to eligible Orestes credit/death; missing-door admission retries before consuming either transition, no early opening or repeated toggle in the native hosted sequence'
