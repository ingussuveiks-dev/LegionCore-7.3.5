param([string]$MigrationFile,[switch]$VerifyLiveData)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/hodir-landing-data/$(Get-Date -Format yyyyMMdd-HHmmss-fff)";New-Item -ItemType Directory -Force $out|Out-Null
if(!$MigrationFile){$MigrationFile="$repo/sql/pending/world/2026_11_08_03_hodir_landing_admission.sql";if(!(Test-Path $MigrationFile)){$MigrationFile="$repo/sql/updates/world/2026_11_08_03_hodir_landing_admission.sql"}}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out) -Test {
 $migration=[IO.File]::ReadAllText($MigrationFile);$c=((Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}) -split '"')[1].Split(';');$script:query=0
 function Query([string]$sql,[switch]$Refusal){
  $path="$out/sql-$script:query";++$script:query;[IO.File]::WriteAllText("$path.sql",$sql);$old=$env:MYSQL_PWD
  try{$env:MYSQL_PWD=$c[3];$rows=@($sql|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --default-character-set=utf8mb4 --batch --raw --skip-column-names --force 2> "$path.stderr");$code=$LASTEXITCODE;[IO.File]::WriteAllLines("$path.stdout",[string[]]$rows);$errors=[IO.File]::ReadAllText("$path.stderr")
   if($Refusal){if($errors -notmatch 'Hodir landing graph/route differ; review before updating' -or @([regex]::Matches($errors,'ERROR [0-9]+')).Count -ne 1){throw "Unexpected refusal: $path.stderr"}}
   elseif($code -or $errors){throw "SQL regression failed: $path.stderr"};return ,$rows
  }finally{$env:MYSQL_PWD=$old}
 }
 $scopes=[ordered]@{smart_scripts='entryorguid=157 AND source_type IN(10,13)';conditions='SourceTypeOrReferenceId=22 AND SourceEntry=157';scenario_data='ScenarioID=1146';eventobject='id=157';eventobject_template='entry=157'}
 $fingerprints=@{};$setup='SET SESSION group_concat_max_len=16777216;'
 foreach($table in $scopes.Keys){
  $columns=@(Query "SHOW COLUMNS FROM $table;"|ForEach-Object {($_ -split "`t")[0]});$values=($columns|ForEach-Object {"COALESCE(HEX(CAST(``$_`` AS BINARY)),'NULL')"}) -join ',';$order=($columns|ForEach-Object {"``$_``"}) -join ','
  $fingerprints[$table]="(SELECT SHA2(COALESCE(GROUP_CONCAT(CONCAT_WS('|',$values) ORDER BY $order SEPARATOR '\n'),''),256) FROM $table WHERE $($scopes[$table]))"
  $setup+="CREATE TEMPORARY TABLE landing_$table LIKE $table;INSERT INTO landing_$table SELECT * FROM $table WHERE $($scopes[$table]);ALTER TABLE landing_$table RENAME TO $table;"
 }
 $snapshot='JSON_OBJECT('+(@($scopes.Keys|ForEach-Object {"'$_',$($fingerprints[$_])"}) -join ',')+')'
 $liveBefore=Query "SET SESSION group_concat_max_len=16777216;SELECT $snapshot;"
 $seed="$setup UPDATE conditions SET SourceId=10 WHERE SourceTypeOrReferenceId=22 AND SourceEntry=157;UPDATE smart_scripts SET event_type=89 WHERE entryorguid=157 AND source_type=13 AND id=1;"
 $expected='UPDATE conditions SET SourceId=13 WHERE SourceTypeOrReferenceId=22 AND SourceEntry=157;UPDATE smart_scripts SET event_type=61 WHERE entryorguid=157 AND source_type=13 AND id=1;'
 $normal=Query "$seed $expected SET @expected=$snapshot;UPDATE conditions SET SourceId=10 WHERE SourceTypeOrReferenceId=22 AND SourceEntry=157;UPDATE smart_scripts SET event_type=89 WHERE entryorguid=157 AND source_type=13 AND id=1;$migration SELECT @expected=$snapshot;SET @once=$snapshot;$migration SELECT @once=$snapshot;"
 if($normal.Count -ne 2 -or @($normal|Where-Object {$_ -ne '1'}).Count){throw 'Not exactly the two intended cells or idempotence failed'}
 $mutations=@(
  'DELETE FROM smart_scripts WHERE entryorguid=157 AND source_type=13 AND id=1;',
  'UPDATE smart_scripts SET id=2 WHERE entryorguid=157 AND source_type=13 AND id=1;',
  'UPDATE smart_scripts SET source_type=10 WHERE entryorguid=157 AND source_type=13;',
  'INSERT INTO smart_scripts SELECT entryorguid,10,id,link,Difficulties,event_type,event_phase_mask,event_chance,event_flags,event_param1,event_param2,event_param3,event_param4,event_param5,action_type,action_param1,action_param2,action_param3,action_param4,action_param5,action_param6,target_type,target_param1,target_param2,target_param3,target_param4,target_x,target_y,target_z,target_o,comment FROM smart_scripts WHERE entryorguid=157 AND source_type=13 AND id=0;',
  'DELETE FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=157;',
  'INSERT INTO conditions SELECT SourceTypeOrReferenceId,2,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=157;',
  'DELETE FROM scenario_data WHERE ScenarioID=1146;',
  'DELETE FROM eventobject WHERE id=157;',
  'DELETE FROM eventobject_template WHERE entry=157;'
 )
 foreach($id in 0,1){
  $values=@{link=7;Difficulties="'12'";event_type=90;event_phase_mask=1;event_chance=99;event_flags=1;event_param1=1;event_param2=1;event_param3=1;event_param4=1;event_param5=1;action_type=1;action_param1=1;action_param2=1;action_param3=1;action_param4=1;action_param5=1;action_param6=1;target_type=1;target_param1=1;target_param2=1;target_param3=1;target_param4=1;target_x=1;target_y=1;target_z=1;target_o=1}
  foreach($key in $values.Keys){$mutations+="UPDATE smart_scripts SET ``$key``=$($values[$key]) WHERE entryorguid=157 AND source_type=13 AND id=$id;"}
 }
 $columns=@{SourceGroup=2;SourceId=9;ElseGroup=1;ConditionTypeOrReference=1;ConditionTarget=1;ConditionValue1=972;ConditionValue2=3;ConditionValue3=1;NegativeCondition=1;ErrorTextId=1;ScriptName="'custom'"}
 foreach($key in $columns.Keys){$mutations+="UPDATE conditions SET ``$key``=$($columns[$key]) WHERE SourceTypeOrReferenceId=22 AND SourceEntry=157;"}
 foreach($key in @('MapID','Class','Team','DifficultyID','LfgDungeonID')){$mutations+="UPDATE scenario_data SET ``$key``=``$key``+1 WHERE ScenarioID=1146;"}
 foreach($key in @('guid','map','spawnMask','phaseMask','PhaseId','position_x','position_y','position_z','orientation')){$mutations+="UPDATE eventobject SET ``$key``=``$key``+1 WHERE id=157;"}
 foreach($key in @('radius','SpellID','WorldSafeLocID','Flags')){$mutations+="UPDATE eventobject_template SET ``$key``=``$key``+1 WHERE entry=157;"}
 $mutations+="UPDATE eventobject_template SET ScriptName='custom' WHERE entry=157;"
 foreach($mutation in $mutations){$rows=Query "$seed $mutation SET @before=$snapshot;$migration SELECT @before=$snapshot;" -Refusal;if($rows[-1] -ne '1'){throw 'Refused migration mutated data'}}
 foreach($partial in @('UPDATE conditions SET SourceId=13 WHERE SourceEntry=157;','UPDATE smart_scripts SET event_type=61 WHERE id=1;')){$rows=Query "$seed $partial $migration SELECT $snapshot;";if($rows[-1] -ne ((Query "$seed $expected SELECT $snapshot;")[-1])){throw 'Partial repair did not normalize exactly'}}
 $custom=Query "$seed UPDATE smart_scripts SET comment='custom retained';UPDATE conditions SET Comment='custom retained';$expected SET @expected=$snapshot;UPDATE conditions SET SourceId=10 WHERE SourceEntry=157;UPDATE smart_scripts SET event_type=89 WHERE id=1;$migration SELECT @expected=$snapshot;";if($custom[-1] -ne '1'){throw 'Harmless comments changed'}
 $liveAfter=Query "SET SESSION group_concat_max_len=16777216;SELECT $snapshot;";if(($liveBefore -join '') -ne ($liveAfter -join '')){throw 'Live tables changed by TEMP tests'}
 if($VerifyLiveData){$hash=[Convert]::ToHexString([Security.Cryptography.SHA1]::HashData([Text.UTF8Encoding]::new($false).GetBytes($migration.Replace("`r`n","`n")))).ToLowerInvariant();$file=[IO.Path]::GetFileName($MigrationFile);$rows=Query "SELECT COUNT(*) FROM updates WHERE name='$file' AND hash='$hash' AND state='RELEASED';SELECT COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=1 AND SourceEntry=157 AND SourceId=13 AND ConditionTypeOrReference=44 AND ConditionValue1=1146 AND ConditionValue2=2;SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=157 AND source_type=13 AND id=1 AND event_type=61;";if($rows.Count -ne 3 -or @($rows|Where-Object {$_ -ne '1'}).Count){throw 'Live repaired graph/RELEASED hash differs'}}
 "PASS Hodir landing full-schema TEMP:exact two cells/all remaining bytes preserved,idempotence,partial normalization,custom comments,$($mutations.Count) no-mutation graph/condition/route/event/template refusals;live scoped data unchanged. Client flight/whole campaign separate."
}
