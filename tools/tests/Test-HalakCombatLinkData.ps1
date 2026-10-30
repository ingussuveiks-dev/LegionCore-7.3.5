param([string]$MigrationFile,[switch]$VerifyLiveData)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/halak-combat-link-data"
if(!$MigrationFile){$MigrationFile="$repo/sql/pending/world/2026_10_30_36_halak_combat_link.sql";if(!(Test-Path $MigrationFile)){$MigrationFile="$repo/sql/updates/world/2026_10_30_36_halak_combat_link.sql"}}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -Test {
 $migration=Get-Content $MigrationFile -Raw
 $c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $script:queryIndex=0
 function Query([string]$sql,[switch]$Refusal){
  $old=$env:MYSQL_PWD;$errorFile="$out/sql-$script:queryIndex.stderr";++$script:queryIndex
  [IO.File]::WriteAllText("$out/sql-$($script:queryIndex-1).sql",$sql+"`nSELECT '__HALAK_PRESENTATION_SQL_COMPLETE__';`n")
  $previousConsole=[Console]::OutputEncoding;$previousPipe=$OutputEncoding
  try{$env:MYSQL_PWD=$c[3];$sqlFile="$out/sql-$($script:queryIndex-1).sql";$stdoutFile="$out/sql-$($script:queryIndex-1).stdout"
   # Preserve complete native stdout/stderr bytes, including the tail after an
   # expected --force error; a PowerShell native pipeline lost part of a row.
   $code=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -Arguments @("--host=$($c[0])","--port=$($c[1])","--user=$($c[2])","--database=$($c[4])",'--default-character-set=utf8mb4','--batch','--raw','--skip-column-names','--force') -InputFile $sqlFile -OutputFile $stdoutFile -ErrorFile $errorFile
   $rows=@(Get-Content $stdoutFile -Encoding UTF8);$errors=[string](Get-Content $errorFile -Raw -Encoding UTF8)
   if($Refusal){if($errors -notmatch 'Halak native combat timers/context differ; review before updating' -or @([regex]::Matches($errors,'ERROR [0-9]+')).Count -ne 1){throw "Unexpected Halak presentation refusal: $errorFile"}}
   elseif($code -ne 0 -or $errors){throw "Halak presentation SQL failed: $errorFile"}
   if(!$rows.Count -or $rows[-1] -cne '__HALAK_PRESENTATION_SQL_COMPLETE__'){throw "Incomplete native SQL output: $stdoutFile"}
   return ,@($rows|Select-Object -SkipLast 1)
  }finally{$env:MYSQL_PWD=$old;[Console]::OutputEncoding=$previousConsole;$OutputEncoding=$previousPipe}
 }
 $presentation=Get-Content "$repo/sql/updates/world/2026_10_30_18_halak_presentation.sql" -Raw
 $fixture=Get-Content "$PSScriptRoot/HalakPresentationOriginal.json" -Raw -Encoding UTF8|ConvertFrom-Json
 $filters=[ordered]@{smart_scripts='source_type=0 AND entryorguid=93949';conditions='SourceTypeOrReferenceId=22 AND SourceEntry=93949';creature_text='CreatureID=93949';creature_template='entry=93949';creature_template_wdb='Entry=93949';creature='id=93949';conversation_data='entry=741';conversation_creature='entry=741';conversation_actor='entry=741'}
 $order=@{smart_scripts='entryorguid,source_type,id,link';conditions='SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2';creature_text='CreatureID,GroupID,ID';creature_template='entry';creature_template_wdb='Entry';creature='guid';conversation_data='entry,id,idx';conversation_creature='entry,id';conversation_actor='entry,id'}
 function Value($v){if($null -eq $v){'NULL'}elseif($v -is [string]){if($v.Length){"CONVERT(0x"+([BitConverter]::ToString([Text.Encoding]::UTF8.GetBytes($v))).Replace('-','')+" USING utf8mb4)"}else{"''"}}else{[Convert]::ToString($v,[Globalization.CultureInfo]::InvariantCulture)}}
 $setup='';$baseline=''
 foreach($table in $filters.Keys){
  $setup+="CREATE TEMPORARY TABLE saved_$table LIKE $table;INSERT INTO saved_$table SELECT * FROM $table WHERE $($filters[$table]);CREATE TEMPORARY TABLE $table LIKE saved_$table;`n"
  $baseline+="DELETE FROM $table;`n"
  foreach($row in $fixture.$table){$cols=($row.PSObject.Properties.Name|ForEach-Object {"``$_``"}) -join ',';$values=($row.PSObject.Properties|ForEach-Object {Value $_.Value}) -join ',';$baseline+="INSERT INTO $table($cols) VALUES($values);`n"}
 }
 $baseline+=$presentation
 function Snapshot([string]$tag){$sql='';foreach($table in $filters.Keys){$sql+="SELECT '$tag-$table',t.* FROM $table t WHERE $($filters[$table]) ORDER BY $($order[$table]);"};$sql}
 function Signature($rows,[string]$tag){[string]::Join("`n",[string[]]@($rows|Where-Object {$_ -like "$tag`t*"}|ForEach-Object {$_.Substring($tag.Length+1)}))}
 $rows=Query ($setup+$baseline+(Snapshot 'original')+$migration+(Snapshot 'first')+$migration+(Snapshot 'second'))
 foreach($table in $filters.Keys){if((Signature $rows "first-$table") -cne (Signature $rows "second-$table")){throw "Not idempotent $table"};if($table -ne 'smart_scripts' -and (Signature $rows "original-$table") -cne (Signature $rows "first-$table")){throw "Native return data changed $table"}}
 $original=@((Signature $rows 'original-smart_scripts').Split("`n"));$applied=@((Signature $rows 'first-smart_scripts').Split("`n"))
 if($original.Count -ne 6 -or $applied.Count -ne 6){throw 'Missing native Halak rows'}
 $edits=0;for($i=0;$i -lt 6;++$i){$f=$original[$i].Split("`t");if($f[2] -eq '5'){if($f[3] -ne '3' -or $f[5] -ne '0' -or $f[14] -ne '11' -or $f[15] -ne '195272'){throw 'Original Halak cast edge differs'};$f[3]='0';++$edits};if(($f -join "`t") -cne $applied[$i]){throw 'Unexpected Halak combat row mutation'}};if($edits -ne 1){throw 'Expected one link field edit'}
 [IO.File]::WriteAllLines("$out/original.tsv",[string[]]$original)
 [IO.File]::WriteAllLines("$out/repaired.tsv",[string[]]$applied)
 [IO.File]::WriteAllLines("$out/texts.tsv",[string[]]((Signature $rows 'first-creature_text').Split("`n")))
 [IO.File]::WriteAllLines("$out/sql-evidence.tsv",[string[]]$rows)
 $variants=[ordered]@{
 'conversation-id'='UPDATE smart_scripts SET action_param1=1 WHERE id=0;';
 'conversation-pose'='UPDATE smart_scripts SET action_param2=1 WHERE id=0;';
 'aggro-target'='UPDATE smart_scripts SET target_type=7 WHERE id=1;';
 'death-text'='UPDATE smart_scripts SET action_param1=0 WHERE id=2;';
 'cast-timer'='UPDATE smart_scripts SET event_param1=1 WHERE id=3;';
 'repeat-cast'='UPDATE smart_scripts SET action_param1=1 WHERE id=4;';
 'foreign-link'='UPDATE smart_scripts SET link=4 WHERE id=5;';
 'native-one-shot'='UPDATE smart_scripts SET event_flags=0 WHERE id=5;';
 'custom-ai'="UPDATE creature_template SET ScriptName='custom';";
 'native-flags'='UPDATE creature_template SET unit_flags=1;';
 'native-name'="UPDATE creature_template_wdb SET Name1='custom';";
 'native-text'='UPDATE creature_text SET BroadcastTextID=1 WHERE GroupID=1;';
 'native-spawn'='UPDATE creature SET PhaseId=1;';
 'conversation-line'='UPDATE conversation_data SET textId=1;';
 'conversation-actor'='UPDATE conversation_actor SET duration=1;';
 'custom-condition'="INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,ConditionTypeOrReference,ConditionValue1) VALUES(22,1,93949,9,1);";
 'partial-speech'='UPDATE smart_scripts SET target_type=7 WHERE id=2;';
 'partial-conversation'='UPDATE smart_scripts SET target_type=2 WHERE id=0;';
 'applied-range'='UPDATE smart_scripts SET target_param1=71 WHERE id=0;';
 'applied-extra-target'='UPDATE smart_scripts SET target_param2=1 WHERE id=0;';
 'applied-restored-link'='UPDATE smart_scripts SET link=3 WHERE id=5; UPDATE smart_scripts SET event_param4=1 WHERE id=3;'
 }
 foreach($name in $variants.Keys){$base=$setup+$baseline+$(if($name -like 'applied-*'){$migration}else{''});$result=Query ($base+$variants[$name]+(Snapshot 'before')+$migration+(Snapshot 'after')) -Refusal
  foreach($table in $filters.Keys){if((Signature $result "before-$table") -cne (Signature $result "after-$table")){throw "Custom Halak data mutated $name/$table"}}
 }
 if($VerifyLiveData){$sha=[Security.Cryptography.SHA1]::Create();try{$hash=([BitConverter]::ToString($sha.ComputeHash([Text.Encoding]::UTF8.GetBytes($migration.Replace("`r`n","`n"))))).Replace('-','')}finally{$sha.Dispose()}
  $live=Query "SELECT COUNT(*) FROM updates WHERE name='2026_10_30_36_halak_combat_link.sql' AND hash='$hash' AND state='RELEASED';";if($live -notcontains '1'){throw 'Live Halak migration/hash differs'}
  $live=Query (Snapshot 'live');foreach($table in $filters.Keys){if((Signature $live "live-$table") -cne (Signature $rows "first-$table")){throw "Live native Halak context differs $table"}}
 }
 'PASS full9table Halak combat: exactly one link field, all six actions/three cast timers/native741 owner speech unchanged; idempotence/21 custom-state refusals and optional live equality'
}
