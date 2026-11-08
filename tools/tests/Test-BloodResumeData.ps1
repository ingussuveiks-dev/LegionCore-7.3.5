param([string]$MigrationFile,[switch]$VerifyLiveData)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/blood-resume-data/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"
New-Item -ItemType Directory -Force $out|Out-Null
if(!$MigrationFile){$MigrationFile="$repo/sql/pending/world/2026_11_08_01_blood_resume_entrance.sql";if(!(Test-Path $MigrationFile)){$MigrationFile="$repo/sql/updates/world/2026_11_08_01_blood_resume_entrance.sql"}}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -Test {
 $migration=[IO.File]::ReadAllText($MigrationFile);$c=((Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}) -split '"')[1].Split(';');$script:query=0
 function Query([string]$sql,[switch]$Refusal){
  $path="$out/sql-$script:query";++$script:query;[IO.File]::WriteAllText("$path.sql",$sql);$old=$env:MYSQL_PWD
  try{$env:MYSQL_PWD=$c[3];$rows=@($sql|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --default-character-set=utf8mb4 --batch --raw --skip-column-names --force 2> "$path.stderr");$code=$LASTEXITCODE;[IO.File]::WriteAllLines("$path.stdout",[string[]]$rows);$errors=[IO.File]::ReadAllText("$path.stderr")
   if($Refusal){if($errors -notmatch 'Blood resume entrance/source/route differ; review before updating' -or @([regex]::Matches($errors,'ERROR [0-9]+')).Count -ne 1){throw "Unexpected migration refusal: $path.stderr"}}
   elseif($code -or $errors){throw "SQL regression failed: $path.stderr"};return ,$rows
  }finally{$env:MYSQL_PWD=$old}
 }
 $tables=@('lfg_entrances','spell_target_position','scenario_data','smart_scripts');$fingerprints=@{};$setup='SET SESSION group_concat_max_len=16777216;'
 foreach($table in $tables){
  $columns=@(Query "SHOW COLUMNS FROM $table;"|ForEach-Object {($_ -split "`t")[0]})
  $values=($columns|ForEach-Object {"COALESCE(HEX(CAST(``$_`` AS BINARY)),'NULL')"}) -join ','
  $order=($columns|ForEach-Object {"``$_``"}) -join ','
  $fingerprints[$table]="(SELECT SHA2(COALESCE(GROUP_CONCAT(CONCAT_WS('|',$values) ORDER BY $order SEPARATOR '\n'),''),256) FROM $table)"
  $scope=if($table -eq 'smart_scripts'){' WHERE entryorguid IN(102414,100397)'}else{''}
  if($scope){$fingerprints[$table]=$fingerprints[$table].Replace("FROM $table)","FROM $table$scope)")}
  $setup+="CREATE TEMPORARY TABLE resume_$table LIKE $table;INSERT INTO resume_$table SELECT * FROM $table$scope;ALTER TABLE resume_$table RENAME TO $table;"
 }
 $snapshot="JSON_OBJECT('lfg',$($fingerprints.lfg_entrances),'spell',$($fingerprints.spell_target_position),'route',$($fingerprints.scenario_data),'smart',$($fingerprints.smart_scripts))"
 $liveBefore=Query "SET SESSION group_concat_max_len=16777216;SELECT $snapshot;"
 $seed="$setup DELETE FROM lfg_entrances WHERE dungeonId IN(1059);"
 $others=$fingerprints.lfg_entrances.Replace('FROM lfg_entrances)','FROM lfg_entrances WHERE dungeonId NOT IN(1059))')
 $normal=Query "$seed SET @others=$others;SET @spell=$($fingerprints.spell_target_position);SET @route=$($fingerprints.scenario_data);SET @smart=$($fingerprints.smart_scripts);$migration SET @once=$($fingerprints.lfg_entrances);$migration SELECT JSON_OBJECT('idempotent',@once=$($fingerprints.lfg_entrances),'others',@others=$others,'spell',@spell=$($fingerprints.spell_target_position),'route',@route=$($fingerprints.scenario_data),'smart',@smart=$($fingerprints.smart_scripts),'target',(SELECT COUNT(*) FROM lfg_entrances WHERE dungeonId IN(1059) AND ABS(position_x-240.9)<0.001 AND ABS(position_y-1953.44)<0.001 AND ABS(position_z-(-55.29))<0.001 AND ABS(orientation-0.384845)<0.001));"
 $result=$normal[-1]|ConvertFrom-Json;foreach($name in @('idempotent','others','spell','route','smart','target')){if($result.$name -ne $(if($name -eq 'target'){1}else{1})){throw "Migration changed unexpected data: $name"}}
 $mutations=@(
  'UPDATE spell_target_position SET target_position_x=target_position_x+1 WHERE id=201678;',
  'UPDATE spell_target_position SET target_map=1539 WHERE id=201678;',
  'DELETE FROM spell_target_position WHERE id=201678;',
  'UPDATE scenario_data SET ScenarioID=65000 WHERE ScenarioID=940;',
  'UPDATE scenario_data SET Team=1 WHERE ScenarioID=940;',
  'UPDATE scenario_data SET DifficultyID=12 WHERE ScenarioID=940;',
  "INSERT INTO lfg_entrances VALUES(1059,'custom',1,2,3,4);",
  "INSERT INTO lfg_entrances VALUES(1059,'custom zero',0,0,0,0);",
  'UPDATE scenario_data SET Class=6 WHERE ScenarioID=940;',
  'UPDATE scenario_data SET LfgDungeonID=1059 WHERE ScenarioID=940;',
  'UPDATE spell_target_position SET target_position_y=target_position_y+1 WHERE id=201678;',
  'UPDATE spell_target_position SET target_orientation=target_orientation+1 WHERE id=201678;',
  "ALTER TABLE lfg_entrances MODIFY position_x FLOAT NULL;INSERT INTO lfg_entrances VALUES(1059,'custom nullable',NULL,1953.44,-55.29,0.384845);",
  "ALTER TABLE lfg_entrances MODIFY position_y FLOAT NULL;INSERT INTO lfg_entrances VALUES(1059,'custom nullable',240.9,NULL,-55.29,0.384845);",
  'DELETE FROM scenario_data WHERE ScenarioID=940;',
  'UPDATE spell_target_position SET target_position_z=target_position_z+1 WHERE id=201678;',
  'UPDATE smart_scripts SET action_param1=972 WHERE entryorguid=102414 AND source_type=0 AND id=0;',
  'DELETE FROM smart_scripts WHERE entryorguid=102414 AND source_type=0 AND id=1;'
 )
 foreach($mutation in $mutations){$rows=Query "$seed $mutation SET @before=$snapshot;$migration SELECT JSON_OBJECT('unchanged',@before=$snapshot);" -Refusal;$result=$rows[-1]|ConvertFrom-Json;if($result.unchanged -ne 1){throw 'Refusal changed source/route/entrance data'}}
 $custom=Query "$seed $migration UPDATE lfg_entrances SET name='custom name retained' WHERE dungeonId IN(1059);SET @before=$snapshot;$migration SELECT JSON_OBJECT('unchanged',@before=$snapshot);"
 if(($custom[-1]|ConvertFrom-Json).unchanged -ne 1){throw 'Exact existing entrance custom name overwritten'}

 $liveAfter=Query "SET SESSION group_concat_max_len=16777216;SELECT $snapshot;";if(($liveBefore -join '') -ne ($liveAfter -join '')){throw 'Live tables changed during TEMP regression'}
 if($VerifyLiveData){
  $hash=[Convert]::ToHexString([Security.Cryptography.SHA1]::HashData([Text.UTF8Encoding]::new($false).GetBytes($migration.Replace("`r`n","`n")))).ToLowerInvariant();$file=[IO.Path]::GetFileName($MigrationFile)
  $live=Query "SELECT JSON_OBJECT('target',(SELECT COUNT(*) FROM lfg_entrances WHERE dungeonId IN(1059) AND ABS(position_x-240.9)<0.001 AND ABS(position_y-1953.44)<0.001 AND ABS(position_z-(-55.29))<0.001 AND ABS(orientation-0.384845)<0.001),'released',(SELECT COUNT(*) FROM updates WHERE name='$file' AND hash='$hash' AND state='RELEASED'));"
  $result=$live[-1]|ConvertFrom-Json;if($result.target -ne 1 -or $result.released -ne 1){throw 'Live native entrance/updater RELEASED hash mismatch'}
 }
 'PASS full-schema TEMP Blood entrance: native source/route, exact one-row insert, idempotence, unrelated full-table preservation,18 no-mutation refusals,custom-name preservation;live tables unchanged. Actual native login/terrain/client checks separate.'
}
