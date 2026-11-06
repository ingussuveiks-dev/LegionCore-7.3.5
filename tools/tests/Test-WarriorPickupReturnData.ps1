param([string]$MigrationFile='',[switch]$VerifyLiveData)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if(!$MigrationFile){$MigrationFile="$repo/sql/pending/2026_11_06_04_warrior_pickup_return.sql";if(!(Test-Path -LiteralPath $MigrationFile)){$MigrationFile="$repo/sql/updates/world/2026_11_06_04_warrior_pickup_return.sql"}}
$out="$repo/build-extractors/runtime-test/warrior-pickup-return-data/$(Get-Date -Format yyyyMMdd-HHmmss-fff)";New-Item -ItemType Directory -Force $out|Out-Null
$migration=Get-Content -LiteralPath $MigrationFile -Raw
$setup=@'
CREATE TEMPORARY TABLE saved_conditions LIKE conditions;
INSERT INTO saved_conditions SELECT * FROM conditions WHERE SourceTypeOrReferenceId=26 AND SourceGroup=11869;
CREATE TEMPORARY TABLE conditions LIKE saved_conditions;
INSERT INTO conditions SELECT * FROM saved_conditions;
CREATE TEMPORARY TABLE saved_actor LIKE creature_template;
INSERT INTO saved_actor SELECT * FROM creature_template WHERE entry=93819;
CREATE TEMPORARY TABLE creature_template LIKE saved_actor;
INSERT INTO creature_template SELECT * FROM saved_actor;
CREATE TEMPORARY TABLE saved_area LIKE areatrigger_template;
INSERT INTO saved_area SELECT * FROM areatrigger_template WHERE customEntry=11869;
CREATE TEMPORARY TABLE areatrigger_template LIKE saved_area;
INSERT INTO areatrigger_template SELECT * FROM saved_area;
CREATE TEMPORARY TABLE saved_action LIKE areatrigger_actions;
INSERT INTO saved_action SELECT * FROM areatrigger_actions WHERE customEntry=11869;
CREATE TEMPORARY TABLE areatrigger_actions LIKE saved_action;
INSERT INTO areatrigger_actions SELECT * FROM saved_action;
'@
$db=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
function Run-Sql([string]$caseName,[string]$sql){
 [IO.File]::WriteAllText("$out/$caseName.sql",$sql)
 $prior=$env:MYSQL_PWD
 try{$env:MYSQL_PWD=$db[3];$code=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -Arguments @("--host=$($db[0])","--port=$($db[1])","--user=$($db[2])","--database=$($db[4])",'--batch','--raw','--skip-column-names') -InputFile "$out/$caseName.sql" -OutputFile "$out/$caseName.tsv" -ErrorFile "$out/$caseName.stderr";return $code}finally{$env:MYSQL_PWD=$prior}
}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out) -Test {
 $original="SELECT * FROM conditions WHERE SourceTypeOrReferenceId=26 AND SourceGroup=11869 ORDER BY SourceEntry,ElseGroup,ConditionTypeOrReference,ConditionTarget;SELECT * FROM creature_template WHERE entry=93819;SELECT * FROM areatrigger_template WHERE customEntry=11869;SELECT * FROM areatrigger_actions WHERE customEntry=11869 ORDER BY id;"
 if(Run-Sql 'original-before' $original){throw 'Original shared data read failed'}
 $keys='SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment'
 $hash="SHA2(GROUP_CONCAT(CONCAT_WS('|',$keys) ORDER BY SourceEntry,ElseGroup,ConditionTypeOrReference,ConditionTarget SEPARATOR '\n'),256)"
 $checks=@"
SELECT 'conditions',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=26 AND SourceGroup=11869;
SELECT 'scripts',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=26 AND SourceGroup=11869 AND ScriptName='condition_warrior_pickup_return';
SELECT 'leave',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=26 AND SourceGroup=11869 AND SourceEntry=1;
SELECT 'stromkar_unchanged',(SELECT $hash FROM conditions WHERE SourceEntry=0 AND ElseGroup<>0)=(SELECT $hash FROM saved_conditions WHERE SourceEntry=0 AND ElseGroup<>0);
SELECT 'objective',QuestID,ObjectID,Amount,Flags FROM quest_objectives WHERE QuestID IN(40043,39191) ORDER BY QuestID,ID;
"@
 if(Run-Sql 'repeat' ($setup+$migration+$migration+$checks)){throw 'TEMP warrior exit migration failed'}
 $rows=Get-Content "$out/repeat.tsv"
 foreach($row in @("conditions`t23","scripts`t2","leave`t2","stromkar_unchanged`t1")){if($row -notin $rows){throw "TEMP exit migration/readback differs:$row"}}
 foreach($row in @("39191`t96508`t1`t0","39191`t94851`t1`t2","39191`t103739`t1`t2","40043`t98032`t1`t0","40043`t98033`t1`t2","40043`t103739`t1`t2")){if("objective`t$row" -notin $rows){throw "Matching native quest receipt changed:$row"}}
 foreach($fault in @('custom_enter','custom_leave','visual')){
  $mutation=switch($fault){'custom_enter'{"UPDATE conditions SET ScriptName='local_custom' WHERE SourceEntry=0 AND ElseGroup=0;"}'custom_leave'{"UPDATE conditions SET ScriptName='local_custom' WHERE SourceEntry=1 AND ElseGroup=0;"}'visual'{"UPDATE areatrigger_template SET VisualID=0 WHERE customEntry=11869;"}}
  $code=Run-Sql $fault ($setup+$migration+$mutation+$migration)
  $errorText=Get-Content "$out/$fault.stderr" -Raw
  if(!$code -or $errorText -notmatch 'ERROR 1644 \(45000\)' -or $errorText -notmatch 'Warrior return actor/shared gaze predicates changed'){throw "Expected scoped migration rejection missing:$fault"}
 }
 if(Run-Sql 'original-after' $original){throw 'Original shared data readback failed'}
 if((Get-FileHash "$out/original-before.tsv").Hash -ne (Get-FileHash "$out/original-after.tsv").Hash){throw 'Live shared data changed during TEMP checks'}
 if($VerifyLiveData){
  $migrationHash=(Get-FileHash -LiteralPath $MigrationFile -Algorithm SHA1).Hash.ToLowerInvariant()
  $live="SELECT 'update',hash,state FROM updates WHERE name='2026_11_06_04_warrior_pickup_return.sql';SELECT 'live',COUNT(*),SUM(ScriptName='condition_warrior_pickup_return'),SUM(SourceEntry=1) FROM conditions WHERE SourceTypeOrReferenceId=26 AND SourceGroup=11869;"
  if(Run-Sql 'live-readback' $live){throw 'Actual updater/conditions readback failed'}
  $readback=Get-Content "$out/live-readback.tsv"
  if("update`t$migrationHash`tRELEASED" -notin $readback -or "live`t23`t2`t2" -notin $readback){throw 'Live updater SHA1/state or23 actual conditions differ'}
  'PASS actual updater RELEASED/SHA1 and23 live predicates/two registered owner scripts/two leave branches; read-only readback, not client area traversal.'
 }
 'PASS connection-local MariaDB TEMP migration repeat/idempotency:23 predicates,two scoped owner scripts,all original Stromkar branches byte-hashed unchanged. EXPECTED_GUARD:custom enter/custom leave/native visual conflicts reject before further migration;no live SQL changed.'
}
