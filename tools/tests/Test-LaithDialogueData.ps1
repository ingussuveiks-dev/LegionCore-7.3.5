param([string]$MigrationFile,[switch]$VerifyLiveData)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/laith-dialogue-data"
if(!$MigrationFile){$MigrationFile="$repo/sql/pending/world/2026_10_29_11_laith_dialogue_dispatch.sql";if(!(Test-Path $MigrationFile)){$MigrationFile="$repo/sql/updates/world/2026_10_29_11_laith_dialogue_dispatch.sql"}}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -Test {
 $migration=Get-Content $MigrationFile -Raw
 $c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $script:queryIndex=0
 function Query([string]$sql,[switch]$Refusal){
  $old=$env:MYSQL_PWD;$errorFile="$out/sql-$script:queryIndex.stderr";++$script:queryIndex
  [IO.File]::WriteAllText("$out/sql-$($script:queryIndex-1).sql",$sql)
  $previousConsole=[Console]::OutputEncoding;$previousPipe=$OutputEncoding
  try{$env:MYSQL_PWD=$c[3];[Console]::OutputEncoding=[Text.UTF8Encoding]::new($false);$OutputEncoding=[Text.UTF8Encoding]::new($false);$ErrorActionPreference='Continue';$rows=@($sql|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --default-character-set=utf8mb4 --batch --raw --skip-column-names --force 2>$errorFile);$code=$LASTEXITCODE;$ErrorActionPreference='Stop';$errors=[string](Get-Content $errorFile -Raw)
   [IO.File]::WriteAllLines("$out/sql-$($script:queryIndex-1).stdout",[string[]]$rows)
   if($Refusal){if($errors -notmatch 'Laith dialogue/conditions/actor differ; review before updating' -or @([regex]::Matches($errors,'ERROR [0-9]+')).Count -ne 1){throw "Unexpected Laith refusal: $errorFile"}}
   elseif($code -ne 0 -or $errors){throw "Laith SQL failed: $errorFile"};return ,$rows
  }finally{$env:MYSQL_PWD=$old;[Console]::OutputEncoding=$previousConsole;$OutputEncoding=$previousPipe}
 }
 $fixture=Get-Content "$PSScriptRoot/LaithDialogueOriginal.json" -Raw -Encoding UTF8|ConvertFrom-Json
 $filters=[ordered]@{smart_scripts='entryorguid=102459 AND source_type=0';conditions='SourceTypeOrReferenceId=22 AND SourceEntry=102459';creature_text='CreatureID=102459';creature_template='entry=102459'}
 $order=@{smart_scripts='entryorguid,source_type,id';conditions='SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference';creature_text='CreatureID,GroupID,ID';creature_template='entry'}
 function Value($v){if($null -eq $v){'NULL'}elseif($v -is [string]){if($v.Length){"CONVERT(0x"+([BitConverter]::ToString([Text.Encoding]::UTF8.GetBytes($v))).Replace('-','')+" USING utf8mb4)"}else{"''"}}else{[Convert]::ToString($v,[Globalization.CultureInfo]::InvariantCulture)}}
 $setup='';$baseline=''
 foreach($table in $filters.Keys){
  $setup+="CREATE TEMPORARY TABLE saved_$table LIKE $table;INSERT INTO saved_$table SELECT * FROM $table WHERE $($filters[$table]);CREATE TEMPORARY TABLE $table LIKE saved_$table;`n"
  $baseline+="DELETE FROM $table;`n"
  foreach($row in $fixture.$table){$cols=($row.PSObject.Properties.Name|ForEach-Object {"``$_``"}) -join ',';$values=($row.PSObject.Properties|ForEach-Object {Value $_.Value}) -join ',';$baseline+="INSERT INTO $table($cols) VALUES($values);`n"}
 }
 function Snapshot([string]$tag){$sql='';foreach($table in $filters.Keys){$sql+="SELECT '$tag-$table',t.* FROM $table t WHERE $($filters[$table]) ORDER BY $($order[$table]);"};$sql}
 function Signature($rows,[string]$tag){[string]::Join("`n",[string[]]@($rows|Where-Object {$_ -like "$tag`t*"}|ForEach-Object {$_.Substring($tag.Length+1)}))}
 $rows=Query ($setup+$baseline+(Snapshot 'original')+$migration+(Snapshot 'first')+$migration+(Snapshot 'second'))
 foreach($table in $filters.Keys){if((Signature $rows "first-$table") -cne (Signature $rows "second-$table")){throw "Not idempotent $table"};if($table -ne 'smart_scripts' -and (Signature $rows "original-$table") -cne (Signature $rows "first-$table")){throw "Unrelated table changed $table"}}
 $original=@($rows|Where-Object {$_ -like "original-smart_scripts`t*"}|ForEach-Object {$_.Substring(23)})
 $repaired=@($rows|Where-Object {$_ -like "first-smart_scripts`t*"}|ForEach-Object {$_.Substring(20)})
 [IO.File]::WriteAllLines("$out/original.tsv",[string[]]$original);[IO.File]::WriteAllLines("$out/repaired.tsv",[string[]]$repaired)
 [IO.File]::WriteAllLines("$out/sql-evidence.tsv",[string[]]$rows)
 if($original.Count -ne 6 -or $repaired.Count -ne 6){throw 'Expected six original/repaired actions'}
 for($i=0;$i -lt 6;++$i){$expected=$original[$i].Split("`t");if($expected[2] -eq '0'){$expected[3]='0'};if(($expected -join "`t") -cne $repaired[$i]){throw "Unexpected row mutation $i"}}
 $variants=[ordered]@{
  'wrong-text'='UPDATE smart_scripts SET action_param1=99 WHERE id=0;';'wrong-delay'='UPDATE smart_scripts SET action_param2=1 WHERE id=0;';
  'lost-text-over'='UPDATE smart_scripts SET event_type=61 WHERE id=1;';'wrong-attack'='UPDATE smart_scripts SET target_type=7 WHERE id=2;';
  'wrong-death'='UPDATE smart_scripts SET action_param1=0 WHERE id=3;';'extra-row'="INSERT INTO smart_scripts(entryorguid,source_type,id,comment) VALUES(102459,0,99,'custom');";
  'missing-row'='DELETE FROM smart_scripts WHERE id=1;';'custom-condition'="INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,ConditionTypeOrReference,ConditionValue1) VALUES(22,1,102459,9,1);";
  'custom-speech'="UPDATE creature_text SET Text='custom' WHERE GroupID=0;";'custom-ai'="UPDATE creature_template SET AIName='custom';";
  'applied-custom'='UPDATE smart_scripts SET action_param1=99 WHERE id=1;'
 }
 foreach($name in $variants.Keys){$base=$setup+$baseline+$(if($name -like 'applied-*'){$migration}else{''});$result=Query ($base+$variants[$name]+(Snapshot 'before')+$migration+(Snapshot 'after')) -Refusal
  foreach($table in $filters.Keys){if((Signature $result "before-$table") -cne (Signature $result "after-$table")){throw "Custom state changed $name/$table"}}
 }
 if($VerifyLiveData){$sha=[Security.Cryptography.SHA1]::Create();try{$hash=([BitConverter]::ToString($sha.ComputeHash([Text.Encoding]::UTF8.GetBytes($migration.Replace("`r`n","`n"))))).Replace('-','')}finally{$sha.Dispose()}
  $live=Query "SELECT 'row',id,link,event_type,event_param1,event_param2,action_type,action_param1,action_param2,target_type FROM smart_scripts WHERE entryorguid=102459 AND source_type=0 ORDER BY id;SELECT 'released',COUNT(*) FROM updates WHERE name='2026_10_29_11_laith_dialogue_dispatch.sql' AND hash='$hash' AND state='RELEASED';"
  if($live -notcontains "released`t1" -or $live -notcontains "row`t0`t0`t54`t0`t0`t1`t0`t3000`t1" -or $live -notcontains "row`t1`t2`t52`t0`t102459`t1`t1`t3000`t1"){throw 'Live Laith migration/hash differs'}
  $fullLive=Query (Snapshot 'live');foreach($table in $filters.Keys){if((Signature $fullLive "live-$table") -cne (Signature $rows "first-$table")){throw "Live differs from native regression $table"}}
 }
 'PASS full-schema TEMP Laith repeat, only root link1->0 changes; eleven custom-state refusals, retained delayed TEXT_OVER/LINK2 attack, six actions/two texts/actor/conditions'
}
