param([string]$MigrationFile,[switch]$VerifyLiveData)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/frost-souls-presentation-data"
if(!$MigrationFile){$MigrationFile="$repo/sql/pending/world/2026_10_30_19_frost_souls_presentation.sql";if(!(Test-Path $MigrationFile)){$MigrationFile="$repo/sql/updates/world/2026_10_30_19_frost_souls_presentation.sql"}}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -Test {
 $migration=Get-Content $MigrationFile -Raw
 $c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $script:queryIndex=0
 function Query([string]$sql,[switch]$Refusal){
  $old=$env:MYSQL_PWD;$errorFile="$out/sql-$script:queryIndex.stderr";++$script:queryIndex
  [IO.File]::WriteAllText("$out/sql-$($script:queryIndex-1).sql",$sql+"`nSELECT '__FROST_KINGS_PRESENTATION_SQL_COMPLETE__';`n")
  $previousConsole=[Console]::OutputEncoding;$previousPipe=$OutputEncoding
  try{$env:MYSQL_PWD=$c[3];$sqlFile="$out/sql-$($script:queryIndex-1).sql";$stdoutFile="$out/sql-$($script:queryIndex-1).stdout"
   # Preserve complete native stdout/stderr bytes, including the tail after an
   # expected --force error; a PowerShell native pipeline lost part of a row.
   $code=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -Arguments @("--host=$($c[0])","--port=$($c[1])","--user=$($c[2])","--database=$($c[4])",'--default-character-set=utf8mb4','--batch','--raw','--skip-column-names','--force') -InputFile $sqlFile -OutputFile $stdoutFile -ErrorFile $errorFile
   $rows=@(Get-Content $stdoutFile -Encoding UTF8);$errors=[string](Get-Content $errorFile -Raw -Encoding UTF8)
   if($Refusal){if($errors -notmatch 'Frost souls native presentation/combat/context differ; review before updating' -or @([regex]::Matches($errors,'ERROR [0-9]+')).Count -ne 1){throw "Unexpected Halak presentation refusal: $errorFile"}}
   elseif($code -ne 0 -or $errors){throw "Halak presentation SQL failed: $errorFile"}
   if(!$rows.Count -or $rows[-1] -cne '__FROST_KINGS_PRESENTATION_SQL_COMPLETE__'){throw "Incomplete native SQL output: $stdoutFile"}
   return ,@($rows|Select-Object -SkipLast 1)
  }finally{$env:MYSQL_PWD=$old;[Console]::OutputEncoding=$previousConsole;$OutputEncoding=$previousPipe}
 }
 $fixture=Get-Content "$PSScriptRoot/FrostSoulsPresentationOriginal.json" -Raw -Encoding UTF8|ConvertFrom-Json
 $filters=[ordered]@{smart_scripts='source_type=0 AND entryorguid IN(93948,93950,93951)';conditions='SourceTypeOrReferenceId=22 AND SourceEntry IN(93948,93950,93951)';creature_text='CreatureID IN(93948,93950,93951)';creature_template='entry IN(93948,93950,93951)';creature_template_wdb='Entry IN(93948,93950,93951)';creature='id IN(93948,93950,93951)';conversation_data='entry IN(738,740,742,743)';conversation_creature='entry IN(738,740,742,743)';conversation_actor='entry IN(738,740,742,743)'}
 $order=@{smart_scripts='entryorguid,source_type,id,link';conditions='SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2';creature_text='CreatureID,GroupID,ID';creature_template='entry';creature_template_wdb='Entry';creature='guid';conversation_data='entry,id,idx';conversation_creature='entry,id';conversation_actor='entry,id'}
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
 if($original.Count -ne 18 -or $applied.Count -ne 18){throw 'Missing native three Frost souls rows'}
 $edits=0;for($i=0;$i -lt 18;++$i){$f=$original[$i].Split("`t");if($f[0] -notin @('93948','93950','93951') -or $f[1] -ne '0'){throw 'Unexpected soul script scope'}
  if(($f[0] -eq '93948' -and $f[2] -eq '1') -or($f[0] -in @('93950','93951') -and $f[2] -eq '0')){if($f[14] -ne '206' -or $f[21] -ne '2' -or $f[22] -ne '0'){throw 'Original conversation target differs'};$f[21]='130';$f[22]='70';$edits+=2}
  elseif($f[14] -eq '1'){if($f[21] -notin @('2','7')){throw 'Original soul speaker differs'};$f[21]='1';++$edits}
  if(($f -join "`t") -cne $applied[$i]){throw 'Unexpected soul row mutation'}
 };if($edits -ne 13){throw 'Expected exactly13 soul target fields'}
 [IO.File]::WriteAllLines("$out/original.tsv",[string[]]$original)
 [IO.File]::WriteAllLines("$out/repaired.tsv",[string[]]$applied)
 [IO.File]::WriteAllLines("$out/texts.tsv",[string[]]((Signature $rows 'first-creature_text').Split("`n")))
 [IO.File]::WriteAllLines("$out/sql-evidence.tsv",[string[]]$rows)
 $variants=[ordered]@{
 'proximity-conversation'='UPDATE smart_scripts SET action_param1=1 WHERE entryorguid=93948 AND id=0;';
 'baelgun-conversation'='UPDATE smart_scripts SET action_param1=1 WHERE entryorguid=93948 AND id=1;';
 'baelgun-cast-link'='UPDATE smart_scripts SET link=0 WHERE entryorguid=93948 AND id=7;';
 'baelgun-cast-timer'='UPDATE smart_scripts SET event_param1=1 WHERE entryorguid=93948 AND id=5;';
 'baelgun-linked-text'='UPDATE smart_scripts SET action_param1=0 WHERE entryorguid=93948 AND id=3;';
 'magroth-conversation'='UPDATE smart_scripts SET action_param1=1 WHERE entryorguid=93950 AND id=0;';
 'magroth-death'='UPDATE smart_scripts SET action_param1=0 WHERE entryorguid=93950 AND id=2;';
 'magroth-cast'='UPDATE smart_scripts SET action_param1=1 WHERE entryorguid=93950 AND id=3;';
 'gavinrad-conversation'='UPDATE smart_scripts SET action_param1=1 WHERE entryorguid=93951 AND id=0;';
 'gavinrad-death'='UPDATE smart_scripts SET action_param1=0 WHERE entryorguid=93951 AND id=2;';
 'gavinrad-cast'='UPDATE smart_scripts SET action_param1=1 WHERE entryorguid=93951 AND id=4;';
 'custom-ai'="UPDATE creature_template SET ScriptName='custom' WHERE entry=93948;";
 'native-name'="UPDATE creature_template_wdb SET Name1='custom' WHERE Entry=93951;";
 'native-text'='UPDATE creature_text SET BroadcastTextID=1 WHERE CreatureID=93950 AND GroupID=1;';
 'native-spawn'='UPDATE creature SET PhaseId=1 WHERE id=93951;';
 'conversation-line'='UPDATE conversation_data SET textId=1 WHERE entry=740;';
 'conversation-actor'='UPDATE conversation_actor SET duration=1 WHERE entry=742;';
 'custom-condition'="INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,ConditionTypeOrReference,ConditionValue1) VALUES(22,1,93948,9,1);";
 'partial-speech'='UPDATE smart_scripts SET target_type=1 WHERE entryorguid=93950 AND id=2;';
 'partial-conversation'='UPDATE smart_scripts SET target_type=130 WHERE entryorguid=93948 AND id=1;';
 'applied-range'='UPDATE smart_scripts SET target_param1=71 WHERE entryorguid=93951 AND id=0;';
 'applied-extra-target'='UPDATE smart_scripts SET target_param2=1 WHERE entryorguid=93950 AND id=0;'
 }
 foreach($name in $variants.Keys){$base=$setup+$baseline+$(if($name -like 'applied-*'){$migration}else{''});$result=Query ($base+$variants[$name]+(Snapshot 'before')+$migration+(Snapshot 'after')) -Refusal
  foreach($table in $filters.Keys){if((Signature $result "before-$table") -cne (Signature $result "after-$table")){throw "Custom Frost souls data mutated $name/$table"}}
 }
 if($VerifyLiveData){$sha=[Security.Cryptography.SHA1]::Create();try{$hash=([BitConverter]::ToString($sha.ComputeHash([Text.Encoding]::UTF8.GetBytes($migration.Replace("`r`n","`n"))))).Replace('-','')}finally{$sha.Dispose()}
  $live=Query "SELECT COUNT(*) FROM updates WHERE name='2026_10_30_19_frost_souls_presentation.sql' AND hash='$hash' AND state='RELEASED';";if($live -notcontains '1'){throw 'Live Frost souls migration/hash differs'}
  $live=Query (Snapshot 'live');foreach($table in $filters.Keys){if((Signature $live "live-$table") -cne (Signature $rows "first-$table")){throw "Live native Frost souls context differs $table"}}
 }
 'PASS full9table three Frost souls: exactly13 target fields;740/742/743 and original738/combat/flags/delays/validLINKs unchanged, idempotence/22 partial-custom refusals; live checked only when requested'
}
