param([string]$MigrationFile,[switch]$VerifyLiveData)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/meryl-wave-data"
if(!$MigrationFile){$MigrationFile="$repo/sql/pending/world/2026_10_29_18_meryl_sixth_wave.sql";if(!(Test-Path $MigrationFile)){$MigrationFile="$repo/sql/updates/world/2026_10_29_18_meryl_sixth_wave.sql"}}
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
   if($Refusal){if($errors -notmatch 'Meryl wave caller/lists/gates differ; review before updating' -or @([regex]::Matches($errors,'ERROR [0-9]+')).Count -ne 1){throw "Unexpected Meryl refusal: $errorFile"}}
   elseif($code -ne 0 -or $errors){throw "Meryl SQL failed: $errorFile"};return ,$rows
  }finally{$env:MYSQL_PWD=$old;[Console]::OutputEncoding=$previousConsole;$OutputEncoding=$previousPipe}
 }
 $fixture=Get-Content "$PSScriptRoot/MerylWaveOriginal.json" -Raw -Encoding UTF8|ConvertFrom-Json
 $filters=[ordered]@{smart_scripts='(source_type=0 AND entryorguid IN(107691,108097,107739,107742,107744,107745,107990)) OR (source_type=9 AND (entryorguid=107691 OR entryorguid BETWEEN 10809700 AND 10809706))';conditions='SourceTypeOrReferenceId=22 AND SourceEntry IN(107691,108097,10809700,10809701,10809702,10809703,10809704,10809705,10809706)';creature_template='entry IN(107691,107739,107742,107744,107745,107990)';scenario_data='ScenarioID IN(1122,1221)'}
 $order=@{smart_scripts='entryorguid,source_type,id,link';conditions='SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference';creature_template='entry';scenario_data='ScenarioID'}
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
 $original=@((Signature $rows 'original-smart_scripts').Split("`n"));$repaired=@((Signature $rows 'first-smart_scripts').Split("`n"))
 $edits=0;$selection=$null
 $expected=@($original|ForEach-Object {$f=$_.Split("`t");if($f[0] -eq '107691' -and $f[1] -eq '0' -and $f[2] -eq '1'){if($f[20] -ne '10809706'){throw 'Original sixth branch differs'};$f[20]='10809705';$selection=$f;++$edits};$f -join "`t"})
 if($edits -ne 1 -or ($expected -join "`n") -cne ($repaired -join "`n")){throw 'Expected exactly one action_param6 field update'}
 $available=@($repaired|ForEach-Object {$f=$_.Split("`t");if($f[1] -eq '9'){$f[0]}}|Sort-Object -Unique)
 foreach($id in $selection[15..20]){if($id -notin $available){throw "Still missing native selected list $id"}}
 if('10809706' -in $available -or ($selection[15..20]|Sort-Object -Unique).Count -ne 6){throw 'Unexpected sixth-list/weight scope'}
 $jailer=@($repaired|Where-Object {$f=$_.Split("`t");$f[0] -eq '10809705' -and $f[1] -eq '9'})
 if($jailer.Count -ne 1 -or $jailer[0].Split("`t")[15] -ne '107744' -or $jailer[0].Split("`t")[2] -ne '4'){throw 'Original Jailer payload/first id4 differs'}
 [IO.File]::WriteAllLines("$out/original.tsv",[string[]]$original);[IO.File]::WriteAllLines("$out/repaired.tsv",[string[]]$repaired)
 [IO.File]::WriteAllLines("$out/sql-evidence.tsv",[string[]]$rows)
 $variants=[ordered]@{
  'caller-first-choice'='UPDATE smart_scripts SET action_param1=10809701 WHERE entryorguid=107691 AND source_type=0 AND id=1;';
  'caller-timing'='UPDATE smart_scripts SET event_param1=9000 WHERE entryorguid=107691 AND source_type=0 AND id=1;';
  'caller-vfx'='UPDATE smart_scripts SET action_param1=107692 WHERE entryorguid=107691 AND source_type=0 AND id=0;';
  'jailer-position'='UPDATE smart_scripts SET target_x=0 WHERE entryorguid=10809705 AND source_type=9;';
  'jailer-type'='UPDATE smart_scripts SET action_param2=7 WHERE entryorguid=10809705 AND source_type=9;';
  'jailer-duration'='UPDATE smart_scripts SET action_param3=5000 WHERE entryorguid=10809705 AND source_type=9;';
  'missing-list-now-custom'="INSERT INTO smart_scripts(entryorguid,source_type,id,comment) VALUES(10809706,9,0,'custom');";
  'stage'='UPDATE conditions SET ConditionValue2=4 WHERE SourceEntry=107691 AND SourceGroup=2;';
  'shared-actor-ai'="UPDATE creature_template SET AIName='custom' WHERE entry=107691;";
  'jailer-ai'="UPDATE creature_template SET AIName='custom' WHERE entry=107744;";
  'scenario-map'='UPDATE scenario_data SET MapID=0 WHERE ScenarioID=1122;';
  'applied-sixth-choice'='UPDATE smart_scripts SET action_param6=10809704 WHERE entryorguid=107691 AND source_type=0 AND id=1;'
 }
 foreach($name in $variants.Keys){$base=$setup+$baseline+$(if($name -like 'applied-*'){$migration}else{''});$result=Query ($base+$variants[$name]+(Snapshot 'before')+$migration+(Snapshot 'after')) -Refusal
  foreach($table in $filters.Keys){if((Signature $result "before-$table") -cne (Signature $result "after-$table")){throw "Custom state changed $name/$table"}}
 }
 if($VerifyLiveData){$sha=[Security.Cryptography.SHA1]::Create();try{$hash=([BitConverter]::ToString($sha.ComputeHash([Text.Encoding]::UTF8.GetBytes($migration.Replace("`r`n","`n"))))).Replace('-','')}finally{$sha.Dispose()}
  $live=Query "SELECT COUNT(*) FROM updates WHERE name='2026_10_29_18_meryl_sixth_wave.sql' AND hash='$hash' AND state='RELEASED';"
  if($live -notcontains '1'){throw 'Live Meryl migration/hash differs'}
  $fullLive=Query (Snapshot 'live');foreach($table in $filters.Keys){if((Signature $fullLive "live-$table") -cne (Signature $rows "first-$table")){throw "Live differs from Meryl regression $table"}}
 }
 'PASS full-schema TEMP Meryl sixth-wave replay/idempotence: exactly one caller parameter06->05, all six selected lists present; original Jailer first id4/data/position, all other waves/AI/timing/stage1122 guards retained; twelve custom-state refusals'
}
