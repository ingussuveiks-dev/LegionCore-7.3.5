param([string]$MigrationFile,[switch]$VerifyLiveData)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/maelstrom-wave-data"
if(!$MigrationFile){$MigrationFile="$repo/sql/pending/world/2026_10_29_23_maelstrom_wave_dispatch.sql";if(!(Test-Path $MigrationFile)){$MigrationFile="$repo/sql/updates/world/2026_10_29_23_maelstrom_wave_dispatch.sql"}}
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
   if($Refusal){if($errors -notmatch 'Maelstrom interaction/wave/conditions differ; review before updating' -or @([regex]::Matches($errors,'ERROR [0-9]+')).Count -ne 1){throw "Unexpected Maelstrom refusal: $errorFile"}}
   elseif($code -ne 0 -or $errors){throw "Maelstrom SQL failed: $errorFile"};return ,$rows
  }finally{$env:MYSQL_PWD=$old;[Console]::OutputEncoding=$previousConsole;$OutputEncoding=$previousPipe}
 }
 $fixture=Get-Content "$PSScriptRoot/MaelstromWaveOriginal.json" -Raw -Encoding UTF8|ConvertFrom-Json
 $filters=[ordered]@{smart_scripts='(source_type=0 AND entryorguid IN(97168,97436,96757,96539,97027,97054,97386,94721,96527,96530,96541,96747)) OR (source_type=1 AND entryorguid IN(244431,247612)) OR (source_type=9 AND entryorguid IN(94721,247612))';conditions='SourceTypeOrReferenceId=22 AND SourceEntry IN(97168,97436,244431,96757,96539,97027,97054,97386,94721,96527,247612)';creature_text='CreatureID IN(96757,96539,97027,97386,94721,96527,96530,96541,96747)';creature_template='entry IN(97168,97436,96757,96539,97027,97054,97386,94721,96527,96530,96541,96747)';gameobject_template='entry IN(244431,247612)';waypoints='entry IN(97386,97027,96527,9652700,9652701,9652702)'}
 $order=@{smart_scripts='entryorguid,source_type,id,link';conditions='SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2';creature_text='CreatureID,GroupID,ID';creature_template='entry';gameobject_template='entry';waypoints='entry,pointid'}
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
 $original=@((Signature $rows 'original-smart_scripts').Split("`n"));$repaired=@((Signature $rows 'first-smart_scripts').Split("`n"));$conditions=@((Signature $rows 'first-conditions').Split("`n"))
 if($original.Count -ne @($fixture.smart_scripts).Count -or $repaired.Count -ne $original.Count){throw 'Original/repaired row count differs'}
 $edits=0
 for($i=0;$i -lt $original.Count;++$i){$f=$original[$i].Split("`t");if($f[2] -eq '2' -and (($f[1] -eq '0' -and $f[0] -in @('97168','97436')) -or ($f[1] -eq '1' -and $f[0] -eq '244431'))){if($f[3] -ne '3'){throw 'Original immediate call differs'};$f[3]='0';++$edits};if(($f -join "`t") -cne $repaired[$i]){throw "Unexpected row mutation $i"}}
 if($edits -ne 3){throw 'Expected exactly three malformed calls'}
 [IO.File]::WriteAllLines("$out/original.tsv",[string[]]$original);[IO.File]::WriteAllLines("$out/repaired.tsv",[string[]]$repaired);[IO.File]::WriteAllLines("$out/conditions.tsv",[string[]]$conditions)
 [IO.File]::WriteAllLines("$out/sql-evidence.tsv",[string[]]$rows)
 $variants=[ordered]@{
 'lost-update-root'='UPDATE smart_scripts SET event_type=61 WHERE entryorguid=97168 AND id=3;';
 'wave-actor'='UPDATE smart_scripts SET action_param1=94721 WHERE entryorguid=97436 AND id=3;';
 'wave-position'='UPDATE smart_scripts SET target_x=1 WHERE entryorguid=244431 AND id=3;';
 'fire-spell'='UPDATE smart_scripts SET action_param1=0 WHERE entryorguid=97436 AND id=1;';
 'data-recipient'='UPDATE smart_scripts SET target_param1=96527 WHERE entryorguid=97168 AND id=0;';
 'portal-producer'='UPDATE smart_scripts SET action_param1=1 WHERE entryorguid=244431 AND id=2;';
 'last-living-negation'='UPDATE conditions SET NegativeCondition=0 WHERE SourceEntry=97168 AND ConditionTypeOrReference=29;';
 'boss-radius'='UPDATE conditions SET ConditionValue2=1 WHERE SourceEntry=244431 AND ConditionTypeOrReference=29;';
 'step-guard'='UPDATE conditions SET ConditionValue2=1 WHERE SourceEntry=97436 AND ConditionTypeOrReference=44;';
 'condition-target'='UPDATE conditions SET ConditionTarget=0 WHERE SourceEntry=244431 AND ConditionTypeOrReference=44;';
 'exit-timing'='UPDATE smart_scripts SET event_param1=1 WHERE entryorguid=247612 AND source_type=9 AND id=7;';
 'custom-condition'="INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,ConditionTypeOrReference,ConditionValue1) VALUES(22,4,97168,9,1);";
 'custom-ai'="UPDATE gameobject_template SET AIName='custom' WHERE entry=244431;";
 'waypoint'='UPDATE waypoints SET position_x=1 WHERE entry=97386 AND pointid=1;';
 'missing-row'='DELETE FROM smart_scripts WHERE entryorguid=97168 AND id=9;';
 'applied-custom'='UPDATE smart_scripts SET link=3 WHERE entryorguid=244431 AND id=2;'
 }
 foreach($name in $variants.Keys){$base=$setup+$baseline+$(if($name -like 'applied-*'){$migration}else{''});$result=Query ($base+$variants[$name]+(Snapshot 'before')+$migration+(Snapshot 'after')) -Refusal
  foreach($table in $filters.Keys){if((Signature $result "before-$table") -cne (Signature $result "after-$table")){throw "Custom state changed $name/$table"}}
 }
 if($VerifyLiveData){$sha=[Security.Cryptography.SHA1]::Create();try{$hash=([BitConverter]::ToString($sha.ComputeHash([Text.Encoding]::UTF8.GetBytes($migration.Replace("`r`n","`n"))))).Replace('-','')}finally{$sha.Dispose()}
  $live=Query "SELECT COUNT(*) FROM updates WHERE name='2026_10_29_23_maelstrom_wave_dispatch.sql' AND hash='$hash' AND state='RELEASED';"
  if($live -notcontains '1'){throw 'Live Maelstrom wave migration/hash differs'}
  $fullLive=Query (Snapshot 'live');foreach($table in $filters.Keys){if((Signature $fullLive "live-$table") -cne (Signature $rows "first-$table")){throw "Live differs from Maelstrom regression $table"}}
 }
 'PASS full-schema TEMP Maelstrom wave replay/idempotence: exactly three malformed immediate calls; independent UPDATE and original1038/2/no-nearby97386 wave conditions, seven summons, initial interactions/dialogue/native exit retained; sixteen custom-state refusals'
}
