param([string]$MigrationFile,[switch]$VerifyLiveData)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/artifact-resource-data"
if(!$MigrationFile){$MigrationFile="$repo/sql/pending/world/2026_10_30_22_nexus_energy_owner.sql";if(!(Test-Path $MigrationFile)){$MigrationFile="$repo/sql/updates/world/2026_10_30_22_nexus_energy_owner.sql"}}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -Test {
 $migration=Get-Content $MigrationFile -Raw
 $c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $script:queryIndex=0
 function Query([string]$sql,[switch]$Refusal){
  $old=$env:MYSQL_PWD;$errorFile="$out/sql-$script:queryIndex.stderr";++$script:queryIndex
  [IO.File]::WriteAllText("$out/sql-$($script:queryIndex-1).sql",$sql+"`nSELECT '__NEXUS_ENERGY_SQL_COMPLETE__';`n")
  $previousConsole=[Console]::OutputEncoding;$previousPipe=$OutputEncoding
  try{$env:MYSQL_PWD=$c[3];$sqlFile="$out/sql-$($script:queryIndex-1).sql";$stdoutFile="$out/sql-$($script:queryIndex-1).stdout"
   # Preserve complete native stdout/stderr bytes, including the tail after an
   # expected --force error; a PowerShell native pipeline lost part of a row.
   $code=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -Arguments @("--host=$($c[0])","--port=$($c[1])","--user=$($c[2])","--database=$($c[4])",'--default-character-set=utf8mb4','--batch','--raw','--skip-column-names','--force') -InputFile $sqlFile -OutputFile $stdoutFile -ErrorFile $errorFile
   $rows=@(Get-Content $stdoutFile -Encoding UTF8);$errors=[string](Get-Content $errorFile -Raw -Encoding UTF8)
   if($Refusal){if($errors -notmatch 'Nexus energy death/bar/quest context differs; review before updating' -or @([regex]::Matches($errors,'ERROR [0-9]+')).Count -ne 1){throw "Unexpected Nexus energy refusal: $errorFile"}}
   elseif($code -ne 0 -or $errors){throw "Nexus energy SQL failed: $errorFile"}
   if(!$rows.Count -or $rows[-1] -cne '__NEXUS_ENERGY_SQL_COMPLETE__'){throw "Incomplete native SQL output: $stdoutFile"}
   return ,@($rows|Select-Object -SkipLast 1)
  }finally{$env:MYSQL_PWD=$old;[Console]::OutputEncoding=$previousConsole;$OutputEncoding=$previousPipe}
 }
 $fixture=Get-Content "$PSScriptRoot/ArtifactResourceOriginal.json" -Raw -Encoding UTF8|ConvertFrom-Json
 $filters=[ordered]@{smart_scripts='(source_type=0 AND entryorguid IN(104627,106569,106566)) OR(source_type=9 AND entryorguid=106566)';conditions='(SourceTypeOrReferenceId=22 AND SourceEntry IN(104627,106569,106566)) OR(SourceTypeOrReferenceId IN(13,17) AND SourceEntry IN(152435,207066,211691,211692,207430,207434,207437)) OR(SourceTypeOrReferenceId=19 AND SourceEntry IN(41629,42009))';creature_template='entry IN(104627,106569,106566)';creature_template_wdb='Entry IN(104627,106569,106566)';creature='id IN(104627,106569,106566)';spell_script_names='spell_id IN(152435,207066,211691,211692,207430,207434,207437)';spell_area='spell IN(207066,211691)';quest_template='ID IN(41629,42009)';quest_template_addon='ID IN(41629,42009)';quest_template_locale='ID IN(41629,42009) AND locale=''enUS''';quest_objectives='QuestID IN(41629,42009)'}
 $order=@{smart_scripts='entryorguid,source_type,id,link';conditions='SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2';creature_template='entry';creature_template_wdb='Entry';creature='guid';spell_script_names='spell_id,ScriptName';spell_area='spell,area,quest_start,quest_end';quest_template='ID';quest_template_addon='ID';quest_template_locale='ID,locale';quest_objectives='ID'}
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
 if($original.Count -ne @($fixture.smart_scripts).Count -or $applied.Count -ne $original.Count){throw 'Missing native Nexus energy/context rows'}
 $edits=0;for($i=0;$i -lt $original.Count;++$i){$f=$original[$i].Split("`t")
  if($f[1] -eq '0' -and (($f[0] -eq '104627' -and $f[2] -eq '1') -or($f[0] -eq '106569' -and $f[2] -eq '0'))){if($f[5] -ne '6' -or $f[14] -ne '85' -or $f[15] -ne '152435' -or $f[16] -ne '0' -or $f[21] -ne '7' -or $f[22] -ne '0'){throw 'Original Nexus energy tuple differs'};$f[16]='128';$f[21]='130';$f[22]='100';$edits+=3}
  if(($f -join "`t") -cne $applied[$i]){throw 'Unexpected Nexus energy row mutation'}
 };if($edits -ne 6){throw 'Expected exactly6 native Nexus energy fields'}
 foreach($pair in @(@('original','baseline-row'),@('first','row'))){$native=@((Signature $rows "$($pair[0])-smart_scripts").Split("`n")|Where-Object {$f=$_.Split("`t");$f[1] -eq '0' -and (($f[0] -eq '104627' -and $f[2] -eq '1') -or($f[0] -eq '106569' -and $f[2] -eq '0'))});if($native.Count -ne 2){throw 'Missing native Nexus energy rows'};$tuples=@($native|ForEach-Object {$f=$_.Split("`t");($f[14],$f[15],$f[16],$f[21],$f[22]) -join "`t"});if($tuples[0] -cne $tuples[1]){throw 'Two Nexus rows do not use same native payload'};[IO.File]::WriteAllText("$out/$($pair[1]).tsv",$tuples[0]+"`n")}
 [IO.File]::WriteAllLines("$out/original.tsv",[string[]]$original)
 [IO.File]::WriteAllLines("$out/repaired.tsv",[string[]]$applied)
 [IO.File]::WriteAllLines("$out/sql-evidence.tsv",[string[]]$rows)
 $variants=[ordered]@{
 'native-spell'='UPDATE smart_scripts SET action_param1=1 WHERE entryorguid=104627 AND source_type=0 AND id=1;';
 'native-flags'='UPDATE smart_scripts SET action_param2=2 WHERE entryorguid=104627 AND source_type=0 AND id=1;';
 'native-event'='UPDATE smart_scripts SET event_type=4 WHERE entryorguid=106569 AND source_type=0 AND id=0;';
 'native-update-credit'='UPDATE smart_scripts SET action_param1=1 WHERE entryorguid=104627 AND source_type=0 AND id=0;';
 'native-combat'='UPDATE smart_scripts SET action_param1=1 WHERE entryorguid=106569 AND source_type=0 AND id=1;';
 'native-breach-delay'='UPDATE smart_scripts SET event_param1=1 WHERE entryorguid=106566 AND source_type=9 AND id=0;';
 'native-breach-despawn'='UPDATE smart_scripts SET action_type=1 WHERE entryorguid=106566 AND source_type=9 AND id=1;';
 'native-ai'="UPDATE creature_template SET ScriptName='custom' WHERE entry=106569;";
 'native-spawn'='UPDATE creature SET phaseMask=2 WHERE id=104627 LIMIT 1;';
 'bar-admission'='UPDATE spell_area SET quest_start_status=64 WHERE spell=207066;';
 'bar-class'='UPDATE spell_area SET classmask=1 WHERE spell=211691;';
 'quest-prerequisite'='UPDATE quest_template_addon SET PrevQuestID=1 WHERE ID=41629;';
 'quest-objective'='UPDATE quest_objectives SET Amount=1 WHERE ObjectID=104789 AND QuestID=42009;';
 'custom-condition'="INSERT INTO conditions(SourceTypeOrReferenceId,SourceEntry,ConditionTypeOrReference,ConditionValue1) VALUES(17,152435,9,1);";
 'custom-spell-script'="INSERT INTO spell_script_names(spell_id,ScriptName) VALUES(152435,'custom');";
 'partial-one-row'='UPDATE smart_scripts SET action_param2=128,target_type=130,target_param1=100 WHERE entryorguid=104627 AND source_type=0 AND id=1;';
 'partial-target'='UPDATE smart_scripts SET target_type=130 WHERE entryorguid=106569 AND source_type=0 AND id=0;';
 'applied-range'='UPDATE smart_scripts SET target_param1=101 WHERE entryorguid=106569 AND source_type=0 AND id=0;';
 'applied-extra-target'='UPDATE smart_scripts SET target_param2=1 WHERE entryorguid=104627 AND source_type=0 AND id=1;'
 }
 foreach($name in $variants.Keys){$base=$setup+$baseline+$(if($name -like 'applied-*'){$migration}else{''});$result=Query ($base+$variants[$name]+(Snapshot 'before')+$migration+(Snapshot 'after')) -Refusal
  foreach($table in $filters.Keys){if((Signature $result "before-$table") -cne (Signature $result "after-$table")){throw "Custom Nexus energy data mutated $name/$table"}}
 }
 if($VerifyLiveData){$sha=[Security.Cryptography.SHA1]::Create();try{$hash=([BitConverter]::ToString($sha.ComputeHash([Text.Encoding]::UTF8.GetBytes($migration.Replace("`r`n","`n"))))).Replace('-','')}finally{$sha.Dispose()}
  $live=Query "SELECT COUNT(*) FROM updates WHERE name='2026_10_30_22_nexus_energy_owner.sql' AND hash='$hash' AND state='RELEASED';";if($live -notcontains '1'){throw 'Live Nexus energy migration/hash differs'}
  $live=Query (Snapshot 'live');foreach($table in $filters.Keys){if((Signature $live "live-$table") -cne (Signature $rows "first-$table")){throw "Live native Nexus energy context differs $table"}}
 }
 'PASS full11table Nexus energy: exactly6 fields on2 death rows; native152435,100yd range,bar quest/class admission,objectives/combat/breach unchanged; twice-idempotent/19 partial-custom refusals/live when requested'
}
