param([string]$MigrationFile,[switch]$VerifyLiveData,[string]$OutputDirectory="$PSScriptRoot/../../build-extractors/runtime-test/fuzan-temple-door-data")
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out=[IO.Path]::GetFullPath($OutputDirectory)
if(!$MigrationFile){$MigrationFile="$repo/sql/pending/world/2026_11_01_04_fuzan_sanctuary_door_admission.sql";if(!(Test-Path $MigrationFile)){$MigrationFile="$repo/sql/updates/world/2026_11_01_04_fuzan_sanctuary_door_admission.sql"}}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -Test {
 $migration=Get-Content $MigrationFile -Raw
 $fixture=Get-Content "$PSScriptRoot/FuZanTempleDoorOriginal.json" -Raw|ConvertFrom-Json
 $connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $script:index=0
 function Query([string]$sql,[switch]$Refusal){
  $stem="$out/sql-$script:index";++$script:index
  [IO.File]::WriteAllText("$stem.sql",$sql+"`nSELECT 'EOF','fuzan-door-data';`n")
  $old=$env:MYSQL_PWD
  try{
   $env:MYSQL_PWD=$connection[3]
   $code=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -Arguments @("--host=$($connection[0])","--port=$($connection[1])","--user=$($connection[2])","--database=$($connection[4])",'--default-character-set=utf8mb4','--batch','--raw','--skip-column-names','--force') -InputFile "$stem.sql" -OutputFile "$stem.stdout" -ErrorFile "$stem.stderr"
   $rows=@(Get-Content "$stem.stdout" -Encoding UTF8);$errors=[IO.File]::ReadAllText("$stem.stderr")
   if($rows[-1] -cne "EOF`tfuzan-door-data"){throw 'Sanctuary door SQL incomplete EOF'}
   if($Refusal){if($errors -notmatch 'fuzan_sanctuary_door_native_guard' -or @([regex]::Matches($errors,'ERROR [0-9]+')).Count -ne 1){throw "Unexpected refusal diagnostics: $stem"}}
   elseif($code -or $errors){throw "Sanctuary door SQL failed: $stem"}
   return ,$rows
  }finally{$env:MYSQL_PWD=$old}
 }
 $filters=[ordered]@{gameobject_template='entry=251591';smart_scripts='entryorguid IN(251591,-138378) AND source_type=1';conditions='SourceTypeOrReferenceId=22 AND SourceEntry=251591';gameobject='id=251591';gameobject_scripts='id=138378'}
 $orders=@{gameobject_template='entry';smart_scripts='entryorguid,id';conditions='SourceGroup,SourceId,ElseGroup,ConditionTypeOrReference';gameobject='guid';gameobject_scripts='delay,command'}
 $setup='';foreach($table in $filters.Keys){$setup+="CREATE TEMPORARY TABLE saved_$table LIKE $table;INSERT INTO saved_$table SELECT * FROM $table WHERE $($filters[$table]);CREATE TEMPORARY TABLE $table LIKE saved_$table;INSERT INTO $table SELECT * FROM saved_$table;`n"}
 # Certify the complete pinned context before normalizing this known one-field
 # delta in private TEMP tables, so both original and deployed data can test it.
 $restore="UPDATE gameobject_template SET ScriptName='' WHERE entry=251591 AND ScriptName='go_fuzan_temple_sanctuary_door';`n"
 $expected="UPDATE gameobject_template SET ScriptName='go_fuzan_temple_sanctuary_door' WHERE entry=251591;`n"
 $certified=$setup+$migration+$restore
 function Snapshot([string]$tag){$s='';foreach($table in $filters.Keys){$s+="SELECT '$tag-$table',t.* FROM $table t WHERE $($filters[$table]) ORDER BY $($orders[$table]);`n"};$s}
 function Signature($rows,[string]$tag){[string]::Join("`n",[string[]]@($rows|Where-Object {$_ -like "$tag`t*"}|ForEach-Object {$_.Substring($tag.Length+1)}))}
 $pins='';foreach($table in $filters.Keys){if(!@($fixture.$table).Count){continue};$columns=@($fixture.$table[0].PSObject.Properties.Name|ForEach-Object {'`'+$_+'`'}) -join ',';$pins+="SELECT 'pin-$table',$columns FROM $table WHERE $($filters[$table]) ORDER BY $($orders[$table]);`n"}
 $rows=Query ($certified+$pins+(Snapshot 'original')+$expected+(Snapshot 'expected')+$restore+$migration+(Snapshot 'first')+$migration+(Snapshot 'second'))
 foreach($table in $filters.Keys){
  if((Signature $rows "expected-$table") -cne (Signature $rows "first-$table") -or (Signature $rows "first-$table") -cne (Signature $rows "second-$table")){throw "Door exact one-field delta/idempotence differs: $table"}
  if($table -ne 'gameobject_template' -and (Signature $rows "original-$table") -cne (Signature $rows "first-$table")){throw "Unrelated door rows changed: $table"}
  if(@($fixture.$table).Count){$pin=@($fixture.$table|ForEach-Object {($_.PSObject.Properties.Value|ForEach-Object {[Convert]::ToString($_,[Globalization.CultureInfo]::InvariantCulture)}) -join "`t"});if(($pin -join "`n") -cne (Signature $rows "pin-$table")){throw "Pinned native door context differs: $table"}}
 }
 $variants=[ordered]@{
  'custom-script'="UPDATE gameobject_template SET ScriptName='custom';"
  'wrong-ai'="UPDATE gameobject_template SET AIName='';"
  'wrong-type'='UPDATE gameobject_template SET type=1;'
  'wrong-display'='UPDATE gameobject_template SET displayId=1;'
  'custom-flags'='UPDATE gameobject_template SET flags=1;'
  'custom-size'='UPDATE gameobject_template SET size=1;'
  'custom-lock'='UPDATE gameobject_template SET Data1=1;'
  'custom-autoclose'='UPDATE gameobject_template SET Data2=1;'
  'custom-native-data'='UPDATE gameobject_template SET Data9=0;'
  'custom-visual'='UPDATE gameobject_template SET SpellVisualID=1;'
  'root-action'='UPDATE smart_scripts SET action_param2=1;'
  'root-repeat'='UPDATE smart_scripts SET event_flags=0;'
  'root-target'='UPDATE smart_scripts SET target_type=1;'
  'root-link'='UPDATE smart_scripts SET link=1;'
  'root-comment'="UPDATE smart_scripts SET comment='custom';"
  'missing-root'='DELETE FROM smart_scripts;'
  'wrong-map'='UPDATE gameobject SET map=1;'
  'wrong-phase'="UPDATE gameobject SET phaseId='1';"
  'wrong-position'='UPDATE gameobject SET position_x=position_x+1;'
  'wrong-state'='UPDATE gameobject SET state=0;'
  'duplicate-spawn'='CREATE TEMPORARY TABLE extra_spawn LIKE gameobject;INSERT INTO extra_spawn SELECT * FROM gameobject;UPDATE extra_spawn SET guid=138379;INSERT INTO gameobject SELECT * FROM extra_spawn;'
  'custom-condition'="INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment) VALUES(22,1,251591,1,0,31,0,4,0,0,0,0,'','custom');"
  'custom-map-script'='INSERT INTO gameobject_scripts(id,delay,command,datalong,datalong2,dataint,x,y,z,o) VALUES(138378,0,0,0,0,0,0,0,0,0);'
  'spawn-smart-override'='CREATE TEMPORARY TABLE extra_smart LIKE smart_scripts;INSERT INTO extra_smart SELECT * FROM smart_scripts;UPDATE extra_smart SET entryorguid=-138378;INSERT INTO smart_scripts SELECT * FROM extra_smart;'
 }
 foreach($name in $variants.Keys){$bad=Query ($certified+$variants[$name]+(Snapshot 'before')+$migration+(Snapshot 'after')) -Refusal;foreach($table in $filters.Keys){if((Signature $bad "before-$table") -cne (Signature $bad "after-$table")){throw "Refusal mutated rows: $name/$table"}};"PASS refusal $name"}
 if($VerifyLiveData){$live=Query (Snapshot 'live');foreach($table in $filters.Keys){if((Signature $live "live-$table") -cne (Signature $rows "first-$table")){throw "Actual live sanctuary door differs: $table"}}}
 'PASS native sanctuary door TEMP5: only ScriptName changes, native62-column template/one205 root/spawn/empty conditions+map script and absent negative-GUID override retained; idempotence/24 refusals and optional full live equality'
}
