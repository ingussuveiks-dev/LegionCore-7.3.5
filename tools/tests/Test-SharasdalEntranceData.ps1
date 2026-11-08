param([string]$OutputDirectory='')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/sharasdal-entrance-data/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"};$out=[IO.Path]::GetFullPath($OutputDirectory)
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out) -Test {
 New-Item -ItemType Directory -Force $out|Out-Null
 $migration="$repo/sql/pending/world/2026_11_08_04_sharasdal_resume_entrance.sql";if(!(Test-Path $migration)){$migration="$repo/sql/updates/world/2026_11_08_04_sharasdal_resume_entrance.sql"};$body=Get-Content $migration -Raw
 $c=((Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}) -split '"')[1].Split(';');$old=$env:MYSQL_PWD
 try{$env:MYSQL_PWD=$c[3]
  $args=@("--host=$($c[0])","--port=$($c[1])","--user=$($c[2])","--database=$($c[4])",'--default-character-set=utf8mb4','--batch','--raw','--skip-column-names')
  $tables=[ordered]@{lfg_entrances='1';scenario_data='ScenarioID=1066';spell_target_position='id=207683';creature='guid=362063';creature_template='entry=102826';quest_objectives='QuestID=40341'};$columns=@{}
  foreach($table in $tables.Keys){$raw=@("SHOW COLUMNS FROM $table;"|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe @args 2> "$out/schema-$table.stderr");if($LASTEXITCODE -or (Get-Item "$out/schema-$table.stderr").Length){throw 'Native schema read failed'};$columns[$table]=@($raw|ForEach-Object {($_ -split "`t")[0]})}
  function Signature([string]$label,[string]$table,[string]$suffix='',[string]$where=''){
   $value=($columns[$table]|ForEach-Object {"COALESCE(HEX(CAST(``$_`` AS BINARY)),'NULL')"}) -join ',';$order=($columns[$table]|ForEach-Object {"``$_``"}) -join ','
   "SELECT '$label','$table$suffix',SHA2(COALESCE(GROUP_CONCAT(CONCAT_WS('|',$value) ORDER BY $order SEPARATOR '\n'),''),256) FROM $table $where;`n"
  }
  $sql="SET SESSION group_concat_max_len=67108864;`n"
  foreach($table in $tables.Keys){$sql+="CREATE TEMPORARY TABLE entrance_seed_$table LIKE $table;INSERT INTO entrance_seed_$table SELECT * FROM $table WHERE $($tables[$table]);CREATE TEMPORARY TABLE $table LIKE entrance_seed_$table;INSERT INTO $table SELECT * FROM entrance_seed_$table;`n"}
  # Replay the historical row in TEMP even after the live migration is applied.
  $sql+="DELETE FROM entrance_seed_lfg_entrances WHERE dungeonId=1229;INSERT INTO entrance_seed_lfg_entrances(dungeonId,name,position_x,position_y,position_z,orientation) VALUES(1229,NULL,836.16,1035.4,48.61,1.7935);`n"
  $cases=[ordered]@{original='';twice='';missing='DELETE FROM lfg_entrances WHERE dungeonId=1229;';correct_custom="UPDATE lfg_entrances SET name='custom canonical name',position_x=-629.42,position_y=808.68,position_z=247.67,orientation=6.2797 WHERE dungeonId=1229;"}
  $negative=[ordered]@{
   custom_name="UPDATE lfg_entrances SET name='custom old route' WHERE dungeonId=1229;"
   old_x='UPDATE lfg_entrances SET position_x=position_x+1 WHERE dungeonId=1229;'
   old_y='UPDATE lfg_entrances SET position_y=position_y+1 WHERE dungeonId=1229;'
   old_z='UPDATE lfg_entrances SET position_z=position_z+1 WHERE dungeonId=1229;'
   old_o='UPDATE lfg_entrances SET orientation=orientation+1 WHERE dungeonId=1229;'
   route_map='UPDATE scenario_data SET MapID=1495 WHERE ScenarioID=1066;'
   route_class='UPDATE scenario_data SET Class=1 WHERE ScenarioID=1066;'
   route_team='UPDATE scenario_data SET Team=469 WHERE ScenarioID=1066;'
   route_diff='UPDATE scenario_data SET DifficultyID=1 WHERE ScenarioID=1066;'
   route_lfg='UPDATE scenario_data SET LfgDungeonID=1243 WHERE ScenarioID=1066;'
   route_absent='DELETE FROM scenario_data;'
   portal_absent='DELETE FROM spell_target_position;'
   portal_map='UPDATE spell_target_position SET target_map=1332;'
   portal_x='UPDATE spell_target_position SET target_position_x=target_position_x+1;'
   portal_y='UPDATE spell_target_position SET target_position_y=target_position_y+1;'
   portal_z='UPDATE spell_target_position SET target_position_z=target_position_z+1;'
   portal_o='UPDATE spell_target_position SET target_orientation=target_orientation+1;'
   actor_absent='DELETE FROM creature;'
   actor_map='UPDATE creature SET map=1495;'
   actor_phase="UPDATE creature SET phaseId='7001';"
   actor_pose='UPDATE creature SET position_z=position_z+1;'
   actor_script="UPDATE creature_template SET ScriptName='custom_actor';"
   objective_absent='DELETE FROM quest_objectives WHERE ObjectID=105805;'
   objective_amount='UPDATE quest_objectives SET Amount=2 WHERE ObjectID=105805;'
   objective_type='UPDATE quest_objectives SET Type=1 WHERE ObjectID=105805;'
  }
  foreach($key in $negative.Keys){$cases[$key]=$negative[$key]}
  foreach($name in $cases.Keys){
   foreach($table in $tables.Keys){$sql+="DELETE FROM $table;INSERT INTO $table SELECT * FROM entrance_seed_$table;`n"}
   $sql+=$cases[$name]+"`n"
   foreach($table in $tables.Keys){$sql+=Signature "$name-before" $table};$sql+=Signature "$name-before" 'lfg_entrances' '-other' 'WHERE dungeonId<>1229'
   $sql+=$body+"`n";if($name -eq 'twice'){$sql+=$body+"`n"}
   foreach($table in $tables.Keys){$sql+=Signature "$name-after" $table};$sql+=Signature "$name-after" 'lfg_entrances' '-other' 'WHERE dungeonId<>1229'
   $sql+="SELECT '$name-after','location',COUNT(*)=1 AND MIN(ABS(position_x-(-629.42))<0.001 AND ABS(position_y-808.68)<0.001 AND ABS(position_z-247.67)<0.001 AND ABS(orientation-6.2797)<0.001) FROM lfg_entrances WHERE dungeonId=1229;`n"
  }
  [IO.File]::WriteAllText("$out/fixture.sql",$sql)
  # Continue only deliberate TEMP refusals so the same connection can verify
  # unchanged rows afterwards. Every error/status is checked and retained.
  $rows=@($sql|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe @args --force 2> "$out/fixture.stderr");$code=$LASTEXITCODE
  [IO.File]::WriteAllLines("$out/fixture.tsv",[string[]]$rows)
  $errors=@(Get-Content "$out/fixture.stderr"|Where-Object {$_ -match '^ERROR '});if($code -notin @(0,1) -or $errors.Count -ne $negative.Count -or @($errors|Where-Object {$_ -notmatch '^ERROR 1644 \(45000\).*Sharasdal resume entrance/portal/route/initial actor differ; review before updating'}).Count){throw 'Unexpected SQL status/refusal;original stderr retained'}
  $values=@{};foreach($row in $rows){$parts=$row -split "`t";if($parts.Count -eq 3){$key="$($parts[0])/$($parts[1])";if($values.ContainsKey($key)){throw 'Duplicate fixture result'};$values[$key]=$parts[2]}}
  foreach($name in $cases.Keys){
   foreach($table in @($tables.Keys)+@('lfg_entrances-other')){if($table -eq 'lfg_entrances' -and $name -in @('original','twice','missing')){continue};if(!$values.ContainsKey("$name-before/$table") -or $values["$name-before/$table"] -cne $values["$name-after/$table"]){throw "Preservation failed: $name/$table"}}
   if(!$negative.Contains($name) -and $values["$name-after/location"] -ne '1'){throw 'Accepted entrance not repaired'}
  }
  "PASS Sharasdal entrance TEMP$($cases.Count):original update/missing insert/idempotence/correct custom name,25 deliberate SQLSTATE45000 refusals,full-column graph and non1229 LFG preservation;no persistent world/character writes. Expected refusal stderr retained."
 }finally{$env:MYSQL_PWD=$old}
}
