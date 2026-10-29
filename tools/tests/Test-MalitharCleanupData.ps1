param([string]$MigrationFile,[switch]$VerifyLiveData)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/malithar-cleanup-data"
if(!$MigrationFile){$MigrationFile="$repo/sql/pending/world/2026_10_29_12_malithar_claw_cleanup.sql";if(!(Test-Path $MigrationFile)){$MigrationFile="$repo/sql/updates/world/2026_10_29_12_malithar_claw_cleanup.sql"}}
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
   if($Refusal){if($errors -notmatch 'Malithar cleanup/conditions/actor differ; review before updating' -or @([regex]::Matches($errors,'ERROR [0-9]+')).Count -ne 1){throw "Unexpected Malithar refusal: $errorFile"}}
   elseif($code -ne 0 -or $errors){throw "Malithar SQL failed: $errorFile"};return ,$rows
  }finally{$env:MYSQL_PWD=$old;[Console]::OutputEncoding=$previousConsole;$OutputEncoding=$previousPipe}
 }
 $fixture=Get-Content "$PSScriptRoot/MalitharCleanupOriginal.json" -Raw -Encoding UTF8|ConvertFrom-Json
 $filters=[ordered]@{smart_scripts='entryorguid IN(101390,105331) AND source_type=0';conditions='SourceTypeOrReferenceId=22 AND SourceEntry IN(101390,105331)';creature_text='CreatureID IN(101390,105331)';creature_template='entry IN(101390,105331)'}
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
 foreach($table in $filters.Keys){if((Signature $rows "first-$table") -cne (Signature $rows "second-$table")){throw "Not idempotent $table"};if($table -notin @('smart_scripts','conditions') -and (Signature $rows "original-$table") -cne (Signature $rows "first-$table")){throw "Unrelated table changed $table"}}
 $original=@($rows|Where-Object {$_ -like "original-smart_scripts`t*"}|ForEach-Object {$_.Substring(23)})
 $repaired=@($rows|Where-Object {$_ -like "first-smart_scripts`t*"}|ForEach-Object {$_.Substring(20)})
 [IO.File]::WriteAllLines("$out/original.tsv",[string[]]$original);[IO.File]::WriteAllLines("$out/repaired.tsv",[string[]]$repaired)
 [IO.File]::WriteAllLines("$out/sql-evidence.tsv",[string[]]$rows)
 if($original.Count -ne 12 -or $repaired.Count -ne 14){throw 'Expected twelve original/fourteen repaired actions'}
 foreach($line in $original){$expected=$line.Split("`t");$prefix=($expected[0..2] -join "`t")+"`t";$actual=@($repaired|Where-Object {$_.StartsWith($prefix)})
  if($expected[0] -eq '101390' -and $expected[2] -eq '10'){$expected[5]='61'}
  if($expected[0] -eq '101390' -and $expected[2] -eq '2'){$expected[3]='0';$expected[14]='22';$expected[15]='1'}
  if($actual.Count -ne 1 -or ($expected -join "`t") -cne $actual[0]){throw "Unexpected native row mutation $prefix"}
 }
 foreach($id in @(11,12)){
  $expected=@('0')*31;$expected[0]='101390';$expected[2]=[string]$id;$expected[4]='';$expected[6]='1';$expected[7]='100';$expected[21]='1'
  if($id -eq 11){$expected[5]='1';$expected[11]='500';$expected[12]='500';$expected[14]='11';$expected[15]='208720';$expected[30]='Pending Bear Form - retry native self cast'}
  else{$expected[3]='3';$expected[5]='60';$expected[11]='100';$expected[12]='100';$expected[14]='22';$expected[15]='2';$expected[30]='Actual Bear Form aura - release native dialogue and cleanup'}
  if($repaired -notcontains ($expected -join "`t")){throw "Wrong new event $id"}
 }
 $conditionRows=@($rows|Where-Object {$_ -like "first-conditions`t*"}|ForEach-Object {$_.Substring("first-conditions`t".Length)})
 if($conditionRows.Count -ne 11){throw 'Expected eleven base-actor transition conditions'}
 foreach($group in @(3,12,13)){
  $rules=@(@(22,1536,0,0),@(36,0,0,0),@(44,990,6,0));if($group -ne 3){$rules+=,@(1,208720,0,[int]($group -eq 12))}
  foreach($rule in $rules){$expected=@(22,$group,101390,0,0,$rule[0],1,$rule[1],$rule[2],0,$rule[3],0,'','Malithar native Fallen transition - base actor context') -join "`t";if($conditionRows -notcontains $expected){throw "Wrong condition $group/$($rule[0])"}}
 }
 [IO.File]::WriteAllLines("$out/conditions.tsv",[string[]]$conditionRows)
 $variants=[ordered]@{
  'wrong-transform'='UPDATE smart_scripts SET action_param1=99 WHERE entryorguid=101390 AND id=2;';'wrong-delay'='UPDATE smart_scripts SET action_param2=1 WHERE entryorguid=101390 AND id=3;';
  'wrong-link'='UPDATE smart_scripts SET link=0 WHERE entryorguid=101390 AND id=4;';'wrong-cleanup-target'='UPDATE smart_scripts SET target_param1=1 WHERE entryorguid=101390 AND id=10;';
  'wrong-death'='UPDATE smart_scripts SET action_param2=0 WHERE entryorguid=101390 AND id=7;';'extra-row'="INSERT INTO smart_scripts(entryorguid,source_type,id,comment) VALUES(101390,0,99,'custom');";
  'missing-row'='DELETE FROM smart_scripts WHERE entryorguid=101390 AND id=10;';'custom-condition'="INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,ConditionTypeOrReference,ConditionValue1) VALUES(22,11,101390,9,1);";
  'custom-speech'="UPDATE creature_text SET Text='custom' WHERE CreatureID=101390 AND GroupID=1;";'custom-ai'="UPDATE creature_template SET AIName='custom' WHERE entry=105331;";
  'applied-custom'='UPDATE smart_scripts SET action_param1=99 WHERE entryorguid=105331 AND id=0;';
  'applied-wrong-aura'='UPDATE conditions SET ConditionValue1=1 WHERE SourceGroup=13 AND ConditionTypeOrReference=1;';
  'applied-wrong-phase'='UPDATE smart_scripts SET event_phase_mask=0 WHERE entryorguid=101390 AND id=11;';
  'applied-once'='UPDATE smart_scripts SET event_flags=1 WHERE entryorguid=101390 AND id=11;';
  'applied-extra-condition'="INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,ConditionTypeOrReference,ConditionValue1) VALUES(22,13,101390,9,1);";
  'applied-missing-ack'='DELETE FROM smart_scripts WHERE entryorguid=101390 AND id=12;'
 }
 foreach($name in $variants.Keys){$base=$setup+$baseline+$(if($name -like 'applied-*'){$migration}else{''});$result=Query ($base+$variants[$name]+(Snapshot 'before')+$migration+(Snapshot 'after')) -Refusal
  foreach($table in $filters.Keys){if((Signature $result "before-$table") -cne (Signature $result "after-$table")){throw "Custom state changed $name/$table"}}
 }
 if($VerifyLiveData){$sha=[Security.Cryptography.SHA1]::Create();try{$hash=([BitConverter]::ToString($sha.ComputeHash([Text.Encoding]::UTF8.GetBytes($migration.Replace("`r`n","`n"))))).Replace('-','')}finally{$sha.Dispose()}
  $live=Query "SELECT 'row',entryorguid,id,link,event_type,event_param1,event_param2,action_type,action_param1,action_param2,target_type,target_param1 FROM smart_scripts WHERE entryorguid IN(101390,105331) AND source_type=0 ORDER BY entryorguid,id;SELECT 'released',COUNT(*) FROM updates WHERE name='2026_10_29_12_malithar_claw_cleanup.sql' AND hash='$hash' AND state='RELEASED';"
  if($live -notcontains "released`t1" -or $live -notcontains "row`t101390`t10`t0`t61`t0`t0`t45`t1`t1`t19`t105331"){throw 'Live Malithar migration/hash differs'}
  $fullLive=Query (Snapshot 'live');foreach($table in $filters.Keys){if((Signature $fullLive "live-$table") -cne (Signature $rows "first-$table")){throw "Live differs from native regression $table"}}
 }
 'PASS full-schema TEMP Malithar repeat: exact WP2 arm/LINK cleanup plus two scoped retry/actual-aura events and eleven base conditions; sixteen custom-state refusals, all other native actions/texts/both actors retained'
}
