param([string]$MigrationPath="$PSScriptRoot/../../sql/updates/world/2026_10_26_14_tuure_scene_equip.sql",[switch]$VerifyLive)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/tuure-presentation-data"
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -RuntimeDirectories @($out) -Test {
 $migration=Get-Content -LiteralPath $MigrationPath -Raw
 $setup="CREATE TEMPORARY TABLE saved_scene LIKE spell_scene;INSERT INTO saved_scene SELECT * FROM spell_scene WHERE MiscValue=1265;CREATE TEMPORARY TABLE spell_scene LIKE saved_scene;INSERT INTO spell_scene SELECT * FROM saved_scene;UPDATE spell_scene SET ScriptName=NULL;CREATE TEMPORARY TABLE saved_events LIKE spell_scene_event;CREATE TEMPORARY TABLE spell_scene_event LIKE saved_events;"
 function Snapshot([string]$label){"SELECT '$label-count',COUNT(*) FROM spell_scene;SELECT '$label-scene',SceneScriptPackageID,MiscValue,PlaybackFlags,CustomDuration,COALESCE(ScriptName,'NULL'),comment FROM spell_scene ORDER BY 2,3;SELECT '$label-event',MiscValue,Event,trigerSpell,MonsterCredit,comment FROM spell_scene_event ORDER BY 2,3;"}
 $db=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $prior=$env:MYSQL_PWD
 try{
  $env:MYSQL_PWD=$db[3]
  $sql="$setup`n$migration`n$migration`n$(Snapshot 'bound')"
  $rows=@($sql|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($db[0]) --port=$($db[1]) --user=$($db[2]) --database=$($db[4]) --batch --raw --skip-column-names)
  if($LASTEXITCODE){throw 'Tuure TEMP idempotence failed'}
  if($rows -notcontains "bound-count`t1" -or !@($rows|Where-Object {$_ -match '^bound-scene\t1565\t1265\t21\t0\tscene_tuure_pickup\t'}).Count){throw 'TEMP binding changed native metadata'}
  [IO.File]::WriteAllLines("$out/idempotent.tsv",[string[]]$rows)
  $variants=[ordered]@{
   handler="UPDATE spell_scene SET ScriptName='custom' WHERE MiscValue=1265;"
   package='UPDATE spell_scene SET SceneScriptPackageID=1 WHERE MiscValue=1265;'
   flags='UPDATE spell_scene SET PlaybackFlags=22 WHERE MiscValue=1265;'
   duration='UPDATE spell_scene SET CustomDuration=1 WHERE MiscValue=1265;'
   event="INSERT INTO spell_scene_event VALUES(1265,'complete',1,0,'custom');"
   missing='DELETE FROM spell_scene WHERE MiscValue=1265;'
   duplicate='INSERT INTO spell_scene SELECT SceneScriptPackageID+1,MiscValue,PlaybackFlags,CustomDuration,ScriptName,comment FROM saved_scene;'
  }
  foreach($name in $variants.Keys){
   $sql=if($name -eq 'duplicate'){"$setup`n$(Snapshot 'before')`n$($variants[$name])`n$(Snapshot 'after')`n$migration`n$(Snapshot 'bound')"}else{"$setup`n$($variants[$name])`n$(Snapshot 'before')`n$migration`n$(Snapshot 'after')"}
   # --force is used only for this explicitly expected refusal, so after-state can be inspected.
   $stderr="$out/expected-$name-stderr.txt"
   $rows=@($sql|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($db[0]) --port=$($db[1]) --user=$($db[2]) --database=$($db[4]) --batch --raw --skip-column-names --force 2> $stderr)
   $errorText=Get-Content $stderr -Raw
   if($name -eq 'duplicate'){
    if($errorText -notmatch '1062 \(23000\)' -or $errorText -notmatch "Duplicate entry '1265'"){throw 'Native unique scene index did not refuse duplicate'}
    if(!@($rows|Where-Object {$_ -match '^bound-scene\t1565\t1265\t21\t0\tscene_tuure_pickup\t'}).Count){throw 'Migration failed after native duplicate refusal'}
   }elseif($errorText -notmatch '1644 \(45000\)' -or $errorText -notmatch 'Tuure scene1265 custom/missing'){throw "Missing explicit custom-data refusal: $name"}
   $before=@($rows|Where-Object {$_ -like 'before-*'}|ForEach-Object {$_.Replace('before-','')})
   $after=@($rows|Where-Object {$_ -like 'after-*'}|ForEach-Object {$_.Replace('after-','')})
   if(($before -join "`n") -cne ($after -join "`n")){throw "Refused migration changed TEMP data: $name"}
   [IO.File]::WriteAllLines("$out/expected-$name.tsv",[string[]]$rows)
   "PASS expected counterfactual refusal: $name; TEMP scene/events unchanged"
  }
  if($VerifyLive){
   # Native Windows UpdateFetcher::ReadSQLUpdate uses text-mode ifstream (CRLF -> LF).
   # Match its stored hash; do not rewrite already applied SQL bytes to fit a raw-file hash.
   $updater=Get-Content "$repo/src/server/database/Updater/UpdateFetcher.cpp" -Raw
   if(!$updater.Contains('std::ifstream in(file.c_str());')){throw 'Native SQL hash read mode changed'}
   $sha=[Security.Cryptography.SHA1]::Create()
   try{$hash=[BitConverter]::ToString($sha.ComputeHash([Text.Encoding]::UTF8.GetBytes(([IO.File]::ReadAllText((Resolve-Path $MigrationPath))).Replace("`r`n","`n")))).Replace('-','')}finally{$sha.Dispose()}
   $rows=@("SELECT 'update',hash,state FROM updates WHERE name='2026_10_26_14_tuure_scene_equip.sql';SELECT 'scene',SceneScriptPackageID,MiscValue,PlaybackFlags,CustomDuration,ScriptName FROM spell_scene WHERE MiscValue=1265;SELECT 'events',COUNT(*) FROM spell_scene_event WHERE MiscValue=1265;"|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($db[0]) --port=$($db[1]) --user=$($db[2]) --database=$($db[4]) --batch --raw --skip-column-names)
   if($LASTEXITCODE){throw 'Live Tuure readback failed'}
   foreach($row in @("update`t$hash`tRELEASED","scene`t1565`t1265`t21`t0`tscene_tuure_pickup","events`t0")){if($rows -notcontains $row){throw 'Live Tuure binding/hash mismatch'}}
   'PASS canonical updater hash and exact live scene binding'
  }
 }finally{$env:MYSQL_PWD=$prior}
 'PASS twice-applied TEMP scene binding preserves package/flags/duration and six explicit custom/missing refusals plus native duplicate-index refusal'
}
