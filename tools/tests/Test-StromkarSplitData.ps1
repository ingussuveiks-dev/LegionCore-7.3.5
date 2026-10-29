param([string]$MigrationFile,[switch]$VerifyLiveData)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/stromkar-split-data"
if(!$MigrationFile){$MigrationFile="$repo/sql/pending/world/2026_10_29_08_stromkar_shadowy_split.sql";if(!(Test-Path $MigrationFile)){$MigrationFile="$repo/sql/updates/world/2026_10_29_08_stromkar_shadowy_split.sql"}}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -Test {
 $migration=Get-Content $MigrationFile -Raw
 $connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $script:queryIndex=0
 function Query([string]$sql,[switch]$Refusal){
  $prior=$env:MYSQL_PWD;$errorFile="$out/sql-$script:queryIndex.stderr";++$script:queryIndex
  try{$env:MYSQL_PWD=$connection[3];$ErrorActionPreference='Continue';$rows=@($sql|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names --force 2>$errorFile);$code=$LASTEXITCODE;$ErrorActionPreference='Stop';$errors=[string](Get-Content $errorFile -Raw)
   if($Refusal){if($errors -notmatch 'Stromkar split actor/graph/spell bindings differ; review before promotion' -or @([regex]::Matches($errors,'ERROR [0-9]+')).Count -ne 1){throw "Unexpected split refusal: $errorFile"}}
   elseif($code -ne 0 -or $errors){throw "Split SQL failed: $errorFile"};return ,$rows
  }finally{$env:MYSQL_PWD=$prior}
 }
 $filters=[ordered]@{creature='id=103339';creature_template='entry IN(103339,104737)';smart_scripts='entryorguid IN(103339,104737)';spell_script_names='spell_id IN(207244,207249,207258,207275,207297,207299)'}
 $order=@{creature='guid';creature_template='entry';smart_scripts='entryorguid,source_type,id,link';spell_script_names='spell_id,ScriptName'}
 $setup='';foreach($table in $filters.Keys){$setup+="CREATE TEMPORARY TABLE saved_$table LIKE $table;INSERT INTO saved_$table SELECT * FROM $table WHERE $($filters[$table]);CREATE TEMPORARY TABLE $table LIKE saved_$table;INSERT INTO $table SELECT * FROM saved_$table;`n"}
 $baseline="UPDATE creature_template SET ScriptName='' WHERE entry=103339;UPDATE smart_scripts SET target_type=21,target_param1=50 WHERE entryorguid=103339 AND source_type=0 AND id=4;DELETE FROM spell_script_names WHERE ScriptName='spell_stromkar_split_transport';"
 function Snapshot([string]$name){$sql='';foreach($table in $filters.Keys){$sql+="SELECT '$name-$table',t.* FROM $table t ORDER BY $($order[$table]);"};return $sql}
 function Signature($rows,[string]$name){[string]::Join("`n",[string[]]@($rows|Where-Object {$_ -like "$name`t*"}|ForEach-Object {$_.Substring($name.Length+1)}))}
 $rows=Query ($setup+$migration+$baseline+(Snapshot 'before')+$migration+(Snapshot 'first')+$migration+(Snapshot 'second')+"SELECT 'row',id,link,event_type,action_type,action_param1,action_param2,target_type,target_param1 FROM smart_scripts WHERE entryorguid=103339 AND source_type=0 ORDER BY id;")
 foreach($table in $filters.Keys){if((Signature $rows "first-$table") -cne (Signature $rows "second-$table")){throw "Split not idempotent: $table"}}
 if((Signature $rows 'before-creature') -cne (Signature $rows 'first-creature')){throw 'Split changed original spawn'}
 $roundtrip=Query ($setup+$baseline+(Snapshot 'before')+$migration+$baseline+(Snapshot 'after'))
 foreach($table in $filters.Keys){if((Signature $roundtrip "before-$table") -cne (Signature $roundtrip "after-$table")){throw "Unrelated split metadata changed: $table"}}
 [IO.File]::WriteAllLines("$out/rows.tsv",[string[]]@($rows|Where-Object {$_ -like "row`t*"}|ForEach-Object {$_.Substring(4)}))
 [IO.File]::WriteAllLines("$out/temp.tsv",[string[]]$rows)
 $variants=[ordered]@{
  'actor-script'="UPDATE creature_template SET ScriptName='custom' WHERE entry=103339;"
  'actor-flags'='UPDATE creature_template SET unit_flags=1 WHERE entry=103339;'
  'clone-script'="UPDATE creature_template SET ScriptName='custom' WHERE entry=104737;"
  'clone-spell'='UPDATE creature_template SET spell1=207275 WHERE entry=104737;'
  'custom-binding'="INSERT INTO spell_script_names VALUES(207244,'custom');"
  'extra-event'="INSERT INTO smart_scripts(entryorguid,source_type,id,comment) VALUES(103339,0,99,'custom');"
  'altered-cast'='UPDATE smart_scripts SET action_param1=1 WHERE entryorguid=103339 AND id=1;'
  'altered-link'='UPDATE smart_scripts SET link=0 WHERE entryorguid=103339 AND id=3;'
  'altered-target'='UPDATE smart_scripts SET target_param1=55 WHERE entryorguid=103339 AND id=4;'
  'wrong-map'='UPDATE creature SET map=1 WHERE guid=370996;'
  'extra-spawn'='UPDATE creature SET guid=1 WHERE guid=370996;INSERT INTO creature SELECT * FROM saved_creature;'
  'applied-missing'='DELETE FROM spell_script_names WHERE spell_id=207297;'
  'applied-actor'="UPDATE creature_template SET ScriptName='' WHERE entry=103339;"
 }
 foreach($name in $variants.Keys){$base=$setup+$baseline+$(if($name -like 'applied-*'){$migration}else{''});$result=Query ($base+$variants[$name]+(Snapshot 'before')+$migration+(Snapshot 'after')) -Refusal
  foreach($table in $filters.Keys){if((Signature $result "before-$table") -cne (Signature $result "after-$table")){throw "Custom state overwritten: $name"}}
  [IO.File]::WriteAllLines("$out/refusal-$name.tsv",[string[]]$result)
 }
 if($VerifyLiveData){$sha1=[Security.Cryptography.SHA1]::Create();try{$hash=([BitConverter]::ToString($sha1.ComputeHash([Text.Encoding]::UTF8.GetBytes($migration.Replace("`r`n","`n"))))).Replace('-','')}finally{$sha1.Dispose()};$live=Query "SELECT spell_id,ScriptName FROM spell_script_names WHERE $($filters.spell_script_names) ORDER BY spell_id;SELECT 'released',COUNT(*) FROM updates WHERE name='2026_10_29_08_stromkar_shadowy_split.sql' AND hash='$hash' AND state='RELEASED';SELECT 'binding',ScriptName FROM creature_template WHERE entry=103339;SELECT 'credit',target_type,target_param1 FROM smart_scripts WHERE entryorguid=103339 AND source_type=0 AND id=4;";if($live -notcontains "released`t1" -or $live -notcontains "binding`tnpc_stromkar_illusionist" -or $live -notcontains "credit`t7`t0"){throw 'Live split binding/hash differs'}}
 'PASS full-schema TEMP split repeat/roundtrip and13 custom-state refusals; exact native five rows/six bindings and invoker credit export preserved. No live writes by fixture.'
}
