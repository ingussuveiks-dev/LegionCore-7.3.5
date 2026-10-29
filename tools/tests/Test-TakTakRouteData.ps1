param([string]$MigrationFile,[switch]$VerifyLiveData)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/tak-tak-route-data"
if(!$MigrationFile){$MigrationFile="$repo/sql/pending/world/2026_10_29_09_tak_tak_link.sql";if(!(Test-Path $MigrationFile)){$MigrationFile="$repo/sql/updates/world/2026_10_29_09_tak_tak_link.sql"}}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -Test {
 $migration=Get-Content $MigrationFile -Raw
 $c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $script:queryIndex=0
 function Query([string]$sql,[switch]$Refusal){
  $old=$env:MYSQL_PWD;$errorFile="$out/sql-$script:queryIndex.stderr";++$script:queryIndex
  try{$env:MYSQL_PWD=$c[3];$ErrorActionPreference='Continue';$rows=@($sql | & C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names --force 2>$errorFile);$code=$LASTEXITCODE;$ErrorActionPreference='Stop';$errors=[string](Get-Content $errorFile -Raw)
   if($Refusal){if($errors -notmatch 'Tak-Tak graph/menu/conditions/actor differ; review before updating' -or @([regex]::Matches($errors,'ERROR [0-9]+')).Count -ne 1){throw "Unexpected Tak-Tak refusal: $errorFile"}}
   elseif($code -ne 0 -or $errors){throw "Tak-Tak SQL failed: $errorFile"};return ,$rows
  }finally{$env:MYSQL_PWD=$old}
 }
 $fixture=Get-Content "$PSScriptRoot/TakTakRouteOriginal.json" -Raw | ConvertFrom-Json
 $filters=[ordered]@{smart_scripts='entryorguid=101880 AND source_type=0';conditions='(SourceTypeOrReferenceId=15 AND SourceGroup=19254) OR (SourceTypeOrReferenceId=22 AND SourceEntry=101880)';gossip_menu_option='MenuID=19254';creature_template='entry=101880'}
 $order=@{smart_scripts='entryorguid,source_type,id';conditions='SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference';gossip_menu_option='MenuID,OptionID';creature_template='entry'}
 function Value($v){if($null -eq $v){'NULL'}elseif($v -is [string]){"'"+$v.Replace('\','\\').Replace("'","''")+"'"}else{[Convert]::ToString($v,[Globalization.CultureInfo]::InvariantCulture)}}
 $setup='';$baseline=''
 foreach($table in $filters.Keys){
  $setup+="CREATE TEMPORARY TABLE saved_$table LIKE $table;INSERT INTO saved_$table SELECT * FROM $table WHERE $($filters[$table]);CREATE TEMPORARY TABLE $table LIKE saved_$table;`n"
  $baseline+="DELETE FROM $table;`n"
  foreach($row in $fixture.$table){$cols=($row.PSObject.Properties.Name | ForEach-Object {"``$_``"}) -join ',';$values=($row.PSObject.Properties | ForEach-Object {Value $_.Value}) -join ',';$baseline+="INSERT INTO $table($cols) VALUES($values);`n"}
 }
 function Snapshot([string]$tag){$sql='';foreach($table in $filters.Keys){$sql+="SELECT '$tag-$table',t.* FROM $table t ORDER BY $($order[$table]);"};$sql}
 function Signature($rows,[string]$tag){[string]::Join("`n",[string[]]@($rows | Where-Object {$_ -like "$tag`t*"} | ForEach-Object {$_.Substring($tag.Length+1)}))}
 $rows=Query ($setup+$baseline+(Snapshot 'original')+$migration+(Snapshot 'first')+$migration+(Snapshot 'second'))
 foreach($table in $filters.Keys){if((Signature $rows "first-$table") -cne (Signature $rows "second-$table")){throw "Not idempotent $table"};if($table -ne 'smart_scripts' -and (Signature $rows "original-$table") -cne (Signature $rows "first-$table")){throw "Unrelated table changed $table"}}
 [IO.File]::WriteAllLines("$out/original.tsv",[string[]]@($rows | Where-Object {$_ -like "original-smart_scripts`t*"} | ForEach-Object {$_.Substring(23)}))
 [IO.File]::WriteAllLines("$out/repaired.tsv",[string[]]@($rows | Where-Object {$_ -like "first-smart_scripts`t*"} | ForEach-Object {$_.Substring(20)}))
 [IO.File]::WriteAllLines("$out/sql-evidence.tsv",[string[]]$rows)
 $before=@(Get-Content "$out/original.tsv");$after=@(Get-Content "$out/repaired.tsv")
 if($before.Count -ne 12 -or $after.Count -ne 12){throw 'Native graph row count differs'}
 for($i=0;$i -lt 12;++$i){$expected=$before[$i].Split("`t");if($expected[2] -eq '6'){$expected[5]='61';$expected[9]='0';$expected[10]='0';$expected[30]='Link - Kill Credit'};if(($expected -join "`t") -cne $after[$i]){throw "Unexpected row mutation $i"}}
 $variants=[ordered]@{
  'wrong-link'='UPDATE smart_scripts SET link=7 WHERE id=5;';'wrong-credit'='UPDATE smart_scripts SET action_param1=1 WHERE id=6;';'wrong-target'='UPDATE smart_scripts SET target_type=21 WHERE id=6;';'wrong-transfer'='UPDATE smart_scripts SET target_x=1 WHERE id=7;';
  'other-option'='UPDATE smart_scripts SET action_param1=1 WHERE id=10;';'extra-row'="INSERT INTO smart_scripts(entryorguid,source_type,id,comment) VALUES(101880,0,99,'custom');";'missing-row'='DELETE FROM smart_scripts WHERE id=7;';
  'quest-gate'='DELETE FROM conditions WHERE SourceEntry=3;';'custom-condition'="INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,ConditionTypeOrReference,ConditionValue1) VALUES(22,7,101880,9,1);";
  'custom-menu'="UPDATE gossip_menu_option SET OptionText='custom' WHERE OptionID=3;";'custom-ai'="UPDATE creature_template SET AIName='custom';";
  'applied-custom'="UPDATE smart_scripts SET action_param1=1 WHERE id=6;"
 }
 foreach($name in $variants.Keys){$base=$setup+$baseline+$(if($name -like 'applied-*'){$migration}else{''});$result=Query ($base+$variants[$name]+(Snapshot 'before')+$migration+(Snapshot 'after')) -Refusal
  foreach($table in $filters.Keys){if((Signature $result "before-$table") -cne (Signature $result "after-$table")){throw "Custom state changed $name/$table"}}
 }
 if($VerifyLiveData){$sha=[Security.Cryptography.SHA1]::Create();try{$hash=([BitConverter]::ToString($sha.ComputeHash([Text.Encoding]::UTF8.GetBytes($migration.Replace("`r`n","`n"))))).Replace('-','')}finally{$sha.Dispose()};$live=Query "SELECT 'row',id,link,event_type,event_param1,event_param2,action_type,action_param1,target_type FROM smart_scripts WHERE entryorguid=101880 AND source_type=0 AND id IN(5,6,7) ORDER BY id;SELECT 'released',COUNT(*) FROM updates WHERE name='2026_10_29_09_tak_tak_link.sql' AND hash='$hash' AND state='RELEASED';";if($live -notcontains "released`t1" -or $live -notcontains "row`t6`t7`t61`t0`t0`t33`t105485`t7"){throw 'Live Tak-Tak migration/hash differs'}}
 'PASS full-schema TEMP Tak-Tak repeat, exact four-field/single-row change and12 customized-state refusals; all other actions/credits/transfers/conditions/menu/template preserved'
}
