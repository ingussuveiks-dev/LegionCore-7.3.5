param([string]$MigrationFile,[switch]$VerifyLiveData)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/talonclaw-boarded-departure-data"
if(!$MigrationFile){$MigrationFile="$repo/sql/pending/world/2026_10_30_37_talonclaw_boarded_departure.sql";if(!(Test-Path $MigrationFile)){$MigrationFile="$repo/sql/updates/world/2026_10_30_37_talonclaw_boarded_departure.sql"}}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -Test {
 & "$PSScriptRoot/Invoke-WithTemporarySqlReader.ps1" -EvidenceDirectory "$out/reader" -Test {
 $migration=Get-Content $MigrationFile -Raw
 $c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $script:queryIndex=0
 function Query([string]$sql,[switch]$Refusal){
  $old=$env:MYSQL_PWD;$errorFile="$out/sql-$script:queryIndex.stderr";++$script:queryIndex
  [IO.File]::WriteAllText("$out/sql-$($script:queryIndex-1).sql",$sql+"`nSELECT '__TALON_BOARDED_SQL_COMPLETE__';`n")
  $previousConsole=[Console]::OutputEncoding;$previousPipe=$OutputEncoding
  try{$env:MYSQL_PWD=$env:CODEX_SQL_READER_PASSWORD;$sqlFile="$out/sql-$($script:queryIndex-1).sql";$stdoutFile="$out/sql-$($script:queryIndex-1).stdout"
   # Preserve complete native stdout/stderr bytes, including the tail after an
   # expected --force error; a PowerShell native pipeline lost part of a row.
   $code=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -Arguments @("--host=$($c[0])","--port=$($c[1])","--user=$env:CODEX_SQL_READER_USER","--database=$($c[4])",'--default-character-set=utf8mb4','--batch','--raw','--skip-column-names','--force') -InputFile $sqlFile -OutputFile $stdoutFile -ErrorFile $errorFile
   $rows=@(Get-Content $stdoutFile -Encoding UTF8);$errors=[string](Get-Content $errorFile -Raw -Encoding UTF8)
   if($Refusal){if($errors -notmatch 'Talonclaw boarding/transfer context differs; review before applying' -or @([regex]::Matches($errors,'ERROR [0-9]+')).Count -ne 1){throw "Unexpected Talonclaw boarded departure refusal: $errorFile"}}
   elseif($code -ne 0 -or $errors){throw "Talonclaw boarded departure SQL failed: $errorFile"}
   if(!$rows.Count -or $rows[-1] -cne '__TALON_BOARDED_SQL_COMPLETE__'){throw "Incomplete native SQL output: $stdoutFile"}
   return ,@($rows|Select-Object -SkipLast 1)
  }finally{$env:MYSQL_PWD=$old;[Console]::OutputEncoding=$previousConsole;$OutputEncoding=$previousPipe}
 }
 $fixture=Get-Content "$PSScriptRoot/TalonclawBoardedDepartureOriginal.json" -Raw -Encoding UTF8|ConvertFrom-Json
 $context=Get-Content "$PSScriptRoot/TalonclawBoardedDepartureContext.json" -Raw|ConvertFrom-Json -AsHashtable
 $filters=$context.filters;$columns=$context.columns
 function Value($v){if($null -eq $v){'NULL'}elseif($v -is [string]){if($v.Length){"CONVERT(0x"+([BitConverter]::ToString([Text.Encoding]::UTF8.GetBytes($v))).Replace('-','')+" USING utf8mb4)"}else{"''"}}else{[Convert]::ToString($v,[Globalization.CultureInfo]::InvariantCulture)}}
 $setup='';$baseline=''
 foreach($table in $filters.Keys){$setup+="CREATE TEMPORARY TABLE saved_$table LIKE $table;INSERT INTO saved_$table SELECT * FROM $table WHERE $($filters[$table]);CREATE TEMPORARY TABLE $table LIKE saved_$table;`n";$baseline+="DELETE FROM $table;`n"
  foreach($row in $fixture.$table){$cols=($row.PSObject.Properties.Name|ForEach-Object {"``$_``"}) -join ',';$values=($row.PSObject.Properties|ForEach-Object {Value $_.Value}) -join ',';$baseline+="INSERT INTO $table($cols) VALUES($values);`n"}
 }
 function Snapshot([string]$tag){$sql='';foreach($table in $filters.Keys){$order=($columns[$table]|ForEach-Object {"``$_``"}) -join ',';$sql+="SELECT '$tag-$table',t.* FROM $table t WHERE $($filters[$table]) ORDER BY $order;"};$sql}
 function Signature($rows,[string]$tag){[string]::Join("`n",[string[]]@($rows|Where-Object {$_ -like "$tag`t*"}|ForEach-Object {$_.Substring($tag.Length+1)}))}
 $rows=Query ($setup+$baseline+(Snapshot 'original')+$migration+(Snapshot 'first')+$migration+(Snapshot 'second'))
 foreach($table in $filters.Keys){if((Signature $rows "first-$table") -cne (Signature $rows "second-$table")){throw "Not idempotent $table"};if($table -notin @('smart_scripts','creature_template','spell_script_names') -and (Signature $rows "original-$table") -cne (Signature $rows "first-$table")){throw "Unrelated context changed $table"}}
 $original=@((Signature $rows 'original-smart_scripts').Split("`n"));$applied=@((Signature $rows 'first-smart_scripts').Split("`n"));if($original.Count -ne 2 -or $applied.Count -ne 2){throw 'Expected exactly two original Smart actions'}
 $edits=0;for($i=0;$i -lt 2;++$i){$f=$original[$i].Split("`t");if($f[0] -ne '100965' -or $f[1] -ne '0' -or [int]$f[2] -ne $i){throw 'Unexpected Smart identity'}
  if($i -eq 0){if($f[3] -ne '1' -or $f[5] -ne '73' -or $f[14] -ne '33' -or $f[15] -ne '100111'){throw 'Original credit graph differs'};$f[3]='0';$f[5]='38';$f[9]='100111';$f[10]='1';$f[30]='Talonclaw: accepted native boarded departure receipt - original group credit';$edits+=5}
  else{if($f[5] -ne '61' -or $f[14] -ne '85' -or $f[15] -ne '198950' -or $f[16] -ne '0' -or $f[21] -ne '7' -or $f[22] -ne '0'){throw 'Original transfer graph differs'};$f[5]='73';$f[16]='128';$f[21]='130';$f[22]='15';$f[30]='Talonclaw: actual boarded owner after native exit - cast original198950';$edits+=5}
  if(($f -join "`t") -cne $applied[$i]){throw 'Unexpected Smart field changed'}
 };if($edits -ne 10){throw 'Expected exactly ten Smart fields including two comments'}
 $before=@((Signature $rows 'original-creature_template').Split("`n"));$after=@((Signature $rows 'first-creature_template').Split("`n"));$scriptIndex=[Array]::IndexOf($columns.creature_template,'ScriptName');if($before.Count -ne 2 -or $after.Count -ne 2 -or $scriptIndex -lt 0){throw 'Template context differs'}
 for($i=0;$i -lt 2;++$i){$f=$before[$i].Split("`t");if($f[$scriptIndex] -ne ''){throw 'Native template already scripted'};$f[$scriptIndex]=if($f[0] -eq '100965'){'npc_talonclaw_boarding_source'}else{'npc_talonclaw_boarded_ride'};if(($f -join "`t") -cne $after[$i]){throw 'Unexpected template field changed'}}
 if((Signature $rows 'original-spell_script_names') -ne '' -or (Signature $rows 'first-spell_script_names') -cne "198941`tspell_talonclaw_boarding_summon`n198950`tspell_talonclaw_boarded_transfer"){throw 'Unexpected spell registrations'}
 [IO.File]::WriteAllLines("$out/original.tsv",[string[]]$original);[IO.File]::WriteAllLines("$out/repaired.tsv",[string[]]$applied);[IO.File]::WriteAllLines("$out/sql-evidence.tsv",[string[]]$rows)
 $variants=[ordered]@{
 'credit-id'='UPDATE smart_scripts SET action_param1=1 WHERE id=0;';
 'credit-link'='UPDATE smart_scripts SET link=0 WHERE id=0;';
 'native-transfer'='UPDATE smart_scripts SET action_param1=1 WHERE id=1;';
 'native-cast-flags'='UPDATE smart_scripts SET action_param2=2 WHERE id=1;';
 'native-player-target'='UPDATE smart_scripts SET target_type=130 WHERE id=1;';
 'custom-smart-row'="INSERT INTO smart_scripts(entryorguid,source_type,id,event_type,action_type,target_type,comment) VALUES(100965,0,99,73,1,1,'custom');";
 'source-custom-ai'="UPDATE creature_template SET ScriptName='custom' WHERE entry=100965;";
 'ride-custom-ai'="UPDATE creature_template SET AIName='SmartAI' WHERE entry=100967;";
 'vehicle-seat'='UPDATE creature_template SET VehicleId=4536 WHERE entry=100967;';
 'spellclick'='UPDATE npc_spellclick_spells SET cast_flags=3;';
 'custom-condition'='UPDATE conditions SET ConditionValue1=39427 WHERE ConditionTypeOrReference=9;';
 'source-spawn'='UPDATE creature SET position_x=position_x+1;';
 'missing-source'='DELETE FROM creature;';
 'native-display'='UPDATE creature_template_wdb SET Displayid1=1 WHERE Entry=100967;';
 'native-destination'='UPDATE spell_target_position SET target_position_x=0 WHERE id=198950;';
 'quest-amount'='UPDATE quest_objectives SET Amount=2 WHERE ObjectID=100111;';
 'partial-binding'="UPDATE creature_template SET ScriptName='npc_talonclaw_boarded_ride' WHERE entry=100967;";
 'partial-registration'="INSERT INTO spell_script_names(spell_id,ScriptName) VALUES(198941,'spell_talonclaw_boarding_summon');";
 'partial-graph'='UPDATE smart_scripts SET link=0,event_type=38,event_param1=100111,event_param2=1 WHERE id=0;';
 'applied-source-identity'="UPDATE creature_template SET ScriptName='custom' WHERE entry=100965;";
 'applied-event'='UPDATE smart_scripts SET event_type=73 WHERE id=0;';
 'applied-triggered-cast'='UPDATE smart_scripts SET action_param2=2 WHERE id=1;';
 'applied-target'='UPDATE smart_scripts SET target_type=7 WHERE id=1;';
 'applied-range'='UPDATE smart_scripts SET target_param1=16 WHERE id=1;';
 'applied-registration'="DELETE FROM spell_script_names WHERE spell_id=198950;"
 }
 foreach($name in $variants.Keys){$base=$setup+$baseline+$(if($name -like 'applied-*'){$migration}else{''});$result=Query ($base+$variants[$name]+(Snapshot 'before')+$migration+(Snapshot 'after')) -Refusal;foreach($table in $filters.Keys){if((Signature $result "before-$table") -cne (Signature $result "after-$table")){throw "Customized/partial context changed $name/$table"}}}
 if($VerifyLiveData){$sha=[Security.Cryptography.SHA1]::Create();try{$hash=([BitConverter]::ToString($sha.ComputeHash([Text.Encoding]::UTF8.GetBytes($migration.Replace("`r`n","`n"))))).Replace('-','')}finally{$sha.Dispose()}
  $live=Query "SELECT COUNT(*) FROM updates WHERE name='2026_10_30_37_talonclaw_boarded_departure.sql' AND hash='$hash' AND state='RELEASED';";if($live -notcontains '1'){throw 'Live updater state/hash differs'}
  $live=Query (Snapshot 'live');foreach($table in $filters.Keys){if((Signature $live "live-$table") -cne (Signature $rows "first-$table")){throw "Live differs from exact regression context $table"}}
 }
 "PASS full9-table TEMP context: ten Smart fields/two ScriptNames/two registrations, original native summons/vehicle/click/21 conditions/destination/quest unchanged; repeat idempotence and$($variants.Count) atomic custom/partial refusals, optional live equality"
 }
}
