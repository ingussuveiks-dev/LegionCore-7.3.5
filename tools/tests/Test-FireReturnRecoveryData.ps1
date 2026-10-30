param([string]$MigrationFile,[switch]$VerifyLiveData)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/fire-return-recovery-data"
if(!$MigrationFile){$MigrationFile="$repo/sql/pending/world/2026_10_30_17_fire_return_recovery.sql";if(!(Test-Path $MigrationFile)){$MigrationFile="$repo/sql/updates/world/2026_10_30_17_fire_return_recovery.sql"}}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -Test {
 $migration=Get-Content $MigrationFile -Raw
 $c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $script:queryIndex=0
 function Query([string]$sql,[switch]$Refusal){
  $old=$env:MYSQL_PWD;$errorFile="$out/sql-$script:queryIndex.stderr";++$script:queryIndex
  [IO.File]::WriteAllText("$out/sql-$($script:queryIndex-1).sql",$sql+"`nSELECT '__FIRE_RETURN_RECOVERY_SQL_COMPLETE__';`n")
  $previousConsole=[Console]::OutputEncoding;$previousPipe=$OutputEncoding
  try{$env:MYSQL_PWD=$c[3];$sqlFile="$out/sql-$($script:queryIndex-1).sql";$stdoutFile="$out/sql-$($script:queryIndex-1).stdout"
   # Preserve complete native stdout/stderr bytes, including the tail after an
   # expected --force error; a PowerShell native pipeline lost part of a row.
   $code=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -Arguments @("--host=$($c[0])","--port=$($c[1])","--user=$($c[2])","--database=$($c[4])",'--default-character-set=utf8mb4','--batch','--raw','--skip-column-names','--force') -InputFile $sqlFile -OutputFile $stdoutFile -ErrorFile $errorFile
   $rows=@(Get-Content $stdoutFile -Encoding UTF8);$errors=[string](Get-Content $errorFile -Raw -Encoding UTF8)
   if($Refusal){if($errors -notmatch 'Fire native return actor/scope/route differ; review before updating' -or @([regex]::Matches($errors,'ERROR [0-9]+')).Count -ne 1){throw "Unexpected Fire return recovery refusal: $errorFile"}}
   elseif($code -ne 0 -or $errors){throw "Fire return recovery SQL failed: $errorFile"}
   if(!$rows.Count -or $rows[-1] -cne '__FIRE_RETURN_RECOVERY_SQL_COMPLETE__'){throw "Incomplete native SQL output: $stdoutFile"}
   return ,@($rows|Select-Object -SkipLast 1)
  }finally{$env:MYSQL_PWD=$old;[Console]::OutputEncoding=$previousConsole;$OutputEncoding=$previousPipe}
 }
 $fixture=Get-Content "$PSScriptRoot/FireReturnRecoveryOriginal.json" -Raw -Encoding UTF8|ConvertFrom-Json
 $filters=[ordered]@{conditions='SourceTypeOrReferenceId=22 AND SourceEntry=99763 AND SourceId=0';smart_scripts='source_type=0 AND entryorguid IN(99615,99763)';creature_template='entry IN(99615,99763,99861)';waypoints='entry=99763';npc_spellclick_spells='npc_entry=99861';spell_target_position='id=196604'}
 $order=@{conditions='SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2';smart_scripts='entryorguid,source_type,id,link';creature_template='entry';waypoints='entry,pointid';npc_spellclick_spells='npc_entry,spell_id';spell_target_position='id'}
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
 foreach($table in $filters.Keys){if((Signature $rows "first-$table") -cne (Signature $rows "second-$table")){throw "Not idempotent $table"};if($table -ne 'creature_template' -and (Signature $rows "original-$table") -cne (Signature $rows "first-$table")){throw "Native return data changed $table"}}
 $original=@((Signature $rows 'original-creature_template').Split("`n"));$applied=@((Signature $rows 'first-creature_template').Split("`n"));$column=@($fixture.creature_template[0].PSObject.Properties.Name).IndexOf('ScriptName');if($column -lt 0 -or $original.Count -ne 3 -or $applied.Count -ne 3){throw 'Missing native actor templates/script column'}
 $changes=0;for($i=0;$i -lt 3;++$i){$f=$original[$i].Split("`t");if($f[0] -eq '99763'){if($f[$column] -cne ''){throw 'Original Aethas already has a custom script'};$f[$column]='npc_fire_artifact_aethas';++$changes};if(($f -join "`t") -cne $applied[$i]){throw 'Unexpected template mutation'}};if($changes -ne 1){throw 'Expected exactly one script binding'}
 [IO.File]::WriteAllLines("$out/actors.tsv",[string[]]((Signature $rows 'first-smart_scripts').Split("`n")))
 [IO.File]::WriteAllLines("$out/conditions.tsv",[string[]]((Signature $rows 'first-conditions').Split("`n")))
 [IO.File]::WriteAllLines("$out/path.tsv",[string[]]((Signature $rows 'first-waypoints').Split("`n")))
 [IO.File]::WriteAllLines("$out/sql-evidence.tsv",[string[]]$rows)
 $variants=[ordered]@{
 'custom-ai'="UPDATE creature_template SET ScriptName='custom' WHERE entry=99763;";
 'source-ai'="UPDATE creature_template SET AIName='custom' WHERE entry=99763;";
 'boss-ai'="UPDATE creature_template SET ScriptName='custom' WHERE entry=99615;";
 'portal-ai'="UPDATE creature_template SET ScriptName='custom' WHERE entry=99861;";
 'native-summon'='UPDATE smart_scripts SET action_param1=1 WHERE entryorguid=99615 AND id=23;';
 'native-pose'='UPDATE smart_scripts SET target_z=0 WHERE entryorguid=99615 AND id=23;';
 'unknown-link'='UPDATE smart_scripts SET link=0 WHERE entryorguid=99615 AND id=31;';
 'intro-target'='UPDATE conditions SET ConditionTarget=0;';
 'intro-stage'='UPDATE conditions SET ConditionValue2=4 WHERE ConditionTypeOrReference=44;';
 'cleanup'='UPDATE smart_scripts SET action_type=1 WHERE entryorguid=99763 AND id=4;';
 'native-path'='UPDATE smart_scripts SET action_param2=1 WHERE entryorguid=99763 AND id=5;';
 'native-portal'='UPDATE smart_scripts SET action_param1=1 WHERE entryorguid=99763 AND id=9;';
 'native-conversation'='UPDATE smart_scripts SET action_param1=1 WHERE entryorguid=99763 AND id=10;';
 'waypoint'='UPDATE waypoints SET position_z=0;';
 'native-click'='UPDATE npc_spellclick_spells SET spell_id=1;';
 'native-destination'='UPDATE spell_target_position SET target_position_z=0;';
 'missing-actor'='DELETE FROM creature_template WHERE entry=99763;';
 'applied-flags'='UPDATE creature_template SET unit_flags=0 WHERE entry=99763;';
 'applied-extra-condition'="INSERT INTO conditions VALUES(22,5,99763,0,0,9,0,11997,0,0,0,0,'','custom');"
 }
 foreach($name in $variants.Keys){$base=$setup+$baseline+$(if($name -like 'applied-*'){$migration}else{''});$result=Query ($base+$variants[$name]+(Snapshot 'before')+$migration+(Snapshot 'after')) -Refusal
  foreach($table in $filters.Keys){if((Signature $result "before-$table") -cne (Signature $result "after-$table")){throw "Custom return data mutated $name/$table"}}
 }
 if($VerifyLiveData){$sha=[Security.Cryptography.SHA1]::Create();try{$hash=([BitConverter]::ToString($sha.ComputeHash([Text.Encoding]::UTF8.GetBytes($migration.Replace("`r`n","`n"))))).Replace('-','')}finally{$sha.Dispose()}
  $live=Query "SELECT COUNT(*) FROM updates WHERE name='2026_10_30_17_fire_return_recovery.sql' AND hash='$hash' AND state='RELEASED';";if($live -notcontains '1'){throw 'Live Fire return migration/hash differs'}
  $live=Query (Snapshot 'live');foreach($table in $filters.Keys){if((Signature $live "live-$table") -cne (Signature $rows "first-$table")){throw "Live native Fire return differs $table"}}
 }
 'PASS full6table Fire return: exactly99763 ScriptName binding, native boss/actor SmartAI/events/intro scope/waypoint/portal/click/destination unchanged, idempotence/19 custom-partial refusals/live equality; binary/native lifetime recovery verified separately'
}
