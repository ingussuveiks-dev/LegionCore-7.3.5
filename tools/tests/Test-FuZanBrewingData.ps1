param([string]$MigrationFile,[switch]$VerifyLiveData,[string]$OutputDirectory="$PSScriptRoot/../../build-extractors/runtime-test/fuzan-brewing-data")
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..");$out=[IO.Path]::GetFullPath($OutputDirectory)
if(!$MigrationFile){$MigrationFile="$repo/sql/pending/world/2026_11_01_10_fuzan_brewing.sql";if(!(Test-Path $MigrationFile)){$MigrationFile="$repo/sql/updates/world/2026_11_01_10_fuzan_brewing.sql"}}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -RuntimeDirectories @($repo) -Test {
 $context=Get-Content "$PSScriptRoot/FuZanBrewingOriginal.json" -Raw|ConvertFrom-Json
 $migration=[IO.File]::ReadAllText($MigrationFile)
 $connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $script:index=0
 function Query([string]$sql,[switch]$Refusal){
  $stem="$out/sql-$script:index";++$script:index;[IO.File]::WriteAllText("$stem.sql",$sql+"`nSELECT 'EOF','fuzan-brewing-data';`n")
  $prior=$env:MYSQL_PWD
  try{
   $env:MYSQL_PWD=$connection[3]
   $code=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -Arguments @("--host=$($connection[0])","--port=$($connection[1])","--user=$($connection[2])","--database=$($connection[4])",'--default-character-set=utf8mb4','--batch','--raw','--skip-column-names','--force') -InputFile "$stem.sql" -OutputFile "$stem.stdout" -ErrorFile "$stem.stderr"
  }finally{$env:MYSQL_PWD=$prior}
  $rows=@(Get-Content "$stem.stdout");$errors=@(Get-Content "$stem.stderr"|Where-Object {$_ -match '\S'})
  if($rows[-1] -cne "EOF`tfuzan-brewing-data"){throw 'Native SQL did not reach full EOF'}
  if($Refusal){if($errors.Count -ne 4 -or $errors[0] -cne '--------------' -or $errors[1] -cne 'INSERT INTO fuzan_brewing_guard VALUES(@fuzan_brewing_safe)' -or $errors[2] -cne '--------------' -or $errors[3] -notmatch '^ERROR 4025 .*fuzan_brewing_native_guard'){throw 'Refusal contains unexplained native SQL diagnostics'}}
  elseif($code -or $errors.Count){throw 'Native SQL exit/stderr failure; full evidence retained'}
  $rows
 }
 function Literal($value){
  if($null -eq $value){return 'NULL'}
  if($value -is [string]){if(!$value.Length){return "''"};return 'CONVERT(0x'+[Convert]::ToHexString([Text.Encoding]::UTF8.GetBytes($value))+' USING utf8mb4)'}
  [Convert]::ToString($value,[Globalization.CultureInfo]::InvariantCulture)
 }
 $tables=@($context.filters.PSObject.Properties.Name);$setup='';$expected=''
 foreach($table in $tables){
  $setup+="CREATE TEMPORARY TABLE schema_$table LIKE $table;CREATE TEMPORARY TABLE $table LIKE schema_$table;DROP TEMPORARY TABLE schema_$table;`n"
  foreach($row in @($context.tables.$table)){
   $columns=($row.PSObject.Properties.Name|ForEach-Object {'`'+$_+'`'}) -join ','
   $values=($row.PSObject.Properties.Value|ForEach-Object {Literal $_}) -join ','
   $setup+="INSERT INTO $table($columns) VALUES($values);`n"
  }
 }
 foreach($table in @($context.changes.PSObject.Properties.Name)){
  foreach($row in $context.changes.$table.PSObject.Properties){$expected+="UPDATE $table SET ScriptName=$(Literal $row.Value) WHERE entry=$($row.Name);`n"}
 }
 foreach($row in $context.bindings){$expected+="INSERT INTO spell_script_names(spell_id,ScriptName) VALUES($($row.spell_id),$(Literal $row.ScriptName));`n"}
 function Snapshot([string]$tag){$result='';foreach($table in $tables){$result+="SELECT '$tag-$table',t.* FROM $table t WHERE $($context.filters.$table) ORDER BY $($context.orders.$table);`n"};$result}
 function Signature($rows,[string]$tag){[string]::Join("`n",[string[]]@($rows|Where-Object {$_ -like "$tag`t*"}|ForEach-Object {$_.Substring($tag.Length+1)}))}
 # Separate connections: neither baseline nor expected metadata is installed live.
 $expectedRows=Query ($setup+$expected+(Snapshot 'expected'))
 $rows=Query ($setup+(Snapshot 'before')+$migration+(Snapshot 'first')+$migration+(Snapshot 'second'))
 foreach($table in $tables){
  if((Signature $rows "first-$table") -cne (Signature $rows "second-$table") -or (Signature $rows "first-$table") -cne (Signature $expectedRows "expected-$table")){throw "Full-column delta/idempotence differs: $table"}
  if($table -notin @('creature_template','gameobject_template','spell_script_names') -and (Signature $rows "before-$table") -cne (Signature $rows "first-$table")){throw "Unrelated native table changed: $table"}
 }
 $variants=[ordered]@{
  'custom-creature-script'="UPDATE creature_template SET ScriptName='custom' WHERE entry=109224;"
  'custom-fire-script'="UPDATE gameobject_template SET ScriptName='custom' WHERE entry=251490;"
  'partial-template-cohort'="UPDATE creature_template SET ScriptName='npc_fuzan_brewing' WHERE entry=109224;"
  'partial-spell-cohort'="INSERT INTO spell_script_names VALUES(217244,'spell_fuzan_brewing_receipt');"
  'unknown-positive-spell'="INSERT INTO spell_script_names VALUES(217244,'custom');"
  'unknown-negative-spell'="INSERT INTO spell_script_names VALUES(-217244,'custom');"
  'changed-native-campfire-spell'='UPDATE gameobject_template SET Data10=217297 WHERE entry=251490;'
  'changed-native-ingredient-spell'='UPDATE gameobject_template SET Data0=217214 WHERE entry=251501;'
  'changed-native-player-caster'='UPDATE gameobject_template SET Data6=0 WHERE entry=251505;'
  'changed-native-charges'='UPDATE gameobject_template SET Data1=1 WHERE entry=251510;'
  'changed-source-spawn'='UPDATE gameobject SET guid=128075 WHERE guid=128074;'
  'changed-source-phase'="UPDATE gameobject SET PhaseId='1' WHERE guid=128074;"
  'missing-ingredient-spawn'='DELETE FROM gameobject WHERE guid=128068;'
  'changed-public-king'='UPDATE creature SET position_x=900 WHERE guid=366254;'
  'missing-private-template'='DELETE FROM creature_template WHERE entry=109224;'
  'changed-quest-objective'='UPDATE quest_objectives SET ObjectID=109224 WHERE ID=284852;'
  'changed-sequencing'='UPDATE quest_objectives SET Flags=0 WHERE ID=284852;'
  'missing-fire-objective'='DELETE FROM quest_objectives WHERE ID=284846;'
  'changed-quest-receiver'='DELETE FROM creature_questender WHERE quest=42868;'
  'changed-acceptance-text'='UPDATE creature_text SET BroadcastTextID=0 WHERE CreatureID=109187 AND GroupID=3;'
  'changed-unrelated-margaux-waypoint'='UPDATE waypoints SET position_x=0 WHERE entry=109224 AND pointid=1;'
 }
 foreach($name in $variants.Keys){
  $refusal=Query ($setup+$variants[$name]+(Snapshot 'before')+$migration+(Snapshot 'after')) -Refusal
  foreach($table in $tables){if((Signature $refusal "before-$table") -cne (Signature $refusal "after-$table")){throw "Refused $name mutated $table"}}
 }
 if($VerifyLiveData){
  $live=Query ('SET SESSION TRANSACTION READ ONLY;'+(Snapshot 'live'))
  foreach($table in $tables){if((Signature $live "live-$table") -cne (Signature $expectedRows "expected-$table")){throw "Live native context/registration differs: $table"}}
 }
 "PASS native SQL:15 complete schemas, exactly7 ScriptNames+15 bindings, idempotence,$($variants.Count) unchanged refusal snapshots; live=$VerifyLiveData"
}
