param([string]$MigrationFile,[switch]$VerifyLiveData)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/stromkar-terminal-body-data"
if(!$MigrationFile){$MigrationFile="$repo/sql/pending/world/2026_10_29_06_stromkar_terminal_body.sql";if(!(Test-Path $MigrationFile)){$MigrationFile="$repo/sql/updates/world/2026_10_29_06_stromkar_terminal_body.sql"}}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -Test {
 $migration=Get-Content $MigrationFile -Raw
 $connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $script:queryIndex=0
 function Query([string]$sql,[switch]$Refusal){
  $prior=$env:MYSQL_PWD;$errorFile="$out/sql-$script:queryIndex.stderr";++$script:queryIndex
  try{
   $env:MYSQL_PWD=$connection[3];$ErrorActionPreference='Continue'
   $rows=@($sql|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names --force 2>$errorFile)
   $code=$LASTEXITCODE;$ErrorActionPreference='Stop';[IO.File]::WriteAllText("$errorFile.exit.txt",[string]$code)
   $errors=[string](Get-Content $errorFile -Raw)
   if($Refusal){if($errors -notmatch 'Stromkar terminal body AI/list differ; review before promotion' -or @([regex]::Matches($errors,'ERROR [0-9]+')).Count -ne 1){throw "Unexpected refusal: $errorFile"}}
   elseif($code -ne 0 -or $errors){throw "Terminal body SQL failed: $errorFile"}
   return ,$rows
  }finally{$env:MYSQL_PWD=$prior}
 }
 $setup=''
 foreach($table in @('creature_template','smart_scripts')){
  $filter=if($table -eq 'creature_template'){'entry IN(104276,103398)'}else{'entryorguid IN(104276,103398)'}
  $setup+="CREATE TEMPORARY TABLE saved_$table LIKE $table;INSERT INTO saved_$table SELECT * FROM $table WHERE $filter;CREATE TEMPORARY TABLE $table LIKE saved_$table;INSERT INTO $table SELECT * FROM saved_$table;`n"
 }
 $baseline=@'
UPDATE smart_scripts SET action_param2=0 WHERE entryorguid=104276 AND source_type=9 AND id=0;
INSERT INTO smart_scripts(entryorguid,source_type,id,event_chance,event_flags,action_type,target_type,comment)
SELECT 104276,9,3,100,1,41,1,'TS - Despawn' WHERE NOT EXISTS(SELECT 1 FROM smart_scripts WHERE entryorguid=104276 AND source_type=9 AND id=3);
'@
 function Snapshot([string]$name){"SELECT '$name-actors',t.* FROM creature_template t ORDER BY entry;SELECT '$name-smart',t.* FROM smart_scripts t ORDER BY entryorguid,source_type,id,link;"}
 function Signature($rows,[string]$name){[string]::Join("`n",[string[]]@($rows|Where-Object {$_ -like "$name`t*"}|ForEach-Object {$_.Substring($name.Length+1)}))}
 $rows=Query ($setup+$migration+$baseline+(Snapshot 'before')+$migration+(Snapshot 'first')+$migration+(Snapshot 'second')+"SELECT 'cosmetic',action_param1,action_param2 FROM smart_scripts WHERE entryorguid=104276 AND source_type=9 AND id=0;SELECT 'despawn',COUNT(*) FROM smart_scripts WHERE entryorguid=104276 AND source_type=9 AND action_type=41;")
 foreach($table in @('actors','smart')){if((Signature $rows "first-$table") -cne (Signature $rows "second-$table")){throw 'Body migration not idempotent'}}
 if((Signature $rows 'before-actors') -cne (Signature $rows 'first-actors')){throw 'Actor metadata changed'}
 foreach($expected in @("cosmetic`t207454`t2","despawn`t0")){if($rows -notcontains $expected){throw "Terminal body result missing: $expected"}}
 # Reverse only the two intended changes in TEMP and compare every original column.
 $roundtrip=Query ($setup+$baseline+(Snapshot 'before')+$migration+$baseline+(Snapshot 'after'))
 if((Signature $roundtrip 'before-smart') -cne (Signature $roundtrip 'after-smart')){throw 'Unrelated native row/column changed'}
 [IO.File]::WriteAllLines("$out/temp.tsv",[string[]]$rows)
 $variants=@{
  'actor'="UPDATE creature_template SET ScriptName='custom' WHERE entry=104276;"
  'cast'='UPDATE smart_scripts SET action_param1=1 WHERE entryorguid=104276 AND source_type=9 AND id=0;'
  'credit'='UPDATE smart_scripts SET action_param2=1 WHERE entryorguid=104276 AND source_type=9 AND id=2;'
  'dialogue'='DELETE FROM smart_scripts WHERE entryorguid=104276 AND source_type=9 AND id=1;'
  'partial'='DELETE FROM smart_scripts WHERE entryorguid=104276 AND source_type=9 AND id=3;'
  'comment'="UPDATE smart_scripts SET comment='custom' WHERE entryorguid=104276 AND source_type=9 AND id=0;"
  'applied-cast'='UPDATE smart_scripts SET action_param2=0 WHERE entryorguid=104276 AND source_type=9 AND id=0;'
  'applied-extra'="INSERT INTO smart_scripts(entryorguid,source_type,id,comment)VALUES(104276,9,99,'custom');"
 }
 foreach($name in $variants.Keys){
  $base=$setup+$baseline+$(if($name -like 'applied-*'){$migration}else{''})
  $check=Query ($base+$variants[$name]+(Snapshot 'before')+$migration+(Snapshot 'after')) -Refusal
  foreach($table in @('actors','smart')){if((Signature $check "before-$table") -cne (Signature $check "after-$table")){throw "Custom state overwritten: $name"}}
  [IO.File]::WriteAllLines("$out/refusal-$name.tsv",[string[]]$check)
 }
 if($VerifyLiveData){
  $sha=(Get-FileHash $MigrationFile -Algorithm SHA1).Hash
  $live=Query "SELECT 'release',hash,state FROM updates WHERE name='2026_10_29_06_stromkar_terminal_body.sql';SELECT 'cosmetic',action_param1,action_param2 FROM smart_scripts WHERE entryorguid=104276 AND source_type=9 AND id=0;SELECT 'despawn',COUNT(*) FROM smart_scripts WHERE entryorguid=104276 AND source_type=9 AND action_type=41;"
  foreach($line in @("release`t$sha`tRELEASED","cosmetic`t207454`t2","despawn`t0")){if($live -notcontains $line){throw "Live body state differs: $line"}}
 }
 'PASS terminal body full-schema TEMP repeat/roundtrip, eight custom-state refusals and native Thoradin/credit/unrelated-column preservation. No live changes by fixture; actual client pose/dissolve/shrink remains separate.'
}
