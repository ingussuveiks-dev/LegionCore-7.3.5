param([string]$MigrationFile,[switch]$VerifyLiveData,[string]$OutputDirectory="$PSScriptRoot/../../build-extractors/runtime-test/fuzan-temple-intro-data")
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out=[IO.Path]::GetFullPath($OutputDirectory)
if(!$MigrationFile){$MigrationFile="$repo/sql/pending/world/2026_11_01_03_fuzan_temple_intro_acknowledgement.sql";if(!(Test-Path $MigrationFile)){$MigrationFile="$repo/sql/updates/world/2026_11_01_03_fuzan_temple_intro_acknowledgement.sql"}}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -Test {
 $migration=Get-Content $MigrationFile -Raw
 $fixture=Get-Content "$PSScriptRoot/FuZanTempleIntroOriginal.json" -Raw|ConvertFrom-Json
 $connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $script:index=0
 function Query([string]$sql,[switch]$Refusal){
  $stem="$out/sql-$script:index";++$script:index
  [IO.File]::WriteAllText("$stem.sql",$sql+"`nSELECT 'EOF','fuzan-intro-ack-data';`n")
  $old=$env:MYSQL_PWD
  try{
   $env:MYSQL_PWD=$connection[3]
   $code=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -Arguments @("--host=$($connection[0])","--port=$($connection[1])","--user=$($connection[2])","--database=$($connection[4])",'--default-character-set=utf8mb4','--batch','--raw','--skip-column-names','--force') -InputFile "$stem.sql" -OutputFile "$stem.stdout" -ErrorFile "$stem.stderr"
   $rows=@(Get-Content "$stem.stdout" -Encoding UTF8);$errors=[IO.File]::ReadAllText("$stem.stderr")
   if($rows[-1] -cne "EOF`tfuzan-intro-ack-data"){throw 'Fu Zan acknowledgement SQL incomplete EOF'}
   if($Refusal){if($errors -notmatch 'fuzan_intro_ack_native_guard' -or @([regex]::Matches($errors,'ERROR [0-9]+')).Count -ne 1){throw "Unexpected refusal diagnostics: $stem"}}
   elseif($code -or $errors){throw "Fu Zan acknowledgement SQL failed: $stem"}
   return ,$rows
  }finally{$env:MYSQL_PWD=$old}
 }
 $filters=[ordered]@{
  smart_scripts='entryorguid=109413 AND source_type IN(0,9)'
  conditions='(SourceTypeOrReferenceId=22 AND SourceEntry=109413) OR (SourceTypeOrReferenceId=17 AND SourceEntry IN(217774,217775))'
  creature_template='entry IN(109413,109353)';creature='id IN(109413,109353)'
  conversation_data='entry=2770';conversation_creature='entry=2770';conversation_actor='entry=2770'
  spell_script_names='spell_id IN(217774,217775)'
 }
 $orders=@{smart_scripts='source_type,id';conditions='SourceTypeOrReferenceId,SourceGroup,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1';creature_template='entry';creature='id,guid';conversation_data='idx';conversation_creature='id';conversation_actor='id';spell_script_names='spell_id,ScriptName'}
 $setup='';foreach($table in $filters.Keys){$setup+="CREATE TEMPORARY TABLE saved_$table LIKE $table;INSERT INTO saved_$table SELECT * FROM $table WHERE $($filters[$table]);CREATE TEMPORARY TABLE $table LIKE saved_$table;INSERT INTO $table SELECT * FROM saved_$table;`n"}
 # The first guarded migration certifies either the exact old or exact new
 # binding state before restoring only this known delta in private TEMP tables.
 $restore="UPDATE creature_template SET ScriptName='' WHERE entry=109413 AND ScriptName='npc_fuzan_temple_intro';DELETE FROM spell_script_names WHERE (spell_id=217775 AND ScriptName='spell_fuzan_temple_intro') OR (spell_id=217774 AND ScriptName='spell_fuzan_temple_complete');`n"
 $expected="UPDATE creature_template SET ScriptName='npc_fuzan_temple_intro' WHERE entry=109413;INSERT INTO spell_script_names VALUES(217775,'spell_fuzan_temple_intro'),(217774,'spell_fuzan_temple_complete');`n"
 $certified=$setup+$migration+$restore
 function Snapshot([string]$tag){$s='';foreach($table in $filters.Keys){$s+="SELECT '$tag-$table',t.* FROM $table t WHERE $($filters[$table]) ORDER BY $($orders[$table]);`n"};$s}
 function Signature($rows,[string]$tag){[string]::Join("`n",[string[]]@($rows|Where-Object {$_ -like "$tag`t*"}|ForEach-Object {$_.Substring($tag.Length+1)}))}
 $pins=''
 foreach($table in $filters.Keys){
  if(!@($fixture.$table).Count){continue}
  $columns=@($fixture.$table[0].PSObject.Properties.Name|ForEach-Object {'`'+$_+'`'}) -join ','
  $pins+="SELECT 'pin-$table',$columns FROM $table WHERE $($filters[$table]) ORDER BY $($orders[$table]);`n"
 }
 $rows=Query ($certified+$pins+(Snapshot 'original')+$expected+(Snapshot 'expected')+$restore+$migration+(Snapshot 'first')+$migration+(Snapshot 'second'))
 foreach($table in $filters.Keys){
  if((Signature $rows "expected-$table") -cne (Signature $rows "first-$table") -or (Signature $rows "first-$table") -cne (Signature $rows "second-$table")){throw "Fu Zan exact delta/idempotence differs: $table"}
  if($table -notin @('creature_template','spell_script_names') -and (Signature $rows "original-$table") -cne (Signature $rows "first-$table")){throw "Unrelated Fu Zan rows changed: $table"}
  if(@($fixture.$table).Count){
   $expectedPin=@($fixture.$table|ForEach-Object {($_.PSObject.Properties.Value|ForEach-Object {[Convert]::ToString($_,[Globalization.CultureInfo]::InvariantCulture)}) -join "`t"})
   if(($expectedPin -join "`n") -cne (Signature $rows "pin-$table")){throw "Pinned native metadata differs: $table"}
  }
 }
 $variants=[ordered]@{
  'root-range'='UPDATE smart_scripts SET event_param2=17 WHERE source_type=0 AND id=0;'
  'broken-link'='UPDATE smart_scripts SET link=2 WHERE source_type=0 AND id=0;'
  'altered-timer'='UPDATE smart_scripts SET event_param1=1 WHERE source_type=9;'
  'altered-credit'='UPDATE smart_scripts SET action_param2=1 WHERE source_type=9;'
  'custom-script'="UPDATE creature_template SET ScriptName='custom' WHERE entry=109413;"
  'custom-guide'="UPDATE creature_template SET ScriptName='custom' WHERE entry=109353;"
  'wrong-spawn'='UPDATE creature SET map=1 WHERE id=109413;'
  'guide-phase'="UPDATE creature SET phaseId='1' WHERE id=109353;"
  'guide-position'='UPDATE creature SET position_x=position_x+1 WHERE id=109353;'
  'partial-conditions'='DELETE FROM conditions WHERE ElseGroup=1 AND ConditionTypeOrReference=31;'
  'altered-admission'='UPDATE conditions SET ConditionValue1=1 WHERE ConditionTypeOrReference=15;'
  'changed-comment'="UPDATE conditions SET Comment='custom' WHERE ElseGroup=0 AND ConditionTypeOrReference=31;"
  'missing-line'='DELETE FROM conversation_data WHERE idx=8;'
  'line-time'='UPDATE conversation_data SET textId=1 WHERE idx=8;'
  'line-flags'='UPDATE conversation_data SET unk2=0 WHERE idx=8;'
  'actor-duration'='UPDATE conversation_creature SET duration=1 WHERE id=0;'
  'actor-source'='UPDATE conversation_creature SET creatureId=1 WHERE id=2;'
  'partial-bindings'="INSERT INTO spell_script_names VALUES(217775,'spell_fuzan_temple_intro');"
  'template-only'="UPDATE creature_template SET ScriptName='npc_fuzan_temple_intro' WHERE entry=109413;"
  'bindings-only'="INSERT INTO spell_script_names VALUES(217775,'spell_fuzan_temple_intro'),(217774,'spell_fuzan_temple_complete');"
  'custom-binding'="INSERT INTO spell_script_names VALUES(217775,'custom');"
  'spell-condition'="INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment) VALUES(17,0,217775,0,0,15,0,512,0,0,0,0,'','custom');"
 }
 foreach($name in $variants.Keys){
  $bad=Query ($certified+$variants[$name]+(Snapshot 'before')+$migration+(Snapshot 'after')) -Refusal
  foreach($table in $filters.Keys){if((Signature $bad "before-$table") -cne (Signature $bad "after-$table")){throw "Refusal mutated rows: $name/$table"}}
  "PASS refusal $name"
 }
 if($VerifyLiveData){$live=Query (Snapshot 'live');foreach($table in $filters.Keys){if((Signature $live "live-$table") -cne (Signature $rows "first-$table")){throw "Live Fu Zan acknowledgement differs: $table"}}}
 'PASS Fu Zan acknowledgement full-schema TEMP8: exact template+two spell bindings only; native3 Smart/14 admission/9 lines/3 roles preserved; idempotence/22 refusals and optional live equality'
}
