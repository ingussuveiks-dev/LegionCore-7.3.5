param([switch]$VerifyLiveData)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/marksmanship-recovery-data/$(Get-Date -Format yyyyMMdd-HHmmss-fff)";New-Item -ItemType Directory -Force $out|Out-Null
$migration="$repo/sql/pending/world/2026_11_07_03_marksmanship_return_recovery.sql";if(!(Test-Path $migration)){$migration="$repo/sql/updates/world/2026_11_07_03_marksmanship_return_recovery.sql"}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -Test {
 $c=((Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}) -split '"')[1].Split(';');$script:q=0
 function Query([string]$sql,[switch]$Refusal){$path="$out/query-$script:q";++$script:q;[IO.File]::WriteAllText("$path.sql",$sql);$old=$env:MYSQL_PWD
  try{$env:MYSQL_PWD=$c[3];$rows=@($sql|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names --force 2> "$path.stderr");$code=$LASTEXITCODE;[IO.File]::WriteAllLines("$path.stdout",[string[]]$rows);$error=[IO.File]::ReadAllText("$path.stderr")
   if($Refusal){if($error -notmatch 'Marksmanship recovery native graph differs; review before updating' -or @([regex]::Matches($error,'ERROR [0-9]+')).Count -ne 1){throw 'Unexpected SQL refusal'}}elseif($code -or $error){throw "SQL regression failed: $path.stderr"};return ,$rows
  }finally{$env:MYSQL_PWD=$old}
 }
 $filters=@{conditions='(SourceTypeOrReferenceId=22 AND SourceEntry IN(100397,100398)) OR (SourceTypeOrReferenceId=15 AND SourceGroup=19331)';smart_scripts='entryorguid IN(100397,100398) AND source_type=0';creature_template='entry IN(100397,100398)';quest_objectives='QuestID=40419';spell_script_names='spell_id IN(205040,198950)'}
 $hashes=@{};$setup='SET SESSION group_concat_max_len=16777216;'
 foreach($table in @('conditions','smart_scripts','creature_template','quest_objectives','spell_script_names')){
  $columns=@(Query "SHOW COLUMNS FROM $table;"|ForEach-Object {($_ -split "`t")[0]});$values=($columns|ForEach-Object {"COALESCE(HEX(CAST(``$_`` AS BINARY)),'NULL')"}) -join ',';$order=($columns|ForEach-Object {"``$_``"}) -join ','
  $hashes[$table]="(SELECT SHA2(COALESCE(GROUP_CONCAT(SHA2(CONCAT_WS('|',$values),256) ORDER BY $order SEPARATOR ''),''),256) FROM $table WHERE $($filters[$table]))"
  $setup+="CREATE TEMPORARY TABLE seed_$table LIKE $table;INSERT INTO seed_$table SELECT * FROM $table WHERE $($filters[$table]);CREATE TEMPORARY TABLE $table LIKE seed_$table;INSERT INTO $table SELECT * FROM seed_$table;"
 }
 $snapshot='JSON_OBJECT('+((@('conditions','smart_scripts','creature_template','quest_objectives','spell_script_names')|ForEach-Object {"'$_',$($hashes[$_])"}) -join ',')+')'
 $before=Query "SET SESSION group_concat_max_len=16777216;SELECT $snapshot;"
 $where='SourceTypeOrReferenceId=22 AND SourceGroup=1 AND SourceEntry=100397 AND SourceId=0 AND ElseGroup=0 AND ConditionTypeOrReference=32 AND ConditionTarget=0 AND ConditionValue1=16'
 $setup+="UPDATE conditions SET ScriptName='' WHERE $where;";$sql=[IO.File]::ReadAllText($migration)
 $normal=Query "$setup SET @before=$snapshot;$sql SET @once=$snapshot;$sql SELECT JSON_OBJECT('idempotent',@once=$snapshot,'bound',(SELECT COUNT(*) FROM conditions WHERE $where AND ScriptName='condition_marksmanship_recovered_arrival'));UPDATE conditions SET ScriptName='' WHERE $where;SELECT JSON_OBJECT('only_one_cell',@before=$snapshot);"
 if(($normal[-2]|ConvertFrom-Json).idempotent -ne 1 -or ($normal[-2]|ConvertFrom-Json).bound -ne 1 -or ($normal[-1]|ConvertFrom-Json).only_one_cell -ne 1){throw 'Migration changed more than the single binding cell'}
 $mutations=@("UPDATE conditions SET ScriptName='custom' WHERE $where;","DELETE FROM conditions WHERE $where;",'UPDATE conditions SET ConditionValue1=973 WHERE SourceTypeOrReferenceId=22 AND SourceGroup=1 AND SourceEntry=100397 AND ConditionTypeOrReference=44;',"UPDATE creature_template SET ScriptName='custom' WHERE entry=100397;","UPDATE creature_template SET AIName='custom' WHERE entry=100397;",'UPDATE creature_template SET gossip_menu_id=1 WHERE entry=100397;','UPDATE smart_scripts SET action_param1=1 WHERE entryorguid=100397 AND source_type=0 AND id=0;','DELETE FROM spell_script_names WHERE spell_id=205040;', 'UPDATE quest_objectives SET Amount=2 WHERE QuestID=40419 AND ObjectID=103490;','UPDATE quest_objectives SET StorageIndex=4 WHERE QuestID=40419 AND ObjectID=103490;',"UPDATE conditions SET ConditionValue1=8 WHERE $where;",'DELETE FROM creature_template WHERE entry=100397;')
 foreach($mutation in $mutations){$rows=Query "$setup $mutation SET @before=$snapshot;$sql SELECT JSON_OBJECT('unchanged',@before=$snapshot);" -Refusal;if(($rows[-1]|ConvertFrom-Json).unchanged -ne 1){throw 'Refusal mutated managed tables'}}
 $after=Query "SET SESSION group_concat_max_len=16777216;SELECT $snapshot;";if(($before -join '') -ne ($after -join '')){throw 'Live records changed during TEMP regression'}
 if($VerifyLiveData){$hash=[Convert]::ToHexString([Security.Cryptography.SHA1]::HashData([Text.UTF8Encoding]::new($false).GetBytes($sql.Replace("`r`n","`n")))).ToLowerInvariant();$name=[IO.Path]::GetFileName($migration);$rows=Query "SELECT JSON_OBJECT('bound',(SELECT COUNT(*) FROM conditions WHERE $where AND ScriptName='condition_marksmanship_recovered_arrival'),'released',(SELECT COUNT(*) FROM updates WHERE name='$name' AND hash='$hash' AND state='RELEASED'));";$live=$rows[-1]|ConvertFrom-Json;if($live.bound -ne 1 -or $live.released -ne 1){throw 'Live binding/RELEASED hash mismatch'}}
 'PASS recovery data:full native schemas/scoped records,exactly one conditions binding cell,idempotence,12 explicit no-mutation graph/source/objective refusals,other actor/return/menu/objective rows preserved;live records unchanged by TEMP tests'
}
