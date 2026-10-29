param([string]$MigrationFile,[switch]$VerifyLiveData)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/desmond-search-data"
if(!$MigrationFile){$MigrationFile="$repo/sql/pending/world/2026_10_29_15_desmond_native_search.sql";if(!(Test-Path $MigrationFile)){$MigrationFile="$repo/sql/updates/world/2026_10_29_15_desmond_native_search.sql"}}
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
   if($Refusal){if($errors -notmatch 'Desmond dialogue/conditions/actor differ; review before updating' -or @([regex]::Matches($errors,'ERROR [0-9]+')).Count -ne 1){throw "Unexpected Desmond refusal: $errorFile"}}
   elseif($code -ne 0 -or $errors){throw "Desmond SQL failed: $errorFile"};return ,$rows
  }finally{$env:MYSQL_PWD=$old;[Console]::OutputEncoding=$previousConsole;$OutputEncoding=$previousPipe}
 }
 $fixture=Get-Content "$PSScriptRoot/DesmondSearchOriginal.json" -Raw -Encoding UTF8|ConvertFrom-Json
 $filters=[ordered]@{smart_scripts='entryorguid=105332 AND source_type=0';conditions='(SourceTypeOrReferenceId=22 AND SourceEntry=105332) OR (SourceTypeOrReferenceId=15 AND SourceGroup=19470) OR (SourceTypeOrReferenceId=17 AND SourceEntry=208755)';gossip_menu_option='MenuID=19470';creature_template='entry=105332';quest_objectives='ObjectID=105332';spell_script_names='spell_id=208755'}
 $order=@{smart_scripts='entryorguid,source_type,id';conditions='SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference';gossip_menu_option='MenuID,OptionID';creature_template='entry';quest_objectives='ID';spell_script_names='spell_id,ScriptName'}
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
 if($original.Count -ne 1 -or $repaired.Count -ne 2){throw 'Expected original root and repaired root/cast'}
 $expected=$original[0].Split("`t");$expected[14]='72';$expected[15]='0';$expected[30]='Desmond - search option - close gossip before native Searching cast'
 if(($expected -join "`t") -cne $repaired[0]){throw 'Unexpected search root mutation'}
 $cast=$repaired[1].Split("`t");if($cast[2] -ne '1' -or $cast[3] -ne '0' -or $cast[5] -ne '61' -or $cast[8] -ne '0' -or $cast[14] -ne '11' -or $cast[15] -ne '208755' -or $cast[16] -ne '16' -or $cast[21] -ne '7'){throw 'Native non-triggered player self cast differs'}
 $priorConditions=@($fixture.conditions);$newConditions=@($rows|Where-Object {$_ -like "first-conditions`t22`t*"})
 if($newConditions.Count -ne 6 -or @($rows|Where-Object {$_ -like "first-conditions`t15`t*"}).Count -ne $priorConditions.Count){throw 'Expected six scoped event conditions and retained gossip quest gate'}
 [IO.File]::WriteAllLines("$out/conditions.tsv",[string[]]@($newConditions|ForEach-Object {$_.Substring(17)}))
 $variants=[ordered]@{
  'option'='UPDATE smart_scripts SET event_param2=1 WHERE id=0;';
  'credit'='UPDATE smart_scripts SET action_param1=105434 WHERE id=0;';
  'target'='UPDATE smart_scripts SET target_type=1 WHERE id=0;';
  'root-link'='UPDATE smart_scripts SET link=0 WHERE id=0;';
  'chance'='UPDATE smart_scripts SET event_chance=50 WHERE id=0;';
  'spell-binding'="INSERT INTO spell_script_names VALUES(208755,'custom');";
  'extra-row'="INSERT INTO smart_scripts(entryorguid,source_type,id,comment) VALUES(105332,0,99,'custom');";
  'condition'="INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,ConditionTypeOrReference,ConditionValue1) VALUES(22,1,105332,9,1);";
  'menu'="UPDATE gossip_menu_option SET OptionText='custom';";
  'ai'="UPDATE creature_template SET AIName='custom';";
  'objective'='UPDATE quest_objectives SET Amount=2;';
  'applied-custom'='UPDATE smart_scripts SET action_param2=18 WHERE id=1;'
 }
 foreach($name in $variants.Keys){$base=$setup+$baseline+$(if($name -like 'applied-*'){$migration}else{''});$result=Query ($base+$variants[$name]+(Snapshot 'before')+$migration+(Snapshot 'after')) -Refusal
  foreach($table in $filters.Keys){if((Signature $result "before-$table") -cne (Signature $result "after-$table")){throw "Custom state changed $name/$table"}}
 }
 if($VerifyLiveData){$sha=[Security.Cryptography.SHA1]::Create();try{$hash=([BitConverter]::ToString($sha.ComputeHash([Text.Encoding]::UTF8.GetBytes($migration.Replace("`r`n","`n"))))).Replace('-','')}finally{$sha.Dispose()}
  $live=Query "SELECT 'row',id,link,event_type,action_type,action_param1,action_param2,target_type FROM smart_scripts WHERE entryorguid=105332 AND source_type=0 ORDER BY id;SELECT 'released',COUNT(*) FROM updates WHERE name='2026_10_29_15_desmond_native_search.sql' AND hash='$hash' AND state='RELEASED';"
  if($live -notcontains "released`t1" -or $live -notcontains "row`t0`t1`t62`t72`t0`t0`t7" -or $live -notcontains "row`t1`t0`t61`t11`t208755`t16`t7"){throw 'Live Desmond migration/hash differs'}
  $fullLive=Query (Snapshot 'live');foreach($table in $filters.Keys){if((Signature $fullLive "live-$table") -cne (Signature $rows "first-$table")){throw "Live differs from native regression $table"}}
 }
 'PASS full-schema TEMP Desmond replay/idempotence: root closes gossip, LINK1 normal player self cast208755, six active/live-player guards; preserved menu/objective/actor/native bindings; twelve custom-state refusals'
}
