#requires -Version 7.0
param([string]$OutputDirectory,[switch]$VerifyLiveData)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/wardstone-linked-cast-data/run-$([DateTime]::UtcNow.ToString('yyyyMMdd-HHmmss-fffffff'))"};$out=$OutputDirectory
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/data-diagnostics" -Test {
 & "$PSScriptRoot/Invoke-WithTemporarySqlReader.ps1" -EvidenceDirectory "$out/reader" -Test {
  $context=Get-Content "$PSScriptRoot/WardstoneLinkedCastContext.json" -Raw|ConvertFrom-Json -AsHashtable;$fixture=Get-Content "$PSScriptRoot/WardstoneLinkedCastOriginal.json" -Raw|ConvertFrom-Json -AsHashtable
  $path="$repo/sql/updates/world/2026_10_30_35_talonclaw_linked_cast_retry.sql";if(!(Test-Path $path)){$path="$repo/sql/pending/world/2026_10_30_35_talonclaw_linked_cast_retry.sql"};$migration=Get-Content $path -Raw
  function Value($v){if($null -eq $v){return 'NULL'};if($v -is [string]){if(!$v.Length){return "''"};return 'CONVERT(0x'+[Convert]::ToHexString([Text.Encoding]::UTF8.GetBytes($v))+' USING utf8mb4)'};[Convert]::ToString($v,[Globalization.CultureInfo]::InvariantCulture)}
  $setup='';$baseline='';foreach($table in $context.filters.Keys){$setup+="CREATE TEMPORARY TABLE saved_$table LIKE $table;INSERT INTO saved_$table SELECT * FROM $table WHERE $($context.filters[$table]);CREATE TEMPORARY TABLE $table LIKE saved_$table;INSERT INTO $table SELECT * FROM saved_$table;`n";$baseline+="DELETE FROM $table;`n";$cols=($context.columns[$table]|ForEach-Object{"``$_``"}) -join ',';foreach($row in $fixture[$table]){$values=($context.columns[$table]|ForEach-Object{Value $row[$_]}) -join ',';$baseline+="INSERT INTO $table($cols) VALUES($values);`n"}}
  function Snapshot($tag){$sql='';foreach($table in $context.filters.Keys){$order=($context.columns[$table]|ForEach-Object{"``$_``"}) -join ',';$sql+="SELECT '$tag-$table',$table.* FROM $table WHERE $($context.filters[$table]) ORDER BY $order;`n"};$sql}
  function Shape($tag){"SELECT '$tag',entryorguid,source_type,id,link,event_type,event_phase_mask,event_chance,event_flags,event_param1,event_param2,event_param3,event_param4,action_type,action_param1,action_param2,target_type,target_param1,target_param2 FROM smart_scripts WHERE entryorguid=101325 AND source_type=0 ORDER BY id;"}
  $sql=$setup+$baseline+(Snapshot original)+(Shape original-events)+$migration+(Snapshot first)+(Shape events)+$migration+(Snapshot second)
  $variants=@(
   'UPDATE smart_scripts SET action_param2=512 WHERE entryorguid=101325 AND source_type=0 AND id=1;'
   'UPDATE smart_scripts SET action_param2=2 WHERE entryorguid=101325 AND source_type=0 AND id=1;'
   'UPDATE smart_scripts SET action_param1=199823 WHERE entryorguid=101325 AND source_type=0 AND id=1;'
   'UPDATE smart_scripts SET target_type=7 WHERE entryorguid=101325 AND source_type=0 AND id=1;'
   'UPDATE smart_scripts SET event_type=60 WHERE entryorguid=101325 AND source_type=0 AND id=1;'
   'UPDATE smart_scripts SET event_flags=0 WHERE entryorguid=101325 AND source_type=0 AND id=0;'
   'UPDATE smart_scripts SET action_param2=48173 WHERE entryorguid=101325 AND source_type=0 AND id=0;'
   'UPDATE smart_scripts SET link=0 WHERE entryorguid=101325 AND source_type=0 AND id=3;'
   'UPDATE smart_scripts SET action_param1=101361 WHERE entryorguid=101325 AND source_type=0 AND id=3;'
   'UPDATE smart_scripts SET target_z=1 WHERE entryorguid=101325 AND source_type=0 AND id=3;'
   'UPDATE conditions SET ConditionValue2=2 WHERE SourceTypeOrReferenceId=22 AND SourceEntry=101325 AND ConditionTypeOrReference=44 AND ConditionValue2=3;'
   'UPDATE conditions SET ConditionValue1=1 WHERE SourceTypeOrReferenceId=18 AND SourceGroup=101325 AND ConditionTypeOrReference=15;'
   'UPDATE conditions SET ConditionValue2=1 WHERE SourceTypeOrReferenceId=13 AND SourceEntry=199823;'
   "UPDATE creature_template SET ScriptName='custom' WHERE entry=101325;"
   'UPDATE creature SET PhaseId=1 WHERE id=101325;'
   'UPDATE creature_template_scaling SET LevelScalingMax=1 WHERE Entry=101325;'
   'UPDATE npc_spellclick_spells SET cast_flags=0 WHERE npc_entry=101325;'
   'UPDATE smart_scripts SET target_type=2 WHERE entryorguid=100207 AND source_type=0 AND id=4;'
   'UPDATE smart_scripts SET action_param1=1 WHERE entryorguid=100207 AND source_type=0 AND id=6;'
   "UPDATE creature_template_wdb SET Name1='custom' WHERE Entry=101360;"
  )
  for($i=0;$i -lt $variants.Count;++$i){$sql+=$baseline+$variants[$i]+(Snapshot "refuse-$i-before")+$migration+(Snapshot "refuse-$i-after")}
  if($VerifyLiveData){foreach($table in $context.filters.Keys){$sql+="DELETE FROM $table;INSERT INTO $table SELECT * FROM saved_$table;`n"};$sql+=(Snapshot live);$hash=[Convert]::ToHexString([Security.Cryptography.SHA1]::HashData([Text.Encoding]::UTF8.GetBytes([IO.File]::ReadAllText($path).Replace("`r`n","`n"))));$sql+="SELECT 'release',hash,state FROM updates WHERE name='2026_10_30_35_talonclaw_linked_cast_retry.sql';"}
  $sql+="SELECT '__WARD_RETRY_COMPLETE__';";[IO.File]::WriteAllText("$out/replay.sql",$sql)
  $c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object{$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';');$prior=$env:MYSQL_PWD
  try{$env:MYSQL_PWD=$env:CODEX_SQL_READER_PASSWORD;$code=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -Arguments @("--host=$($c[0])","--port=$($c[1])","--user=$env:CODEX_SQL_READER_USER","--database=$($c[4])",'--batch','--raw','--skip-column-names','--force','--default-character-set=utf8mb4') -InputFile "$out/replay.sql" -OutputFile "$out/replay.tsv" -ErrorFile "$out/replay.stderr"}finally{$env:MYSQL_PWD=$prior}
  [IO.File]::WriteAllText("$out/replay.exit.txt",[string]$code);$errors=[IO.File]::ReadAllLines("$out/replay.stderr");if($code -notin 0,1 -or @($errors|Where-Object{$_ -match '^ERROR 1644 .*Wardstone linked-cast/context differ'}).Count -ne $variants.Count -or @($errors|Where-Object{$_ -match '^ERROR ' -and $_ -notmatch '^ERROR 1644 .*Wardstone linked-cast/context differ'}).Count){throw 'Unexpected native SQL/refusal result; full evidence retained'}
  $rows=[IO.File]::ReadAllLines("$out/replay.tsv");if($rows[-1] -cne '__WARD_RETRY_COMPLETE__'){throw 'Incomplete native TEMP replay output'};$groups=@{};foreach($line in $rows){$tab=$line.IndexOf("`t");if($tab -lt 1){continue};$tag=$line.Substring(0,$tab);if(!$groups.ContainsKey($tag)){$groups[$tag]=[Collections.Generic.List[string]]::new()};$groups[$tag].Add($line.Substring($tab+1))}
  function Signature($tag){if(!$groups.ContainsKey($tag)){return ''};[string]::Join("`n",$groups[$tag])}
  foreach($table in $context.filters.Keys){if((Signature "first-$table") -cne (Signature "second-$table")){throw "Non-idempotent $table"};if($table -ne 'smart_scripts' -and (Signature "original-$table") -cne (Signature "first-$table")){throw "Changed native $table"};for($i=0;$i -lt $variants.Count;++$i){if((Signature "refuse-$i-before-$table") -cne (Signature "refuse-$i-after-$table")){throw "Refused $table/$i changed"}};if($VerifyLiveData -and (Signature "first-$table") -cne (Signature "live-$table")){throw "Actual live $table differs"}}
  $old=@($groups['original-smart_scripts']);$new=@($groups['first-smart_scripts']);if($old.Count -ne 11 -or $new.Count -ne 11){throw 'Native Smart row count differs'};$edits=0;for($i=0;$i -lt $old.Count;++$i){$f=$old[$i].Split("`t");if($f[0] -eq '101325' -and $f[1] -eq '0' -and $f[2] -in @('1','2')){if($f[16] -ne '0'){throw 'Original cast flags differ'};$f[16]='512';++$edits};if(($f -join "`t") -cne $new[$i]){throw 'Unrelated native Smart field changed'}};if($edits -ne 2){throw 'Expected two linked cast opt-ins'}
  foreach($tag in @('original-events','events')){[IO.File]::WriteAllLines("$out/$tag.tsv",$groups[$tag])};if($VerifyLiveData -and (Signature release) -cne "$hash`tRELEASED"){throw 'Actual updater LF hash/state differs'}
  "PASS all11 context tables, exactly2 linked-cast opt-ins, repeat idempotence and20 customized/partial refusals; native admission/credit/unknownLINK4/MistWarder/scaling unchanged. Persistent DML denied."
 }
}
