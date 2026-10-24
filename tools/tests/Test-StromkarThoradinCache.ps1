param([switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/stromkar-thoradin-cache"
New-Item -ItemType Directory -Force $out|Out-Null
& "$PSScriptRoot/Test-StromkarThoradinPresence.ps1" -VerifyReleaseBinary:$VerifyReleaseBinary *> "$out/presence.log"
& "$PSScriptRoot/Test-StromkarZakajz.ps1" -VerifyReleaseBinary:$VerifyReleaseBinary *> "$out/native-cache.log"
$migration=Get-Content "$repo/sql/updates/world/2026_10_24_52_stromkar_thoradin_guid_retry.sql" -Raw
$reset=@'
DELETE FROM creature_template;INSERT INTO creature_template SELECT * FROM saved_actor;
DELETE FROM smart_scripts;INSERT INTO smart_scripts SELECT * FROM saved_smart;
DELETE FROM conditions;INSERT INTO conditions SELECT * FROM saved_conditions;
'@
$sql=@'
CREATE TEMPORARY TABLE saved_actor LIKE creature_template;INSERT INTO saved_actor SELECT * FROM creature_template WHERE entry=104276;
CREATE TEMPORARY TABLE creature_template LIKE saved_actor;
CREATE TEMPORARY TABLE saved_smart LIKE smart_scripts;INSERT INTO saved_smart SELECT * FROM smart_scripts WHERE entryorguid=104276 AND source_type=0 AND id=9;
CREATE TEMPORARY TABLE smart_scripts LIKE saved_smart;
CREATE TEMPORARY TABLE saved_conditions LIKE conditions;INSERT INTO saved_conditions SELECT * FROM conditions WHERE (SourceTypeOrReferenceId=22 AND SourceEntry=104276) OR (SourceTypeOrReferenceId=17 AND SourceEntry=206456);
UPDATE saved_conditions SET ScriptName='' WHERE SourceTypeOrReferenceId=22 AND SourceGroup=10 AND SourceEntry=104276 AND SourceId=0 AND ConditionTypeOrReference=29 AND ScriptName='condition_stromkar_thoradin_absence';
CREATE TEMPORARY TABLE conditions LIKE saved_conditions;
'@
$sql+="$reset`nSELECT 'others-before',conditions.* FROM conditions WHERE NOT(SourceTypeOrReferenceId=22 AND ConditionTypeOrReference=29) ORDER BY SourceTypeOrReferenceId,ElseGroup,ConditionTypeOrReference;`n$migration`nSELECT 'first',conditions.* FROM conditions ORDER BY SourceTypeOrReferenceId,ElseGroup,ConditionTypeOrReference;`n$migration`nSELECT 'second',conditions.* FROM conditions ORDER BY SourceTypeOrReferenceId,ElseGroup,ConditionTypeOrReference;SELECT 'script',COUNT(*) FROM conditions WHERE ScriptName='condition_stromkar_thoradin_absence';SELECT 'others-after',conditions.* FROM conditions WHERE NOT(SourceTypeOrReferenceId=22 AND ConditionTypeOrReference=29) ORDER BY SourceTypeOrReferenceId,ElseGroup,ConditionTypeOrReference;`n"
$variants=[ordered]@{
 'custom-owner'="UPDATE creature_template SET ScriptName='custom';"
 'custom-root'='UPDATE smart_scripts SET action_param1=103144;'
 'custom-timer'='UPDATE smart_scripts SET event_param3=250;'
 'custom-gate-script'="UPDATE conditions SET ScriptName='custom' WHERE SourceTypeOrReferenceId=22 AND ConditionTypeOrReference=29;"
 'custom-target'='UPDATE conditions SET ConditionTarget=0 WHERE SourceTypeOrReferenceId=22 AND ConditionTypeOrReference=29;'
 'custom-entry'='UPDATE conditions SET ConditionValue1=103144 WHERE SourceTypeOrReferenceId=22 AND ConditionTypeOrReference=29;'
 'custom-range'='UPDATE conditions SET ConditionValue2=100 WHERE SourceTypeOrReferenceId=22 AND ConditionTypeOrReference=29;'
 'custom-positive'='UPDATE conditions SET NegativeCondition=0 WHERE SourceTypeOrReferenceId=22 AND ConditionTypeOrReference=29;'
}
foreach($name in $variants.Keys){$sql+="$reset`n$($variants[$name])`nSELECT '$name-before',conditions.* FROM conditions ORDER BY SourceTypeOrReferenceId,ElseGroup,ConditionTypeOrReference;`n$migration`nSELECT '$name-after',conditions.* FROM conditions ORDER BY SourceTypeOrReferenceId,ElseGroup,ConditionTypeOrReference;`n"}
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
function Query([string]$query){$previous=$env:MYSQL_PWD;try{$env:MYSQL_PWD=$connection[3];$result=@($query|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Cache TEMP query failed'};return $result}finally{$env:MYSQL_PWD=$previous}}
$rows=Query $sql
[IO.File]::WriteAllLines("$out/fixture-diagnostics.tsv",[string[]]$rows)
function Snapshot([string]$label){[string]::Join("`n",[string[]]@($rows|Where-Object {$_ -like "$label`t*"}|ForEach-Object {$_.Substring($label.Length+1)}))}
if($rows -notcontains "script`t1"){throw 'Expected one scoped absence script'}
foreach($pair in @(@('first','second'),@('others-before','others-after'))){if((Snapshot $pair[0]) -cne (Snapshot $pair[1])){throw "Cache changed $pair"}}
foreach($name in $variants.Keys){if((Snapshot "$name-before") -cne (Snapshot "$name-after")){throw "Custom cache overwritten: $name"}}
if($VerifyReleaseBinary){
 $hash=(Get-FileHash "$repo/sql/updates/world/2026_10_24_52_stromkar_thoradin_guid_retry.sql" -Algorithm SHA1).Hash
 $live=Query "SELECT 'release',hash,state FROM updates WHERE name='2026_10_24_52_stromkar_thoradin_guid_retry.sql';SELECT 'bound',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=10 AND SourceEntry=104276 AND SourceId=0 AND ElseGroup=0 AND ConditionTypeOrReference=29 AND ConditionTarget=1 AND ConditionValue1=104307 AND ConditionValue2=50 AND ConditionValue3=0 AND NegativeCondition=1 AND ErrorTextId=0 AND ScriptName='condition_stromkar_thoradin_absence';"
 if($live -notcontains "release`t$hash`tRELEASED" -or $live -notcontains "bound`t1"){throw 'Live52 hash/binding differs'}
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_stromkar_thoradin_cache_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Canonical GUID cache failed'}}finally{$env:PYTHONPATH=$previous}
}
'PASS: combined51/52 creation preserves the real summon callback and tests native condition conjunction with a range-independent live actor GUID; missing/dead/uninserted actors retry. Far-boss final-dialogue travel and actual client VFX remain unverified.'
