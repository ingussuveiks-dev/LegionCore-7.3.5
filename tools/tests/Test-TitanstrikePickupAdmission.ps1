param([switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/titanstrike-pickup-admission"
New-Item -ItemType Directory -Force $out | Out-Null
$migration=Get-Content "$repo/sql/updates/world/2026_10_24_15_titanstrike_pickup_admission.sql" -Raw
$scope="((SourceTypeOrReferenceId=4 AND SourceGroup=249812 AND SourceEntry=128861 AND SourceId=0) OR (SourceTypeOrReferenceId=22 AND SourceGroup=1 AND SourceEntry=249812 AND SourceId=1))"
$reset=@"
DELETE FROM conditions WHERE $scope;
DELETE FROM smart_scripts;
INSERT INTO smart_scripts SELECT * FROM saved_smart;
UPDATE gameobject_template current JOIN saved_go old USING(entry) SET current.ScriptName=old.ScriptName,current.AIName=old.AIName;
UPDATE gameobject_loot_template current JOIN saved_loot old USING(Entry,Item) SET current.Reference=old.Reference,current.Chance=old.Chance;
"@
$sql=@"
CREATE TEMPORARY TABLE saved_go LIKE gameobject_template;
INSERT INTO saved_go SELECT * FROM gameobject_template WHERE entry=249812;
CREATE TEMPORARY TABLE gameobject_template LIKE saved_go;
INSERT INTO gameobject_template SELECT * FROM saved_go;
CREATE TEMPORARY TABLE saved_loot LIKE gameobject_loot_template;
INSERT INTO saved_loot SELECT * FROM gameobject_loot_template WHERE Entry=249812;
CREATE TEMPORARY TABLE gameobject_loot_template LIKE saved_loot;
INSERT INTO gameobject_loot_template SELECT * FROM saved_loot;
CREATE TEMPORARY TABLE saved_smart LIKE smart_scripts;
INSERT INTO saved_smart SELECT * FROM smart_scripts WHERE entryorguid=249812 AND source_type=1;
CREATE TEMPORARY TABLE smart_scripts LIKE saved_smart;
INSERT INTO smart_scripts SELECT * FROM saved_smart;
CREATE TEMPORARY TABLE saved_conditions LIKE conditions;
INSERT INTO saved_conditions SELECT * FROM conditions WHERE (SourceTypeOrReferenceId=4 AND SourceGroup IN(249812,248419)) OR (SourceTypeOrReferenceId=22 AND SourceEntry=249812 AND SourceId=1);
CREATE TEMPORARY TABLE conditions LIKE saved_conditions;
INSERT INTO conditions SELECT * FROM saved_conditions;
$reset
SELECT 'baseline-count',COUNT(*) FROM conditions WHERE $scope;
SELECT 'before-smart',current.* FROM smart_scripts current ORDER BY id;
$migration
$migration
SELECT 'after-smart',current.* FROM smart_scripts current ORDER BY id;
SELECT 'loot-count',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=4 AND SourceGroup=249812 AND SourceEntry=128861;
SELECT 'root-count',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=1 AND SourceEntry=249812 AND SourceId=1;
SELECT 'loot-row',ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition FROM conditions WHERE SourceTypeOrReferenceId=4 AND SourceGroup=249812 AND SourceEntry=128861 ORDER BY ElseGroup,ConditionTypeOrReference;
SELECT 'root-row',ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=1 AND SourceEntry=249812 AND SourceId=1 ORDER BY ElseGroup,ConditionTypeOrReference;
SELECT 'object',COUNT(*) FROM gameobject_template current JOIN saved_go old USING(entry) WHERE current.Data0=old.Data0 AND current.Data1=old.Data1 AND current.flags=old.flags AND current.displayId=old.displayId AND current.AIName=old.AIName AND current.ScriptName=old.ScriptName;
SELECT 'loot',COUNT(*) FROM gameobject_loot_template current JOIN saved_loot old USING(Entry,Item) WHERE current.Currency=old.Currency AND current.Reference=old.Reference AND current.Chance=old.Chance AND current.MinCount=old.MinCount AND current.MaxCount=old.MaxCount;
SELECT 'other',COUNT(*) FROM conditions current JOIN saved_conditions old ON current.SourceTypeOrReferenceId=old.SourceTypeOrReferenceId AND current.SourceGroup=old.SourceGroup AND current.SourceEntry=old.SourceEntry AND current.SourceId=old.SourceId AND current.ElseGroup=old.ElseGroup AND current.ConditionTypeOrReference=old.ConditionTypeOrReference AND current.ConditionTarget=old.ConditionTarget AND current.ConditionValue1=old.ConditionValue1 AND current.ConditionValue2=old.ConditionValue2 AND current.ConditionValue3=old.ConditionValue3 AND current.NegativeCondition=old.NegativeCondition WHERE NOT ((current.SourceTypeOrReferenceId=4 AND current.SourceGroup=249812 AND current.SourceEntry=128861 AND current.SourceId=0) OR (current.SourceTypeOrReferenceId=22 AND current.SourceGroup=1 AND current.SourceEntry=249812 AND current.SourceId=1));
SELECT 'saved-other',COUNT(*) FROM saved_conditions WHERE NOT $scope;
"@
$variants=[ordered]@{
 'custom-script'="UPDATE gameobject_template SET ScriptName='custom_pickup';"
 'custom-ai'="UPDATE gameobject_template SET AIName='CustomAI';"
 'custom-spell'='UPDATE smart_scripts SET action_param1=123 WHERE id=0;'
 'custom-chance'='UPDATE smart_scripts SET event_chance=50 WHERE id=0;'
 'custom-stage'='UPDATE smart_scripts SET event_phase_mask=1 WHERE id=0;'
 'custom-link'='UPDATE smart_scripts SET link=0 WHERE id=0;'
 'custom-bird'='UPDATE smart_scripts SET action_param1=123 WHERE id=3;'
 'custom-root'='UPDATE smart_scripts SET event_type=64 WHERE id=0;'
 'custom-extra'='INSERT INTO smart_scripts(entryorguid,source_type,id,event_type,event_chance,action_type,target_type,comment) VALUES(249812,1,20,64,100,1,1,''Custom added event'');'
 'custom-loot'='UPDATE gameobject_loot_template SET Reference=123;'
}
foreach($name in $variants.Keys){$sql+="$reset`n$($variants[$name])`n$migration`nSELECT '$name',COUNT(*) FROM conditions WHERE $scope;`n"}
$sql+=@"
$reset
INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,Comment) VALUES(22,1,249812,1,0,4,0,1,'Custom root');
$migration
SELECT 'existing-root',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=1 AND SourceEntry=249812 AND SourceId=1;
SELECT 'existing-root-kept',COUNT(*) FROM conditions WHERE Comment='Custom root' AND ConditionValue1=1;
$reset
INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,Comment) VALUES(4,249812,128861,0,0,4,0,1,'Custom loot');
$migration
SELECT 'existing-loot',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=4 AND SourceGroup=249812 AND SourceEntry=128861;
SELECT 'existing-loot-kept',COUNT(*) FROM conditions WHERE Comment='Custom loot' AND ConditionValue1=1;
"@
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$connection[3];$rows=@($sql|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Titanstrike TEMP fixture failed'}}finally{$env:MYSQL_PWD=$previous}
[IO.File]::WriteAllLines("$out/fixture-diagnostics.tsv",[string[]]$rows)
foreach($expected in @("baseline-count`t0","loot-count`t20","root-count`t24","object`t1","loot`t1","existing-root`t1","existing-root-kept`t1","existing-loot`t1","existing-loot-kept`t1")+@($variants.Keys|ForEach-Object {"$_`t0"})){if($rows -notcontains $expected){throw "Titanstrike admission mismatch: $expected"}}
$before=@($rows|Where-Object {$_ -like "before-smart`t*"}|ForEach-Object {$_.Substring(13)})
$after=@($rows|Where-Object {$_ -like "after-smart`t*"}|ForEach-Object {$_.Substring(12)})
if(Compare-Object $before $after){throw 'Native pickup actions changed'}
if((($rows|Where-Object {$_ -like "other`t*"}) -replace '^other','') -ne (($rows|Where-Object {$_ -like "saved-other`t*"}) -replace '^saved-other','')){throw 'Unrelated conditions changed'}
foreach($kind in @('loot','root')){
 $path="$out/$kind.tsv"
 [IO.File]::WriteAllLines($path,[string[]]@($rows|Where-Object {$_ -like "$kind-row`t*"}|ForEach-Object {$_.Substring($kind.Length+5)}))
 & "$PSScriptRoot/Test-TalonclawPickupAdmission.ps1" -TitanstrikeRowsPath $path -TitanstrikeRoot:($kind -eq 'root')
}
if($VerifyReleaseBinary){
 $previousPython=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previousPython";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_titanstrike_pickup_admission_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Titanstrike deployed guard verification failed'}}finally{$env:PYTHONPATH=$previousPython}
}
'PASS: repeated actual-schema Titanstrike source4/root admission retains native scene/criterion/bird/cleanup, custom definitions and unrelated loot; actual native conditions and cached-slot recheck admit only1099/4-5 active Hunter42185'
