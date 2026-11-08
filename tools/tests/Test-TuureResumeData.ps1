param([string]$MigrationFile,[switch]$VerifyLiveData)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/tuure-resume-data/$(Get-Date -Format yyyyMMdd-HHmmss-fff)";New-Item -ItemType Directory -Force $out|Out-Null
if(!$MigrationFile){$MigrationFile="$repo/sql/pending/world/2026_11_08_02_tuure_resume_entrance.sql";if(!(Test-Path $MigrationFile)){$MigrationFile="$repo/sql/updates/world/2026_11_08_02_tuure_resume_entrance.sql"}}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -Test {
 $migration=[IO.File]::ReadAllText($MigrationFile);$c=((Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}) -split '"')[1].Split(';');$script:query=0
 function Query([string]$sql,[switch]$Refusal){
  $path="$out/sql-$script:query";++$script:query;[IO.File]::WriteAllText("$path.sql",$sql);$old=$env:MYSQL_PWD
  try{$env:MYSQL_PWD=$c[3];$rows=@($sql|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --default-character-set=utf8mb4 --batch --raw --skip-column-names --force 2> "$path.stderr");$code=$LASTEXITCODE;[IO.File]::WriteAllLines("$path.stdout",[string[]]$rows);$errors=[IO.File]::ReadAllText("$path.stderr")
   if($Refusal){if($errors -notmatch 'Tuure resume entrance/route/initial actor differ; review before updating' -or @([regex]::Matches($errors,'ERROR [0-9]+')).Count -ne 1){throw "Unexpected refusal: $path.stderr"}}
   elseif($code -or $errors){throw "SQL regression failed: $path.stderr"};return ,$rows
  }finally{$env:MYSQL_PWD=$old}
 }
 $tables=@('lfg_entrances','scenario_data','spell_target_position','creature','creature_template');$fingerprints=@{};$setup='SET SESSION group_concat_max_len=16777216;'
 foreach($table in $tables){
  $columns=@(Query "SHOW COLUMNS FROM $table;"|ForEach-Object {($_ -split "`t")[0]});$values=($columns|ForEach-Object {"COALESCE(HEX(CAST(``$_`` AS BINARY)),'NULL')"}) -join ',';$order=($columns|ForEach-Object {"``$_``"}) -join ','
  $scope=switch($table){'creature'{' WHERE guid IN(361419,361428)'}'creature_template'{' WHERE entry IN(106134,100397)'}default{''}}
  $fingerprints[$table]="(SELECT SHA2(COALESCE(GROUP_CONCAT(CONCAT_WS('|',$values) ORDER BY $order SEPARATOR '\n'),''),256) FROM $table$scope)"
  $setup+="CREATE TEMPORARY TABLE resume_$table LIKE $table;INSERT INTO resume_$table SELECT * FROM $table$scope;ALTER TABLE resume_$table RENAME TO $table;"
 }
 $snapshot="JSON_OBJECT('lfg',$($fingerprints.lfg_entrances),'route',$($fingerprints.scenario_data),'spell',$($fingerprints.spell_target_position),'actor',$($fingerprints.creature),'template',$($fingerprints.creature_template))"
 $liveBefore=Query "SET SESSION group_concat_max_len=16777216;SELECT $snapshot;"
 $seed="$setup DELETE FROM lfg_entrances WHERE dungeonId=1243;INSERT INTO lfg_entrances VALUES(1243,NULL,-828.85,4372,738.64,1.86768);"
 $other=$fingerprints.lfg_entrances.Replace('FROM lfg_entrances)','FROM lfg_entrances WHERE dungeonId<>1243)')
 $target="(SELECT COUNT(*) FROM lfg_entrances WHERE dungeonId=1243 AND ABS(position_x-(-43.1440964))<0.001 AND ABS(position_y-1160.33679)<0.001 AND ABS(position_z-(-74.353569))<0.001 AND ABS(orientation-0.483893174)<0.001)"
 $normal=Query "$seed SET @other=$other;SET @route=$($fingerprints.scenario_data);SET @spell=$($fingerprints.spell_target_position);SET @actor=$($fingerprints.creature);SET @template=$($fingerprints.creature_template);$migration SET @once=$snapshot;$migration SELECT JSON_OBJECT('idempotent',@once=$snapshot,'others',@other=$other,'route',@route=$($fingerprints.scenario_data),'spell',@spell=$($fingerprints.spell_target_position),'actor',@actor=$($fingerprints.creature),'template',@template=$($fingerprints.creature_template),'target',$target,'name',(SELECT name IS NULL FROM lfg_entrances WHERE dungeonId=1243));"
 $result=$normal[-1]|ConvertFrom-Json;foreach($name in @('idempotent','others','route','spell','actor','template','target','name')){if($result.$name -ne 1){throw "Unexpected migration change: $name"}}
 $mutations=@(
  "UPDATE lfg_entrances SET name='custom legacy' WHERE dungeonId=1243;",
  'UPDATE lfg_entrances SET position_x=position_x+1 WHERE dungeonId=1243;',
  'UPDATE lfg_entrances SET position_y=position_y+1 WHERE dungeonId=1243;',
  'UPDATE lfg_entrances SET position_z=position_z+1 WHERE dungeonId=1243;',
  'UPDATE lfg_entrances SET orientation=orientation+1 WHERE dungeonId=1243;',
  'UPDATE lfg_entrances SET position_x=0,position_y=0,position_z=0,orientation=0 WHERE dungeonId=1243;',
  'ALTER TABLE lfg_entrances MODIFY position_x FLOAT NULL;UPDATE lfg_entrances SET position_x=NULL WHERE dungeonId=1243;',
  'UPDATE scenario_data SET Class=2 WHERE ScenarioID=1085;',
  'UPDATE scenario_data SET Team=1 WHERE ScenarioID=1085;',
  'UPDATE scenario_data SET DifficultyID=12 WHERE ScenarioID=1085;',
  'UPDATE scenario_data SET LfgDungeonID=1243 WHERE ScenarioID=1085;',
  'UPDATE scenario_data SET MapID=1539 WHERE ScenarioID=1085;',
  'DELETE FROM scenario_data WHERE ScenarioID=1085;',
  'UPDATE scenario_data SET ScenarioID=65000 WHERE ScenarioID=1085;',
  'UPDATE creature SET map=1539 WHERE guid=361419;',
  'UPDATE creature SET id=100397 WHERE guid=361419;',
  'UPDATE creature SET phaseId=6375 WHERE guid=361419;',
  'UPDATE creature SET position_x=position_x+1 WHERE guid=361419;',
  'DELETE FROM creature WHERE guid=361419;',
  "UPDATE creature_template SET ScriptName='custom' WHERE entry=106134;",
  'DELETE FROM creature_template WHERE entry=106134;',
  'UPDATE lfg_entrances SET position_x=251.07,position_y=1956.25,position_z=-53.22,orientation=5.89799 WHERE dungeonId=1243;'
 )
 foreach($mutation in $mutations){$rows=Query "$seed $mutation SET @before=$snapshot;$migration SELECT JSON_OBJECT('unchanged',@before=$snapshot);" -Refusal;if(($rows[-1]|ConvertFrom-Json).unchanged -ne 1){throw 'Refused migration mutated managed data'}}
 $custom=Query "$seed $migration UPDATE lfg_entrances SET name='custom correct name' WHERE dungeonId=1243;SET @before=$snapshot;$migration SELECT @before=$snapshot;";if($custom[-1] -ne '1'){throw 'Correct custom entrance overwritten'}
 $absent=Query "$setup DELETE FROM lfg_entrances WHERE dungeonId=1243;$migration SELECT $target;";if($absent[-1] -ne '1'){throw 'Absent entrance not inserted'}
 $liveAfter=Query "SET SESSION group_concat_max_len=16777216;SELECT $snapshot;";if(($liveBefore -join '') -ne ($liveAfter -join '')){throw 'Live tables changed by TEMP tests'}
 if($VerifyLiveData){$hash=[Convert]::ToHexString([Security.Cryptography.SHA1]::HashData([Text.UTF8Encoding]::new($false).GetBytes($migration.Replace("`r`n","`n")))).ToLowerInvariant();$file=[IO.Path]::GetFileName($MigrationFile);$live=Query "SELECT JSON_OBJECT('target',$target,'released',(SELECT COUNT(*) FROM updates WHERE name='$file' AND hash='$hash' AND state='RELEASED'));";$result=$live[-1]|ConvertFrom-Json;if($result.target -ne 1 -or $result.released -ne 1){throw 'Live entrance/RELEASED hash differs'}}
 'PASS Tuure entrance full-schema TEMP:exact four legacy-coordinate cells/name retained,absent insertion,idempotence,whole other entrances/routes/spells and scoped actors/templates preserved,22 no-mutation refusals including custom legacy/nullable/Hodir;live data unchanged. Native5480/floor/client separate.'
}
