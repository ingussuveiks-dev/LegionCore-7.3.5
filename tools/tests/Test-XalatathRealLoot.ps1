param([switch]$Baseline,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/xalatath-real-loot"
New-Item -ItemType Directory -Force $out | Out-Null
$migration=Get-Content "$repo/sql/updates/world/2026_10_24_36_xalatath_real_loot.sql" -Raw
$reset=@'
DELETE FROM gameobject_template; INSERT INTO gameobject_template SELECT * FROM saved_object;
UPDATE gameobject_template SET Data1=0,ScriptName='' WHERE entry=247690;
DELETE FROM gameobject_loot_template; INSERT INTO gameobject_loot_template SELECT * FROM saved_loot;
DELETE FROM smart_scripts; INSERT INTO smart_scripts SELECT * FROM saved_smart;
DELETE FROM conditions; INSERT INTO conditions SELECT * FROM saved_conditions;
DELETE FROM conditions WHERE SourceTypeOrReferenceId=4 AND SourceGroup=247690;
'@
$sql=@"
CREATE TEMPORARY TABLE saved_object LIKE gameobject_template;
INSERT INTO saved_object SELECT * FROM gameobject_template WHERE entry=247690;
CREATE TEMPORARY TABLE gameobject_template LIKE saved_object;
CREATE TEMPORARY TABLE saved_loot LIKE gameobject_loot_template;
INSERT INTO saved_loot SELECT * FROM gameobject_loot_template WHERE Entry=247690;
CREATE TEMPORARY TABLE gameobject_loot_template LIKE saved_loot;
CREATE TEMPORARY TABLE saved_smart LIKE smart_scripts;
INSERT INTO saved_smart SELECT * FROM smart_scripts WHERE entryorguid=247690 AND source_type=1;
CREATE TEMPORARY TABLE smart_scripts LIKE saved_smart;
CREATE TEMPORARY TABLE saved_conditions LIKE conditions;
INSERT INTO saved_conditions SELECT * FROM conditions WHERE SourceEntry=247690 OR (SourceTypeOrReferenceId=4 AND SourceGroup IN(247690,248419));
CREATE TEMPORARY TABLE conditions LIKE saved_conditions;
$reset
SELECT 'before-object',gameobject_template.* FROM gameobject_template;
SELECT 'before-smart',smart_scripts.* FROM smart_scripts ORDER BY id;
SELECT 'before-loot',gameobject_loot_template.* FROM gameobject_loot_template;
SELECT 'before-other',conditions.* FROM conditions ORDER BY SourceTypeOrReferenceId,SourceGroup,SourceEntry,ElseGroup,ConditionTypeOrReference;
$migration
SELECT 'first-object',gameobject_template.* FROM gameobject_template;
SELECT 'first-conditions',conditions.* FROM conditions ORDER BY SourceTypeOrReferenceId,SourceGroup,SourceEntry,ElseGroup,ConditionTypeOrReference;
$migration
SELECT 'second-object',gameobject_template.* FROM gameobject_template;
SELECT 'second-conditions',conditions.* FROM conditions ORDER BY SourceTypeOrReferenceId,SourceGroup,SourceEntry,ElseGroup,ConditionTypeOrReference;
SELECT 'after-smart',smart_scripts.* FROM smart_scripts ORDER BY id;
SELECT 'after-loot',gameobject_loot_template.* FROM gameobject_loot_template;
SELECT 'after-other',conditions.* FROM conditions WHERE NOT(SourceTypeOrReferenceId=4 AND SourceGroup=247690) ORDER BY SourceTypeOrReferenceId,SourceGroup,SourceEntry,ElseGroup,ConditionTypeOrReference;
SELECT 'row',ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition FROM conditions WHERE SourceTypeOrReferenceId=4 AND SourceGroup=247690 ORDER BY ElseGroup,ConditionTypeOrReference;
"@
$variants=[ordered]@{
 'custom-ai'="UPDATE gameobject_template SET AIName='';"
 'custom-script'="UPDATE gameobject_template SET ScriptName='custom_blade';"
 'custom-type'='UPDATE gameobject_template SET type=10;'
 'custom-lock'='UPDATE gameobject_template SET Data0=1;'
 'custom-open-text'='UPDATE gameobject_template SET Data14=1;'
 'existing-binding'='UPDATE gameobject_template SET Data1=247690;'
 'custom-binding'='UPDATE gameobject_template SET Data1=1;'
 'missing-loot'='DELETE FROM gameobject_loot_template;'
 'custom-item'='UPDATE gameobject_loot_template SET Item=1;'
 'custom-reference'='UPDATE gameobject_loot_template SET Reference=1;'
 'custom-chance'='UPDATE gameobject_loot_template SET Chance=50;'
 'custom-count'='UPDATE gameobject_loot_template SET MaxCount=2;'
 'custom-root'='UPDATE smart_scripts SET action_param1=1 WHERE id=0;'
 'invalid-link'='UPDATE smart_scripts SET event_type=70,event_param1=3 WHERE id=1;'
 'custom-conversation'='UPDATE smart_scripts SET action_param1=1 WHERE id=4;'
 'missing-row'='DELETE FROM smart_scripts WHERE id=3;'
 'existing-condition'="INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,ConditionTypeOrReference,ConditionValue1) VALUES(4,247690,128827,4,1);"
}
foreach($name in $variants.Keys){
 $sql+="$reset`n$($variants[$name])`nSELECT '$name-before',gameobject_template.* FROM gameobject_template;`nSELECT '$name-cond-before',conditions.* FROM conditions ORDER BY SourceTypeOrReferenceId,SourceGroup,SourceEntry,ElseGroup,ConditionTypeOrReference;`n$migration`nSELECT '$name-after',gameobject_template.* FROM gameobject_template;`nSELECT '$name-cond-after',conditions.* FROM conditions ORDER BY SourceTypeOrReferenceId,SourceGroup,SourceEntry,ElseGroup,ConditionTypeOrReference;`n"
}
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
function Query([string]$query){$previous=$env:MYSQL_PWD;try{$env:MYSQL_PWD=$connection[3];$result=@($query|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Xalatath real loot TEMP query failed'};return $result}finally{$env:MYSQL_PWD=$previous}}
$rows=Query $sql
function Snapshot([string]$label){@($rows|Where-Object {$_ -like "$label`t*"}|ForEach-Object {$_.Substring($label.Length+1)})}
if(Compare-Object (Snapshot 'first-object') (Snapshot 'second-object')){throw 'Binding not idempotent'}
if(Compare-Object (Snapshot 'first-conditions') (Snapshot 'second-conditions')){throw 'Conditions not idempotent'}
$columns=@(Query 'SHOW COLUMNS FROM gameobject_template;'|ForEach-Object {$_.Split("`t")[0]})
$expected=@(Snapshot 'before-object'|ForEach-Object {$fields=$_.Split("`t");$fields[$columns.IndexOf('Data1')]='247690';$fields -join "`t"})
if(Compare-Object $expected (Snapshot 'first-object')){throw 'Changed object fields beyond Data1'}
foreach($kind in @('smart','loot','other')){if(Compare-Object (Snapshot "before-$kind") (Snapshot "after-$kind")){throw "Changed native $kind"}}
foreach($name in $variants.Keys){
 if(Compare-Object (Snapshot "$name-before") (Snapshot "$name-after")){throw "Custom object changed: $name"}
 if(Compare-Object (Snapshot "$name-cond-before") (Snapshot "$name-cond-after")){throw "Custom conditions changed: $name"}
}
$conditions=Snapshot 'row';if($conditions.Count -ne 20){throw 'Expected four five-rule admission alternatives'}
[IO.File]::WriteAllLines("$out/conditions.tsv",[string[]]$conditions)
& "$PSScriptRoot/Test-TalonclawPickupAdmission.ps1" -XalatathRowsPath "$out/conditions.tsv" -XalatathLootId $(if($Baseline){0}else{247690}) *> "$out/native.log"
if($LASTEXITCODE){throw 'Native real-loot binding/admission failed; see native.log'}
$previous=$env:LEGION_DIALOGUE_CONFIG
try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile XalatathPickupCatalog.cpp -ProbeName XalatathPickupCatalog -OutputFile "$out/native.tsv" *> "$out/catalog.log"}finally{$env:LEGION_DIALOGUE_CONFIG=$previous}
$native=Get-Content "$out/native.tsv" -Raw
foreach($expected in @("ITEM`t128827`t1`t1`t-1`t47","CHILD`t128827`t133958`t16","CRITERION`t7`t29410`t92`t48421`t1","CRITERION`t8`t29458`t92`t48493`t0","CRITERION`t9`t29459`t92`t48528`t0",'SendTrigger("Smooth Phase Complete")','WEAPONID = 128827','WEAPONIDOFFHAND = 133958','WEAPONEFFECT = 6','CASTVISUALID = 52603','clone:EquipWeapon(WeaponSlot.MainHand, WEAPONID)','clone:EquipWeapon(WeaponSlot.OffHand, WEAPONIDOFFHAND)','scene:EndScene()')){if(!$native.Contains($expected)){throw "Native scene/stage finding changed: $expected"}}
$playerSource=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
$start=$playerSource.IndexOf('void Player::StoreLootItem(')
$store=$playerSource.Substring($start,$playerSource.IndexOf('void Player::learnSpellHighRank(',$start)-$start)
if($store.IndexOf('CanStoreNewItem(') -ge $store.IndexOf('StoreNewItem(dest,') -or !$store.Contains('if (msg != EQUIP_ERR_OK)') -or !$store.Contains('if (!newitem)')){throw 'Native capacity/creation failure path changed'}
if($VerifyReleaseBinary){
 $hash=(Get-FileHash "$repo/sql/updates/world/2026_10_24_36_xalatath_real_loot.sql" -Algorithm SHA1).Hash
 $live=Query "SELECT 'release',hash,state FROM updates WHERE name='2026_10_24_36_xalatath_real_loot.sql'; SELECT 'binding',type,Data0,Data1,Data14,AIName FROM gameobject_template WHERE entry=247690; SELECT 'live-row',ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition FROM conditions WHERE SourceTypeOrReferenceId=4 AND SourceGroup=247690 AND SourceEntry=128827 ORDER BY ElseGroup,ConditionTypeOrReference; SELECT 'scenario',ScenarioID,Class FROM scenario_data WHERE MapID=1539 AND Class=5;"
 foreach($expected in @("release`t$hash`tRELEASED","binding`t3`t1691`t247690`t23645`tSmartGameObjectAI","scenario`t991`t5")){if($live -notcontains $expected){throw "Live loot mismatch: $expected"}}
 $liveConditions=@($live|Where-Object {$_ -like "live-row`t*"}|ForEach-Object {$_.Substring(9)})
 if(Compare-Object $conditions $liveConditions){throw 'Live conditions differ from native-tested TEMP rows'}
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_xalatath_real_loot_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Canonical loot consumers missing'}}finally{$env:PYTHONPATH=$previous}
}
'PASS: genuine Xalatath loot247690->128827 bound with native active Priest40710/map1539/scenario991/8-9 item/slot admission;17 custom variants and all SmartAI/presentation payloads preserved. Equip callback/client traversal remain pending.'
