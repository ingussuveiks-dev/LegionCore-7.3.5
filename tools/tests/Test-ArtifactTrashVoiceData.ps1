param([string]$MigrationFile,[switch]$VerifyLiveData)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/artifact-trash-voice-data"
if(!$MigrationFile){$MigrationFile="$repo/sql/pending/world/2026_10_30_23_artifact_trash_voice.sql";if(!(Test-Path $MigrationFile)){$MigrationFile="$repo/sql/updates/world/2026_10_30_23_artifact_trash_voice.sql"}}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -Test {
 $migration=Get-Content $MigrationFile -Raw
 $c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $script:queryIndex=0
 function Query([string]$sql,[switch]$Refusal){
  $old=$env:MYSQL_PWD;$errorFile="$out/sql-$script:queryIndex.stderr";++$script:queryIndex
  [IO.File]::WriteAllText("$out/sql-$($script:queryIndex-1).sql",$sql+"`nSELECT '__TRASH_VOICE_SQL_COMPLETE__';`n")
  $previousConsole=[Console]::OutputEncoding;$previousPipe=$OutputEncoding
  try{$env:MYSQL_PWD=$c[3];$sqlFile="$out/sql-$($script:queryIndex-1).sql";$stdoutFile="$out/sql-$($script:queryIndex-1).stdout"
   # Preserve complete native stdout/stderr bytes, including the tail after an
   # expected --force error; a PowerShell native pipeline lost part of a row.
   $code=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -Arguments @("--host=$($c[0])","--port=$($c[1])","--user=$($c[2])","--database=$($c[4])",'--default-character-set=utf8mb4','--batch','--raw','--skip-column-names','--force') -InputFile $sqlFile -OutputFile $stdoutFile -ErrorFile $errorFile
   $rows=@(Get-Content $stdoutFile -Encoding UTF8);$errors=[string](Get-Content $errorFile -Raw -Encoding UTF8)
   if($Refusal){if($errors -notmatch 'Artifact trash voice/combat context differs; review before updating' -or @([regex]::Matches($errors,'ERROR [0-9]+')).Count -ne 1){throw "Unexpected Artifact trash voice refusal: $errorFile"}}
   elseif($code -ne 0 -or $errors){throw "Artifact trash voice SQL failed: $errorFile"}
   if(!$rows.Count -or $rows[-1] -cne '__TRASH_VOICE_SQL_COMPLETE__'){throw "Incomplete native SQL output: $stdoutFile"}
   return ,@($rows|Select-Object -SkipLast 1)
  }finally{$env:MYSQL_PWD=$old;[Console]::OutputEncoding=$previousConsole;$OutputEncoding=$previousPipe}
 }
 $fixture=Get-Content "$PSScriptRoot/ArtifactTrashVoiceOriginal.json" -Raw -Encoding UTF8|ConvertFrom-Json
 $filters=[ordered]@{smart_scripts='source_type=0 AND entryorguid IN(95433,95434,97993,97994,98892)';conditions='SourceTypeOrReferenceId=22 AND SourceEntry IN(95433,95434,97993,97994,98892)';creature_text='CreatureID IN(95433,95434,97993,97994,98892)';creature_template='entry IN(95433,95434,97993,97994,98892)';creature_template_wdb='Entry IN(95433,95434,97993,97994,98892)';creature='id IN(95433,95434,97993,97994,98892)';spell_script_names='spell_id IN(200663,195577,195575,187207,195584)'}
 $order=@{smart_scripts='entryorguid,source_type,id,link';conditions='SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2';creature_text='CreatureID,GroupID,ID';creature_template='entry';creature_template_wdb='Entry';creature='guid';spell_script_names='spell_id,ScriptName'}
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
 foreach($table in $filters.Keys){if((Signature $rows "first-$table") -cne (Signature $rows "second-$table")){throw "Not idempotent $table"};if($table -ne 'smart_scripts' -and (Signature $rows "original-$table") -cne (Signature $rows "first-$table")){throw "Native return data changed $table"}}
 $original=@((Signature $rows 'original-smart_scripts').Split("`n"));$applied=@((Signature $rows 'first-smart_scripts').Split("`n"))
 if($original.Count -ne @($fixture.smart_scripts).Count -or $applied.Count -ne $original.Count){throw 'Missing native Artifact trash voice/context rows'}
 $edits=0;for($i=0;$i -lt $original.Count;++$i){$f=$original[$i].Split("`t")
  if($f[14] -eq '1'){if($f[15] -ne '0' -or $f[21] -notin @('2','7')){throw 'Original trash TALK tuple differs'};$f[21]='1';++$edits}
  if(($f -join "`t") -cne $applied[$i]){throw 'Unexpected trash voice row mutation'}
 };if($edits -ne 5){throw 'Expected exactly5 native trash voice fields'}
 [IO.File]::WriteAllLines("$out/original.tsv",[string[]]$original)
 [IO.File]::WriteAllLines("$out/repaired.tsv",[string[]]$applied)
 [IO.File]::WriteAllLines("$out/texts.tsv",[string[]]((Signature $rows 'first-creature_text').Split("`n")))
 [IO.File]::WriteAllLines("$out/sql-evidence.tsv",[string[]]$rows)
 $variants=[ordered]@{
 'native-event'='UPDATE smart_scripts SET event_type=6 WHERE entryorguid=95433 AND id=0;';
 'native-flags'='UPDATE smart_scripts SET event_flags=0 WHERE entryorguid=95434 AND id=0;';
 'native-death-repeat'='UPDATE smart_scripts SET event_flags=1 WHERE entryorguid=97993 AND id=2;';
 'native-group'='UPDATE smart_scripts SET action_param1=1 WHERE entryorguid=97994 AND id=1;';
 'native-combat'='UPDATE smart_scripts SET action_param1=1 WHERE entryorguid=98892 AND id=0;';
 'native-cadence'='UPDATE smart_scripts SET event_param1=1 WHERE entryorguid=97993 AND id=0;';
 'native-ai'="UPDATE creature_template SET ScriptName='custom' WHERE entry=95433;";
 'native-spawn'='UPDATE creature SET phaseMask=2 WHERE id=95434 LIMIT 1;';
 'native-text'='UPDATE creature_text SET BroadcastTextID=1 WHERE CreatureID=97994 AND GroupID=0;';
 'native-volume'='UPDATE creature_text SET Type=14 WHERE CreatureID=98892 AND GroupID=0;';
 'native-kill-credit'='UPDATE creature_template_wdb SET KillCredit1=1 WHERE Entry=97993;';
 'custom-condition'="INSERT INTO conditions(SourceTypeOrReferenceId,SourceEntry,ConditionTypeOrReference,ConditionValue1) VALUES(22,95433,9,1);";
 'custom-spell-script'="INSERT INTO spell_script_names(spell_id,ScriptName) VALUES(195584,'custom');";
 'partial-one-row'='UPDATE smart_scripts SET target_type=1 WHERE entryorguid=95433 AND id=0;';
 'applied-extra-target'='UPDATE smart_scripts SET target_param1=10 WHERE entryorguid=97994 AND id=1;'
 }
 foreach($name in $variants.Keys){$base=$setup+$baseline+$(if($name -like 'applied-*'){$migration}else{''});$result=Query ($base+$variants[$name]+(Snapshot 'before')+$migration+(Snapshot 'after')) -Refusal
  foreach($table in $filters.Keys){if((Signature $result "before-$table") -cne (Signature $result "after-$table")){throw "Custom Artifact trash voice data mutated $name/$table"}}
 }
 if($VerifyLiveData){$sha=[Security.Cryptography.SHA1]::Create();try{$hash=([BitConverter]::ToString($sha.ComputeHash([Text.Encoding]::UTF8.GetBytes($migration.Replace("`r`n","`n"))))).Replace('-','')}finally{$sha.Dispose()}
  $live=Query "SELECT COUNT(*) FROM updates WHERE name='2026_10_30_23_artifact_trash_voice.sql' AND hash='$hash' AND state='RELEASED';";if($live -notcontains '1'){throw 'Live Artifact trash voice migration/hash differs'}
  $live=Query (Snapshot 'live');foreach($table in $filters.Keys){if((Signature $live "live-$table") -cne (Signature $rows "first-$table")){throw "Live native Artifact trash voice context differs $table"}}
 }
 'PASS full7table artifact trash voice: exactly5 speaker targets; ten native Smart rows/13text variants/all46 poses,casts/repeat flags/kill credit unchanged; twice-idempotent/15 partial-custom refusals/live when requested'
}
