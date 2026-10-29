param([string]$MigrationFile,[switch]$VerifyLiveData)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/modera-facing-data"
if(!$MigrationFile){$MigrationFile="$repo/sql/pending/world/2026_10_29_13_modera_facing_dispatch.sql";if(!(Test-Path $MigrationFile)){$MigrationFile="$repo/sql/updates/world/2026_10_29_13_modera_facing_dispatch.sql"}}
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
   if($Refusal){if($errors -notmatch 'Modera facing/conditions/actor differ; review before updating' -or @([regex]::Matches($errors,'ERROR [0-9]+')).Count -ne 1){throw "Unexpected Modera refusal: $errorFile"}}
   elseif($code -ne 0 -or $errors){throw "Modera SQL failed: $errorFile"};return ,$rows
  }finally{$env:MYSQL_PWD=$old;[Console]::OutputEncoding=$previousConsole;$OutputEncoding=$previousPipe}
 }
 $fixture=Get-Content "$PSScriptRoot/ModeraFacingOriginal.json" -Raw -Encoding UTF8|ConvertFrom-Json
 $filters=[ordered]@{smart_scripts='(entryorguid=90418 AND source_type=0) OR (entryorguid=9041700 AND source_type=9) OR (entryorguid=90417 AND source_type=0 AND id=15)';conditions='(SourceTypeOrReferenceId=22 AND ((SourceEntry IN(90418,9041700)) OR (SourceEntry=90417 AND SourceGroup=16 AND SourceId=0))) OR (SourceTypeOrReferenceId=15 AND SourceGroup=18558 AND SourceEntry=5)';gossip_menu_option='MenuID=18558 AND OptionID=5';creature_template='entry IN(90417,90418)'}
 $order=@{smart_scripts='entryorguid,source_type,id';conditions='SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference';gossip_menu_option='MenuID,OptionID';creature_template='entry'}
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
 if($original.Count -ne 24 -or $repaired.Count -ne 24){throw 'Expected 24 original/repaired actions'}
 for($i=0;$i -lt 24;++$i){$expected=$original[$i].Split("`t");if($expected[0] -eq '90418' -and $expected[2] -eq '0'){$expected[3]='0'};if(($expected -join "`t") -cne $repaired[$i]){throw "Unexpected row mutation $i"}}
 $variants=[ordered]@{
  'first-angle'='UPDATE smart_scripts SET target_o=0 WHERE entryorguid=90418 AND id=0;';'last-angle'='UPDATE smart_scripts SET target_o=0 WHERE entryorguid=90418 AND id=2;';
  'last-message'='UPDATE smart_scripts SET event_param1=1 WHERE entryorguid=90418 AND id=2;';'wrong-emote'='UPDATE smart_scripts SET action_param1=0 WHERE entryorguid=90418 AND id=1;';
  'producer-delay'='UPDATE smart_scripts SET event_param1=0 WHERE entryorguid=9041700 AND id=7;';'producer-message'='UPDATE smart_scripts SET action_param1=3 WHERE entryorguid=9041700 AND id=5;';
  'caller-option'='UPDATE smart_scripts SET event_param2=0 WHERE entryorguid=90417 AND id=15;';'extra-row'="INSERT INTO smart_scripts(entryorguid,source_type,id,comment) VALUES(90418,0,99,'custom');";
  'custom-condition'="INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,ConditionTypeOrReference,ConditionValue1) VALUES(22,1,90418,9,1);";
  'custom-menu'="UPDATE gossip_menu_option SET OptionText='custom';";'custom-ai'="UPDATE creature_template SET AIName='custom' WHERE entry=90418;";
  'applied-custom'='UPDATE smart_scripts SET target_o=0 WHERE entryorguid=90418 AND id=2;'
 }
 foreach($name in $variants.Keys){$base=$setup+$baseline+$(if($name -like 'applied-*'){$migration}else{''});$result=Query ($base+$variants[$name]+(Snapshot 'before')+$migration+(Snapshot 'after')) -Refusal
  foreach($table in $filters.Keys){if((Signature $result "before-$table") -cne (Signature $result "after-$table")){throw "Custom state changed $name/$table"}}
 }
 if($VerifyLiveData){$sha=[Security.Cryptography.SHA1]::Create();try{$hash=([BitConverter]::ToString($sha.ComputeHash([Text.Encoding]::UTF8.GetBytes($migration.Replace("`r`n","`n"))))).Replace('-','')}finally{$sha.Dispose()}
  $live=Query "SELECT 'row',id,link,event_type,event_param1,event_param2,action_type,target_o FROM smart_scripts WHERE entryorguid=90418 AND source_type=0 AND id IN(0,2) ORDER BY id;SELECT 'released',COUNT(*) FROM updates WHERE name='2026_10_29_13_modera_facing_dispatch.sql' AND hash='$hash' AND state='RELEASED';"
  if($live -notcontains "released`t1" -or $live -notcontains "row`t0`t0`t38`t1`t1`t66`t6.09605" -or $live -notcontains "row`t2`t0`t38`t3`t3`t66`t4.61951"){throw 'Live Modera migration/hash differs'}
  $fullLive=Query (Snapshot 'live');foreach($table in $filters.Keys){if((Signature $fullLive "live-$table") -cne (Signature $rows "first-$table")){throw "Live differs from native regression $table"}}
 }
 'PASS full-schema TEMP Modera repeat: only root link2->0 changes; twelve custom-state refusals, all24 consumer/producer/caller actions, menu, both actors and conditions retained'
}
