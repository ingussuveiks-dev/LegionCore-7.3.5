param([string]$MigrationFile,[switch]$VerifyLiveData,[string]$OutputDirectory="$PSScriptRoot/../../build-extractors/runtime-test/fuzan-summerpetal-data")
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..");$out=[IO.Path]::GetFullPath($OutputDirectory)
if(!$MigrationFile){$MigrationFile="$repo/sql/pending/world/2026_11_01_06_fuzan_summerpetal_continuation.sql";if(!(Test-Path $MigrationFile)){$MigrationFile="$repo/sql/updates/world/2026_11_01_06_fuzan_summerpetal_continuation.sql"}}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -RuntimeDirectories @($repo) -Test {
 $migration=Get-Content $MigrationFile -Raw;$fixture=Get-Content "$PSScriptRoot/FuZanSummerpetalOriginal.json" -Raw|ConvertFrom-Json
 $connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $script:index=0
 function Query([string]$sql,[switch]$Refusal){
  $stem="$out/sql-$script:index";++$script:index;[IO.File]::WriteAllText("$stem.sql",$sql+"`nSELECT 'EOF','fuzan-summerpetal-data';`n")
  $prior=$env:MYSQL_PWD
  try{
   $env:MYSQL_PWD=$connection[3]
   $code=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -Arguments @("--host=$($connection[0])","--port=$($connection[1])","--user=$($connection[2])","--database=$($connection[4])",'--default-character-set=utf8mb4','--batch','--raw','--skip-column-names','--force') -InputFile "$stem.sql" -OutputFile "$stem.stdout" -ErrorFile "$stem.stderr"
   $rows=@(Get-Content "$stem.stdout" -Encoding UTF8);$errors=[IO.File]::ReadAllText("$stem.stderr")
   if($rows[-1] -cne "EOF`tfuzan-summerpetal-data"){throw 'Native Summerpetal SQL incomplete EOF'}
   if($Refusal){if($errors -notmatch 'fuzan_summerpetal_native_guard' -or @([regex]::Matches($errors,'ERROR [0-9]+')).Count -ne 1){throw "Unexpected refusal diagnostics: $stem"}}
   elseif($code -or $errors){throw "Summerpetal SQL failed: $stem"}
   return ,$rows
  }finally{$env:MYSQL_PWD=$prior}
 }
 $filters=[ordered]@{creature_template='entry IN(109355,109397,109610)';creature_template_wdb='entry IN(109355,109397,109610)';creature='id IN(109355,109397,109610)';smart_scripts='entryorguid IN(109355,109397,109610,-373384,-373405) AND source_type IN(0,9)';conditions='SourceTypeOrReferenceId=22 AND SourceEntry IN(109355,109397,109610,-373384,-373405)';creature_template_addon='entry IN(109355,109397,109610)';creature_addon='guid IN(373384,373405)';creature_template_scaling='Entry IN(109355,109397,109610)';creature_loot_template='Entry IN(109355,109397,109610)';creature_text='CreatureID IN(109355,109397,109610)';waypoints='entry IN(109355,109610)'}
 $orders=@{creature_template='entry';creature_template_wdb='entry';creature='guid';smart_scripts='entryorguid,source_type,id';conditions='SourceEntry,SourceGroup,ElseGroup';creature_template_addon='entry';creature_addon='guid';creature_template_scaling='Entry';creature_loot_template='Entry,Item';creature_text='CreatureID,GroupID,ID';waypoints='entry,pointid'}
 $setup='';foreach($table in $filters.Keys){$setup+="CREATE TEMPORARY TABLE saved_$table LIKE $table;INSERT INTO saved_$table SELECT * FROM $table WHERE $($filters[$table]);CREATE TEMPORARY TABLE $table LIKE saved_$table;INSERT INTO $table SELECT * FROM saved_$table;`n"}
 $restore="UPDATE creature_template SET ScriptName='' WHERE entry=109355 AND ScriptName='npc_fuzan_summerpetal';DELETE FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry IN(109355,109610);`n"
 $expected="UPDATE creature_template SET ScriptName='npc_fuzan_summerpetal' WHERE entry=109355;INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment) VALUES(22,6,109355,0,0,31,1,3,109355,0,0,0,'condition_fuzan_summerpetal_continuation','Summerpetal actual acknowledged handoff');INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment) VALUES(22,7,109355,0,0,31,1,3,109355,0,0,0,'condition_fuzan_summerpetal_continuation','Summerpetal actual owned follow');INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment) VALUES(22,2,109610,0,0,44,1,1137,4,0,0,0,'','Monkey King native stage4 path admission');`n"
 $certified=$setup+$migration+$restore
 function Snapshot([string]$tag){$result='';foreach($table in $filters.Keys){$result+="SELECT '$tag-$table',t.* FROM $table t WHERE $($filters[$table]) ORDER BY $($orders[$table]);`n"};$result}
 function Signature($rows,[string]$tag){[string]::Join("`n",[string[]]@($rows|Where-Object {$_ -like "$tag`t*"}|ForEach-Object {$_.Substring($tag.Length+1)}))}
 $pins='';foreach($table in $filters.Keys){if(!@($fixture.$table).Count){continue};$columns=@($fixture.$table[0].PSObject.Properties.Name|ForEach-Object {'`'+$_+'`'}) -join ',';$pins+="SELECT 'pin-$table',$columns FROM $table WHERE $($filters[$table]) ORDER BY $($orders[$table]);`n"}
 $rows=Query ($certified+$pins+(Snapshot 'original')+$expected+(Snapshot 'expected')+$restore+$migration+(Snapshot 'first')+$migration+(Snapshot 'second'))
 foreach($table in $filters.Keys){
  if((Signature $rows "expected-$table") -cne (Signature $rows "first-$table") -or (Signature $rows "first-$table") -cne (Signature $rows "second-$table")){throw "Native one-field delta/idempotence differs: $table"}
  if($table -notin @('creature_template','conditions') -and (Signature $rows "original-$table") -cne (Signature $rows "first-$table")){throw "Unrelated native rows changed: $table"}
  if(@($fixture.$table).Count){$pin=@($fixture.$table|ForEach-Object {($_.PSObject.Properties.Value|ForEach-Object {if($null -eq $_){'NULL'}else{[Convert]::ToString($_,[Globalization.CultureInfo]::InvariantCulture)}}) -join "`t"});if(($pin -join "`n") -cne (Signature $rows "pin-$table")){throw "Pinned native context differs: $table"}}
 }
 $variants=[ordered]@{
  'custom-maiden-script'="UPDATE creature_template SET ScriptName='custom' WHERE entry=109355;"
  'wrong-maiden-ai'="UPDATE creature_template SET AIName='' WHERE entry=109355;"
  'custom-king-script'="UPDATE creature_template SET ScriptName='custom' WHERE entry=109610;"
  'custom-boss-script'="UPDATE creature_template SET ScriptName='' WHERE entry=109397;"
  'wrong-credit'="UPDATE smart_scripts SET action_param2=1 WHERE entryorguid=109355 AND id=4;"
  'wrong-credit-selector'="UPDATE smart_scripts SET target_type=18 WHERE entryorguid=109355 AND id=4;"
  'wrong-credit-radius'="UPDATE smart_scripts SET target_param1=60 WHERE entryorguid=109355 AND id=4;"
  'wrong-credit-link'="UPDATE smart_scripts SET link=0 WHERE entryorguid=109355 AND id=4;"
  'wrong-credit-repeat'="UPDATE smart_scripts SET event_flags=0 WHERE entryorguid=109355 AND id=4;"
  'wrong-speech-delay'="UPDATE smart_scripts SET action_param2=10 WHERE entryorguid=109355 AND id=3;"
  'wrong-follow'="UPDATE smart_scripts SET action_param3=1 WHERE entryorguid=109355 AND id=6;"
  'wrong-king-data'="UPDATE smart_scripts SET action_param2=2 WHERE entryorguid=109355 AND id=5;"
  'wrong-king-path'="UPDATE smart_scripts SET action_param2=109355 WHERE entryorguid=109610 AND id=2;"
  'missing-farewell'="DELETE FROM smart_scripts WHERE entryorguid=109355 AND id=8;"
  'wrong-boss-summon'="UPDATE smart_scripts SET action_param1=109610 WHERE entryorguid=109397 AND id=5;"
  'wrong-king-map'="UPDATE creature SET map=1 WHERE id=109610;"
  'wrong-king-guid'="UPDATE creature SET guid=373383 WHERE id=109610;"
  'static-maiden'="CREATE TEMPORARY TABLE extra_spawn LIKE creature;INSERT INTO extra_spawn SELECT * FROM creature WHERE id=109610;UPDATE extra_spawn SET guid=12345,id=109355;INSERT INTO creature SELECT * FROM extra_spawn;"
  'maiden-addon'="INSERT INTO creature_template_addon(entry) VALUES(109355);"
  'king-addon'="INSERT INTO creature_addon(guid) VALUES(373384);"
  'wrong-path-point'="UPDATE waypoints SET position_z=position_z+1 WHERE entry=109355;"
  'wrong-text'="UPDATE creature_text SET Text='custom' WHERE CreatureID=109355 AND GroupID=2;"
  'wrong-scaling'="UPDATE creature_template_scaling SET LevelScalingMin=0 WHERE Entry=109355;"
  'custom-source-condition'="INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2) VALUES(22,6,109355,31,1,3,109355);"
  'partial-new-cohort'="UPDATE creature_template SET ScriptName='npc_fuzan_summerpetal' WHERE entry=109355;INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment) VALUES(22,6,109355,0,0,31,1,3,109355,0,0,0,'condition_fuzan_summerpetal_continuation','Summerpetal actual acknowledged handoff');"
  'spawn-smart-override'="CREATE TEMPORARY TABLE extra_smart LIKE smart_scripts;INSERT INTO extra_smart SELECT * FROM smart_scripts WHERE entryorguid=109610;UPDATE extra_smart SET entryorguid=-373384;INSERT INTO smart_scripts SELECT * FROM extra_smart;"
  'custom-boss-prerequisite'="UPDATE conditions SET ConditionValue2=1 WHERE SourceEntry=109397;"
 }
 foreach($name in $variants.Keys){$bad=Query ($certified+$variants[$name]+(Snapshot 'before')+$migration+(Snapshot 'after')) -Refusal;foreach($table in $filters.Keys){if((Signature $bad "before-$table") -cne (Signature $bad "after-$table")){throw "Refusal mutated rows: $name/$table"}};"PASS refusal $name"}
 if($VerifyLiveData){$live=Query (Snapshot 'live');foreach($table in $filters.Keys){if((Signature $live "live-$table") -cne (Signature $rows "first-$table")){throw "Actual live Summerpetal differs: $table"}}}
 'PASS full native Summerpetal TEMP11: exact ScriptName/three-condition delta, original21 SmartAI/nine text/16 waypoint rows retained; idempotence and27 strict refusals, optional full live equality'
}
