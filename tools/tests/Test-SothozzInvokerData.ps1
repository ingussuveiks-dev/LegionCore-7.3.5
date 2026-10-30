param([string]$MigrationFile,[switch]$VerifyLiveData)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/sothozz-invoker-data"
if(!$MigrationFile){$MigrationFile="$repo/sql/pending/world/2026_10_30_21_sothozz_invoker.sql";if(!(Test-Path $MigrationFile)){$MigrationFile="$repo/sql/updates/world/2026_10_30_21_sothozz_invoker.sql"}}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -Test {
 $migration=Get-Content $MigrationFile -Raw
 $c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $script:queryIndex=0
 function Query([string]$sql,[switch]$Refusal){
  $old=$env:MYSQL_PWD;$errorFile="$out/sql-$script:queryIndex.stderr";++$script:queryIndex
  [IO.File]::WriteAllText("$out/sql-$($script:queryIndex-1).sql",$sql+"`nSELECT '__SOTHOZZ_SQL_COMPLETE__';`n")
  $previousConsole=[Console]::OutputEncoding;$previousPipe=$OutputEncoding
  try{$env:MYSQL_PWD=$c[3];$sqlFile="$out/sql-$($script:queryIndex-1).sql";$stdoutFile="$out/sql-$($script:queryIndex-1).stdout"
   # Preserve complete native stdout/stderr bytes, including the tail after an
   # expected --force error; a PowerShell native pipeline lost part of a row.
   $code=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -Arguments @("--host=$($c[0])","--port=$($c[1])","--user=$($c[2])","--database=$($c[4])",'--default-character-set=utf8mb4','--batch','--raw','--skip-column-names','--force') -InputFile $sqlFile -OutputFile $stdoutFile -ErrorFile $errorFile
   $rows=@(Get-Content $stdoutFile -Encoding UTF8);$errors=[string](Get-Content $errorFile -Raw -Encoding UTF8)
   if($Refusal){if($errors -notmatch 'Sothozz native invoker/producer/context differ; review before updating' -or @([regex]::Matches($errors,'ERROR [0-9]+')).Count -ne 1){throw "Unexpected Halak presentation refusal: $errorFile"}}
   elseif($code -ne 0 -or $errors){throw "Halak presentation SQL failed: $errorFile"}
   if(!$rows.Count -or $rows[-1] -cne '__SOTHOZZ_SQL_COMPLETE__'){throw "Incomplete native SQL output: $stdoutFile"}
   return ,@($rows|Select-Object -SkipLast 1)
  }finally{$env:MYSQL_PWD=$old;[Console]::OutputEncoding=$previousConsole;$OutputEncoding=$previousPipe}
 }
 $fixture=Get-Content "$PSScriptRoot/SothozzInvokerOriginal.json" -Raw -Encoding UTF8|ConvertFrom-Json
 $filters=[ordered]@{smart_scripts='(source_type=0 AND entryorguid IN(101461,103226,103151,104605,103144)) OR(source_type=9 AND entryorguid=101461) OR(source_type=1 AND entryorguid=251349)';conditions='(SourceTypeOrReferenceId=22 AND SourceEntry IN(101461,103226,103151,104605,103144,251349)) OR(SourceTypeOrReferenceId IN(13,17) AND SourceEntry IN(204364,204356))';creature_text='CreatureID IN(101461,103226,103151,104605,103144)';creature_template='entry IN(101461,103226,103151,104605,103144)';creature_template_wdb='Entry IN(101461,103226,103151,104605,103144)';creature='id IN(101461,103226,103151,104605,103144)';conversation_data='entry=1338';conversation_creature='entry=1338';conversation_actor='entry=1338';gameobject_template='entry=251349';gameobject='guid IN(136767,136768)';spell_script_names='spell_id IN(204364,204356)'}
 $order=@{smart_scripts='entryorguid,source_type,id,link';conditions='SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2';creature_text='CreatureID,GroupID,ID';creature_template='entry';creature_template_wdb='Entry';creature='guid';conversation_data='entry,id,idx';conversation_creature='entry,id';conversation_actor='entry,id';gameobject_template='entry';gameobject='guid';spell_script_names='spell_id,ScriptName'}
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
 if($original.Count -ne @($fixture.smart_scripts).Count -or $applied.Count -ne $original.Count){throw 'Missing native Sothozz/context rows'}
 $edits=0;for($i=0;$i -lt $original.Count;++$i){$f=$original[$i].Split("`t")
  if($f[0] -eq '103226' -and $f[1] -eq '0' -and $f[2] -eq '7'){if($f[14] -ne '85' -or $f[15] -ne '204364' -or $f[16] -ne '0' -or $f[21] -ne '7' -or $f[22] -ne '0'){throw 'Original Sothozz caster tuple differs'};$f[16]='128';$f[21]='130';$f[22]='70';$edits+=3}
  if(($f -join "`t") -cne $applied[$i]){throw 'Unexpected Sothozz row mutation'}
 };if($edits -ne 3){throw 'Expected exactly3 native Sothozz caster fields'}
 foreach($pair in @(@('original','baseline-row'),@('first','row'))){$native=@((Signature $rows "$($pair[0])-smart_scripts").Split("`n")|Where-Object {$f=$_.Split("`t");$f[0] -eq '103226' -and $f[1] -eq '0' -and $f[2] -eq '7'});if($native.Count -ne 1){throw 'Missing native Sothozz row'};$f=$native[0].Split("`t");[IO.File]::WriteAllText("$out/$($pair[1]).tsv",(($f[14],$f[15],$f[16],$f[21],$f[22]) -join "`t")+"`n")}
 $payload=@();foreach($name in @('actor','creature','line')){$table=@{actor='conversation_actor';creature='conversation_creature';line='conversation_data'}[$name];$payload+=@((Signature $rows "first-$table").Split("`n")|Where-Object {$_}|ForEach-Object {"$name`t$_"})};[IO.File]::WriteAllLines("$out/conversation.tsv",[string[]]$payload)
 [IO.File]::WriteAllLines("$out/original.tsv",[string[]]$original)
 [IO.File]::WriteAllLines("$out/repaired.tsv",[string[]]$applied)
 [IO.File]::WriteAllLines("$out/texts.tsv",[string[]]((Signature $rows 'first-creature_text').Split("`n")))
 [IO.File]::WriteAllLines("$out/sql-evidence.tsv",[string[]]$rows)
 $variants=[ordered]@{
 'native-story'='UPDATE smart_scripts SET action_param1=1 WHERE entryorguid=103226 AND id=7 AND source_type=0;';
 'native-flags'='UPDATE smart_scripts SET action_param2=2 WHERE entryorguid=103226 AND id=7 AND source_type=0;';
 'native-event'='UPDATE smart_scripts SET event_type=6 WHERE entryorguid=103226 AND id=7 AND source_type=0;';
 'native-once'='UPDATE smart_scripts SET event_flags=0 WHERE entryorguid=103226 AND id=7 AND source_type=0;';
 'native-credit'='UPDATE smart_scripts SET action_param2=1 WHERE entryorguid=103226 AND id=0 AND source_type=0;';
 'receiver-guid'='UPDATE smart_scripts SET target_param1=1 WHERE entryorguid=103226 AND id=4 AND source_type=0;';
 'door-a'='UPDATE smart_scripts SET target_param1=1 WHERE entryorguid=103226 AND id=5 AND source_type=0;';
 'door-b'='UPDATE smart_scripts SET target_param1=1 WHERE entryorguid=103226 AND id=6 AND source_type=0;';
 'producer-summon'='UPDATE smart_scripts SET action_param1=1 WHERE entryorguid=101461 AND source_type=9 AND id=1;';
 'producer-delay'='UPDATE smart_scripts SET event_param1=1 WHERE entryorguid=101461 AND source_type=9 AND id=1;';
 'unknown-link'='UPDATE smart_scripts SET link=0 WHERE entryorguid=103151 AND id=1 AND source_type=0;';
 'native-ai'="UPDATE creature_template SET ScriptName='custom' WHERE entry=103226;";
 'native-spawn'='UPDATE creature SET PhaseId=1 WHERE guid=369686;';
 'native-door-template'='UPDATE gameobject_template SET displayId=1 WHERE entry=251349;';
 'native-door-pose'='UPDATE gameobject SET position_z=0 WHERE guid=136767;';
 'story-line'='UPDATE conversation_data SET textId=1;';
 'story-actor'='UPDATE conversation_actor SET duration=1;';
 'custom-condition'="INSERT INTO conditions(SourceTypeOrReferenceId,SourceEntry,ConditionTypeOrReference,ConditionValue1) VALUES(17,204364,9,1);";
 'custom-spell-script'="INSERT INTO spell_script_names(spell_id,ScriptName) VALUES(204364,'custom');";
 'partial-target'='UPDATE smart_scripts SET target_type=130 WHERE entryorguid=103226 AND id=7;';
 'partial-flags'='UPDATE smart_scripts SET action_param2=128 WHERE entryorguid=103226 AND id=7;';
 'applied-range'='UPDATE smart_scripts SET target_param1=71 WHERE entryorguid=103226 AND id=7;';
 'applied-extra-target'='UPDATE smart_scripts SET target_param2=1 WHERE entryorguid=103226 AND id=7;'
 }
 foreach($name in $variants.Keys){$base=$setup+$baseline+$(if($name -like 'applied-*'){$migration}else{''});$result=Query ($base+$variants[$name]+(Snapshot 'before')+$migration+(Snapshot 'after')) -Refusal
  foreach($table in $filters.Keys){if((Signature $result "before-$table") -cne (Signature $result "after-$table")){throw "Custom Sothozz data mutated $name/$table"}}
 }
 if($VerifyLiveData){$sha=[Security.Cryptography.SHA1]::Create();try{$hash=([BitConverter]::ToString($sha.ComputeHash([Text.Encoding]::UTF8.GetBytes($migration.Replace("`r`n","`n"))))).Replace('-','')}finally{$sha.Dispose()}
  $live=Query "SELECT COUNT(*) FROM updates WHERE name='2026_10_30_21_sothozz_invoker.sql' AND hash='$hash' AND state='RELEASED';";if($live -notcontains '1'){throw 'Live Sothozz migration/hash differs'}
  $live=Query (Snapshot 'live');foreach($table in $filters.Keys){if((Signature $live "live-$table") -cne (Signature $rows "first-$table")){throw "Live native Sothozz context differs $table"}}
 }
 'PASS full12table Sothozz: exactly3 fields on103226/7;85/204364 ordinary219/1338,49052,producer,receiver,unknownLINK2 and doors preserved; idempotence/23 partial-custom refusals/live when requested'
}
