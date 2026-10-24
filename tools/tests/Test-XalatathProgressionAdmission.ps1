param([switch]$Baseline,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/xalatath-progression-admission"
New-Item -ItemType Directory -Force $out | Out-Null
$migration=Get-Content "$repo/sql/updates/world/2026_10_24_38_xalatath_progression_admission.sql" -Raw
$reset=@'
DELETE FROM gameobject_template; INSERT INTO gameobject_template SELECT * FROM saved_object;
UPDATE gameobject_template SET ScriptName='' WHERE entry=247690;
DELETE FROM smart_scripts; INSERT INTO smart_scripts SELECT * FROM saved_smart;
DELETE FROM conditions; INSERT INTO conditions SELECT * FROM saved_conditions;
DELETE FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=247690 AND SourceId=1;
INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment)
VALUES(22,3,247690,1,0,2,0,128827,1,0,0,0,'','Smart Only If Player Has Artifact');
'@
$sql=@"
CREATE TEMPORARY TABLE saved_object LIKE gameobject_template;
INSERT INTO saved_object SELECT * FROM gameobject_template WHERE entry=247690;
CREATE TEMPORARY TABLE gameobject_template LIKE saved_object;
CREATE TEMPORARY TABLE saved_smart LIKE smart_scripts;
INSERT INTO saved_smart SELECT * FROM smart_scripts WHERE entryorguid=247690 AND source_type=1;
CREATE TEMPORARY TABLE smart_scripts LIKE saved_smart;
CREATE TEMPORARY TABLE saved_conditions LIKE conditions;
INSERT INTO saved_conditions SELECT * FROM conditions WHERE SourceEntry=247690 OR (SourceTypeOrReferenceId=4 AND SourceGroup=247690);
CREATE TEMPORARY TABLE conditions LIKE saved_conditions;
$reset
SELECT 'object-before',gameobject_template.* FROM gameobject_template;
SELECT 'smart-before',smart_scripts.* FROM smart_scripts ORDER BY id;
SELECT 'other-before',conditions.* FROM conditions WHERE SourceTypeOrReferenceId<>22 ORDER BY SourceTypeOrReferenceId,SourceGroup,SourceEntry,ElseGroup,ConditionTypeOrReference;
$migration
SELECT 'first',conditions.* FROM conditions ORDER BY SourceTypeOrReferenceId,SourceGroup,SourceEntry,ElseGroup,ConditionTypeOrReference;
$migration
SELECT 'second',conditions.* FROM conditions ORDER BY SourceTypeOrReferenceId,SourceGroup,SourceEntry,ElseGroup,ConditionTypeOrReference;
SELECT 'object-after',gameobject_template.* FROM gameobject_template;
SELECT 'smart-after',smart_scripts.* FROM smart_scripts ORDER BY id;
SELECT 'other-after',conditions.* FROM conditions WHERE SourceTypeOrReferenceId<>22 ORDER BY SourceTypeOrReferenceId,SourceGroup,SourceEntry,ElseGroup,ConditionTypeOrReference;
SELECT 'row',SourceGroup,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=247690 AND SourceId=1 ORDER BY SourceGroup,ElseGroup,ConditionTypeOrReference;
"@
$variants=[ordered]@{
 'custom-ai'="UPDATE gameobject_template SET AIName='';"
 'custom-script'="UPDATE gameobject_template SET ScriptName='custom';"
 'missing-loot-binding'='UPDATE gameobject_template SET Data1=0;'
 'invalid-link'='UPDATE smart_scripts SET event_type=70,event_param1=3 WHERE id=1;'
 'custom-scene'='UPDATE smart_scripts SET action_param1=1 WHERE id=0;'
 'custom-criterion'='UPDATE smart_scripts SET action_param2=1 WHERE id=1;'
 'custom-extra-action'='UPDATE smart_scripts SET action_param1=1 WHERE id=3;'
 'custom-conversation'='UPDATE smart_scripts SET action_param1=1 WHERE id=4;'
 'custom-target'='UPDATE smart_scripts SET target_type=21 WHERE id=2;'
 'missing-action'='DELETE FROM smart_scripts WHERE id=3;'
 'custom-item'='UPDATE conditions SET ConditionValue1=1 WHERE SourceTypeOrReferenceId=22;'
 'custom-bank'='UPDATE conditions SET ConditionValue3=1 WHERE SourceTypeOrReferenceId=22;'
 'custom-error'='UPDATE conditions SET ErrorTextId=1 WHERE SourceTypeOrReferenceId=22;'
 'extra-condition'='INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ConditionTypeOrReference,ConditionValue1) VALUES(22,1,247690,1,9,40710);'
}
foreach($name in $variants.Keys){$sql+="$reset`n$($variants[$name])`nSELECT '$name-before',conditions.* FROM conditions ORDER BY SourceTypeOrReferenceId,SourceGroup,SourceEntry,ElseGroup,ConditionTypeOrReference;`n$migration`nSELECT '$name-after',conditions.* FROM conditions ORDER BY SourceTypeOrReferenceId,SourceGroup,SourceEntry,ElseGroup,ConditionTypeOrReference;`n"}
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
function Query([string]$query){$previous=$env:MYSQL_PWD;try{$env:MYSQL_PWD=$connection[3];$result=@($query|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Progression TEMP query failed'};return $result}finally{$env:MYSQL_PWD=$previous}}
$rows=Query $sql
function Snapshot([string]$label){@($rows|Where-Object {$_ -like "$label`t*"}|ForEach-Object {$_.Substring($label.Length+1)})}
if(Compare-Object (Snapshot 'first') (Snapshot 'second')){throw 'Root conditions not idempotent'}
foreach($kind in @('object','smart','other')){if(Compare-Object (Snapshot "$kind-before") (Snapshot "$kind-after")){throw "Changed native $kind"}}
foreach($name in $variants.Keys){if(Compare-Object (Snapshot "$name-before") (Snapshot "$name-after")){throw "Custom variant changed: $name"}}
$nativeRows=Snapshot 'row';if($nativeRows.Count -ne 56){throw 'Expected56 root/gossip rules'}
foreach($group in @(1,3)){
 $subset=@($nativeRows|Where-Object {$_ -like "$group`t*"}|ForEach-Object {$_.Substring(2)})
 if($subset.Count -ne 28){throw 'Expected28 conditions for each native root'}
 [IO.File]::WriteAllLines("$out/group$group.tsv",[string[]]$subset)
 & "$PSScriptRoot/Test-TalonclawPickupAdmission.ps1" -XalatathRowsPath "$out/group$group.tsv" -XalatathRoot:($group -eq 1) -XalatathGossip:($group -eq 3) -XalatathBaseline:$Baseline *> "$out/group$group-native.log"
 if($LASTEXITCODE){throw "Native root$group admission failed"}
}
if($VerifyReleaseBinary){
 $hash=(Get-FileHash "$repo/sql/updates/world/2026_10_24_38_xalatath_progression_admission.sql" -Algorithm SHA1).Hash
 $live=Query "SELECT 'release',hash,state FROM updates WHERE name='2026_10_24_38_xalatath_progression_admission.sql'; SELECT 'row',SourceGroup,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=247690 AND SourceId=1 ORDER BY SourceGroup,ElseGroup,ConditionTypeOrReference;"
 if($live -notcontains "release`t$hash`tRELEASED"){throw 'Live migration hash mismatch'}
 $liveRows=@($live|Where-Object {$_ -like "row`t*"}|ForEach-Object {$_.Substring(4)})
 if(Compare-Object $nativeRows $liveRows){throw 'Live roots differ from native-tested TEMP rows'}
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_xalatath_progression_admission_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Canonical SmartAI admission missing'}}finally{$env:PYTHONPATH=$previous}
}
'PASS: both Xalatath scene/criterion/extra-action roots require live Priest40710/scenario991/8-9 and an actual nonbank blade;14 custom variants, source4 loot and all five native actions preserved. Pre-open/despawn and delayed-cast success remain separate.'
