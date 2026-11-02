param([string]$MigrationFile,[switch]$VerifyLiveData,[string]$OutputDirectory="$PSScriptRoot/../../build-extractors/runtime-test/monk-fel-stone-data/$(Get-Date -Format yyyyMMdd-HHmmss-fff)")
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..");$out=[IO.Path]::GetFullPath($OutputDirectory)
if(!$MigrationFile){$MigrationFile="$repo/sql/pending/world/2026_11_02_02_monk_fel_stone.sql";if(!(Test-Path $MigrationFile)){$MigrationFile="$repo/sql/updates/world/2026_11_02_02_monk_fel_stone.sql"}}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -RuntimeDirectories @($repo) -Test {
 $context=Get-Content "$PSScriptRoot/MonkFelStoneOriginal.json" -Raw|ConvertFrom-Json
 $migration=[IO.File]::ReadAllText($MigrationFile)
 $connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $script:index=0
 function Query([string]$sql,[switch]$Refusal){
  $stem="$out/sql-$script:index";++$script:index;[IO.File]::WriteAllText("$stem.sql",$sql+"`nSELECT 'EOF','monk-fel-stone-data';`n")
  $prior=$env:MYSQL_PWD
  try{
   $env:MYSQL_PWD=$connection[3]
   $code=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -Arguments @("--host=$($connection[0])","--port=$($connection[1])","--user=$($connection[2])","--database=$($connection[4])",'--default-character-set=utf8mb4','--batch','--raw','--skip-column-names','--force') -InputFile "$stem.sql" -OutputFile "$stem.stdout" -ErrorFile "$stem.stderr"
  }finally{$env:MYSQL_PWD=$prior}
  $rows=@(Get-Content "$stem.stdout");$errors=@(Get-Content "$stem.stderr"|Where-Object {$_ -match '\S'})
  if($rows[-1] -cne "EOF`tmonk-fel-stone-data"){throw 'Native SQL did not reach full EOF'}
  if($Refusal){if($errors.Count -ne 4 -or $errors[0] -cne '--------------' -or $errors[1] -cne 'INSERT INTO monk_fel_stone_guard VALUES(@monk_fel_stone_safe)' -or $errors[2] -cne '--------------' -or $errors[3] -notmatch '^ERROR 4025 .*monk_fel_stone_native_guard'){throw 'Refusal contains unexplained native SQL diagnostics'}}
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
 $expected+="UPDATE creature_template SET ScriptName='npc_monk_fel_stone' WHERE entry=98353;`n"
 function Snapshot([string]$tag){$result='';foreach($table in $tables){$result+="SELECT '$tag-$table',t.* FROM $table t WHERE $($context.filters.$table) ORDER BY $($context.orders.$table);`n"};$result}
 function Signature($rows,[string]$tag){[string]::Join("`n",[string[]]@($rows|Where-Object {$_ -like "$tag`t*"}|ForEach-Object {$_.Substring($tag.Length+1)}))}
 # Every replay shadows all twenty-two production tables, on its own connection.
 $expectedRows=Query ($setup+$expected+(Snapshot 'expected'))
 $rows=Query ($setup+(Snapshot 'before')+$migration+(Snapshot 'first')+$migration+(Snapshot 'second'))
 foreach($table in $tables){
  if((Signature $rows "first-$table") -cne (Signature $rows "second-$table") -or (Signature $rows "first-$table") -cne (Signature $expectedRows "expected-$table")){throw "Full-column delta/idempotence differs: $table"}
  if($table -notin @('spell_script_names','creature_template') -and (Signature $rows "before-$table") -cne (Signature $rows "first-$table")){throw "Unrelated native table changed: $table"}
 }
 $variants=[ordered]@{
  'unknown-positive-binding'="INSERT INTO spell_script_names VALUES(193890,'custom');"
  'unknown-negative-binding'="INSERT INTO spell_script_names VALUES(-193890,'custom');"
  'unknown-event-binding'="INSERT INTO spell_script_names VALUES(194141,'custom');"
  'unknown-marker-binding'="INSERT INTO spell_script_names VALUES(-200617,'custom');"
  'partial-parent-only'="INSERT INTO spell_script_names VALUES(193890,'spell_monk_fel_stone');"
  'partial-template-only'="UPDATE creature_template SET ScriptName='npc_monk_fel_stone' WHERE entry=98353;"
  'partial-two-bindings'="UPDATE creature_template SET ScriptName='npc_monk_fel_stone' WHERE entry=98353;INSERT INTO spell_script_names VALUES(193890,'spell_monk_fel_stone'),(194141,'spell_monk_fel_stone_event');"
  'extra-after-installed'=$expected+"INSERT INTO spell_script_names VALUES(-193890,'custom');"
  'other-native-marker-binding'="INSERT INTO spell_script_names VALUES(194181,'custom');"
  'different-actor-script'="UPDATE creature_template SET ScriptName='custom' WHERE entry=98353;"
  'different-actor-AI'="UPDATE creature_template SET AIName='NullCreatureAI' WHERE entry=98353;"
  'missing-boss-template'='DELETE FROM creature_template WHERE entry=98217;'
  'changed-source-guid'='UPDATE creature SET guid=362199 WHERE guid=362198;'
  'missing-second-source'='DELETE FROM creature WHERE guid=12691299;'
  'changed-boss-map'='UPDATE creature SET map=1 WHERE guid=362203;'
  'changed-source-phase'="UPDATE creature SET phaseId='7001' WHERE guid=362198;"
  'missing-real-boss-death'='DELETE FROM smart_scripts WHERE entryorguid=98217 AND event_type=6;'
  'changed-old-return-destination'='UPDATE smart_scripts SET target_x=0 WHERE entryorguid=98353 AND id=0;'
  'missing-old-native-link'='DELETE FROM smart_scripts WHERE entryorguid=98353 AND id=1;'
  'click-no-player-caster'='UPDATE npc_spellclick_spells SET cast_flags=0 WHERE npc_entry=98353;'
  'missing-click'='DELETE FROM npc_spellclick_spells WHERE npc_entry=98353;'
  'changed-quest-class'='UPDATE quest_template_addon SET AllowableClasses=1 WHERE ID=12103;'
  'changed-objective'='UPDATE quest_objectives SET Amount=2 WHERE QuestID=12103 AND ObjectID=98353;'
  'changed-sequencing'='UPDATE quest_objectives SET Flags=0 WHERE QuestID=12103 AND ObjectID=98353;'
  'missing-quest-ender'='DELETE FROM creature_questender WHERE quest=12103;'
  'changed-scenario'='UPDATE scenario_data SET ScenarioID=942 WHERE ScenarioID=943;'
  'changed-phase'="UPDATE phase_definitions SET phaseId='7002' WHERE zoneId=5841 AND entry=2;"
  'missing-quest-phase-condition'='DELETE FROM conditions WHERE SourceTypeOrReferenceId=23 AND SourceGroup=5841 AND SourceEntry=2;'
  'different-native-scene'='UPDATE spell_scene SET SceneScriptPackageID=1493 WHERE MiscValue=1111;'
  'custom-scene-duration'='UPDATE spell_scene SET CustomDuration=10000 WHERE MiscValue=1111;'
  'changed-scene-flags'='UPDATE spell_scene SET PlaybackFlags=1 WHERE MiscValue=1111;'
  'custom-scene-script'="UPDATE spell_scene SET ScriptName='custom' WHERE MiscValue=1111;"
  'extra-scene-event'="INSERT INTO spell_scene_event(MiscValue,trigerSpell,MonsterCredit,Event) VALUES(1111,194141,0,'complete');"
 }

 foreach($variantName in $variants.Keys){
  $refusal=Query ($setup+$variants[$variantName]+(Snapshot 'before')+$migration+(Snapshot 'after')) -Refusal
  foreach($table in $tables){if((Signature $refusal "before-$table") -cne (Signature $refusal "after-$table")){throw "Refused $variantName mutated $table"}}
 }
 if($VerifyLiveData){
  $live=Query ('SET SESSION TRANSACTION READ ONLY;'+(Snapshot 'live'))
  foreach($table in $tables){if((Signature $live "live-$table") -cne (Signature $expectedRows "expected-$table")){throw "Live native context/registration differs: $table"}}
 }
 "PASS native SQL:22 complete schemas, exactly1 template ScriptName and3 spell bindings, idempotence,$($variants.Count) unchanged refusal snapshots; live=$VerifyLiveData"
}
