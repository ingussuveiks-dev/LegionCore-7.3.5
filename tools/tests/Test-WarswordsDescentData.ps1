param([switch]$VerifyLiveData)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$migration="$repo/sql/pending/2026_11_06_05_warswords_descent.sql";if(!(Test-Path $migration)){$migration="$repo/sql/updates/world/2026_11_06_05_warswords_descent.sql"}
$out="$repo/build-extractors/runtime-test/warswords-descent-data/$(Get-Date -Format yyyyMMdd-HHmmss-fff)";New-Item -ItemType Directory -Force $out|Out-Null
$db=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
function Run([string]$caseName,[string]$sql){
 [IO.File]::WriteAllText("$out/$caseName.sql",$sql);$old=$env:MYSQL_PWD
 try{$env:MYSQL_PWD=$db[3];& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -Arguments @("--host=$($db[0])","--port=$($db[1])","--user=$($db[2])","--database=$($db[4])",'--batch','--raw','--skip-column-names') -InputFile "$out/$caseName.sql" -OutputFile "$out/$caseName.tsv" -ErrorFile "$out/$caseName.stderr"}finally{$env:MYSQL_PWD=$old}
}
$setup='CREATE TEMPORARY TABLE saved_landing LIKE spell_target_position;INSERT INTO saved_landing SELECT * FROM spell_target_position;CREATE TEMPORARY TABLE spell_target_position LIKE saved_landing;INSERT INTO spell_target_position SELECT * FROM saved_landing;'
$body=Get-Content $migration -Raw
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out) -Test {
 if(Run 'before' 'SELECT * FROM spell_target_position ORDER BY id;'){throw 'Original native target read failed'}
 $query="SELECT 'landing',id,target_map,ROUND(target_position_x,2),ROUND(target_position_y,2),ROUND(target_position_z,2),target_orientation FROM spell_target_position WHERE id=205744;SELECT 'other',COUNT(*) FROM (SELECT * FROM spell_target_position WHERE id<>205744 EXCEPT SELECT * FROM saved_landing WHERE id<>205744) changed;"
 if(Run 'repeat' ($setup+$body+$body+$query)){throw 'TEMP native landing migration failed'}
 $rows=Get-Content "$out/repeat.tsv";if("landing`t205744`t1511`t3386.46`t1406.94`t68.94`t0" -notin $rows -or "other`t0" -notin $rows){throw 'Native landing/idempotency/other-row preservation differs'}
 foreach($change in @('target_map=1220','target_position_x=1','target_position_y=1','target_position_z=1','target_orientation=1')){
  $caseName=($change -split '=')[0]
  $code=Run $caseName ($setup+$body+"UPDATE spell_target_position SET $change WHERE id=205744;"+$body)
  if(!$code -or (Get-Content "$out/$caseName.stderr" -Raw) -notmatch 'ERROR 1644 \(45000\)'){throw 'Custom native landing was not rejected'}
 }
 if(Run 'after' 'SELECT * FROM spell_target_position ORDER BY id;'){throw 'Live target readback failed'}
 if((Get-FileHash "$out/before.tsv").Hash -ne (Get-FileHash "$out/after.tsv").Hash){throw 'TEMP test changed live target data'}
 if($VerifyLiveData){$sha=(Get-FileHash $migration -Algorithm SHA1).Hash.ToLowerInvariant();if(Run 'live' ($query.Replace('SELECT ''other'',COUNT(*) FROM (SELECT * FROM spell_target_position WHERE id<>205744 EXCEPT SELECT * FROM saved_landing WHERE id<>205744) changed;','')+"SELECT 'update',hash,state FROM updates WHERE name='2026_11_06_05_warswords_descent.sql';")){throw 'Updater readback failed'};$live=Get-Content "$out/live.tsv";if("update`t$sha`tRELEASED" -notin $live -or "landing`t205744`t1511`t3386.46`t1406.94`t68.94`t0" -notin $live){throw 'Actual updater/hash/native landing differ'}}
 'PASS MariaDB TEMP native205744 landing/idempotency/other-row preservation and five foreign-data rejection controls; live data unchanged by test. Actual updater checked only with VerifyLiveData.'
}
