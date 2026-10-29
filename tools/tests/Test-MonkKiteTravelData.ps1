param([string]$MigrationFile,[switch]$VerifyLiveData)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/monk-kite-travel-data"
New-Item -ItemType Directory -Force $out|Out-Null
if(!$MigrationFile){$MigrationFile="$repo/sql/pending/world/2026_10_29_05_monk_kite_native_travel.sql";if(!(Test-Path $MigrationFile)){$MigrationFile="$repo/sql/updates/world/2026_10_29_05_monk_kite_native_travel.sql"}}
$migration=Get-Content $MigrationFile -Raw
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$script:queryIndex=0
function Query([string]$sql,[switch]$Refusal){
 $previous=$env:MYSQL_PWD
 $errorFile="$out/sql-$script:queryIndex.stderr";++$script:queryIndex
 try{
  $env:MYSQL_PWD=$connection[3]
  $ErrorActionPreference='Continue'
  $rows=@($sql|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names --force 2>$errorFile)
  $code=$LASTEXITCODE
  [IO.File]::WriteAllText("$errorFile.exit.txt",[string]$code)
  $ErrorActionPreference='Stop'
  $errors=[string](Get-Content $errorFile -Raw)
  if($Refusal){
   # --force permits the post-refusal snapshot. Every mutation is also guarded
   # by @monk_kite_safe; the expected SIGNAL is a counterfactual, not a pass.
   if($errors -notmatch 'Monk kite native click/actions/binding differ; review before promotion' -or
      @([regex]::Matches($errors,'ERROR [0-9]+')).Count -ne 1){throw "Unexpected refusal diagnostics: $errorFile"}
  }elseif($code -ne 0 -or $errors){throw "Monk kite SQL failed (exit $code): $errorFile"}
  return ,$rows
 }finally{$env:MYSQL_PWD=$previous}
}

$setup=''
$filters=@{creature_template='entry IN(102531,100475)';npc_spellclick_spells='npc_entry=102531';spell_script_names='spell_id IN(202475,202481)';smart_scripts='entryorguid IN(102531,100475)';conditions='SourceTypeOrReferenceId=22 AND SourceEntry IN(102531,100475)';spell_target_position='id IN(202481,218172)'}
foreach($table in $filters.Keys){
 $setup+="CREATE TEMPORARY TABLE saved_$table LIKE $table;INSERT INTO saved_$table SELECT * FROM $table WHERE $($filters[$table]);CREATE TEMPORARY TABLE $table LIKE saved_$table;INSERT INTO $table SELECT * FROM saved_$table;`n"
}
$baseline=@'
DELETE FROM smart_scripts WHERE entryorguid=102531 AND source_type=0;
DELETE FROM spell_script_names WHERE spell_id=202475;
INSERT INTO smart_scripts(entryorguid,source_type,id,link,event_type,action_type,action_param1,target_type,comment)
VALUES(102531,0,0,1,73,33,102531,7,'Spell Click - KC'),(102531,0,1,0,61,85,202481,7,'Link - ICS');
'@
function Snapshot([string]$label){
 $s=''
 foreach($table in @('creature_template','npc_spellclick_spells','spell_script_names','smart_scripts','conditions','spell_target_position')){
  $order=@{creature_template='entry';npc_spellclick_spells='npc_entry,spell_id';spell_script_names='spell_id,ScriptName';smart_scripts='entryorguid,source_type,id';conditions='SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference';spell_target_position='id'}[$table]
  $s+="SELECT '$label-$table',t.* FROM $table t ORDER BY $order;`n"
 }
 return $s
}
function Signature($rows,[string]$label){[string]::Join("`n",[string[]]@($rows|Where-Object {$_ -like "$label`t*"}|ForEach-Object {$_.Substring($label.Length+1)}))}
$rows=Query ($setup+$migration+$baseline+(Snapshot 'before')+$migration+(Snapshot 'first')+$migration+(Snapshot 'second')+"SELECT 'state',COUNT(*) FROM smart_scripts WHERE entryorguid=102531 AND source_type=0;SELECT 'binding',spell_id,ScriptName FROM spell_script_names WHERE spell_id=202475;")
[IO.File]::WriteAllLines("$out/temp.tsv",[string[]]$rows)
foreach($table in $filters.Keys){
 if((Signature $rows "first-$table") -cne (Signature $rows "second-$table")){throw "Monk migration not idempotent: $table"}
 if($table -notin @('smart_scripts','spell_script_names') -and (Signature $rows "before-$table") -cne (Signature $rows "first-$table")){throw "Native Monk metadata changed: $table"}
 if($table -in @('smart_scripts','spell_script_names')){
  $pattern=if($table -eq 'smart_scripts'){'^102531\t0\t'}else{'^202475\t'}
  $a=@((Signature $rows "before-$table") -split "`n"|Where-Object {$_ -notmatch $pattern})
  $b=@((Signature $rows "first-$table") -split "`n"|Where-Object {$_ -notmatch $pattern})
  if(($a -join "`n") -cne ($b -join "`n")){throw "Unrelated Monk data changed: $table"}
 }
}
if($rows -notcontains "state`t0" -or $rows -notcontains "binding`t202475`tspell_monk_kite_travel"){throw 'Monk native binding/retired duplicate differs'}
$variants=[ordered]@{
 'ai'="UPDATE creature_template SET AIName='' WHERE entry=102531;"
 'script'="UPDATE creature_template SET ScriptName='custom' WHERE entry=102531;"
 'flags'='UPDATE creature_template SET npcflag=0 WHERE entry=102531;'
 'click-spell'='UPDATE npc_spellclick_spells SET spell_id=1 WHERE npc_entry=102531;'
 'click-caster'='UPDATE npc_spellclick_spells SET cast_flags=0 WHERE npc_entry=102531;'
 'missing-click'='DELETE FROM npc_spellclick_spells WHERE npc_entry=102531;'
 'custom-binding'="INSERT INTO spell_script_names VALUES(202475,'custom');"
 'credit'='UPDATE smart_scripts SET action_param1=1 WHERE entryorguid=102531 AND id=0;'
 'transfer'='UPDATE smart_scripts SET action_param1=1 WHERE entryorguid=102531 AND id=1;'
 'extra-action'="INSERT INTO smart_scripts(entryorguid,source_type,id,comment)VALUES(102531,0,99,'custom');"
 'custom-condition'="INSERT INTO conditions VALUES(22,1,102531,0,0,9,0,40633,0,0,0,0,'','custom');"
 'missing-destination'='DELETE FROM spell_target_position WHERE id=202481;'
 'custom-destination'='UPDATE spell_target_position SET target_map=870 WHERE id=202481;'
 'applied-missing-binding'='DELETE FROM spell_script_names WHERE spell_id=202475;'
 'applied-custom-binding'="UPDATE spell_script_names SET ScriptName='custom' WHERE spell_id=202475;"
 'applied-action'="INSERT INTO smart_scripts(entryorguid,source_type,id,comment)VALUES(102531,0,99,'custom');"
 'applied-extra-binding'="INSERT INTO spell_script_names VALUES(202475,'custom');"
}
foreach($name in $variants.Keys){
 $base=$setup+$migration+$(if($name -notlike 'applied-*'){$baseline}else{''})
 $result=Query ($base+$variants[$name]+(Snapshot 'before')+$migration+(Snapshot 'after')) -Refusal
 [IO.File]::WriteAllLines("$out/refusal-$name.tsv",[string[]]$result)
 foreach($table in $filters.Keys){if((Signature $result "before-$table") -cne (Signature $result "after-$table")){throw "Custom $name overwritten: $table"}}
}
if($VerifyLiveData){
 $hash=(Get-FileHash $MigrationFile -Algorithm SHA1).Hash
 $live=Query "SELECT 'release',hash,state FROM updates WHERE name='2026_10_29_05_monk_kite_native_travel.sql';SELECT 'state',COUNT(*) FROM smart_scripts WHERE entryorguid=102531 AND source_type=0;SELECT 'binding',spell_id,ScriptName FROM spell_script_names WHERE spell_id=202475;"
 foreach($line in @("release`t$hash`tRELEASED","state`t0","binding`t202475`tspell_monk_kite_travel")){if($live -notcontains $line){throw "Monk live metadata differs: $line"}}
}
'PASS Monk native travel full-schema TEMP repeat migration,17 complete original/applied custom refusals, exact click/destination/other-actor preservation and unique binding. No live metadata changed by this fixture.'
