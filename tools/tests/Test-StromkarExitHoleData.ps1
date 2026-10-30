#requires -Version 7.0
param([string]$OutputDirectory,[switch]$VerifyLiveData)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/stromkar-exit-hole-data/run-$([DateTime]::UtcNow.ToString('yyyyMMdd-HHmmss-fffffff'))"};$out=$OutputDirectory
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/data-diagnostics" -Test {
 & "$PSScriptRoot/Invoke-WithTemporarySqlReader.ps1" -EvidenceDirectory "$out/reader" -Test {
  $context=Get-Content "$PSScriptRoot/StromkarExitHoleContext.json" -Raw|ConvertFrom-Json -AsHashtable
  $fixture=Get-Content "$PSScriptRoot/StromkarExitHoleOriginal.json" -Raw|ConvertFrom-Json -AsHashtable
  $path="$repo/sql/updates/world/2026_10_30_34_stromkar_exit_hole.sql";if(!(Test-Path $path)){$path="$repo/sql/pending/world/2026_10_30_34_stromkar_exit_hole.sql"};$migration=Get-Content $path -Raw
  function Value($v){if($null -eq $v){return 'NULL'};if($v -is [string]){if(!$v.Length){return "''"};return 'CONVERT(0x'+[Convert]::ToHexString([Text.Encoding]::UTF8.GetBytes($v))+' USING utf8mb4)'};[Convert]::ToString($v,[Globalization.CultureInfo]::InvariantCulture)}
  $setup='';$baseline='';foreach($table in $context.filters.Keys){
   $setup+="CREATE TEMPORARY TABLE saved_$table LIKE $table;INSERT INTO saved_$table SELECT * FROM $table WHERE $($context.filters[$table]);CREATE TEMPORARY TABLE $table LIKE saved_$table;`n"
   $baseline+="DELETE FROM $table;`n";$cols=($context.columns[$table]|ForEach-Object{"``$_``"}) -join ','
   foreach($row in $fixture[$table]){$values=($context.columns[$table]|ForEach-Object{Value $row[$_]}) -join ',';$baseline+="INSERT INTO $table($cols) VALUES($values);`n"}
  }
  function Snapshot($tag){$sql='';foreach($table in $context.filters.Keys){$order=($context.columns[$table]|ForEach-Object{"``$_``"}) -join ',';$sql+="SELECT '$tag-$table',$table.* FROM $table WHERE $($context.filters[$table]) ORDER BY $order;`n"};$sql}
  function AddonShape($tag){"SELECT '$tag',0,entry,path_id,mount,bytes1,bytes2,emote,auras FROM creature_template_addon ORDER BY entry;SELECT '$tag',1,guid,path_id,mount,bytes1,bytes2,emote,auras FROM creature_addon ORDER BY guid;"}
  $shape="SELECT 'events',entryorguid,source_type,id,link,event_type,event_phase_mask,event_chance,event_flags,event_param1,event_param2,event_param3,event_param4,action_type,action_param1,action_param2,target_type,target_param1,target_param2 FROM smart_scripts WHERE (entryorguid=103151 AND source_type=0 AND id IN(0,1)) OR (entryorguid=103226 AND source_type=0 AND id IN(0,4,5,6)) ORDER BY entryorguid,source_type,id;SELECT 'spawns',guid,id FROM creature ORDER BY guid;"
  $sql=$setup+$baseline+(Snapshot original)+(AddonShape baseline-addons)+$migration+(Snapshot first)+(AddonShape addons)+$shape+$migration+(Snapshot second)
  $variants=@(
   "INSERT INTO creature_addon SELECT 369686,path_id,mount,bytes1,bytes2,emote,'202925' FROM creature_template_addon WHERE entry=103151;"
   "INSERT INTO creature_addon SELECT 369686,path_id,mount,bytes1,bytes2,emote+1,'204345' FROM creature_template_addon WHERE entry=103151;"
   "UPDATE creature_template_addon SET auras='204345' WHERE entry=103151;"
   'UPDATE creature_template_addon SET bytes1=1 WHERE entry=103151;'
   'UPDATE creature_addon SET bytes1=1 WHERE guid=369687;'
   'UPDATE creature SET id=103152 WHERE guid=369686;'
   'UPDATE creature SET map=1 WHERE guid=369686;'
   "UPDATE creature SET PhaseId='6140' WHERE guid=369686;"
   'UPDATE creature SET position_z=76 WHERE guid=369686;'
   'UPDATE smart_scripts SET link=0 WHERE entryorguid=103151 AND source_type=0 AND id=1;'
   'UPDATE smart_scripts SET action_param1=202925 WHERE entryorguid=103151 AND source_type=0 AND id=1;'
   'UPDATE smart_scripts SET target_param1=370945 WHERE entryorguid=103226 AND source_type=0 AND id=4;'
   'UPDATE smart_scripts SET action_param2=49053 WHERE entryorguid=103226 AND source_type=0 AND id=0;'
   'UPDATE smart_scripts SET target_param1=136768 WHERE entryorguid=103226 AND source_type=0 AND id=5;'
   'UPDATE smart_scripts SET event_flags=0 WHERE entryorguid=103151 AND source_type=0 AND id=0;'
   'UPDATE conditions SET ConditionValue2=4 WHERE SourceTypeOrReferenceId=22 AND SourceEntry=101461;'
   "UPDATE creature_template SET AIName='' WHERE entry=103151;"
   'UPDATE creature_template_scaling SET LevelScalingMax=0 WHERE Entry=103226;'
   'UPDATE gameobject SET state=0 WHERE guid=136767;'
   "UPDATE creature_template_addon SET auras='' WHERE entry=101430;"
  )
  for($i=0;$i -lt $variants.Count;++$i){$sql+=$baseline+$variants[$i]+(Snapshot "refuse-$i-before")+$migration+(Snapshot "refuse-$i-after")}
  if($VerifyLiveData){foreach($table in $context.filters.Keys){$sql+="DELETE FROM $table;INSERT INTO $table SELECT * FROM saved_$table;`n"};$sql+=(Snapshot live);$hash=[Convert]::ToHexString([Security.Cryptography.SHA1]::HashData([Text.Encoding]::UTF8.GetBytes([IO.File]::ReadAllText($path).Replace("`r`n","`n"))));$sql+="SELECT 'release',hash,state FROM updates WHERE name='2026_10_30_34_stromkar_exit_hole.sql';"}
  $sql+="SELECT '__EXIT_DATA_COMPLETE__';";[IO.File]::WriteAllText("$out/replay.sql",$sql)
  $c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object{$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';');$prior=$env:MYSQL_PWD
  try{$env:MYSQL_PWD=$env:CODEX_SQL_READER_PASSWORD;$code=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -Arguments @("--host=$($c[0])","--port=$($c[1])","--user=$env:CODEX_SQL_READER_USER","--database=$($c[4])",'--batch','--raw','--skip-column-names','--force','--default-character-set=utf8mb4') -InputFile "$out/replay.sql" -OutputFile "$out/replay.tsv" -ErrorFile "$out/replay.stderr"}finally{$env:MYSQL_PWD=$prior}
  [IO.File]::WriteAllText("$out/replay.exit.txt",[string]$code)
  $errors=[IO.File]::ReadAllLines("$out/replay.stderr");if($code -notin 0,1 -or @($errors|Where-Object{$_ -match '^ERROR 1644 .*Stromkar exit-hole/context differ'}).Count -ne $variants.Count -or @($errors|Where-Object{$_ -match '^ERROR ' -and $_ -notmatch '^ERROR 1644 .*Stromkar exit-hole/context differ'}).Count){throw 'Unexpected native SQL/refusal result; full evidence retained'}
  $rows=[IO.File]::ReadAllLines("$out/replay.tsv");if($rows[-1] -cne '__EXIT_DATA_COMPLETE__'){throw 'Incomplete native TEMP replay output'}
  $groups=@{};foreach($line in $rows){$tab=$line.IndexOf("`t");if($tab -lt 1){continue};$tag=$line.Substring(0,$tab);if(!$groups.ContainsKey($tag)){$groups[$tag]=[Collections.Generic.List[string]]::new()};$groups[$tag].Add($line.Substring($tab+1))}
  function Signature($tag){if(!$groups.ContainsKey($tag)){return ''};[string]::Join("`n",$groups[$tag])}
  foreach($table in $context.filters.Keys){if((Signature "first-$table") -cne (Signature "second-$table")){throw "Non-idempotent $table"};if($table -ne 'creature_addon' -and (Signature "original-$table") -cne (Signature "first-$table")){throw "Changed native $table"};for($i=0;$i -lt $variants.Count;++$i){if((Signature "refuse-$i-before-$table") -cne (Signature "refuse-$i-after-$table")){throw "Refused $table/$i changed"}};if($VerifyLiveData -and (Signature "first-$table") -cne (Signature "live-$table")){throw "Actual live $table differs"}}
  $before=@($groups['original-creature_addon']);$after=@($groups['first-creature_addon']);if($before.Count -ne 6 -or $after.Count -ne 7 -or @($after|Where-Object{$_ -ceq "369686`t0`t0`t0`t0`t0`t204345"}).Count -ne 1 -or (($after|Where-Object{$_ -notlike "369686`t*"}) -join "`n") -cne ($before -join "`n")){throw 'Expected only exact per-spawn inherited-addon aura override'}
  foreach($tag in @('events','baseline-addons','addons','spawns')){[IO.File]::WriteAllLines("$out/$tag.tsv",$groups[$tag])}
  if($VerifyLiveData -and (Signature release) -cne "$hash`tRELEASED"){throw 'Actual updater LF hash/state differs'}
  "PASS all12 full context tables, exactly1 inherited spawn-addon aura override, repeat idempotence and$($variants.Count) customized/partial refusals; native scripts/unknown LINK2, shared actors/auras/doors/credit/scaling unchanged. Persistent DML denied."
 }
}
