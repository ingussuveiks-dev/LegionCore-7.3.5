param([string]$MigrationFile,[switch]$VerifyLiveData)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/guardian-wave-death-data"
if(!$MigrationFile){$MigrationFile="$repo/sql/pending/world/2026_10_29_22_guardian_wave_death_dispatch.sql";if(!(Test-Path $MigrationFile)){$MigrationFile="$repo/sql/updates/world/2026_10_29_22_guardian_wave_death_dispatch.sql"}}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -Test {
 $migration=Get-Content $MigrationFile -Raw
 $c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $script:queryIndex=0
 function Query([string]$sql,[switch]$Refusal){
  $old=$env:MYSQL_PWD;$errorFile="$out/sql-$script:queryIndex.stderr";++$script:queryIndex
  [IO.File]::WriteAllText("$out/sql-$($script:queryIndex-1).sql",$sql)
  $previousConsole=[Console]::OutputEncoding;$previousPipe=$OutputEncoding
  try{$env:MYSQL_PWD=$c[3];[Console]::OutputEncoding=[Text.UTF8Encoding]::new($false);$OutputEncoding=[Text.UTF8Encoding]::new($false);$ErrorActionPreference='Continue';$rows=@($sql|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --default-character-set=utf8mb4 --batch --raw --skip-column-names --force 2>$errorFile);$code=$LASTEXITCODE;$ErrorActionPreference='Stop';$errors=[string](Get-Content $errorFile -Raw)
   [IO.File]::WriteAllLines("$out/sql-$($script:queryIndex-1).stdout",[string[]]$rows)
   if($Refusal){if($errors -notmatch 'Guardian waves/death/conditions differ; review before updating' -or @([regex]::Matches($errors,'ERROR [0-9]+')).Count -ne 1){throw "Unexpected Guardian refusal: $errorFile"}}
   elseif($code -ne 0 -or $errors){throw "Guardian SQL failed: $errorFile"};return ,$rows
  }finally{$env:MYSQL_PWD=$old;[Console]::OutputEncoding=$previousConsole;$OutputEncoding=$previousPipe}
 }
 $fixture=Get-Content "$PSScriptRoot/GuardianWaveDeathOriginal.json" -Raw -Encoding UTF8|ConvertFrom-Json
 $filters=[ordered]@{smart_scripts='(source_type=0 AND entryorguid IN(101384,101388,101389,101742,105243,101362,101390)) OR (source_type=9 AND (entryorguid IN(101742,105243) OR entryorguid BETWEEN 10174200 AND 10174204))';conditions='SourceTypeOrReferenceId=22 AND SourceEntry IN(101384,101388,101389,101742,105243,101362,101390,10174200,10174201,10174202,10174203,10174204)';creature_text='CreatureID IN(101742,105243,101362,101390)';creature_template='entry IN(101384,101388,101389,101742,105243,101362,101390)'}
 $order=@{smart_scripts='entryorguid,source_type,id,link';conditions='SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2';creature_text='CreatureID,GroupID,ID';creature_template='entry'}
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
 foreach($table in $filters.Keys){if((Signature $rows "first-$table") -cne (Signature $rows "second-$table")){throw "Not idempotent $table"};if($table -notin @('smart_scripts','conditions') -and (Signature $rows "original-$table") -cne (Signature $rows "first-$table")){throw "Unrelated table changed $table"}}
 $original=@((Signature $rows 'original-smart_scripts').Split("`n"));$repaired=@((Signature $rows 'first-smart_scripts').Split("`n"));$conditions=@((Signature $rows 'first-conditions').Split("`n"))
 if($original.Count -ne @($fixture.smart_scripts).Count -or $repaired.Count -ne $original.Count){throw 'Original/repaired row count differs'}
 $edits=0
 for($i=0;$i -lt $original.Count;++$i){$f=$original[$i].Split("`t");if($f[1] -eq '0' -and (($f[0] -eq '101388' -and [int]$f[2] -in @(6,7,8)) -or ($f[0] -eq '101389' -and [int]$f[2] -in @(0,1)))){if([int]$f[3] -ne ([int]$f[2]+1)){throw 'Original immediate call differs'};$f[3]='0';++$edits};if(($f -join "`t") -cne $repaired[$i]){throw "Unexpected row mutation $i"}}
 if($edits -ne 5){throw 'Expected exactly five malformed calls'}
 $originalConditions=@((Signature $rows 'original-conditions').Split("`n"))
 if($conditions.Count -ne $originalConditions.Count+4){throw 'Expected four additional own-species guards'}
 foreach($line in $originalConditions){if($conditions -cnotcontains $line){throw 'Original wave condition changed'}}
 foreach($group in 7..10){$added=@($conditions|Where-Object {$originalConditions -cnotcontains $_ -and $_.StartsWith("22`t$group`t101388`t")});if($added.Count -ne 1){throw "Missing unique added guard $group"};$f=$added[0].Split("`t");if(($f[3..12] -join ',') -ne '0,0,29,0,101388,100,0,1,0,'){throw "Changed additional guard $group"}}
 [IO.File]::WriteAllLines("$out/original-conditions.tsv",[string[]]$originalConditions)
 [IO.File]::WriteAllLines("$out/original.tsv",[string[]]$original);[IO.File]::WriteAllLines("$out/repaired.tsv",[string[]]$repaired);[IO.File]::WriteAllLines("$out/conditions.tsv",[string[]]$conditions)
 [IO.File]::WriteAllLines("$out/sql-evidence.tsv",[string[]]$rows)
 $variants=[ordered]@{
  'lost-death-root'='UPDATE smart_scripts SET event_type=61 WHERE entryorguid=101388 AND id=9;';
  'wave-criterion'='UPDATE smart_scripts SET action_param2=48108 WHERE entryorguid=101388 AND id=9;';
  'death-spell'='UPDATE smart_scripts SET action_param1=0 WHERE entryorguid=101389 AND id=0;';
  'death-cast-flags'='UPDATE smart_scripts SET action_param2=2 WHERE entryorguid=101389 AND id=0;';
  'first-wave-summon'='UPDATE smart_scripts SET action_param1=101754 WHERE entryorguid=101388 AND id=2;';
  'spirit-signal'='UPDATE smart_scripts SET action_param2=1 WHERE entryorguid=101388 AND id=10;';
  'last-living-negation'='UPDATE conditions SET NegativeCondition=0 WHERE SourceEntry=101389 AND SourceGroup=2 AND ConditionTypeOrReference=29;';
  'last-living-radius'='UPDATE conditions SET ConditionValue2=1 WHERE SourceEntry=101388 AND SourceGroup=10 AND ConditionTypeOrReference=29;';
  'step-guard'='UPDATE conditions SET ConditionValue2=3 WHERE SourceEntry=101388 AND SourceGroup=10 AND ConditionTypeOrReference=44;';
  'condition-target'='UPDATE conditions SET ConditionTarget=0 WHERE SourceEntry=101388 AND SourceGroup=10 AND ConditionTypeOrReference=44;';
  'unknown-sludge-link'='UPDATE smart_scripts SET link=0 WHERE entryorguid=101384 AND id=0;';
  'finale-loot-producer'='UPDATE smart_scripts SET action_param1=0 WHERE entryorguid=101390 AND id=9;';
  'next-wave-timing'='UPDATE smart_scripts SET event_param1=1 WHERE entryorguid=10174201 AND source_type=9 AND id=0;';
  'custom-condition'="INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,ConditionTypeOrReference,ConditionValue1) VALUES(22,10,101388,9,1);";
  'custom-ai'="UPDATE creature_template SET AIName='custom' WHERE entry=101389;";
  'missing-row'='DELETE FROM smart_scripts WHERE entryorguid=101389 AND id=2;';
   'partial-script-only'='UPDATE smart_scripts SET link=0 WHERE source_type=0 AND ((entryorguid=101388 AND id IN(6,7,8)) OR(entryorguid=101389 AND id IN(0,1)));';
  'partial-condition-only'="INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,ConditionTypeOrReference,ConditionValue1,ConditionValue2,NegativeCondition) VALUES(22,10,101388,29,101388,100,1);";
  'applied-own-species-radius'='UPDATE conditions SET ConditionValue2=10 WHERE SourceEntry=101388 AND ConditionValue1=101388;';
  'applied-own-species-negation'='UPDATE conditions SET NegativeCondition=0 WHERE SourceEntry=101388 AND ConditionValue1=101388;';
  'applied-custom'='UPDATE smart_scripts SET link=7 WHERE entryorguid=101388 AND id=6;'
 }
 foreach($name in $variants.Keys){$base=$setup+$baseline+$(if($name -like 'applied-*'){$migration}else{''});$result=Query ($base+$variants[$name]+(Snapshot 'before')+$migration+(Snapshot 'after')) -Refusal
  foreach($table in $filters.Keys){if((Signature $result "before-$table") -cne (Signature $result "after-$table")){throw "Custom state changed $name/$table"}}
 }
 if($VerifyLiveData){$sha=[Security.Cryptography.SHA1]::Create();try{$hash=([BitConverter]::ToString($sha.ComputeHash([Text.Encoding]::UTF8.GetBytes($migration.Replace("`r`n","`n"))))).Replace('-','')}finally{$sha.Dispose()}
  $live=Query "SELECT COUNT(*) FROM updates WHERE name='2026_10_29_22_guardian_wave_death_dispatch.sql' AND hash='$hash' AND state='RELEASED';"
  if($live -notcontains '1'){throw 'Live Guardian wave migration/hash differs'}
  $fullLive=Query (Snapshot 'live');foreach($table in $filters.Keys){if((Signature $fullLive "live-$table") -cne (Signature $rows "first-$table")){throw "Live differs from Guardian regression $table"}}
 }
 'PASS full-schema TEMP Guardian wave death replay/idempotence: five malformed calls and four missing own-species guards only; independent DEATH roots/descending criteria/last-living and990 stage guards, all waves/spells/helpers/finale/unknown sludge link retained; twenty-one custom/partial-state refusals'
}
