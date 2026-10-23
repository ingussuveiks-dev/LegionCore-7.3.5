param([switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/marksmanship-farewell-budget"
New-Item -ItemType Directory -Force $out | Out-Null
& "$PSScriptRoot/Test-MarksmanshipOrestesFarewell.ps1" *> "$out/credit-retry.log"
& "$PSScriptRoot/Test-MarksmanshipFarewell.ps1" *> "$out/lifetime.log"
$migration=Get-Content "$repo/sql/updates/world/2026_10_24_05_marksmanship_farewell_budget.sql" -Raw
$reset='DELETE FROM smart_scripts; INSERT INTO smart_scripts SELECT * FROM baseline_smart; DELETE FROM conversation_creature; INSERT INTO conversation_creature SELECT * FROM baseline_creatures; DELETE FROM conversation_data; INSERT INTO conversation_data SELECT * FROM baseline_lines; DELETE FROM conversation_actor; INSERT INTO conversation_actor SELECT * FROM baseline_actors;'
$sql=@"
CREATE TEMPORARY TABLE saved_smart LIKE smart_scripts;
INSERT INTO saved_smart SELECT * FROM smart_scripts WHERE entryorguid IN(100398,100749);
CREATE TEMPORARY TABLE smart_scripts LIKE saved_smart;
INSERT INTO smart_scripts SELECT * FROM saved_smart;
UPDATE smart_scripts SET event_param1=10000,event_param2=10000 WHERE entryorguid=100398 AND source_type=9 AND id=0;
UPDATE smart_scripts SET link=0 WHERE entryorguid=100398 AND source_type=0 AND id=6;
CREATE TEMPORARY TABLE baseline_smart LIKE smart_scripts;
INSERT INTO baseline_smart SELECT * FROM smart_scripts;
CREATE TEMPORARY TABLE saved_creatures LIKE conversation_creature;
INSERT INTO saved_creatures SELECT * FROM conversation_creature;
CREATE TEMPORARY TABLE conversation_creature LIKE saved_creatures;
INSERT INTO conversation_creature SELECT * FROM saved_creatures;
UPDATE conversation_creature SET duration=25000 WHERE entry=1072 AND id=0;
CREATE TEMPORARY TABLE baseline_creatures LIKE conversation_creature;
INSERT INTO baseline_creatures SELECT * FROM conversation_creature;
CREATE TEMPORARY TABLE saved_lines LIKE conversation_data;
INSERT INTO saved_lines SELECT * FROM conversation_data;
CREATE TEMPORARY TABLE conversation_data LIKE saved_lines;
INSERT INTO conversation_data SELECT * FROM saved_lines;
CREATE TEMPORARY TABLE baseline_lines LIKE conversation_data;
INSERT INTO baseline_lines SELECT * FROM conversation_data;
CREATE TEMPORARY TABLE saved_actors LIKE conversation_actor;
INSERT INTO saved_actors SELECT * FROM conversation_actor;
CREATE TEMPORARY TABLE conversation_actor LIKE saved_actors;
INSERT INTO conversation_actor SELECT * FROM saved_actors;
CREATE TEMPORARY TABLE baseline_actors LIKE conversation_actor;
INSERT INTO baseline_actors SELECT * FROM conversation_actor;
UPDATE conversation_creature SET duration=40000 WHERE entry=1072 AND id=0;
UPDATE smart_scripts SET event_param1=50500,event_param2=50500 WHERE entryorguid=100398 AND source_type=9 AND id=0;
SELECT 'before-smart',smart_scripts.* FROM smart_scripts ORDER BY entryorguid,source_type,id;
SELECT 'before-creature',conversation_creature.* FROM conversation_creature ORDER BY entry,id;
$reset
$migration
$migration
SELECT 'after-smart',smart_scripts.* FROM smart_scripts ORDER BY entryorguid,source_type,id;
SELECT 'after-creature',conversation_creature.* FROM conversation_creature ORDER BY entry,id;
SELECT 'sum',SUM(duration)+5000 FROM conversation_creature WHERE entry=1072;
SELECT 'wait',event_param1,event_param2 FROM smart_scripts WHERE entryorguid=100398 AND source_type=9 AND id=0;
SELECT 'creature',entry,id,creatureId,creatureGuid,unk1,unk2,duration FROM conversation_creature WHERE entry=1072 ORDER BY id;
SELECT 'line',entry,id,idx,textId,unk1,unk2 FROM conversation_data WHERE entry=1072 ORDER BY idx;
SELECT 'row',source_type,id,link,event_type,event_phase_mask,event_flags,event_param1,event_param2,action_type,action_param1,action_param2,action_param3,action_param4,target_type,target_param1,target_param2 FROM smart_scripts WHERE entryorguid=100398 AND ((source_type=0 AND id IN(5,6)) OR source_type=9) ORDER BY source_type,id;
SELECT 'gate',ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=100398 AND SourceGroup=6 AND SourceId=0 ORDER BY ElseGroup,ConditionTypeOrReference;
"@
$variants=@{
 'custom-duration'='UPDATE conversation_creature SET duration=3000 WHERE entry=1072 AND id=1;'
 'custom-line'='UPDATE conversation_data SET textId=23000 WHERE entry=1072 AND id=2631;'
 'custom-creature'='UPDATE conversation_creature SET creatureId=100836 WHERE entry=1072 AND id=1;'
 'custom-actor'='INSERT INTO conversation_actor(entry,id,actorId,creatureId,displayId,unk1,unk2,unk3,duration) VALUES(1072,0,1,1,1,0,0,0,1);'
 'custom-timer'='UPDATE smart_scripts SET event_param1=12000,event_param2=12000 WHERE entryorguid=100398 AND source_type=9 AND id=0;'
 'custom-arm'='UPDATE smart_scripts SET action_type=23 WHERE entryorguid=100398 AND source_type=9 AND id=0;'
 'custom-credit'='UPDATE smart_scripts SET target_type=21,target_param1=50 WHERE entryorguid=100398 AND source_type=0 AND id=5;'
 'custom-phase'='UPDATE smart_scripts SET event_phase_mask=2 WHERE entryorguid=100398 AND source_type=0 AND id=0;'
 'extra-timer'="INSERT INTO smart_scripts(entryorguid,source_type,id,event_type,action_type,target_type,comment) VALUES(100398,9,1,0,37,1,'Custom');"
}
foreach($name in $variants.Keys){$sql+="$reset`n$($variants[$name])`n$migration`nSELECT '$name',duration FROM conversation_creature WHERE entry=1072 AND id=0;`n"}
$sql+="$reset`nCREATE TEMPORARY TABLE saved_template LIKE creature_template; INSERT INTO saved_template SELECT * FROM creature_template WHERE entry=100398; CREATE TEMPORARY TABLE creature_template LIKE saved_template; INSERT INTO creature_template SELECT * FROM saved_template; UPDATE creature_template SET ScriptName='custom_actor';`n$migration`nSELECT 'custom-script',duration FROM conversation_creature WHERE entry=1072 AND id=0;"
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$connection[3];$rows=@($sql | & C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Farewell budget temporary SQL fixture failed'}}finally{$env:MYSQL_PWD=$previous}
foreach($expected in @("sum`t50300","wait`t50500`t50500","custom-script`t25000")+@($variants.Keys | ForEach-Object {"$_`t25000"})){if($rows -notcontains $expected){throw "Farewell budget mismatch: $expected"}}
foreach($kind in @('smart','creature')){
 $before=@($rows | Where-Object {$_ -like "before-$kind`t*"} | ForEach-Object {$_.Substring(8+$kind.Length)})
 $after=@($rows | Where-Object {$_ -like "after-$kind`t*"} | ForEach-Object {$_.Substring(7+$kind.Length)})
 if(Compare-Object $before $after){throw "Unrelated $kind fields changed"}
}
[IO.File]::WriteAllLines("$out/conversation.tsv",[string[]]@($rows | Where-Object {$_ -like "creature`t*" -or $_ -like "line`t*"}))
[IO.File]::WriteAllLines("$out/rows.tsv",[string[]]@($rows | Where-Object {$_ -like "row`t*"} | ForEach-Object {$_.Substring(4)}))
[IO.File]::WriteAllLines("$out/conditions.tsv",[string[]]@($rows | Where-Object {$_ -like "gate`t*"} | ForEach-Object {$_.Substring(5)}))
& "$repo/build-extractors/runtime-test/marksmanship-farewell/test.exe" "$out/conversation.tsv";if($LASTEXITCODE){throw 'Native50300ms conversation lifetime failed'}
& "$repo/build-extractors/runtime-test/marksmanship-orestes-farewell/test.exe" "$out/rows.tsv" "$out/conditions.tsv";if($LASTEXITCODE){throw 'Native50500ms Orestes transition failed'}
if($VerifyReleaseBinary){
 $previousPython=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previousPython";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_orestes_farewell_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Canonical farewell budget PE verification failed'}}finally{$env:PYTHONPATH=$previousPython}
}
'PASS:50300ms native conversation lifetime plus actual200ms queue fits50500ms transition delay; migration twice-idempotent, every other actor/conversation field and custom variants preserved'
