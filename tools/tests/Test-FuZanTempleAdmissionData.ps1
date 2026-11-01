param([string]$MigrationFile,[switch]$VerifyLiveData,[string]$OutputDirectory="$PSScriptRoot/../../build-extractors/runtime-test/fuzan-temple-admission-data")
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out=[IO.Path]::GetFullPath($OutputDirectory)
if(!$MigrationFile){$MigrationFile="$repo/sql/pending/world/2026_11_01_02_fuzan_temple_intro_admission.sql";if(!(Test-Path $MigrationFile)){$MigrationFile="$repo/sql/updates/world/2026_11_01_02_fuzan_temple_intro_admission.sql"}}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -Test {
 $migration=Get-Content $MigrationFile -Raw
 $fixture=Get-Content "$PSScriptRoot/FuZanTempleAdmissionOriginal.json" -Raw|ConvertFrom-Json
 $connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $script:index=0
 function Query([string]$sql,[switch]$Refusal){
  $stem="$out/sql-$script:index";++$script:index
  [IO.File]::WriteAllText("$stem.sql",$sql+"`nSELECT 'EOF','fuzan-intro-data';`n")
  $old=$env:MYSQL_PWD
  try{
   $env:MYSQL_PWD=$connection[3]
   $code=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -Arguments @("--host=$($connection[0])","--port=$($connection[1])","--user=$($connection[2])","--database=$($connection[4])",'--default-character-set=utf8mb4','--batch','--raw','--skip-column-names','--force') -InputFile "$stem.sql" -OutputFile "$stem.stdout" -ErrorFile "$stem.stderr"
   $rows=@(Get-Content "$stem.stdout" -Encoding UTF8);$errors=[IO.File]::ReadAllText("$stem.stderr")
   if($rows[-1] -cne "EOF`tfuzan-intro-data"){throw 'Fu Zan SQL incomplete EOF'}
   if($Refusal){if($errors -notmatch 'fuzan_intro_native_guard' -or @([regex]::Matches($errors,'ERROR [0-9]+')).Count -ne 1){throw "Unexpected refusal diagnostics: $stem"}}
   elseif($code -or $errors){throw "Fu Zan intro SQL failed: $stem"}
   return ,$rows
  }finally{$env:MYSQL_PWD=$old}
 }
 $filters=[ordered]@{smart_scripts='entryorguid=109413 AND source_type IN(0,9)';conditions='SourceTypeOrReferenceId=22 AND SourceEntry=109413';creature_template='entry=109413';creature='id=109413'}
 $orders=@{smart_scripts='source_type,id';conditions='SourceGroup,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1';creature_template='entry';creature='guid'}
 $setup='';foreach($table in $filters.Keys){$setup+="CREATE TEMPORARY TABLE saved_$table LIKE $table;INSERT INTO saved_$table SELECT * FROM $table WHERE $($filters[$table]);CREATE TEMPORARY TABLE $table LIKE saved_$table;INSERT INTO $table SELECT * FROM saved_$table;`n"}
 # World03 legitimately binds the new scoped controller while retaining
 # this exact admission policy. Certify its complete two-spell delta before
 # normalizing only the ScriptName in this older migration's TEMP fixture.
 $normalize="UPDATE creature_template SET ScriptName='' WHERE entry=109413 AND ScriptName='npc_fuzan_temple_intro' AND (SELECT COUNT(*) FROM spell_script_names WHERE spell_id IN(217775,217774))=2 AND EXISTS(SELECT 1 FROM spell_script_names WHERE spell_id=217775 AND ScriptName='spell_fuzan_temple_intro') AND EXISTS(SELECT 1 FROM spell_script_names WHERE spell_id=217774 AND ScriptName='spell_fuzan_temple_complete');`n"
 $baseline=$normalize+"DELETE FROM conditions WHERE $($filters.conditions);`n"
 function Snapshot([string]$tag){$s='';foreach($table in $filters.Keys){$s+="SELECT '$tag-$table',t.* FROM $table t WHERE $($filters[$table]) ORDER BY $($orders[$table]);`n"};$s}
 function Signature($rows,[string]$tag){[string]::Join("`n",[string[]]@($rows|Where-Object {$_ -like "$tag`t*"}|ForEach-Object {$_.Substring($tag.Length+1)}))}
 $rows=Query ($setup+$baseline+(Snapshot 'original')+$migration+(Snapshot 'first')+$migration+(Snapshot 'second'))
 foreach($table in $filters.Keys){if((Signature $rows "first-$table") -cne (Signature $rows "second-$table")){throw "Fu Zan admission not idempotent: $table"};if($table -ne 'conditions' -and (Signature $rows "original-$table") -cne (Signature $rows "first-$table")){throw "Unrelated Fu Zan rows changed: $table"}}
 $expectedSmart=@($fixture.smart_scripts|Sort-Object source_type,id|ForEach-Object {($_.PSObject.Properties.Value|ForEach-Object {[Convert]::ToString($_,[Globalization.CultureInfo]::InvariantCulture)}) -join "`t"})
 if(($expectedSmart -join "`n") -cne (Signature $rows 'original-smart_scripts')){throw 'Live full native intro graph differs from pinned fixture'}
 $expectedConditions=@($fixture.admission_conditions|Sort-Object SourceGroup,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1|ForEach-Object {($_.PSObject.Properties.Value|ForEach-Object {[Convert]::ToString($_,[Globalization.CultureInfo]::InvariantCulture)}) -join "`t"})
 if(($expectedConditions -join "`n") -cne (Signature $rows 'first-conditions')){throw 'Expected precisely14 source22 admission predicates'}
 [IO.File]::WriteAllLines("$out/conditions.tsv",[string[]]$expectedConditions)
 [IO.File]::WriteAllLines("$out/smart_scripts.tsv",[string[]]$expectedSmart)
 $variants=[ordered]@{
  'root-range'='UPDATE smart_scripts SET event_param2=17 WHERE source_type=0 AND id=0;'
  'root-repeat'='UPDATE smart_scripts SET event_flags=0 WHERE source_type=0 AND id=0;'
  'wrong-spell'='UPDATE smart_scripts SET action_param1=1 WHERE source_type=0 AND id=0;'
  'wrong-target'='UPDATE smart_scripts SET target_type=1 WHERE source_type=0 AND id=0;'
  'broken-link'='UPDATE smart_scripts SET link=2 WHERE source_type=0 AND id=0;'
  'altered-timer'='UPDATE smart_scripts SET event_param1=1 WHERE source_type=9;'
  'altered-credit'='UPDATE smart_scripts SET action_param2=1 WHERE source_type=9;'
  'custom-script'="UPDATE creature_template SET ScriptName='custom' WHERE entry=109413;"
  'wrong-spawn'='UPDATE creature SET map=1 WHERE id=109413;'
  'partial-conditions'=$migration+'DELETE FROM conditions WHERE ElseGroup=1 AND ConditionTypeOrReference=31;'
  'altered-admission'=$migration+'UPDATE conditions SET ConditionValue1=1 WHERE ConditionTypeOrReference=15;'
  'changed-comment'=$migration+"UPDATE conditions SET Comment='custom' WHERE ElseGroup=0 AND ConditionTypeOrReference=31;"
 }
 foreach($name in $variants.Keys){
  $bad=Query ($setup+$baseline+$variants[$name]+(Snapshot 'before')+$migration+(Snapshot 'after')) -Refusal
  foreach($table in $filters.Keys){if((Signature $bad "before-$table") -cne (Signature $bad "after-$table")){throw "Refusal mutated rows: $name/$table"}}
  "PASS refusal $name"
 }
 if($VerifyLiveData){$live=Query ($setup+$normalize+(Snapshot 'live'));foreach($table in $filters.Keys){if((Signature $live "live-$table") -cne (Signature $rows "first-$table")){throw "Live Fu Zan admission differs: $table"}}}
 'PASS Fu Zan intro full-schema TEMP4 tables:14 exact admission predicates only; unchanged native3-row cast/list/30s event, template and spawn; idempotence/12 refusals and optional live equality'
}
