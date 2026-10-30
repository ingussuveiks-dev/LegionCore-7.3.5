param([string]$MigrationFile,[switch]$VerifyLiveData)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/fire-boss-health-data"
if(!$MigrationFile){$MigrationFile="$repo/sql/pending/world/2026_10_30_15_fire_boss_health.sql";if(!(Test-Path $MigrationFile)){$MigrationFile="$repo/sql/updates/world/2026_10_30_15_fire_boss_health.sql"}}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -Test {
 $migration=Get-Content $MigrationFile -Raw
 $c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $script:queryIndex=0
 function Query([string]$sql,[switch]$Refusal){
  $old=$env:MYSQL_PWD;$errorFile="$out/sql-$script:queryIndex.stderr";++$script:queryIndex
  [IO.File]::WriteAllText("$out/sql-$($script:queryIndex-1).sql",$sql+"`nSELECT '__FIRE_BOSS_HEALTH_SQL_COMPLETE__';`n")
  $previousConsole=[Console]::OutputEncoding;$previousPipe=$OutputEncoding
  try{$env:MYSQL_PWD=$c[3];$sqlFile="$out/sql-$($script:queryIndex-1).sql";$stdoutFile="$out/sql-$($script:queryIndex-1).stdout"
   # Preserve complete native stdout/stderr bytes, including the tail after an
   # expected --force error; a PowerShell native pipeline lost part of a row.
   $code=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -Arguments @("--host=$($c[0])","--port=$($c[1])","--user=$($c[2])","--database=$($c[4])",'--default-character-set=utf8mb4','--batch','--raw','--skip-column-names','--force') -InputFile $sqlFile -OutputFile $stdoutFile -ErrorFile $errorFile
   $rows=@(Get-Content $stdoutFile -Encoding UTF8);$errors=[string](Get-Content $errorFile -Raw -Encoding UTF8)
   if($Refusal){if($errors -notmatch 'Fire boss native health/sequence/loot differ; review before updating' -or @([regex]::Matches($errors,'ERROR [0-9]+')).Count -ne 1){throw "Unexpected Fire boss health refusal: $errorFile"}}
   elseif($code -ne 0 -or $errors){throw "Fire boss health SQL failed: $errorFile"};if(!$rows.Count -or $rows[-1] -cne '__FIRE_BOSS_HEALTH_SQL_COMPLETE__'){throw "Incomplete native SQL output: $stdoutFile"};return ,@($rows|Select-Object -SkipLast 1)
  }finally{$env:MYSQL_PWD=$old;[Console]::OutputEncoding=$previousConsole;$OutputEncoding=$previousPipe}
 }
 $fixture=Get-Content "$PSScriptRoot/FireBossHealthOriginal.json" -Raw -Encoding UTF8|ConvertFrom-Json
 $filters=[ordered]@{smart_scripts='(source_type IN(0,9) AND entryorguid IN(99615,98686,99763,103696,99861)) OR(source_type=1 AND entryorguid=247494)';conditions='(SourceTypeOrReferenceId=22 AND SourceEntry IN(99615,98686,99763,103696,99861,247494)) OR(SourceTypeOrReferenceId=4 AND SourceGroup=247494)';creature_text='CreatureID IN(99615,99763)';creature_template='entry IN(99615,98686,99763,103696,99861)';gameobject_template='entry=247494';gameobject_loot_template='Entry=247494'}
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
 for($i=0;$i -lt $original.Count;++$i){$f=$original[$i].Split("`t");if($f[0] -eq '99615' -and $f[1] -eq '0' -and [int]$f[2] -in @(15,16)){if($f[5] -ne '0' -or $f[14] -ne '11' -or $f[15] -ne '196327' -or $f[8] -ne '1'){throw 'Original native Phoenix threshold differs'};$f[5]='2';$f[16]='256';++$edits};if(($f -join "`t") -cne $repaired[$i]){throw "Unexpected row mutation $i"}}
 if($edits -ne 2){throw 'Expected exactly two health event rows'}
 [IO.File]::WriteAllLines("$out/original.tsv",[string[]]$original);[IO.File]::WriteAllLines("$out/repaired.tsv",[string[]]$repaired);[IO.File]::WriteAllLines("$out/conditions.tsv",[string[]]$conditions)
 [IO.File]::WriteAllLines("$out/texts.tsv",[string[]]@((Signature $rows 'first-creature_text').Split("`n")))
 [IO.File]::WriteAllLines("$out/sql-evidence.tsv",[string[]]$rows)
 $variants=[ordered]@{
 'threshold'='UPDATE smart_scripts SET event_param2=79 WHERE entryorguid=99615 AND source_type=0 AND id=15;';
 'spell'='UPDATE smart_scripts SET action_param1=1 WHERE entryorguid=99615 AND source_type=0 AND id=16;';
 'repeat'='UPDATE smart_scripts SET event_flags=0 WHERE entryorguid=99615 AND source_type=0 AND id=15;';
 'half-health'='UPDATE smart_scripts SET action_param1=1 WHERE entryorguid=99615 AND source_type=0 AND id=18;';
 'unknown-link'='UPDATE smart_scripts SET link=0 WHERE entryorguid=99615 AND source_type=0 AND id=21;';
 'death-credit'='UPDATE smart_scripts SET action_param2=1 WHERE entryorguid=99615 AND source_type=0 AND id=31;';
 'aethas-return'='UPDATE smart_scripts SET action_param1=1 WHERE entryorguid=99763 AND source_type=0 AND id=9;';
 'timed-list'='UPDATE smart_scripts SET action_param1=1 WHERE entryorguid=99615 AND source_type=9 AND id=0;';
 'custom-condition'="INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,ConditionTypeOrReference,ConditionValue1) VALUES(22,1,99615,9,1);";
 'custom-ai'="UPDATE creature_template SET ScriptName='custom' WHERE entry=99615;";
 'native-text'='UPDATE creature_text SET BroadcastTextID=1 WHERE CreatureID=99615 AND GroupID=4;';
 'weapon-link'='UPDATE gameobject_template SET Data1=0 WHERE entry=247494;';
 'weapon-loot'='UPDATE gameobject_loot_template SET Chance=1 WHERE Entry=247494;';
 'missing-row'='DELETE FROM smart_scripts WHERE entryorguid=99615 AND source_type=0 AND id=15;';
 'partial'='UPDATE smart_scripts SET event_type=2 WHERE entryorguid=99615 AND source_type=0 AND id=15;';
 'applied-retry'='UPDATE smart_scripts SET action_param2=0 WHERE entryorguid=99615 AND source_type=0 AND id=16;';
 'applied-spell'='UPDATE smart_scripts SET action_param1=1 WHERE entryorguid=99615 AND source_type=0 AND id=16;'
 }
 foreach($name in $variants.Keys){$base=$setup+$baseline+$(if($name -like 'applied-*'){$migration}else{''});$result=Query ($base+$variants[$name]+(Snapshot 'before')+$migration+(Snapshot 'after')) -Refusal
  foreach($table in $filters.Keys){if((Signature $result "before-$table") -cne (Signature $result "after-$table")){throw "Custom state changed $name/$table"}}
 }
 if($VerifyLiveData){$sha=[Security.Cryptography.SHA1]::Create();try{$hash=([BitConverter]::ToString($sha.ComputeHash([Text.Encoding]::UTF8.GetBytes($migration.Replace("`r`n","`n"))))).Replace('-','')}finally{$sha.Dispose()}
  $live=Query "SELECT COUNT(*) FROM updates WHERE name='2026_10_30_15_fire_boss_health.sql' AND hash='$hash' AND state='RELEASED';"
  if($live -notcontains '1'){throw 'Live Fire boss health wave migration/hash differs'}
  $expectedRows=$rows;$expectedTag='first'
  $fullLive=Query (Snapshot 'live')
  # The later focused intro repair intentionally adds two99763 predicates.
  # Validate its complete protected scope and released hash before excluding
  # only that separately checked actor's source22 rows from this older baseline.
  $intro=@($fullLive|Where-Object {$f=$_.Split("`t");$f[0] -ceq 'live-conditions' -and $f[1] -eq '22' -and $f[3] -eq '99763' -and $f[4] -eq '0'})
  if($intro.Count){& "$PSScriptRoot/Test-FireAethasIntroData.ps1" -VerifyLiveData
   $fullLive=@($fullLive|Where-Object {$f=$_.Split("`t");!($f[0] -ceq 'live-conditions' -and $f[1] -eq '22' -and $f[3] -eq '99763' -and $f[4] -eq '0')})
  }
  foreach($table in $filters.Keys){if((Signature $fullLive "live-$table") -cne (Signature $expectedRows "$expectedTag-$table")){throw "Live differs from Fire boss health regression $table"}}
 }
 'PASS Fire boss health full-schema TEMP: exactly2 event types0->2 and2 retry flags0->256, thresholds/once flags/native spell/other actor actions/LINKs/text/loot unchanged; idempotence/17 custom-partial refusals/live equality'
}
