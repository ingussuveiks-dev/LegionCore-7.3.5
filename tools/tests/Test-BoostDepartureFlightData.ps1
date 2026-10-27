param([string]$MigrationPath="$PSScriptRoot/../../sql/updates/world/2026_10_27_10_boost_departure_flight.sql",[switch]$VerifyLive)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..");$out="$repo/build-extractors/runtime-test/boost-departure-flight-data"
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -RuntimeDirectories $out -Test {
 $migration=Get-Content $MigrationPath -Raw
 $db=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $number=0
 function Query([string]$sql,[switch]$Expected){
  $script:number++;$stderr="$out/sql-stderr-$script:number.txt";$prior=$env:MYSQL_PWD;$priorErrors=$ErrorActionPreference
  try{$env:MYSQL_PWD=$db[3];$ErrorActionPreference='Continue';$rows=@($sql|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($db[0]) --port=$($db[1]) --user=$($db[2]) --database=$($db[4]) --batch --raw --skip-column-names --force 2> $stderr);$code=$LASTEXITCODE}finally{$env:MYSQL_PWD=$prior;$ErrorActionPreference=$priorErrors}
  $err=Get-Content $stderr -Raw
  if($Expected){if(@($err -split "`n"|Where-Object {$_ -match '^ERROR '}).Count -ne 1 -or $err -notmatch '1644 \(45000\)'){throw 'Expected custom bird-data refusal missing'}}elseif($code -or $err){throw 'Bird flight TEMP/read-only query failed'}
  $rows
 }
 $setup=@'
CREATE TEMPORARY TABLE creature_template AS SELECT * FROM creature_template WHERE entry IN(107543,112565,100448);
CREATE TEMPORARY TABLE bird_movement_shape LIKE creature_template_movement;
CREATE TEMPORARY TABLE creature_template_movement LIKE bird_movement_shape;
INSERT INTO creature_template_movement VALUES(107543,2,0,0,0,2,123),(100448,1,0,0,1,2,456);
'@
 function Snapshot([string]$label){"SELECT '$label-m',m.* FROM creature_template_movement m ORDER BY CreatureId;SELECT '$label-t',t.* FROM creature_template t ORDER BY entry;"}
 function Signature($rows,[string]$prefix){(@($rows|Where-Object {$_ -like "$prefix`t*"}|ForEach-Object {$_.Substring($prefix.Length+1)})) -join "`n"}
 $rows=Query "$setup $(Snapshot 'before') $migration $(Snapshot 'first') $migration $(Snapshot 'second') SELECT 'bird',CreatureId,Flight FROM creature_template_movement WHERE CreatureId IN(107543,112565) ORDER BY CreatureId; DELETE FROM creature_template_movement WHERE CreatureId=112565;UPDATE creature_template_movement SET Flight=0 WHERE CreatureId=107543;$(Snapshot 'normalized')"
 foreach($table in @('m','t')){
  if((Signature $rows "first-$table") -cne (Signature $rows "second-$table")){throw 'Bird migration not idempotent'}
  if((Signature $rows "before-$table") -cne (Signature $rows "normalized-$table")){throw 'Bird migration changed unrelated fields/data'}
 }
 foreach($line in @("bird`t107543`t1","bird`t112565`t1")){if($rows -notcontains $line){throw 'Both bird flight states not repaired'}}
 $changes=@(
  "UPDATE creature_template SET ScriptName='custom' WHERE entry=107543;",
  "UPDATE creature_template SET ScriptName='custom' WHERE entry=112565;",
  'DELETE FROM creature_template WHERE entry=107543;',
  'DELETE FROM creature_template WHERE entry=112565;',
  'UPDATE creature_template_movement SET Flight=2 WHERE CreatureId=107543;',
  'UPDATE creature_template_movement SET Flight=99 WHERE CreatureId=107543;'
 )
 foreach($change in $changes){$r=Query "$setup $change $(Snapshot 'before') $migration $(Snapshot 'after')" -Expected;foreach($t in @('m','t')){if((Signature $r "before-$t") -cne (Signature $r "after-$t")){throw 'Refused bird migration changed data'}}}
 if($VerifyLive){
  $sha=[Security.Cryptography.SHA1]::Create();try{$hash=[BitConverter]::ToString($sha.ComputeHash([Text.Encoding]::UTF8.GetBytes(([IO.File]::ReadAllText((Resolve-Path $MigrationPath))).Replace("`r`n","`n")))).Replace('-','')}finally{$sha.Dispose()}
  $r=Query "SET SESSION TRANSACTION READ ONLY;SELECT 'update',hash,state FROM updates WHERE name='2026_10_27_10_boost_departure_flight.sql';SELECT 'bird',CreatureId,Flight FROM creature_template_movement WHERE CreatureId IN(107543,112565) ORDER BY CreatureId;"
  foreach($line in @("update`t$hash`tRELEASED","bird`t107543`t1","bird`t112565`t1")){if($r -notcontains $line){throw 'Bird live updater hash/flight data differs'}}
 }
 'PASS bird Flight1 insertion/update/repeat, every other selected movement/template field retained, unrelated actor retained, six custom/absent-definition refusals; TEMP/read-only only, no character changes.'
}
