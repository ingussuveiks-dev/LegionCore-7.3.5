param([string]$MigrationFile,[switch]$VerifyLiveData,[string]$OutputDirectory="$PSScriptRoot/../../build-extractors/runtime-test/fuzan-belphiar-data")
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..");$out=[IO.Path]::GetFullPath($OutputDirectory)
if(!$MigrationFile){$MigrationFile="$repo/sql/pending/world/2026_11_01_05_fuzan_belphiar_admission.sql";if(!(Test-Path $MigrationFile)){$MigrationFile="$repo/sql/updates/world/2026_11_01_05_fuzan_belphiar_admission.sql"}}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -RuntimeDirectories @($repo) -Test {
 $migration=Get-Content $MigrationFile -Raw;$fixture=Get-Content "$PSScriptRoot/FuZanBelphiarOriginal.json" -Raw|ConvertFrom-Json
 $connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $script:index=0
 function Query([string]$sql,[switch]$Refusal){
  $stem="$out/sql-$script:index";++$script:index;[IO.File]::WriteAllText("$stem.sql",$sql+"`nSELECT 'EOF','fuzan-belphiar-data';`n")
  $prior=$env:MYSQL_PWD
  try{
   $env:MYSQL_PWD=$connection[3]
   $code=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -Arguments @("--host=$($connection[0])","--port=$($connection[1])","--user=$($connection[2])","--database=$($connection[4])",'--default-character-set=utf8mb4','--batch','--raw','--skip-column-names','--force') -InputFile "$stem.sql" -OutputFile "$stem.stdout" -ErrorFile "$stem.stderr"
   $rows=@(Get-Content "$stem.stdout" -Encoding UTF8);$errors=[IO.File]::ReadAllText("$stem.stderr")
   if($rows[-1] -cne "EOF`tfuzan-belphiar-data"){throw 'Native Belphiar SQL incomplete EOF'}
   if($Refusal){if($errors -notmatch 'fuzan_belphiar_native_guard' -or @([regex]::Matches($errors,'ERROR [0-9]+')).Count -ne 1){throw "Unexpected refusal diagnostics: $stem"}}
   elseif($code -or $errors){throw "Belphiar SQL failed: $stem"}
   return ,$rows
  }finally{$env:MYSQL_PWD=$prior}
 }
 $filters=[ordered]@{creature_template='entry=109397';creature_template_wdb='entry=109397';creature='id=109397';smart_scripts='entryorguid IN(109397,-373405) AND source_type IN(0,9)';conditions='SourceTypeOrReferenceId=22 AND SourceEntry IN(109397,-109397)';creature_template_addon='entry=109397';creature_addon='guid=373405';creature_template_scaling='Entry=109397';creature_loot_template='Entry=109397'}
 $orders=@{creature_template='entry';creature_template_wdb='entry';creature='guid';smart_scripts='entryorguid,source_type,id';conditions='SourceEntry,SourceGroup,ElseGroup';creature_template_addon='entry';creature_addon='guid';creature_template_scaling='Entry';creature_loot_template='Entry,Item'}
 $setup='';foreach($table in $filters.Keys){$setup+="CREATE TEMPORARY TABLE saved_$table LIKE $table;INSERT INTO saved_$table SELECT * FROM $table WHERE $($filters[$table]);CREATE TEMPORARY TABLE $table LIKE saved_$table;INSERT INTO $table SELECT * FROM saved_$table;`n"}
 $restore="UPDATE creature_template SET ScriptName='' WHERE entry=109397 AND ScriptName='npc_fuzan_belphiar';`n"
 $expected="UPDATE creature_template SET ScriptName='npc_fuzan_belphiar' WHERE entry=109397;`n"
 $certified=$setup+$migration+$restore
 function Snapshot([string]$tag){$result='';foreach($table in $filters.Keys){$result+="SELECT '$tag-$table',t.* FROM $table t WHERE $($filters[$table]) ORDER BY $($orders[$table]);`n"};$result}
 function Signature($rows,[string]$tag){[string]::Join("`n",[string[]]@($rows|Where-Object {$_ -like "$tag`t*"}|ForEach-Object {$_.Substring($tag.Length+1)}))}
 $pins='';foreach($table in $filters.Keys){if(!@($fixture.$table).Count){continue};$columns=@($fixture.$table[0].PSObject.Properties.Name|ForEach-Object {'`'+$_+'`'}) -join ',';$pins+="SELECT 'pin-$table',$columns FROM $table WHERE $($filters[$table]) ORDER BY $($orders[$table]);`n"}
 $rows=Query ($certified+$pins+(Snapshot 'original')+$expected+(Snapshot 'expected')+$restore+$migration+(Snapshot 'first')+$migration+(Snapshot 'second'))
 foreach($table in $filters.Keys){
  if((Signature $rows "expected-$table") -cne (Signature $rows "first-$table") -or (Signature $rows "first-$table") -cne (Signature $rows "second-$table")){throw "Native one-field delta/idempotence differs: $table"}
  if($table -ne 'creature_template' -and (Signature $rows "original-$table") -cne (Signature $rows "first-$table")){throw "Unrelated native rows changed: $table"}
  if(@($fixture.$table).Count){$pin=@($fixture.$table|ForEach-Object {($_.PSObject.Properties.Value|ForEach-Object {if($null -eq $_){'NULL'}else{[Convert]::ToString($_,[Globalization.CultureInfo]::InvariantCulture)}}) -join "`t"});if(($pin -join "`n") -cne (Signature $rows "pin-$table")){throw "Pinned native context differs: $table"}}
 }
 $variants=[ordered]@{
  'custom-script'="UPDATE creature_template SET ScriptName='custom';"
  'wrong-ai'="UPDATE creature_template SET AIName='';"
  'custom-faction'='UPDATE creature_template SET faction=35;'
  'custom-flags'='UPDATE creature_template SET unit_flags=0;'
  'custom-flags2'='UPDATE creature_template SET unit_flags2=1;'
  'custom-health'='UPDATE creature_template_wdb SET HpMulti=HpMulti+1;'
  'custom-combat-spell'='UPDATE smart_scripts SET action_param1=1 WHERE id=2;'
  'custom-credit'='UPDATE smart_scripts SET action_param2=1 WHERE id=4;'
  'custom-repeat'='UPDATE smart_scripts SET event_flags=0 WHERE id=4;'
  'custom-target'='UPDATE smart_scripts SET target_type=7 WHERE id=4;'
  'custom-link'='UPDATE smart_scripts SET link=6 WHERE id=4;'
  'custom-summon'='UPDATE smart_scripts SET action_param1=109393 WHERE id=5;'
  'custom-summon-mode'='UPDATE smart_scripts SET action_param2=1 WHERE id=5;'
  'custom-summon-position'='UPDATE smart_scripts SET target_x=target_x+1 WHERE id=5;'
  'custom-comment'="UPDATE smart_scripts SET comment='custom' WHERE id=4;"
  'missing-native-summon'='DELETE FROM smart_scripts WHERE id=5;'
  'spawn-smart-override'='CREATE TEMPORARY TABLE extra_smart LIKE smart_scripts;INSERT INTO extra_smart SELECT * FROM smart_scripts;UPDATE extra_smart SET entryorguid=-373405;INSERT INTO smart_scripts SELECT * FROM extra_smart;'
  'timed-smart-override'='CREATE TEMPORARY TABLE extra_smart LIKE smart_scripts;INSERT INTO extra_smart SELECT * FROM smart_scripts;UPDATE extra_smart SET source_type=9;INSERT INTO smart_scripts SELECT * FROM extra_smart;'
  'wrong-native-admission'='UPDATE conditions SET ConditionValue2=1;'
  'wrong-map'='UPDATE creature SET map=1;'
  'wrong-phase'="UPDATE creature SET PhaseId='1';"
  'wrong-position'='UPDATE creature SET position_x=position_x+1;'
  'wrong-source-guid'='UPDATE creature SET guid=373406;'
  'duplicate-spawn'='CREATE TEMPORARY TABLE extra_spawn LIKE creature;INSERT INTO extra_spawn SELECT * FROM creature;UPDATE extra_spawn SET guid=373406;INSERT INTO creature SELECT * FROM extra_spawn;'
  'template-addon'='INSERT INTO creature_template_addon(entry) VALUES(109397);'
  'spawn-addon'='INSERT INTO creature_addon(guid) VALUES(373405);'
  'invalid-level-scaling'='UPDATE creature_template_scaling SET LevelScalingMin=0,LevelScalingMax=0;'
  'custom-loot'='UPDATE creature_loot_template SET Chance=50;'
 }
 foreach($name in $variants.Keys){$bad=Query ($certified+$variants[$name]+(Snapshot 'before')+$migration+(Snapshot 'after')) -Refusal;foreach($table in $filters.Keys){if((Signature $bad "before-$table") -cne (Signature $bad "after-$table")){throw "Refusal mutated rows: $name/$table"}};"PASS refusal $name"}
 if($VerifyLiveData){$live=Query (Snapshot 'live');foreach($table in $filters.Keys){if((Signature $live "live-$table") -cne (Signature $rows "first-$table")){throw "Actual live Belphiar differs: $table"}}}
 'PASS full native Belphiar TEMP9: only ScriptName changes; six SmartAI rows/one condition/source/scaling/loot/native models retained, idempotence and28 strict refusals; optional full live equality'
}
