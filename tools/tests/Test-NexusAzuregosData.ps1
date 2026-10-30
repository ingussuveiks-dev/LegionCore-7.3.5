#requires -Version 7.0
param([string]$OutputDirectory,[switch]$VerifyLiveData,[switch]$WithPriestHealing)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/nexus-azuregos-data/run-$([DateTime]::UtcNow.ToString('yyyyMMdd-HHmmss-fffffff'))"};$out=$OutputDirectory
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/data-diagnostics" -Test {
 & "$PSScriptRoot/Invoke-WithTemporarySqlReader.ps1" -EvidenceDirectory "$out/sql-reader" -Test {
  $context=Get-Content "$PSScriptRoot/NexusAzuregosContext.json" -Raw|ConvertFrom-Json -AsHashtable
  $fixture=Get-Content "$PSScriptRoot/NexusAzuregosOriginal.json" -Raw|ConvertFrom-Json -AsHashtable
  $path="$repo/sql/pending/world/2026_10_30_32_priest_azuregos_receiver.sql";if(!(Test-Path $path)){$path="$repo/sql/updates/world/2026_10_30_32_priest_azuregos_receiver.sql"};$migration=Get-Content $path -Raw
  $preMigration='';$healingPath="$repo/sql/updates/world/2026_10_30_33_priest_azuregos_healing.sql"
  if($VerifyLiveData -and (Test-Path $healingPath)){$WithPriestHealing=$true}
  if($WithPriestHealing){if(!(Test-Path $healingPath)){$healingPath="$repo/sql/pending/world/2026_10_30_33_priest_azuregos_healing.sql"};$preMigration=$migration;$migration=Get-Content $healingPath -Raw}
  function Value($v){if($null -eq $v){return 'NULL'};if($v -is [string]){if(!$v.Length){return "''"};return 'CONVERT(0x'+[Convert]::ToHexString([Text.Encoding]::UTF8.GetBytes($v))+' USING utf8mb4)'};[Convert]::ToString($v,[Globalization.CultureInfo]::InvariantCulture)}
  $setup='';$baseline='';foreach($table in $context.filters.Keys){
   # Keep earlier live shadows populated for dependent addon/scaling filters.
   $setup+="CREATE TEMPORARY TABLE saved_$table LIKE $table;INSERT INTO saved_$table SELECT * FROM $table WHERE $($context.filters[$table]);CREATE TEMPORARY TABLE $table LIKE saved_$table;INSERT INTO $table SELECT * FROM saved_$table;`n"
   $baseline+="DELETE FROM $table;`n"
   $cols=$context.columns[$table];$names=($cols|ForEach-Object{"``$_``"}) -join ','
   foreach($row in $fixture[$table]){$values=($cols|ForEach-Object{Value $row[$_]}) -join ',';$baseline+="INSERT INTO $table($names) VALUES($values);`n"}
  }
  function Snapshot($tag){$sql='';foreach($table in $context.filters.Keys){$order=($context.columns[$table]|ForEach-Object{"``$_``"}) -join ',';$sql+="SELECT '$tag-$table',$table.* FROM $table WHERE $($context.filters[$table]) ORDER BY $order;`n"};$sql}
  $shape="SELECT 'events',entryorguid,source_type,id,link,event_type,event_phase_mask,event_chance,event_flags,event_param1,event_param2,event_param3,event_param4,action_type,action_param1,action_param2,target_type,target_param1,target_param2 FROM smart_scripts WHERE (entryorguid=106699 AND source_type=0 AND id IN(0,1)) OR (entryorguid=106699 AND source_type=9) OR (entryorguid IN(106700,106701,106702) AND source_type=0 AND id=2) ORDER BY entryorguid,source_type,id;"
  function ConditionShape($tag){"SELECT '$tag',SourceGroup,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=106699 ORDER BY ConditionValue1;"}
  $sql=$setup+$baseline+(Snapshot original)+(ConditionShape baseline-conditions)+$preMigration+$preMigration+$migration+(Snapshot first)+$shape+(ConditionShape conditions)+$migration+(Snapshot second)
  if($WithPriestHealing){$priestShape=$shape.Replace("'events'","'priest-events'").Replace('106699','104832').Replace('106700,106701,106702','105799,105800,105801').Replace('id IN(0,1)','id IN(0,1,3,4)');$sql+=$priestShape;$sql+=(ConditionShape priest-conditions).Replace('106699','104832').Replace('ORDER BY ConditionValue1','ORDER BY SourceGroup,ConditionTypeOrReference,ConditionValue1');$sql+=$baseline+$priestShape.Replace("'priest-events'","'priest-baseline-events'")+(ConditionShape priest-baseline-conditions).Replace('106699','104832');$sql+="SELECT 'priest-source-health',c.id,c.curhealth,t.RegenHealth FROM creature c JOIN creature_template t ON c.id=t.entry WHERE c.id=104832;"}
  $variants=@(
   'UPDATE conditions SET ConditionTarget=2 WHERE SourceEntry=106699;'
   'UPDATE conditions SET ConditionValue1=106799 WHERE SourceEntry=106699 AND ConditionValue1=106700;'
   'UPDATE conditions SET ConditionValue2=49 WHERE SourceEntry=106699;'
   'UPDATE conditions SET NegativeCondition=0 WHERE SourceEntry=106699;'
   'UPDATE conditions SET ElseGroup=1 WHERE SourceEntry=106699 AND ConditionValue1=106700;'
   'DELETE FROM conditions WHERE SourceEntry=106699 AND ConditionValue1=106700;'
   'UPDATE conditions SET ConditionTarget=1 WHERE SourceEntry=106699 AND ConditionValue1=106700;'
   'UPDATE smart_scripts SET event_param2=2 WHERE entryorguid=106699 AND source_type=0 AND id=0;'
   'UPDATE smart_scripts SET link=0 WHERE entryorguid=106699 AND source_type=0 AND id=0;'
   'UPDATE smart_scripts SET event_flags=0 WHERE entryorguid=106699 AND source_type=0 AND id=0;'
   'UPDATE smart_scripts SET event_param1=1 WHERE entryorguid=106699 AND source_type=9 AND id=1;'
   'UPDATE smart_scripts SET action_param1=2141 WHERE entryorguid=106699 AND source_type=9 AND id=0;'
   'UPDATE smart_scripts SET action_param1=1 WHERE entryorguid=106699 AND source_type=9 AND id=4;'
   'UPDATE smart_scripts SET action_param1=106748 WHERE entryorguid=106699 AND source_type=9 AND id=5;'
   'UPDATE smart_scripts SET action_param2=49865 WHERE entryorguid=106699 AND source_type=9 AND id=6;'
   'UPDATE smart_scripts SET target_param1=104832 WHERE entryorguid=106700 AND source_type=0 AND id=2;'
   'UPDATE smart_scripts SET link=1 WHERE entryorguid=104832 AND source_type=0 AND id=0;'
   "UPDATE creature_template SET ScriptName='custom' WHERE entry=106699;"
   "UPDATE creature SET PhaseId='6140' WHERE id=106699;"
   'UPDATE conversation_data SET unk1=1 WHERE entry=2140;'
  )
  if($WithPriestHealing){$variants[6]='UPDATE conditions SET ConditionTarget=0 WHERE SourceEntry=106699 AND ConditionValue1=106700;';$variants+=@('UPDATE conditions SET ConditionTarget=1 WHERE SourceEntry=104832 AND ConditionValue1=105799;','UPDATE smart_scripts SET link=0 WHERE entryorguid=104832 AND source_type=0 AND id=0;','UPDATE smart_scripts SET event_type=61 WHERE entryorguid=104832 AND source_type=0 AND id=3;','UPDATE smart_scripts SET event_param1=90 WHERE entryorguid=104832 AND source_type=0 AND id=3;','UPDATE creature SET curhealth=100 WHERE id=104832;','UPDATE creature_template SET RegenHealth=1 WHERE entry=104832;','UPDATE creature_template_scaling SET LevelScalingMax=0 WHERE Entry=104832;')}
  for($i=0;$i -lt $variants.Count;++$i){$sql+=$baseline+$preMigration+$variants[$i]+(Snapshot "refuse-$i-before")+$migration+(Snapshot "refuse-$i-after")}
  if($VerifyLiveData){foreach($table in $context.filters.Keys){$sql+="DELETE FROM $table;INSERT INTO $table SELECT * FROM saved_$table;`n"};$sql+=(Snapshot live);$normalized=[IO.File]::ReadAllText($path).Replace("`r`n","`n");$hash=[Convert]::ToHexString([Security.Cryptography.SHA1]::HashData([Text.Encoding]::UTF8.GetBytes($normalized)));$sql+="SELECT 'release',hash,state FROM updates WHERE name='2026_10_30_32_priest_azuregos_receiver.sql';";if($WithPriestHealing){$healingHash=[Convert]::ToHexString([Security.Cryptography.SHA1]::HashData([Text.Encoding]::UTF8.GetBytes([IO.File]::ReadAllText($healingPath).Replace("`r`n","`n"))));$sql+="SELECT 'release-healing',hash,state FROM updates WHERE name='2026_10_30_33_priest_azuregos_healing.sql';"}}
  $sql+="SELECT '__NEXUS_DATA_COMPLETE__';"
  [IO.File]::WriteAllText("$out/replay.sql",$sql)
  $c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object{$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';');$prior=$env:MYSQL_PWD
  try{$env:MYSQL_PWD=$env:CODEX_SQL_READER_PASSWORD;$code=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -Arguments @("--host=$($c[0])","--port=$($c[1])","--user=$env:CODEX_SQL_READER_USER","--database=$($c[4])",'--batch','--raw','--skip-column-names','--force','--default-character-set=utf8mb4') -InputFile "$out/replay.sql" -OutputFile "$out/replay.tsv" -ErrorFile "$out/replay.stderr"}finally{$env:MYSQL_PWD=$prior}
  [IO.File]::WriteAllText("$out/replay.exit.txt",[string]$code)
  $message=if($WithPriestHealing){'Priest Azuregos healing/context differ'}else{'Priest Azuregos/context differ'}
  $errors=[IO.File]::ReadAllLines("$out/replay.stderr");if($code -notin 0,1 -or @($errors|Where-Object{$_ -match "^ERROR 1644 .*$message"}).Count -ne $variants.Count -or @($errors|Where-Object{$_ -match '^ERROR ' -and $_ -notmatch "^ERROR 1644 .*$message"}).Count){throw 'Unexpected native SQL/refusal result; full evidence retained'}
  $rows=[IO.File]::ReadAllLines("$out/replay.tsv");if($rows[-1] -cne '__NEXUS_DATA_COMPLETE__'){throw 'Incomplete native TEMP replay output'}
  $groups=@{};foreach($line in $rows){$tab=$line.IndexOf("`t");if($tab -lt 1){continue};$tag=$line.Substring(0,$tab);if(!$groups.ContainsKey($tag)){$groups[$tag]=[Collections.Generic.List[string]]::new()};$groups[$tag].Add($line.Substring($tab+1))}
  function Signature($tag){if(!$groups.ContainsKey($tag)){return ''};[string]::Join("`n",$groups[$tag])}
  foreach($table in $context.filters.Keys){if((Signature "first-$table") -cne (Signature "second-$table")){throw "Non-idempotent $table"};if($table -ne 'conditions' -and !($WithPriestHealing -and $table -eq 'smart_scripts') -and (Signature "original-$table") -cne (Signature "first-$table")){throw "Changed native $table"};for($i=0;$i -lt $variants.Count;++$i){if((Signature "refuse-$i-before-$table") -cne (Signature "refuse-$i-after-$table")){throw "Refused $table/$i changed"}};if($VerifyLiveData -and (Signature "first-$table") -cne (Signature "live-$table")){throw "Actual live $table differs"}}
  $old=@($groups['original-conditions']);$new=@($groups['first-conditions']);$expectedCount=if($WithPriestHealing){10}else{8};if($old.Count -ne 8 -or $new.Count -ne $expectedCount){throw 'Context condition count differs'};$edits=0
  for($i=0;$i -lt $old.Count;++$i){$f=$old[$i].Split("`t");if($f[0] -eq '22' -and $f[1] -eq '1' -and ($f[2] -eq '106699' -or ($WithPriestHealing -and $f[2] -eq '104832')) -and $f[5] -eq '29'){if($f[6] -ne '0'){throw 'Counterfactual condition differs'};$f[6]='1';++$edits};if(($f -join "`t") -cne $new[$i]){throw 'Unexpected condition delta'}};if($edits -ne $(if($WithPriestHealing){6}else{3})){throw 'Unexpected native condition target delta'}
  if($WithPriestHealing){$expected=@("22`t4`t104832`t0`t0`t44`t1`t1065`t0`t0`t0`t0`t`tPriest Azuregos: healing finale is the native1065 opening step","22`t4`t104832`t0`t0`t52`t1`t47107`t0`t0`t0`t0`t`tPriest Azuregos: healing finale requires actual Scion-clear progress");if(($new[8..9] -join "`n") -cne ($expected -join "`n")){throw 'Exact native healing prerequisites differ'};$os=@($groups['original-smart_scripts']);$ns=@($groups['first-smart_scripts']);$links=0;for($i=0;$i -lt $os.Count;++$i){$f=$os[$i].Split("`t");if($f[0] -eq '104832' -and $f[1] -eq '0' -and $f[2] -eq '0'){if($f[3] -ne '3'){throw 'Native malformed link differs'};$f[3]='0';++$links};if(($f -join "`t") -cne $ns[$i]){throw 'Unrelated Smart field changed'}};if($links -ne 1){throw 'Expected one corrected link'};foreach($tag in @('priest-events','priest-baseline-events','priest-baseline-conditions','priest-conditions','priest-source-health')){[IO.File]::WriteAllLines("$out/$tag.tsv",$groups[$tag])}}
  foreach($tag in @('events','baseline-conditions','conditions')){[IO.File]::WriteAllLines("$out/$tag.tsv",$groups[$tag])}
  if($VerifyLiveData -and (Signature release) -cne "$hash`tRELEASED"){throw 'Actual updater LF hash/state differs'}
  if($VerifyLiveData -and $WithPriestHealing -and (Signature release-healing) -cne "$healingHash`tRELEASED"){throw 'Actual healing updater LF hash/state differs'}
  "PASS all$($context.filters.Count) context tables: exact receiver deltas$(if($WithPriestHealing){', independent Priest healing root and actual47107/1065 opening prerequisites'}), repeat idempotence, $($variants.Count) refused custom/partial cases unchanged; other native event/list/guide/conversation/phase data preserved. Persistent DML denied."
 }
}
