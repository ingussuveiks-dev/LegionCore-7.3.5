param([string]$MigrationFile,[switch]$VerifyLiveData)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/artifact-owner-voice-data"
if(!$MigrationFile){$MigrationFile="$repo/sql/pending/world/2026_10_30_24_artifact_owner_voice.sql";if(!(Test-Path $MigrationFile)){$MigrationFile="$repo/sql/updates/world/2026_10_30_24_artifact_owner_voice.sql"}}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -Test {
 $migration=Get-Content $MigrationFile -Raw
 $c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $script:queryIndex=0
 function Query([string]$sql,[switch]$Refusal){
  $old=$env:MYSQL_PWD;$errorFile="$out/sql-$script:queryIndex.stderr";++$script:queryIndex
  [IO.File]::WriteAllText("$out/sql-$($script:queryIndex-1).sql",$sql+"`nSELECT '__OWNER_VOICE_SQL_COMPLETE__';`n")
  $previousConsole=[Console]::OutputEncoding;$previousPipe=$OutputEncoding
  try{$env:MYSQL_PWD=$c[3];$sqlFile="$out/sql-$($script:queryIndex-1).sql";$stdoutFile="$out/sql-$($script:queryIndex-1).stdout"
   # Preserve complete native stdout/stderr bytes, including the tail after an
   # expected --force error; a PowerShell native pipeline lost part of a row.
   $code=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -Arguments @("--host=$($c[0])","--port=$($c[1])","--user=$($c[2])","--database=$($c[4])",'--default-character-set=utf8mb4','--batch','--raw','--skip-column-names','--force') -InputFile $sqlFile -OutputFile $stdoutFile -ErrorFile $errorFile
   $rows=@(Get-Content $stdoutFile -Encoding UTF8);$errors=[string](Get-Content $errorFile -Raw -Encoding UTF8)
   if($Refusal){if($errors -notmatch 'Artifact owner voice/context differs; review before updating' -or @([regex]::Matches($errors,'ERROR [0-9]+')).Count -ne 1){throw "Unexpected Artifact owner voice refusal: $errorFile"}}
   elseif($code -ne 0 -or $errors){throw "Artifact owner voice SQL failed: $errorFile"}
   if(!$rows.Count -or $rows[-1] -cne '__OWNER_VOICE_SQL_COMPLETE__'){throw "Incomplete native SQL output: $stdoutFile"}
   return ,@($rows|Select-Object -SkipLast 1)
  }finally{$env:MYSQL_PWD=$old;[Console]::OutputEncoding=$previousConsole;$OutputEncoding=$previousPipe}
 }
 $fixture=Get-Content "$PSScriptRoot/ArtifactOwnerVoiceOriginal.json" -Raw -Encoding UTF8|ConvertFrom-Json
 $filters=[ordered]@{smart_scripts='source_type IN(0,9) AND entryorguid IN(98760,99605,99600,100937,100938,102244,102245,102304,102362,109397)';conditions='(SourceTypeOrReferenceId=22 AND SourceEntry IN(98760,99605,99600,100937,100938,102244,102245,102304,102362,109397)) OR(SourceTypeOrReferenceId IN(13,17) AND SourceEntry IN(198484,79859,198525,197760,183345,205660,203373,203481,202177,182217,219583,219426))';creature_text='CreatureID IN(98760,99605,99600,100937,100938,102244,102245,102304,102362,109397)';creature_template='entry IN(98760,99605,99600,100937,100938,102244,102245,102304,102362,109397)';creature_template_wdb='Entry IN(98760,99605,99600,100937,100938,102244,102245,102304,102362,109397)';creature='id IN(98760,99605,99600,100937,100938,102244,102245,102304,102362,109397)';spell_script_names='spell_id IN(198484,79859,198525,197760,183345,205660,203373,203481,202177,182217,219583,219426)';phase_definitions='phaseId IN(''5426'',''5639'',''5719'',''5761'',''5955'',''6905'')'}
 $order=@{smart_scripts='entryorguid,source_type,id,link';conditions='SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2';creature_text='CreatureID,GroupID,ID';creature_template='entry';creature_template_wdb='Entry';creature='guid';spell_script_names='spell_id,ScriptName';phase_definitions='zoneId,entry'}
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
 if($original.Count -ne @($fixture.smart_scripts).Count -or $applied.Count -ne $original.Count){throw 'Missing native Artifact owner voice/context rows'}
 $selected=@('98760/6','100937/1','100938/1','102244/0','102245/0','102245/1','102304/0','102304/1','102362/2','109397/1')
 $edits=0;for($i=0;$i -lt $original.Count;++$i){$f=$original[$i].Split("`t")
  if($f[1] -eq '0' -and ($f[0]+'/'+$f[2]) -in $selected){if($f[14] -ne '1' -or $f[21] -notin @('2','7') -or $f[22] -ne '0'){throw 'Original owner TALK tuple differs'};$f[21]='130';$f[22]='70';$edits+=2}
  if(($f -join "`t") -cne $applied[$i]){throw 'Unexpected owner voice row mutation'}
 };if($edits -ne 20){throw 'Expected exactly20 native owner voice fields'}
 [IO.File]::WriteAllLines("$out/original.tsv",[string[]]$original)
 [IO.File]::WriteAllLines("$out/repaired.tsv",[string[]]$applied)
 [IO.File]::WriteAllLines("$out/texts.tsv",[string[]]((Signature $rows 'first-creature_text').Split("`n")))
 [IO.File]::WriteAllLines("$out/sql-evidence.tsv",[string[]]$rows)
 $variants=[ordered]@{
 'native-event'='UPDATE smart_scripts SET event_type=6 WHERE entryorguid=98760 AND id=6;';
 'native-repeat'='UPDATE smart_scripts SET event_flags=1 WHERE entryorguid=100938 AND id=1;';
 'native-chance'='UPDATE smart_scripts SET event_chance=100 WHERE entryorguid=102245 AND id=0;';
 'native-text-duration'='UPDATE smart_scripts SET action_param2=1 WHERE entryorguid=102304 AND id=1;';
 'native-credit'='UPDATE smart_scripts SET action_param2=1 WHERE entryorguid=98760 AND id=4;';
 'native-free-block'='UPDATE smart_scripts SET target_param2=1 WHERE entryorguid=98760 AND id=3;';
 'native-block-proximity'='UPDATE smart_scripts SET event_param2=1 WHERE entryorguid=99605 AND id=0;';
 'native-geist'='UPDATE smart_scripts SET action_param1=1 WHERE entryorguid=99600 AND id=1;';
 'native-prison-data'='UPDATE smart_scripts SET action_param1=1 WHERE entryorguid=102244 AND id=2;';
 'native-prison-credit'='UPDATE smart_scripts SET action_param1=1 WHERE entryorguid=102304 AND id=5;';
 'native-brew-credit'='UPDATE smart_scripts SET action_param2=1 WHERE entryorguid=109397 AND id=4;';
 'native-brew-summon'='UPDATE smart_scripts SET action_param1=1 WHERE entryorguid=109397 AND id=5;';
 'native-ai'="UPDATE creature_template SET ScriptName='custom' WHERE entry=100937;";
 'native-spawn'='UPDATE creature SET phaseMask=2 WHERE id=102245 LIMIT 1;';
 'native-phase'='UPDATE phase_definitions SET phaseId=1 WHERE phaseId=6905;';
 'native-class-variable'="UPDATE creature_text SET Text='custom' WHERE CreatureID=100938 AND BroadcastTextID=104859;";
 'custom-condition'="INSERT INTO conditions(SourceTypeOrReferenceId,SourceEntry,ConditionTypeOrReference,ConditionValue1) VALUES(22,98760,9,1);";
 'partial-target'='UPDATE smart_scripts SET target_type=130 WHERE entryorguid=98760 AND id=6;';
 'partial-one-row'='UPDATE smart_scripts SET target_type=130,target_param1=70 WHERE entryorguid=100937 AND id=1;';
 'applied-range'='UPDATE smart_scripts SET target_param1=71 WHERE entryorguid=102304 AND id=1;'
 }
 foreach($name in $variants.Keys){$base=$setup+$baseline+$(if($name -like 'applied-*'){$migration}else{''});$result=Query ($base+$variants[$name]+(Snapshot 'before')+$migration+(Snapshot 'after')) -Refusal
  foreach($table in $filters.Keys){if((Signature $result "before-$table") -cne (Signature $result "after-$table")){throw "Custom Artifact owner voice data mutated $name/$table"}}
 }
 if($VerifyLiveData){$sha=[Security.Cryptography.SHA1]::Create();try{$hash=([BitConverter]::ToString($sha.ComputeHash([Text.Encoding]::UTF8.GetBytes($migration.Replace("`r`n","`n"))))).Replace('-','')}finally{$sha.Dispose()}
  $live=Query "SELECT COUNT(*) FROM updates WHERE name='2026_10_30_24_artifact_owner_voice.sql' AND hash='$hash' AND state='RELEASED';";if($live -notcontains '1'){throw 'Live Artifact owner voice migration/hash differs'}
  $live=Query (Snapshot 'live')
  # World25 changes this independently reviewed ordinary conversation, not a
  # World24 TALK. Admit only its complete released ten-table proof, then undo
  # those three exact fields in this local comparison; never mutate live SQL.
  $iceborn=@($live|Where-Object {$_ -like "live-smart_scripts`t98760`t0`t7`t*"})
  if($iceborn.Count -ne 1){throw 'Missing live Iceborn comparison context'}
  $f=$iceborn[0].Split("`t")
  if($f[15] -eq '85' -and $f[16] -eq '197760' -and $f[17] -eq '128' -and $f[22] -eq '130' -and $f[23] -eq '70'){
   & "$PSScriptRoot/Test-FireIcebornStoryData.ps1" -VerifyLiveData
   $f[17]='0';$f[22]='7';$f[23]='0';$normalized=$f -join "`t"
   $live=@($live|ForEach-Object {if($_ -ceq $iceborn[0]){$normalized}else{$_}})
  }
  foreach($table in $filters.Keys){if((Signature $live "live-$table") -cne (Signature $rows "first-$table")){throw "Live native Artifact owner voice context differs $table"}}
 }
 'PASS full8table artifact owner voice: exactly20 fields/10 TALKs on8 actors; native speaker/player text context,Fire block+geist alternatives,prison/Brew credit+recipients,phases unchanged; twice-idempotent/20 partial-custom refusals/live when requested'
}
