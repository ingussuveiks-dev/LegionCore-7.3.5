param([string]$MigrationFile,[switch]$VerifyLiveData)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/valzuun-intel-data"
if(!$MigrationFile){$MigrationFile="$repo/sql/pending/world/2026_10_29_16_valzuun_full_intel.sql";if(!(Test-Path $MigrationFile)){$MigrationFile="$repo/sql/updates/world/2026_10_29_16_valzuun_full_intel.sql"}}
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
   if($Refusal){if($errors -notmatch 'Valzuun dialogue/conditions/actor differ; review before updating' -or @([regex]::Matches($errors,'ERROR [0-9]+')).Count -ne 1){throw "Unexpected Valzuun refusal: $errorFile"}}
   elseif($code -ne 0 -or $errors){throw "Valzuun SQL failed: $errorFile"};return ,$rows
  }finally{$env:MYSQL_PWD=$old;[Console]::OutputEncoding=$previousConsole;$OutputEncoding=$previousPipe}
 }
 $fixture=Get-Content "$PSScriptRoot/ValzuunIntelOriginal.json" -Raw -Encoding UTF8|ConvertFrom-Json
 $filters=[ordered]@{smart_scripts='(entryorguid=105333 AND source_type=0) OR (entryorguid=10533300 AND source_type=9) OR (action_type IN(80,87,88) AND 10533300 IN(action_param1,action_param2,action_param3,action_param4,action_param5,action_param6))';conditions='(SourceTypeOrReferenceId=22 AND SourceEntry IN(105333,10533300)) OR (SourceTypeOrReferenceId=15 AND SourceGroup IN(19472,19473,19474))';gossip_menu_option='MenuID IN(19472,19473,19474)';creature_template='entry=105333';creature_text='CreatureID=105333'}
 $order=@{smart_scripts='entryorguid,source_type,id';conditions='SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference';gossip_menu_option='MenuID,OptionID';creature_template='entry';creature_text='CreatureID,GroupID,ID'}
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
 foreach($table in $filters.Keys){if((Signature $rows "first-$table") -cne (Signature $rows "second-$table")){throw "Not idempotent $table"};if($table -notin @('smart_scripts','conditions','creature_template') -and (Signature $rows "original-$table") -cne (Signature $rows "first-$table")){throw "Unrelated table changed $table"}}
 $original=@($rows|Where-Object {$_ -like "original-smart_scripts`t*"}|ForEach-Object {$_.Substring(23)})
 $repaired=@($rows|Where-Object {$_ -like "first-smart_scripts`t*"}|ForEach-Object {$_.Substring(20)})
 [IO.File]::WriteAllLines("$out/original.tsv",[string[]]$original);[IO.File]::WriteAllLines("$out/repaired.tsv",[string[]]$repaired)
 [IO.File]::WriteAllLines("$out/sql-evidence.tsv",[string[]]$rows)
 if($original.Count -ne 7 -or $repaired.Count -ne 2){throw 'Expected old seven actions and preserved41920 pair'}
 if($original[2] -cne $repaired[0] -or $original[3] -cne $repaired[1]){throw 'Later41920 actions changed'}
 $beforeMenuConditions=(@($rows|Where-Object {$_ -like "original-conditions`t15`t*"}|ForEach-Object {$_.Substring(20)}) -join "`n")
 $afterMenuConditions=(@($rows|Where-Object {$_ -like "first-conditions`t15`t*"}|ForEach-Object {$_.Substring(17)}) -join "`n")
 if($beforeMenuConditions -cne $afterMenuConditions -or @($rows|Where-Object {$_ -like "first-conditions`t22`t*"}).Count){throw 'Native menu gates changed or retired actor/list conditions remain'}
 $beforeActor=@($rows|Where-Object {$_ -like "original-creature_template`t*"})[0].Substring(27).Split("`t")
 $afterActor=@($rows|Where-Object {$_ -like "first-creature_template`t*"})[0].Substring(24)
 $cols=@($fixture.creature_template[0].PSObject.Properties.Name);$beforeActor[[Array]::IndexOf($cols,'AIName')]='';$beforeActor[[Array]::IndexOf($cols,'ScriptName')]='npc_valzuun_intel'
 if(($beforeActor -join "`t") -cne $afterActor){throw 'Unrelated actor fields changed'}
 $variants=[ordered]@{
  'password-option'='UPDATE smart_scripts SET event_param2=1 WHERE entryorguid=105333 AND source_type=0 AND id=0;';
  'credit'='UPDATE smart_scripts SET action_param1=105434 WHERE entryorguid=105333 AND source_type=0 AND id=0;';
  'target'='UPDATE smart_scripts SET target_type=0 WHERE entryorguid=105333 AND source_type=0 AND id=1;';
  'list'='UPDATE smart_scripts SET action_param1=10533301 WHERE entryorguid=105333 AND source_type=0 AND id=1;';
  'delay'='UPDATE smart_scripts SET event_param1=0 WHERE entryorguid=10533300 AND id=1;';
  'later-quest'='UPDATE smart_scripts SET action_param1=0 WHERE entryorguid=105333 AND source_type=0 AND id=3;';
  'extra-row'="INSERT INTO smart_scripts(entryorguid,source_type,id,comment) VALUES(105333,0,99,'custom');";
  'condition'="INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,ConditionTypeOrReference,ConditionValue1) VALUES(22,1,105333,9,1);";
  'menu'="UPDATE gossip_menu_option SET OptionText='custom' WHERE OptionID=0;";
  'ai'="UPDATE creature_template SET AIName='custom';";
  'text'="UPDATE creature_text SET BroadcastTextId=0 WHERE GroupID=3;";
  'applied-custom'="UPDATE creature_template SET ScriptName='custom';";
  'caller'="INSERT INTO smart_scripts(entryorguid,source_type,id,action_type,action_param1,comment) VALUES(999,0,0,80,10533300,'custom caller');";
  'applied-later'='DELETE FROM smart_scripts WHERE entryorguid=105333 AND id=3;'
 }
 foreach($name in $variants.Keys){$base=$setup+$baseline+$(if($name -like 'applied-*'){$migration}else{''});$result=Query ($base+$variants[$name]+(Snapshot 'before')+$migration+(Snapshot 'after')) -Refusal
  foreach($table in $filters.Keys){if((Signature $result "before-$table") -cne (Signature $result "after-$table")){throw "Custom state changed $name/$table"}}
 }
 if($VerifyLiveData){$sha=[Security.Cryptography.SHA1]::Create();try{$hash=([BitConverter]::ToString($sha.ComputeHash([Text.Encoding]::UTF8.GetBytes($migration.Replace("`r`n","`n"))))).Replace('-','')}finally{$sha.Dispose()}
  $live=Query "SELECT 'binding',AIName,ScriptName FROM creature_template WHERE entry=105333;SELECT 'released',COUNT(*) FROM updates WHERE name='2026_10_29_16_valzuun_full_intel.sql' AND hash='$hash' AND state='RELEASED';"
  if($live -notcontains "released`t1" -or $live -notcontains "binding`t`tnpc_valzuun_intel"){throw 'Live full intel migration/hash differs'}
  $fullLive=Query (Snapshot 'live');foreach($table in $filters.Keys){if((Signature $fullLive "live-$table") -cne (Signature $rows "first-$table")){throw "Live differs from native regression $table"}}
 }
 'PASS full-schema TEMP Valzuun full intel/idempotence: removes only retired password/list/condition rows and binds tested derived SmartAI; preserves41920 pair/all menu gates/four text groups/all other actor fields; fourteen custom-state refusals'
}
