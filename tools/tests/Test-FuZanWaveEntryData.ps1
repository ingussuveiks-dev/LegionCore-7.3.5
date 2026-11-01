param([string]$MigrationFile,[switch]$VerifyLiveData,[string]$OutputDirectory="$PSScriptRoot/../../build-extractors/runtime-test/fuzan-wave-entry-data")
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..");$out=[IO.Path]::GetFullPath($OutputDirectory)
if(!$MigrationFile){$MigrationFile="$repo/sql/pending/world/2026_11_01_07_fuzan_wave_entry_attempt.sql";if(!(Test-Path $MigrationFile)){$MigrationFile="$repo/sql/updates/world/2026_11_01_07_fuzan_wave_entry_attempt.sql"}}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -RuntimeDirectories @($repo) -Test {
 $migration=Get-Content $MigrationFile -Raw;$context=Get-Content "$PSScriptRoot/FuZanWaveEntryOriginal.json" -Raw|ConvertFrom-Json;$fixture=$context.tables
 $connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $script:index=0
 function Query([string]$sql,[switch]$Refusal){
  $stem="$out/sql-$script:index";++$script:index;[IO.File]::WriteAllText("$stem.sql",$sql+"`nSELECT 'EOF','fuzan-wave-entry-data';`n")
  $prior=$env:MYSQL_PWD
  try{
   $env:MYSQL_PWD=$connection[3]
   $code=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -Arguments @("--host=$($connection[0])","--port=$($connection[1])","--user=$($connection[2])","--database=$($connection[4])",'--default-character-set=utf8mb4','--batch','--raw','--skip-column-names','--force') -InputFile "$stem.sql" -OutputFile "$stem.stdout" -ErrorFile "$stem.stderr"
   $rows=@(Get-Content "$stem.stdout" -Encoding UTF8);$errors=[IO.File]::ReadAllText("$stem.stderr")
   if($rows[-1] -cne "EOF`tfuzan-wave-entry-data"){throw 'Native WaveEntry SQL incomplete EOF'}
   if($Refusal){if($errors -notmatch 'fuzan_wave_entry_native_guard' -or @([regex]::Matches($errors,'ERROR [0-9]+')).Count -ne 1){throw "Unexpected refusal diagnostics: $stem"}}
   elseif($code -or $errors){throw "WaveEntry SQL failed: $stem"}
   return ,$rows
  }finally{$env:MYSQL_PWD=$prior}
 }
 $filters=[ordered]@{eventobject_template='entry=103';eventobject='id=103';creature_template='entry IN(109465,109350,109352,109391)';creature_template_wdb='entry IN(109465,109350,109352,109391)';creature='id IN(109465,109350,109352,109391)';smart_scripts='(source_type=13 AND entryorguid=103) OR (source_type IN(0,9) AND entryorguid IN(109465,109350,109352,109391,-373425,-373396)) OR (source_type IN(1,9) AND entryorguid=251605)';conditions='(SourceTypeOrReferenceId=22 AND SourceEntry IN(103,109465,109350,109352,109391,-373425,-373396,251605)) OR (SourceTypeOrReferenceId IN(13,17) AND SourceEntry IN(217814,217817,218883,218882,217659,217667,217666,217669))';creature_template_addon='entry IN(109465,109350,109352,109391)';creature_addon='guid IN(373425,373396)';creature_template_scaling='Entry IN(109465,109350,109352,109391)';creature_loot_template='Entry IN(109465,109350,109352,109391)';creature_text='CreatureID IN(109465,109350,109352,109391)';gameobject_template='entry=251605';gameobject='id=251605';conversation_data='entry=2779';conversation_creature='entry=2779';conversation_actor='entry=2779';spell_area='spell IN(217814,217817,218883,218882,217659,217667,217666,217669)';spell_script_names='spell_id IN(217814,217817,218883,218882,217659,217667,217666,217669)';spell_linked_spell='ABS(spell_trigger) IN(217814,217817,218883,218882,217659,217667,217666,217669) OR ABS(spell_effect) IN(217814,217817,218883,218882,217659,217667,217666,217669)';spell_check_cast='spellId IN(217814,217817,218883,218882,217659,217667,217666,217669)';spell_aura_dummy='ABS(spellId) IN(217814,217817,218883,218882,217659,217667,217666,217669) OR ABS(spellDummyId) IN(217814,217817,218883,218882,217659,217667,217666,217669)';spell_target_filter='spellId IN(217814,217817,218883,218882,217659,217667,217666,217669)';spell_trigger='ABS(spell_id) IN(217814,217817,218883,218882,217659,217667,217666,217669) OR ABS(spell_trigger) IN(217814,217817,218883,218882,217659,217667,217666,217669)';spell_dummy_trigger='ABS(spell_id) IN(217814,217817,218883,218882,217659,217667,217666,217669) OR ABS(spell_trigger) IN(217814,217817,218883,218882,217659,217667,217666,217669)';spell_aura_trigger='ABS(spell_id) IN(217814,217817,218883,218882,217659,217667,217666,217669) OR ABS(spell_trigger) IN(217814,217817,218883,218882,217659,217667,217666,217669)';spell_trigger_delay='ABS(spell_id) IN(217814,217817,218883,218882,217659,217667,217666,217669) OR ABS(spell_trigger) IN(217814,217817,218883,218882,217659,217667,217666,217669)'}
 $orders=[ordered]@{eventobject_template='entry';eventobject='guid';creature_template='entry';creature_template_wdb='entry';creature='guid';smart_scripts='entryorguid,source_type,id';conditions='SourceTypeOrReferenceId,SourceEntry,SourceGroup,ElseGroup,ConditionTypeOrReference';creature_template_addon='entry';creature_addon='guid';creature_template_scaling='Entry';creature_loot_template='Entry,Item';creature_text='CreatureID,GroupID,ID';gameobject_template='entry';gameobject='guid';conversation_data='entry,idx';conversation_creature='entry,id';conversation_actor='entry,id';spell_area='spell,area';spell_script_names='spell_id,ScriptName';spell_linked_spell='spell_trigger,spell_effect,type';spell_check_cast='spellId,type';spell_aura_dummy='spellId,spellDummyId';spell_target_filter='spellId';spell_trigger='spell_id,spell_trigger';spell_dummy_trigger='spell_id,spell_trigger';spell_aura_trigger='spell_id,spell_trigger';spell_trigger_delay='spell_id,spell_trigger'}
 $setup='';foreach($table in $filters.Keys){$setup+="CREATE TEMPORARY TABLE saved_$table LIKE $table;INSERT INTO saved_$table SELECT * FROM $table WHERE $($filters[$table]);CREATE TEMPORARY TABLE $table LIKE saved_$table;INSERT INTO $table SELECT * FROM saved_$table;`n"}
 $restore="UPDATE eventobject_template SET ScriptName='SmartEventObject' WHERE entry=103;DELETE FROM spell_script_names WHERE spell_id IN(217814,217817,218883,218882,217659,217667,217666,217669);`n"
 $expected="UPDATE eventobject_template SET ScriptName='eventobject_fuzan_wave_entry' WHERE entry=103;INSERT INTO spell_script_names(spell_id,ScriptName) VALUES(218883,'spell_fuzan_wave_attempt');INSERT INTO spell_script_names(spell_id,ScriptName) VALUES(218882,'spell_fuzan_wave_attempt');INSERT INTO spell_script_names(spell_id,ScriptName) VALUES(217659,'spell_fuzan_wave_attempt');INSERT INTO spell_script_names(spell_id,ScriptName) VALUES(217667,'spell_fuzan_wave_attempt');INSERT INTO spell_script_names(spell_id,ScriptName) VALUES(217666,'spell_fuzan_wave_attempt');INSERT INTO spell_script_names(spell_id,ScriptName) VALUES(217669,'spell_fuzan_wave_attempt');INSERT INTO spell_script_names(spell_id,ScriptName) VALUES(217659,'aura_fuzan_wave_controller');INSERT INTO spell_script_names(spell_id,ScriptName) VALUES(217667,'aura_fuzan_wave_controller');INSERT INTO spell_script_names(spell_id,ScriptName) VALUES(217817,'aura_fuzan_wave_controller');INSERT INTO spell_script_names(spell_id,ScriptName) VALUES(217817,'spell_fuzan_wave_conversation');INSERT INTO spell_script_names(spell_id,ScriptName) VALUES(217814,'spell_fuzan_wave_retrieval');`n"
 $certified=$setup+$migration+$restore
 function Snapshot([string]$tag){$result='';foreach($table in $filters.Keys){$result+="SELECT '$tag-$table',t.* FROM $table t WHERE $($filters[$table]) ORDER BY $($orders[$table]);`n"};$result}
 function Signature($rows,[string]$tag){[string]::Join("`n",[string[]]@($rows|Where-Object {$_ -like "$tag`t*"}|ForEach-Object {$_.Substring($tag.Length+1)}))}
 $pins='';foreach($table in $filters.Keys){if(!@($fixture.$table).Count){continue};$columns=@($fixture.$table[0].PSObject.Properties.Name|ForEach-Object {'`'+$_+'`'}) -join ',';$pins+="SELECT 'pin-$table',$columns FROM $table WHERE $($filters[$table]) ORDER BY $($orders[$table]);`n"}
 $rows=Query ($certified+$pins+(Snapshot 'original')+$expected+(Snapshot 'expected')+$restore+$migration+(Snapshot 'first')+$migration+(Snapshot 'second'))
 foreach($table in $filters.Keys){
  if((Signature $rows "expected-$table") -cne (Signature $rows "first-$table") -or (Signature $rows "first-$table") -cne (Signature $rows "second-$table")){throw "Native one-field delta/idempotence differs: $table"}
  if($table -notin @('eventobject_template','spell_script_names') -and (Signature $rows "original-$table") -cne (Signature $rows "first-$table")){throw "Unrelated native rows changed: $table"}
  if(@($fixture.$table).Count){$pin=@($fixture.$table|ForEach-Object {($_.PSObject.Properties.Value|ForEach-Object {if($null -eq $_){'NULL'}else{[Convert]::ToString($_,[Globalization.CultureInfo]::InvariantCulture)}}) -join "`t"});if(($pin -join "`n") -cne (Signature $rows "pin-$table")){throw "Pinned native context differs: $table"}}
 }
 $variants=[ordered]@{
  'custom-event-script'="UPDATE eventobject_template SET ScriptName='custom' WHERE entry=103;"
  'partial-event-only'="UPDATE eventobject_template SET ScriptName='eventobject_fuzan_wave_entry' WHERE entry=103;"
  'partial-spell-only'="INSERT INTO spell_script_names VALUES(217659,'aura_fuzan_wave_controller');"
  'all-spells-old-event'="INSERT INTO spell_script_names(spell_id,ScriptName) VALUES(218883,'spell_fuzan_wave_attempt');INSERT INTO spell_script_names(spell_id,ScriptName) VALUES(218882,'spell_fuzan_wave_attempt');INSERT INTO spell_script_names(spell_id,ScriptName) VALUES(217659,'spell_fuzan_wave_attempt');INSERT INTO spell_script_names(spell_id,ScriptName) VALUES(217667,'spell_fuzan_wave_attempt');INSERT INTO spell_script_names(spell_id,ScriptName) VALUES(217666,'spell_fuzan_wave_attempt');INSERT INTO spell_script_names(spell_id,ScriptName) VALUES(217669,'spell_fuzan_wave_attempt');INSERT INTO spell_script_names(spell_id,ScriptName) VALUES(217659,'aura_fuzan_wave_controller');INSERT INTO spell_script_names(spell_id,ScriptName) VALUES(217667,'aura_fuzan_wave_controller');INSERT INTO spell_script_names(spell_id,ScriptName) VALUES(217817,'aura_fuzan_wave_controller');INSERT INTO spell_script_names(spell_id,ScriptName) VALUES(217817,'spell_fuzan_wave_conversation');INSERT INTO spell_script_names(spell_id,ScriptName) VALUES(217814,'spell_fuzan_wave_retrieval');"
  'custom-spell-binding'="INSERT INTO spell_script_names VALUES(217814,'custom');"
  'wrong-event-radius'="UPDATE eventobject_template SET radius=6 WHERE entry=103;"
  'wrong-event-map'="UPDATE eventobject SET map=1 WHERE id=103;"
  'wrong-event-source'="UPDATE eventobject SET guid=400105 WHERE id=103;"
  'wrong-root-credit'="UPDATE smart_scripts SET action_param2=1 WHERE entryorguid=103 AND source_type=13 AND id=0;"
  'wrong-root-target'="UPDATE smart_scripts SET target_type=18 WHERE entryorguid=103 AND source_type=13 AND id=0;"
  'wrong-native-cast-flags'="UPDATE smart_scripts SET action_param2=2 WHERE entryorguid=103 AND source_type=13 AND id=3;"
  'wrong-companion-chain'="UPDATE smart_scripts SET link=0 WHERE entryorguid=103 AND source_type=13 AND id=3;"
  'wrong-despawn-delay'="UPDATE smart_scripts SET action_param1=100 WHERE entryorguid=109465 AND source_type=0 AND id=0;"
  'wrong-scene-line'="UPDATE conversation_data SET textId=1 WHERE entry=2779 AND idx=0;"
  'missing-scene-actor'="DELETE FROM conversation_creature WHERE entry=2779 AND id=0;"
  'wrong-scene-duration'="UPDATE conversation_creature SET duration=1 WHERE entry=2779 AND id=1;"
  'wrong-native-receiver'="UPDATE creature SET id=109350 WHERE guid=373396;"
  'wrong-prior-ai'="UPDATE creature_template SET AIName='' WHERE entry=109465;"
  'custom-helper-script'="UPDATE creature_template SET ScriptName='custom' WHERE entry=109350;"
  'wrong-hero-template'="UPDATE creature_template SET faction=1 WHERE entry=109352;"
  'wrong-scaling'="UPDATE creature_template_scaling SET LevelScalingMin=1 WHERE Entry=109391;"
  'changed-yulon-addon'="UPDATE creature_template_addon SET auras='' WHERE entry=109391;"
  'new-prior-addon'="INSERT INTO creature_addon(guid) VALUES(373425);"
  'changed-pickup'="UPDATE smart_scripts SET action_param1=1 WHERE entryorguid=251605 AND source_type=1 AND id=0;"
  'changed-pickup-link'="UPDATE spell_linked_spell SET spell_effect=1 WHERE spell_trigger=-217814;"
  'custom-eo-condition'="INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,ConditionTypeOrReference,ConditionValue1) VALUES(22,1,103,1,1);"
  'custom-spell-condition'="INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,ConditionTypeOrReference,ConditionValue1) VALUES(17,0,218883,1,1);"
 }
 foreach($name in $variants.Keys){$bad=Query ($certified+$variants[$name]+(Snapshot 'before')+$migration+(Snapshot 'after')) -Refusal;foreach($table in $filters.Keys){if((Signature $bad "before-$table") -cne (Signature $bad "after-$table")){throw "Refusal mutated rows: $name/$table"}};"PASS refusal $name"}
 if($VerifyLiveData){
  $live=Query (Snapshot 'live');foreach($table in $filters.Keys){if((Signature $live "live-$table") -cne (Signature $rows "first-$table")){throw "Actual live WaveEntry differs: $table"}}
  # UpdateFetcher::ReadSQLUpdate uses Windows text-mode ifstream: CRLF is LF
  # in the hashed payload. A raw file-byte SHA1 is not its updater hash.
  $payload=[IO.File]::ReadAllText($MigrationFile).Replace("`r`n","`n")
  if($payload.Contains([string][char]26)){throw 'Unexpected DOS text EOF in migration'}
  $sha=[Security.Cryptography.SHA1]::Create()
  try{$hash=[Convert]::ToHexString($sha.ComputeHash([Text.Encoding]::UTF8.GetBytes($payload)))}finally{$sha.Dispose()}
  $name=[IO.Path]::GetFileName($MigrationFile)
  $history=Query "SELECT 'updater',hash,state FROM updates WHERE name='$name';"
  if((Signature $history 'updater') -cne "$hash`tRELEASED"){throw 'Actual native updater payload hash/state differs'}
 }
 'PASS native wave-entry TEMP27: exact EO ScriptName/11-binding delta; original SmartAI, receivers, conversation, native pickup and shared rules retained; idempotence/27 strict refusals and optional full live equality'
}
