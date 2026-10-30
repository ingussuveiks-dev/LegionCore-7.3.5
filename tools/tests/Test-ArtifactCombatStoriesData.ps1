param([string]$MigrationFile,[switch]$VerifyLiveData)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/artifact-combat-stories-data"
if(!$MigrationFile){$MigrationFile="$repo/sql/pending/world/2026_10_30_20_artifact_combat_stories.sql";if(!(Test-Path $MigrationFile)){$MigrationFile="$repo/sql/updates/world/2026_10_30_20_artifact_combat_stories.sql"}}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -Test {
 $migration=Get-Content $MigrationFile -Raw
 $c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $script:queryIndex=0
 function Query([string]$sql,[switch]$Refusal){
  $old=$env:MYSQL_PWD;$errorFile="$out/sql-$script:queryIndex.stderr";++$script:queryIndex
  [IO.File]::WriteAllText("$out/sql-$($script:queryIndex-1).sql",$sql+"`nSELECT '__ARTIFACT_COMBAT_STORIES_SQL_COMPLETE__';`n")
  $previousConsole=[Console]::OutputEncoding;$previousPipe=$OutputEncoding
  try{$env:MYSQL_PWD=$c[3];$sqlFile="$out/sql-$($script:queryIndex-1).sql";$stdoutFile="$out/sql-$($script:queryIndex-1).stdout"
   # Preserve complete native stdout/stderr bytes, including the tail after an
   # expected --force error; a PowerShell native pipeline lost part of a row.
   $code=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -Arguments @("--host=$($c[0])","--port=$($c[1])","--user=$($c[2])","--database=$($c[4])",'--default-character-set=utf8mb4','--batch','--raw','--skip-column-names','--force') -InputFile $sqlFile -OutputFile $stdoutFile -ErrorFile $errorFile
   $rows=@(Get-Content $stdoutFile -Encoding UTF8);$errors=[string](Get-Content $errorFile -Raw -Encoding UTF8)
   if($Refusal){if($errors -notmatch 'Artifact combat conversation/context differ; review before updating' -or @([regex]::Matches($errors,'ERROR [0-9]+')).Count -ne 1){throw "Unexpected Halak presentation refusal: $errorFile"}}
   elseif($code -ne 0 -or $errors){throw "Halak presentation SQL failed: $errorFile"}
   if(!$rows.Count -or $rows[-1] -cne '__ARTIFACT_COMBAT_STORIES_SQL_COMPLETE__'){throw "Incomplete native SQL output: $stdoutFile"}
   return ,@($rows|Select-Object -SkipLast 1)
  }finally{$env:MYSQL_PWD=$old;[Console]::OutputEncoding=$previousConsole;$OutputEncoding=$previousPipe}
 }
 $fixture=Get-Content "$PSScriptRoot/ArtifactCombatStoriesOriginal.json" -Raw -Encoding UTF8|ConvertFrom-Json
 $filters=[ordered]@{smart_scripts='(source_type=0 AND entryorguid IN(101114,105799,106700,104832,106699)) OR(source_type=9 AND entryorguid IN(104832,106699))';conditions='SourceTypeOrReferenceId=22 AND SourceEntry IN(101114,105799,106700,104832,106699)';creature_text='CreatureID IN(101114,105799,106700,104832,106699)';creature_template='entry IN(101114,105799,106700,104832,106699)';creature_template_wdb='Entry IN(101114,105799,106700,104832,106699)';creature='id IN(101114,105799,106700,104832,106699)';conversation_data='entry IN(1331,1336,1667)';conversation_creature='entry IN(1331,1336,1667)';conversation_actor='entry IN(1331,1336,1667)'}
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
 if($original.Count -ne @($fixture.smart_scripts).Count -or $applied.Count -ne $original.Count){throw 'Missing native combat story/context rows'}
 $edits=0;for($i=0;$i -lt $original.Count;++$i){$f=$original[$i].Split("`t")
  if($f[1] -eq '0' -and (($f[0] -eq '101114' -and $f[2] -in @('2','3')) -or($f[0] -in @('105799','106700') -and $f[2] -eq '4'))){if($f[14] -ne '206' -or $f[21] -ne '7' -or $f[22] -ne '0'){throw 'Original raw story invoker differs'};$f[21]='130';$f[22]='70';$edits+=2}
  if(($f -join "`t") -cne $applied[$i]){throw 'Unexpected combat story row mutation'}
 };if($edits -ne 8){throw 'Expected exactly8 story target fields'}
 [IO.File]::WriteAllLines("$out/original.tsv",[string[]]$original)
 [IO.File]::WriteAllLines("$out/repaired.tsv",[string[]]$applied)
 [IO.File]::WriteAllLines("$out/texts.tsv",[string[]]((Signature $rows 'first-creature_text').Split("`n")))
 [IO.File]::WriteAllLines("$out/sql-evidence.tsv",[string[]]$rows)
 $variants=[ordered]@{
 'oppressor-intro-link'='UPDATE smart_scripts SET link=0 WHERE entryorguid=101114 AND id=0 AND source_type=0;';
 'oppressor-native-credit'='UPDATE smart_scripts SET action_param2=1 WHERE entryorguid=101114 AND id=0 AND source_type=0;';
 'oppressor-intro'='UPDATE smart_scripts SET action_param1=1 WHERE entryorguid=101114 AND id=2 AND source_type=0;';
 'oppressor-death'='UPDATE smart_scripts SET event_flags=0 WHERE entryorguid=101114 AND id=3 AND source_type=0;';
 'oppressor-cast'='UPDATE smart_scripts SET action_param1=1 WHERE entryorguid=101114 AND id=1 AND source_type=0;';
 'scion-a-cast'='UPDATE smart_scripts SET target_type=1 WHERE entryorguid=105799 AND id=0 AND source_type=0;';
 'scion-a-recipient'='UPDATE smart_scripts SET target_param1=1 WHERE entryorguid=105799 AND id=2 AND source_type=0;';
 'scion-a-removal'='UPDATE smart_scripts SET action_param1=1 WHERE entryorguid=105799 AND id=3 AND source_type=0;';
 'scion-b-recipient'='UPDATE smart_scripts SET target_param1=1 WHERE entryorguid=106700 AND id=2 AND source_type=0;';
 'scion-b-story'='UPDATE smart_scripts SET action_param1=1 WHERE entryorguid=106700 AND id=4 AND source_type=0;';
 'receiver-unresolved-link'='UPDATE smart_scripts SET link=0 WHERE entryorguid=104832 AND id=0 AND source_type=0;';
 'receiver-health'='UPDATE smart_scripts SET event_param1=0 WHERE entryorguid=104832 AND id=3 AND source_type=0;';
 'receiver-timed'='UPDATE smart_scripts SET action_param1=1 WHERE entryorguid=104832 AND id=0 AND source_type=9;';
 'custom-ai'="UPDATE creature_template SET ScriptName='custom' WHERE entry=106700;";
 'native-spawn'='UPDATE creature SET PhaseId=1 WHERE id=101114;';
 'native-name'="UPDATE creature_template_wdb SET Name1='custom' WHERE Entry=101114;";
 'conversation-line'='UPDATE conversation_data SET textId=1 WHERE entry=1331;';
 'conversation-actor'='UPDATE conversation_actor SET duration=1 WHERE entry=1667;';
 'custom-condition'="INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,ConditionTypeOrReference,ConditionValue1) VALUES(22,1,101114,9,1);";
 'partial-target'='UPDATE smart_scripts SET target_type=130 WHERE entryorguid=105799 AND id=4;';
 'applied-range'='UPDATE smart_scripts SET target_param1=71 WHERE entryorguid=106700 AND id=4;';
 'applied-extra-target'='UPDATE smart_scripts SET target_param2=1 WHERE entryorguid=101114 AND id=3;'
 }
 foreach($name in $variants.Keys){$base=$setup+$baseline+$(if($name -like 'applied-*'){$migration}else{''});$result=Query ($base+$variants[$name]+(Snapshot 'before')+$migration+(Snapshot 'after')) -Refusal
  foreach($table in $filters.Keys){if((Signature $result "before-$table") -cne (Signature $result "after-$table")){throw "Custom combat story data mutated $name/$table"}}
 }
 if($VerifyLiveData){$sha=[Security.Cryptography.SHA1]::Create();try{$hash=([BitConverter]::ToString($sha.ComputeHash([Text.Encoding]::UTF8.GetBytes($migration.Replace("`r`n","`n"))))).Replace('-','')}finally{$sha.Dispose()}
  $live=Query "SELECT COUNT(*) FROM updates WHERE name='2026_10_30_20_artifact_combat_stories.sql' AND hash='$hash' AND state='RELEASED';";if($live -notcontains '1'){throw 'Live combat story migration/hash differs'}
  $live=Query (Snapshot 'live');foreach($table in $filters.Keys){if((Signature $live "live-$table") -cne (Signature $rows "first-$table")){throw "Live native combat story context differs $table"}}
 }
 'PASS full9table artifact combat stories: exactly8 fields on4 story rows;1331/1336/1667 payload,49022 credit,casts,Azuregos recipients/unknownLINK unchanged; idempotence/22 partial-custom refusals/live when requested'
}
