param([string]$MigrationFile,[switch]$VerifyLiveData)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/fleshripper-speakers-data"
if(!$MigrationFile){$MigrationFile="$repo/sql/pending/world/2026_10_30_09_fleshripper_speakers.sql";if(!(Test-Path $MigrationFile)){$MigrationFile="$repo/sql/updates/world/2026_10_30_09_fleshripper_speakers.sql"}}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -Test {
 $migration=Get-Content $MigrationFile -Raw
 $c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $script:queryIndex=0
 function Query([string]$sql,[switch]$Refusal){
  $old=$env:MYSQL_PWD;$errorFile="$out/sql-$script:queryIndex.stderr";++$script:queryIndex
  [IO.File]::WriteAllText("$out/sql-$($script:queryIndex-1).sql",$sql+"`nSELECT '__FLESHRIPPER_SQL_COMPLETE__';`n")
  $previousConsole=[Console]::OutputEncoding;$previousPipe=$OutputEncoding
  try{$env:MYSQL_PWD=$c[3];$sqlFile="$out/sql-$($script:queryIndex-1).sql";$stdoutFile="$out/sql-$($script:queryIndex-1).stdout"
   # Preserve complete native stdout/stderr bytes, including the tail after an
   # expected --force error; a PowerShell native pipeline lost part of a row.
   $code=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -Arguments @("--host=$($c[0])","--port=$($c[1])","--user=$($c[2])","--database=$($c[4])",'--default-character-set=utf8mb4','--batch','--raw','--skip-column-names','--force') -InputFile $sqlFile -OutputFile $stdoutFile -ErrorFile $errorFile
   $rows=@(Get-Content $stdoutFile -Encoding UTF8);$errors=[string](Get-Content $errorFile -Raw -Encoding UTF8)
   if($Refusal){if($errors -notmatch 'Fleshripper native speech/sequence/loot differ; review before updating' -or @([regex]::Matches($errors,'ERROR [0-9]+')).Count -ne 1){throw "Unexpected Fleshripper speaker refusal: $errorFile"}}
   elseif($code -ne 0 -or $errors){throw "Fleshripper speaker SQL failed: $errorFile"};if(!$rows.Count -or $rows[-1] -cne '__FLESHRIPPER_SQL_COMPLETE__'){throw "Incomplete native SQL output: $stdoutFile"};return ,@($rows|Select-Object -SkipLast 1)
  }finally{$env:MYSQL_PWD=$old;[Console]::OutputEncoding=$previousConsole;$OutputEncoding=$previousPipe}
 }
 $fixture=Get-Content "$PSScriptRoot/FleshripperSpeakersOriginal.json" -Raw -Encoding UTF8|ConvertFrom-Json
 $filters=[ordered]@{smart_scripts='(source_type=0 AND entryorguid IN(101778,102561,103795)) OR(source_type=1 AND entryorguid IN(247566,242469))';conditions='(SourceTypeOrReferenceId=22 AND SourceEntry IN(101778,102561,103795,247566,242469)) OR(SourceTypeOrReferenceId=4 AND SourceGroup=247566)';creature_text='CreatureID IN(101778,102561,103795)';creature_template='entry IN(101778,102561,103795)';gameobject_template='entry IN(247566,242469)';gameobject_loot_template='Entry IN(247566,242469)'}
 $order=@{smart_scripts='entryorguid,source_type,id,link';conditions='SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2';creature_text='CreatureID,GroupID,ID';creature_template='entry';gameobject_template='entry';gameobject_loot_template='Entry,Item,Reference,GroupId'}
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
 for($i=0;$i -lt $original.Count;++$i){$f=$original[$i].Split("`t");if($f[1] -eq '0' -and ($f[0] -eq '101778' -and [int]$f[2] -in @(0,1))){if($f[21] -ne '7'){throw 'Original combat speaker differs'};$f[21]='1';++$edits};if(($f -join "`t") -cne $repaired[$i]){throw "Unexpected row mutation $i"}}
 if($edits -ne 2){throw 'Expected exactly two speaker fields'}
 [IO.File]::WriteAllLines("$out/original.tsv",[string[]]$original);[IO.File]::WriteAllLines("$out/repaired.tsv",[string[]]$repaired);[IO.File]::WriteAllLines("$out/conditions.tsv",[string[]]$conditions)
 [IO.File]::WriteAllLines("$out/texts.tsv",[string[]]@((Signature $rows 'first-creature_text').Split("`n")))
 [IO.File]::WriteAllLines("$out/sql-evidence.tsv",[string[]]$rows)
 $variants=[ordered]@{
 'aggro-target'='UPDATE smart_scripts SET target_type=2 WHERE entryorguid=101778 AND source_type=0 AND id=0;';
 'death-text'='UPDATE smart_scripts SET action_param1=0 WHERE entryorguid=101778 AND source_type=0 AND id=1;';
 'death-link'='UPDATE smart_scripts SET link=0 WHERE entryorguid=101778 AND source_type=0 AND id=1;';
 'boss-health'='UPDATE smart_scripts SET event_param2=79 WHERE entryorguid=101778 AND source_type=0 AND id=4;';
 'boss-criterion'='UPDATE smart_scripts SET action_param2=1 WHERE entryorguid=101778 AND source_type=0 AND id=12;';
 'boss-return'='UPDATE smart_scripts SET action_param1=1 WHERE entryorguid=101778 AND source_type=0 AND id=13;';
 'boss-data'='UPDATE smart_scripts SET action_param1=1 WHERE entryorguid=101778 AND source_type=0 AND id=14;';
 'custom-condition'="INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,ConditionTypeOrReference,ConditionValue1) VALUES(22,1,101778,9,1);";
 'custom-ai'="UPDATE creature_template SET ScriptName='custom' WHERE entry=101778;";
 'native-text'='UPDATE creature_text SET BroadcastTextID=1 WHERE CreatureID=101778 AND GroupID=0;';
 'weapon-link'='UPDATE gameobject_template SET Data1=0 WHERE entry=247566;';
 'weapon-loot'='UPDATE gameobject_loot_template SET Chance=1 WHERE Entry=247566;';
 'missing-row'='DELETE FROM smart_scripts WHERE entryorguid=101778 AND source_type=0 AND id=0;';
 'partial-speaker'='UPDATE smart_scripts SET target_type=1 WHERE entryorguid=101778 AND source_type=0 AND id=0;';
 'applied-speaker'='UPDATE smart_scripts SET target_type=2 WHERE entryorguid=101778 AND source_type=0 AND id=1;'
 }
 foreach($name in $variants.Keys){$base=$setup+$baseline+$(if($name -like 'applied-*'){$migration}else{''});$result=Query ($base+$variants[$name]+(Snapshot 'before')+$migration+(Snapshot 'after')) -Refusal
  foreach($table in $filters.Keys){if((Signature $result "before-$table") -cne (Signature $result "after-$table")){throw "Custom state changed $name/$table"}}
 }
 if($VerifyLiveData){$sha=[Security.Cryptography.SHA1]::Create();try{$hash=([BitConverter]::ToString($sha.ComputeHash([Text.Encoding]::UTF8.GetBytes($migration.Replace("`r`n","`n"))))).Replace('-','')}finally{$sha.Dispose()}
  $live=Query "SELECT COUNT(*) FROM updates WHERE name='2026_10_30_09_fleshripper_speakers.sql' AND hash='$hash' AND state='RELEASED';"
  if($live -notcontains '1'){throw 'Live Fleshripper speaker wave migration/hash differs'}
  $expectedRows=$rows;$expectedTag='first'
  $fullLive=Query (Snapshot 'live');foreach($table in $filters.Keys){if((Signature $fullLive "live-$table") -cne (Signature $expectedRows "$expectedTag-$table")){throw "Live differs from Fleshripper speaker regression $table"}}
 }
 'PASS full-schema TEMP Fleshripper: exactly two TALK self targets, unchanged native thresholds/credit/loot/return/full6table context; idempotence/live equality and15 custom/partial refusals'
}
