param([string]$MigrationFile,[switch]$VerifyLiveData)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/earthwarder-resume-data/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"
New-Item -ItemType Directory -Force $out|Out-Null
if(!$MigrationFile){$MigrationFile="$repo/sql/pending/world/2026_11_07_00_earthwarder_resume_entrance.sql";if(!(Test-Path $MigrationFile)){$MigrationFile="$repo/sql/updates/world/2026_11_07_00_earthwarder_resume_entrance.sql"}}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -Test {
 $migration=[IO.File]::ReadAllText($MigrationFile);$c=((Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}) -split '"')[1].Split(';');$script:query=0
 function Query([string]$sql,[switch]$Refusal){
  $path="$out/sql-$script:query";++$script:query;[IO.File]::WriteAllText("$path.sql",$sql);$old=$env:MYSQL_PWD
  try{$env:MYSQL_PWD=$c[3];$rows=@($sql|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --default-character-set=utf8mb4 --batch --raw --skip-column-names --force 2> "$path.stderr");$code=$LASTEXITCODE;[IO.File]::WriteAllLines("$path.stdout",[string[]]$rows);$errors=[IO.File]::ReadAllText("$path.stderr")
   if($Refusal){if($errors -notmatch 'Earthwarder resume entrance/source/route differ; review before updating' -or @([regex]::Matches($errors,'ERROR [0-9]+')).Count -ne 1){throw "Unexpected migration refusal: $path.stderr"}}
   elseif($code -or $errors){throw "SQL regression failed: $path.stderr"};return ,$rows
  }finally{$env:MYSQL_PWD=$old}
 }
 $tables=@('lfg_entrances','spell_target_position','scenario_data');$fingerprints=@{};$setup='SET SESSION group_concat_max_len=16777216;'
 foreach($table in $tables){
  $columns=@(Query "SHOW COLUMNS FROM $table;"|ForEach-Object {($_ -split "`t")[0]})
  $values=($columns|ForEach-Object {"COALESCE(HEX(CAST(``$_`` AS BINARY)),'NULL')"}) -join ','
  $order=($columns|ForEach-Object {"``$_``"}) -join ','
  $fingerprints[$table]="(SELECT SHA2(COALESCE(GROUP_CONCAT(CONCAT_WS('|',$values) ORDER BY $order SEPARATOR '\n'),''),256) FROM $table)"
  $setup+="CREATE TEMPORARY TABLE resume_$table LIKE $table;INSERT INTO resume_$table SELECT * FROM $table;ALTER TABLE resume_$table RENAME TO $table;"
 }
 $snapshot="JSON_OBJECT('lfg',$($fingerprints.lfg_entrances),'spell',$($fingerprints.spell_target_position),'route',$($fingerprints.scenario_data))"
 $liveBefore=Query "SET SESSION group_concat_max_len=16777216;SELECT $snapshot;"
 $seed="$setup DELETE FROM lfg_entrances WHERE dungeonId=1028;"
 $others=$fingerprints.lfg_entrances.Replace('FROM lfg_entrances)','FROM lfg_entrances WHERE dungeonId<>1028)')
 $normal=Query "$seed SET @others=$others;SET @spell=$($fingerprints.spell_target_position);SET @route=$($fingerprints.scenario_data);$migration SET @once=$($fingerprints.lfg_entrances);$migration SELECT JSON_OBJECT('idempotent',@once=$($fingerprints.lfg_entrances),'others',@others=$others,'spell',@spell=$($fingerprints.spell_target_position),'route',@route=$($fingerprints.scenario_data),'target',(SELECT COUNT(*) FROM lfg_entrances WHERE dungeonId=1028 AND ABS(position_x-4916.3)<0.001 AND ABS(position_y+94)<0.001 AND ABS(position_z-66.3)<0.001 AND ABS(orientation-2.45)<0.001));"
 $result=$normal[-1]|ConvertFrom-Json;foreach($name in @('idempotent','others','spell','route','target')){if($result.$name -ne 1){throw "Migration changed unexpected data: $name"}}
 $mutations=@(
  'UPDATE spell_target_position SET target_position_x=target_position_x+1 WHERE id=187803;',
  'UPDATE spell_target_position SET target_map=1539 WHERE id=187803;',
  'DELETE FROM spell_target_position WHERE id=187803;',
  'UPDATE scenario_data SET ScenarioID=65000 WHERE ScenarioID=909;',
  'UPDATE scenario_data SET Team=1 WHERE ScenarioID=909;',
  'UPDATE scenario_data SET DifficultyID=12 WHERE ScenarioID=909;',
  "INSERT INTO lfg_entrances VALUES(1028,'custom',1,2,3,4);",
  "INSERT INTO lfg_entrances VALUES(1028,'custom zero',0,0,0,0);"
 )
 foreach($mutation in $mutations){$rows=Query "$seed $mutation SET @before=$snapshot;$migration SELECT JSON_OBJECT('unchanged',@before=$snapshot);" -Refusal;$result=$rows[-1]|ConvertFrom-Json;if($result.unchanged -ne 1){throw 'Refusal changed source/route/entrance data'}}
 $custom=Query "$seed $migration UPDATE lfg_entrances SET name='custom name retained' WHERE dungeonId=1028;SET @before=$snapshot;$migration SELECT JSON_OBJECT('unchanged',@before=$snapshot);"
 if(($custom[-1]|ConvertFrom-Json).unchanged -ne 1){throw 'Exact existing entrance custom name overwritten'}
 $liveAfter=Query "SET SESSION group_concat_max_len=16777216;SELECT $snapshot;";if(($liveBefore -join '') -ne ($liveAfter -join '')){throw 'Live tables changed during TEMP regression'}
 if($VerifyLiveData){
  $hash=[Convert]::ToHexString([Security.Cryptography.SHA1]::HashData([Text.UTF8Encoding]::new($false).GetBytes($migration.Replace("`r`n","`n")))).ToLowerInvariant();$file=[IO.Path]::GetFileName($MigrationFile)
  $live=Query "SELECT JSON_OBJECT('target',(SELECT COUNT(*) FROM lfg_entrances WHERE dungeonId=1028 AND ABS(position_x-4916.3)<0.001 AND ABS(position_y+94)<0.001 AND ABS(position_z-66.3)<0.001 AND ABS(orientation-2.45)<0.001),'released',(SELECT COUNT(*) FROM updates WHERE name='$file' AND hash='$hash' AND state='RELEASED'));"
  $result=$live[-1]|ConvertFrom-Json;if($result.target -ne 1 -or $result.released -ne 1){throw 'Live native entrance/updater RELEASED hash mismatch'}
 }
 'PASS full-schema TEMP Earthwarder entrance: native source/route, exact one-row insert, idempotence, unrelated full-table preservation,8 no-mutation refusals,custom-name preservation;live tables unchanged. Actual native login/terrain/client checks separate.'
}
