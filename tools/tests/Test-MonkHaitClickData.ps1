param([string]$MigrationFile,[switch]$VerifyLiveData,[string]$OutputDirectory="$PSScriptRoot/../../build-extractors/runtime-test/monk-hait-click-data")
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..");$out=[IO.Path]::GetFullPath($OutputDirectory)
if(!$MigrationFile){$MigrationFile="$repo/sql/pending/world/2026_11_01_11_monk_hait_click.sql";if(!(Test-Path $MigrationFile)){$MigrationFile="$repo/sql/updates/world/2026_11_01_11_monk_hait_click.sql"}}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -RuntimeDirectories @($repo) -Test {
 $context=Get-Content "$PSScriptRoot/MonkHaitClickOriginal.json" -Raw|ConvertFrom-Json
 $migration=[IO.File]::ReadAllText($MigrationFile)
 $connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $script:index=0
 function Query([string]$sql,[switch]$Refusal){
  $stem="$out/sql-$script:index";++$script:index;[IO.File]::WriteAllText("$stem.sql",$sql+"`nSELECT 'EOF','monk-hait-click-data';`n")
  $prior=$env:MYSQL_PWD
  try{
   $env:MYSQL_PWD=$connection[3]
   $code=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -Arguments @("--host=$($connection[0])","--port=$($connection[1])","--user=$($connection[2])","--database=$($connection[4])",'--default-character-set=utf8mb4','--batch','--raw','--skip-column-names','--force') -InputFile "$stem.sql" -OutputFile "$stem.stdout" -ErrorFile "$stem.stderr"
  }finally{$env:MYSQL_PWD=$prior}
  $rows=@(Get-Content "$stem.stdout");$errors=@(Get-Content "$stem.stderr"|Where-Object {$_ -match '\S'})
  if($rows[-1] -cne "EOF`tmonk-hait-click-data"){throw 'Native SQL did not reach full EOF'}
  if($Refusal){if($errors.Count -ne 4 -or $errors[0] -cne '--------------' -or $errors[1] -cne 'INSERT INTO monk_hait_click_guard VALUES(@monk_hait_click_safe)' -or $errors[2] -cne '--------------' -or $errors[3] -notmatch '^ERROR 4025 .*monk_hait_click_native_guard'){throw 'Refusal contains unexplained native SQL diagnostics'}}
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
 foreach($row in $context.bindings){$expected+="INSERT INTO spell_script_names(spell_id,ScriptName) VALUES($($row.spell_id),$(Literal $row.ScriptName));`n"}
 function Snapshot([string]$tag){$result='';foreach($table in $tables){$result+="SELECT '$tag-$table',t.* FROM $table t WHERE $($context.filters.$table) ORDER BY $($context.orders.$table);`n"};$result}
 function Signature($rows,[string]$tag){[string]::Join("`n",[string[]]@($rows|Where-Object {$_ -like "$tag`t*"}|ForEach-Object {$_.Substring($tag.Length+1)}))}
 # Every replay shadows all seventeen production tables, on its own connection.
 $expectedRows=Query ($setup+$expected+(Snapshot 'expected'))
 $rows=Query ($setup+(Snapshot 'before')+$migration+(Snapshot 'first')+$migration+(Snapshot 'second'))
 foreach($table in $tables){
  if((Signature $rows "first-$table") -cne (Signature $rows "second-$table") -or (Signature $rows "first-$table") -cne (Signature $expectedRows "expected-$table")){throw "Full-column delta/idempotence differs: $table"}
  if($table -ne 'spell_script_names' -and (Signature $rows "before-$table") -cne (Signature $rows "first-$table")){throw "Unrelated native table changed: $table"}
 }
 $variants=[ordered]@{
  'unknown-positive-binding'="INSERT INTO spell_script_names VALUES(197120,'custom');"
  'unknown-negative-binding'="INSERT INTO spell_script_names VALUES(-197120,'custom');"
  'native-child-binding'="INSERT INTO spell_script_names VALUES(194020,'custom');"
  'extra-binding-after-install'="INSERT INTO spell_script_names VALUES(197120,'spell_monk_hait_click'),(-197120,'custom');"
  'different-actor-script'="UPDATE creature_template SET ScriptName='custom' WHERE entry=97778;"
  'missing-chuan-template'='DELETE FROM creature_template WHERE entry=98001;'
  'wrong-initial-spawn'='UPDATE creature SET guid=362104 WHERE guid=362105;'
  'changed-later-actor'="UPDATE creature SET phaseId='7001' WHERE guid=362271;"
  'changed-demon-position'='UPDATE creature SET position_x=0 WHERE guid=362135;'
  'click-no-player-caster'='UPDATE npc_spellclick_spells SET cast_flags=0 WHERE npc_entry=97778;'
  'missing-click'='DELETE FROM npc_spellclick_spells WHERE npc_entry=97778;'
  'changed-quest-class'='UPDATE quest_template_addon SET AllowableClasses=1 WHERE ID=12103;'
  'changed-objective'='UPDATE quest_objectives SET Amount=2 WHERE QuestID=12103 AND ObjectID=98514;'
  'changed-sequencing'='UPDATE quest_objectives SET Flags=0 WHERE QuestID=12103 AND ObjectID=97778;'
  'missing-quest-ender'='DELETE FROM creature_questender WHERE quest=12103;'
  'changed-scenario'='UPDATE scenario_data SET ScenarioID=942 WHERE ScenarioID=943;'
  'changed-phase'="UPDATE phase_definitions SET phaseId='7002' WHERE zoneId=5841 AND entry=2;"
  'missing-quest-phase-condition'='DELETE FROM conditions WHERE SourceTypeOrReferenceId=23 AND SourceGroup=5841 AND SourceEntry=2;'
 }
 foreach($variantName in $variants.Keys){
  $refusal=Query ($setup+$variants[$variantName]+(Snapshot 'before')+$migration+(Snapshot 'after')) -Refusal
  foreach($table in $tables){if((Signature $refusal "before-$table") -cne (Signature $refusal "after-$table")){throw "Refused $variantName mutated $table"}}
 }
 if($VerifyLiveData){
  $live=Query ('SET SESSION TRANSACTION READ ONLY;'+(Snapshot 'live'))
  foreach($table in $tables){if((Signature $live "live-$table") -cne (Signature $expectedRows "expected-$table")){throw "Live native context/registration differs: $table"}}
 }
 "PASS native SQL:17 complete schemas, exactly1 spell binding, idempotence,$($variants.Count) unchanged refusal snapshots; live=$VerifyLiveData"
}
