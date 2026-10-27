param([string]$MigrationPath="$PSScriptRoot/../../sql/updates/world/2026_10_27_03_lights_wrath_chest_lifetime.sql",[switch]$VerifyLive)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..");$out="$repo/build-extractors/runtime-test/lights-wrath-chest-data"
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -RuntimeDirectories $out -Test {
 $migration=Get-Content $MigrationPath -Raw
 $db=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $filters=[ordered]@{smart_scripts='entryorguid IN(104985,104986)';creature_template='entry=104985';gameobject_template='entry=248572'}
 $setup='';foreach($t in $filters.Keys){$setup+="CREATE TEMPORARY TABLE saved_$t LIKE $t;INSERT INTO saved_$t SELECT * FROM $t WHERE $($filters[$t]);CREATE TEMPORARY TABLE $t LIKE saved_$t;INSERT INTO $t SELECT * FROM saved_$t;`n"}
 $setup+='UPDATE smart_scripts SET action_param3=0 WHERE entryorguid=104985 AND source_type=9 AND id=2;UPDATE saved_smart_scripts SET action_param3=4 WHERE entryorguid=104985 AND source_type=9 AND id=2;'
 function Snapshot([string]$label){($filters.Keys|ForEach-Object {"SELECT '$label-$_',a.* FROM $_ a;"}) -join "`n"}
 function Signature($rows,[string]$label){(@($rows|Where-Object {$_ -like "$label`t*"}|ForEach-Object {$_.Substring($label.Length+1)}|Sort-Object -CaseSensitive)) -join "`n"}
 function Query([string]$sql,[switch]$Expected){$prior=$env:MYSQL_PWD;try{$env:MYSQL_PWD=$db[3];$rows=@($sql|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($db[0]) --port=$($db[1]) --user=$($db[2]) --database=$($db[4]) --batch --raw --skip-column-names --force 2> "$out/sql-stderr.txt");$code=$LASTEXITCODE}finally{$env:MYSQL_PWD=$prior};$err=Get-Content "$out/sql-stderr.txt" -Raw;if($Expected){if($err -notmatch '1644 \(45000\)' -or @($err -split "`n"|Where-Object {$_ -match '^ERROR '}).Count -ne 1){throw 'Expected chest custom refusal missing'}}elseif($code -or $err){throw 'Chest TEMP/read-only query failed'};$rows}
 $query="$setup`n$migration`n$(Snapshot 'first')`n$migration`n$(Snapshot 'second')"
 foreach($t in $filters.Keys){$query+="SELECT 'expected-$t',a.* FROM saved_$t a;"}
 $rows=Query $query
 foreach($t in $filters.Keys){if((Signature $rows "first-$t") -cne (Signature $rows "second-$t") -or (Signature $rows "first-$t") -cne (Signature $rows "expected-$t")){throw 'Non-idempotent/overbroad chest migration'}}
 foreach($change in @("UPDATE creature_template SET ScriptName='custom';",'UPDATE gameobject_template SET Data1=1;','UPDATE smart_scripts SET action_param3=8 WHERE entryorguid=104985 AND source_type=9 AND id=2;','UPDATE smart_scripts SET action_param2=3600 WHERE entryorguid=104985 AND source_type=9 AND id=2;','UPDATE smart_scripts SET target_type=1 WHERE entryorguid=104985 AND source_type=9 AND id=2;','UPDATE smart_scripts SET action_param1=1 WHERE entryorguid=104985 AND source_type=9 AND id=3;','DELETE FROM smart_scripts WHERE entryorguid=104985 AND source_type=9 AND id=0;')){
  $r=Query "$setup`n$change`n$(Snapshot 'before')`n$migration`n$(Snapshot 'after')" -Expected
  foreach($t in $filters.Keys){if((Signature $r "before-$t") -cne (Signature $r "after-$t")){throw 'Refused chest migration altered native/custom data'}}
 }
 if($VerifyLive){
  $sha=[Security.Cryptography.SHA1]::Create();try{$hash=[BitConverter]::ToString($sha.ComputeHash([Text.Encoding]::UTF8.GetBytes(([IO.File]::ReadAllText((Resolve-Path $MigrationPath))).Replace("`r`n","`n")))).Replace('-','')}finally{$sha.Dispose()}
  $live=Query "SET SESSION TRANSACTION READ ONLY;SELECT 'update',hash,state FROM updates WHERE name='2026_10_27_03_lights_wrath_chest_lifetime.sql'; SELECT 'summon',action_param1,action_param2,action_param3,target_type FROM smart_scripts WHERE entryorguid=104985 AND source_type=9 AND id=2; SELECT 'other-no-creator',COUNT(*) FROM smart_scripts WHERE action_type=50 AND (action_param3 & 4)<>0 AND NOT(entryorguid=104985 AND source_type=9 AND id=2);"
  foreach($e in @("update`t$hash`tRELEASED","summon`t248572`t0`t4`t8","other-no-creator`t0")){if($live -notcontains $e){throw 'Live chest flag/hash differs'}}
 }
 'PASS chest TEMP update twice changes only original104985/source9/id2 flag0->4, retaining lifetime0/pose/whole native graph/nearby actors/templates; seven custom refusals preserve all three tables. No character change.'
}
