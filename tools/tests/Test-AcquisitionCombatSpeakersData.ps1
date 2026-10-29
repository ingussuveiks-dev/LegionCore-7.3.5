param([string]$MigrationFile,[switch]$VerifyLiveData)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/acquisition-combat-speakers-data"
if(!$MigrationFile){$MigrationFile="$repo/sql/pending/world/2026_10_30_03_acquisition_combat_speakers.sql";if(!(Test-Path $MigrationFile)){$MigrationFile="$repo/sql/updates/world/2026_10_30_03_acquisition_combat_speakers.sql"}}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -Test {
 $migration=Get-Content $MigrationFile -Raw
 $c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $script:queryIndex=0
 function Query([string]$sql,[switch]$Refusal){
  $old=$env:MYSQL_PWD;$errorFile="$out/sql-$script:queryIndex.stderr";++$script:queryIndex
  [IO.File]::WriteAllText("$out/sql-$($script:queryIndex-1).sql",$sql)
  $previousConsole=[Console]::OutputEncoding;$previousPipe=$OutputEncoding
  try{$env:MYSQL_PWD=$c[3];$sqlFile="$out/sql-$($script:queryIndex-1).sql";$stdoutFile="$out/sql-$($script:queryIndex-1).stdout"
   # Preserve complete native stdout/stderr bytes, including the tail after an
   # expected --force error; a PowerShell native pipeline lost part of a row.
   $process=Start-Process -FilePath C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -ArgumentList @("--host=$($c[0])","--port=$($c[1])","--user=$($c[2])","--database=$($c[4])",'--default-character-set=utf8mb4','--batch','--raw','--skip-column-names','--force') -RedirectStandardInput $sqlFile -RedirectStandardOutput $stdoutFile -RedirectStandardError $errorFile -WindowStyle Hidden -Wait -PassThru
   try{$process.Refresh();$code=$process.ExitCode}finally{$process.Dispose()}
   $rows=@(Get-Content $stdoutFile -Encoding UTF8);$errors=[string](Get-Content $errorFile -Raw -Encoding UTF8)
   if($Refusal){if($errors -notmatch 'Acquisition combat speech/sequence/loot differ; review before updating' -or @([regex]::Matches($errors,'ERROR [0-9]+')).Count -ne 1){throw "Unexpected Acquisition combat speaker refusal: $errorFile"}}
   elseif($code -ne 0 -or $errors){throw "Acquisition combat speaker SQL failed: $errorFile"};return ,$rows
  }finally{$env:MYSQL_PWD=$old;[Console]::OutputEncoding=$previousConsole;$OutputEncoding=$previousPipe}
 }
 $fixture=Get-Content "$PSScriptRoot/AcquisitionCombatSpeakersOriginal.json" -Raw -Encoding UTF8|ConvertFrom-Json
 $filters=[ordered]@{smart_scripts='(source_type=0 AND entryorguid IN(94836,109978,110533,110556,110548)) OR(source_type=9 AND entryorguid=94836) OR(source_type=1 AND entryorguid IN(251878,252054))';conditions='SourceTypeOrReferenceId=22 AND SourceEntry IN(94836,109978,110533,110556,110548,251878,252054)';creature_text='CreatureID IN(94836,109978,110533,110556,110548)';creature_template='entry IN(94836,109978,110533,110556,110548)';gameobject_template='entry IN(251878,252054)';gameobject_loot_template='Entry IN(251878,252054)'}
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
 for($i=0;$i -lt $original.Count;++$i){$f=$original[$i].Split("`t");if($f[1] -eq '0' -and (($f[0] -eq '94836' -and [int]$f[2] -in @(4,5,10)) -or($f[0] -eq '110556' -and [int]$f[2] -in @(5,6,7)))){if($f[21] -ne '2'){throw 'Original combat speaker differs'};$f[21]='1';++$edits};if(($f -join "`t") -cne $repaired[$i]){throw "Unexpected row mutation $i"}}
 if($edits -ne 6){throw 'Expected exactly six speaker fields'}
 [IO.File]::WriteAllLines("$out/original.tsv",[string[]]$original);[IO.File]::WriteAllLines("$out/repaired.tsv",[string[]]$repaired);[IO.File]::WriteAllLines("$out/conditions.tsv",[string[]]$conditions)
 [IO.File]::WriteAllLines("$out/texts.tsv",[string[]]@((Signature $rows 'first-creature_text').Split("`n")))
 [IO.File]::WriteAllLines("$out/sql-evidence.tsv",[string[]]$rows)
 $variants=[ordered]@{
 'varedis-threshold'='UPDATE smart_scripts SET event_param2=1 WHERE entryorguid=94836 AND source_type=0 AND id=4;';
 'varedis-text'='UPDATE smart_scripts SET action_param1=0 WHERE entryorguid=94836 AND source_type=0 AND id=10;';
 'varedis-agro-link'='UPDATE smart_scripts SET link=0 WHERE entryorguid=94836 AND source_type=0 AND id=1;';
 'varedis-real-credit'='UPDATE smart_scripts SET action_param2=1 WHERE entryorguid=94836 AND source_type=0 AND id=6;';
 'varedis-death-npc'='UPDATE smart_scripts SET action_param1=1 WHERE entryorguid=94836 AND source_type=0 AND id=8;';
 'varedis-death-chest'='UPDATE smart_scripts SET action_param1=1 WHERE entryorguid=94836 AND source_type=0 AND id=9;';
 'varedis-timed'='UPDATE smart_scripts SET event_param1=1 WHERE entryorguid=94836 AND source_type=9 AND id=1;';
 'allaris-intro'='UPDATE smart_scripts SET target_type=1 WHERE entryorguid=110556 AND id=0;';
 'allaris-text-over'='UPDATE smart_scripts SET event_param2=1 WHERE entryorguid=110556 AND id=1;';
 'allaris-hostility'='UPDATE smart_scripts SET action_param1=35 WHERE entryorguid=110556 AND id=2;';
 'allaris-combat-spell'='UPDATE smart_scripts SET target_type=1 WHERE entryorguid=110556 AND id=4;';
 'allaris-death-text'='UPDATE smart_scripts SET action_param1=0 WHERE entryorguid=110556 AND id=7;';
 'guldan-native-credit'='UPDATE smart_scripts SET action_param1=1 WHERE entryorguid=110533 AND id=7;';
 'guldan-summon'='UPDATE smart_scripts SET action_param1=1 WHERE entryorguid=110533 AND id=10;';
 'custom-condition'="INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,ConditionTypeOrReference,ConditionValue1) VALUES(22,6,110556,9,1);";
 'custom-ai'="UPDATE creature_template SET ScriptName='custom' WHERE entry=94836;";
 'native-text'='UPDATE creature_text SET BroadcastTextID=1 WHERE CreatureID=110556 AND GroupID=0;';
 'weapon-link'='UPDATE gameobject_template SET Data1=0 WHERE entry=252054;';
 'weapon-loot'='UPDATE gameobject_loot_template SET Chance=1 WHERE Entry=251878;';
 'missing-row'='DELETE FROM smart_scripts WHERE entryorguid=94836 AND source_type=0 AND id=4;';
 'applied-speaker'='UPDATE smart_scripts SET target_type=2 WHERE entryorguid=110556 AND id=7;'
 }
 foreach($name in $variants.Keys){$base=$setup+$baseline+$(if($name -like 'applied-*'){$migration}else{''});$result=Query ($base+$variants[$name]+(Snapshot 'before')+$migration+(Snapshot 'after')) -Refusal
  foreach($table in $filters.Keys){if((Signature $result "before-$table") -cne (Signature $result "after-$table")){throw "Custom state changed $name/$table"}}
 }
 if($VerifyLiveData){$sha=[Security.Cryptography.SHA1]::Create();try{$hash=([BitConverter]::ToString($sha.ComputeHash([Text.Encoding]::UTF8.GetBytes($migration.Replace("`r`n","`n"))))).Replace('-','')}finally{$sha.Dispose()}
  $live=Query "SELECT COUNT(*) FROM updates WHERE name='2026_10_30_03_acquisition_combat_speakers.sql' AND hash='$hash' AND state='RELEASED';"
  if($live -notcontains '1'){throw 'Live Acquisition combat speaker wave migration/hash differs'}
  # A later focused pickup migration legitimately changes the related chest.
  # Build the complete expected context by replaying that exact released SQL;
  # retain the original speaker baseline and all other field comparisons.
  $expectedRows=$rows;$expectedTag='first';$followup="$repo/sql/updates/world/2026_10_30_04_twinblades_real_loot.sql"
  if(Test-Path $followup){$later=Get-Content $followup -Raw;$sha=[Security.Cryptography.SHA1]::Create();try{$hash=([BitConverter]::ToString($sha.ComputeHash([Text.Encoding]::UTF8.GetBytes($later.Replace("`r`n","`n"))))).Replace('-','')}finally{$sha.Dispose()}
   $applied=Query "SELECT COUNT(*) FROM updates WHERE name='2026_10_30_04_twinblades_real_loot.sql' AND hash='$hash' AND state='RELEASED';"
   if($applied -contains '1'){$expectedRows=Query ($setup+$baseline+$migration+$later+(Snapshot 'known-followup'));$expectedTag='known-followup'}
  }
  $fullLive=Query (Snapshot 'live');foreach($table in $filters.Keys){if((Signature $fullLive "live-$table") -cne (Signature $expectedRows "$expectedTag-$table")){throw "Live differs from Acquisition combat speaker regression $table"}}
 }
 'PASS full-schema TEMP Varedis/Allaris combat speakers: exactly six TALK targets, unchanged intro/timers/combat/credit/loot/knownGO link; repeat/live equality and twenty-one custom refusals'
}
