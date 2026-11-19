param([Parameter(Mandatory=$true)][string]$MigrationFile)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/seat-patrol-collision/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"
New-Item -ItemType Directory -Force $out|Out-Null
$migration=[IO.File]::ReadAllText((Resolve-Path -LiteralPath $MigrationFile).Path)
$sealed=(Get-FileHash -LiteralPath $MigrationFile).Hash
$keys='id,point,position_x,position_y,position_z,orientation,delay,delay_chance,move_type,speed,action,action_chance,entry,wpguid'
$fingerprint="MD5(GROUP_CONCAT(CONCAT_WS('|',$keys) ORDER BY id,point SEPARATOR ';'))"
$prelude=@'
SET SESSION group_concat_max_len=65536;
CREATE TEMPORARY TABLE seat_seed_creature AS SELECT * FROM creature WHERE guid IN(14505201,342743,342793);
CREATE TEMPORARY TABLE seat_seed_addon AS SELECT * FROM creature_addon WHERE guid IN(14505201,342743,342793);
CREATE TEMPORARY TABLE seat_seed_waypoints AS SELECT * FROM waypoint_data WHERE id IN(12961265,1467062100);
CREATE TEMPORARY TABLE creature LIKE seat_seed_creature;
CREATE TEMPORARY TABLE creature_addon LIKE seat_seed_addon;
CREATE TEMPORARY TABLE waypoint_data LIKE seat_seed_waypoints;
INSERT INTO creature SELECT * FROM seat_seed_creature;
INSERT INTO creature_addon SELECT * FROM seat_seed_addon;
INSERT INTO waypoint_data SELECT * FROM seat_seed_waypoints;
CREATE TEMPORARY TABLE seat_shadow_guard(valid TINYINT NOT NULL CHECK(valid=1));
INSERT INTO seat_shadow_guard SELECT (SELECT COUNT(*) FROM creature)=3 AND (SELECT COUNT(*) FROM creature_addon)=3 AND (SELECT COUNT(*) FROM waypoint_data)=19;
'@
$tail=@"
SELECT JSON_OBJECT('seat_nodes',(SELECT COUNT(*) FROM waypoint_data WHERE id=1467062100 AND entry=122404),'outdoor_nodes',(SELECT COUNT(*) FROM waypoint_data WHERE id=12961265 AND entry=0),'foreign_seat_nodes',(SELECT COUNT(*) FROM waypoint_data WHERE id=12961265 AND entry=122404),'seat_addon',(SELECT path_id FROM creature_addon WHERE guid=14505201),'outdoor_addons',(SELECT COUNT(*) FROM creature_addon WHERE guid IN(342743,342793) AND path_id=12961265),'outdoor_unchanged',(SELECT $fingerprint FROM waypoint_data WHERE id=12961265 AND entry=0)=(SELECT $fingerprint FROM seat_seed_waypoints WHERE id=12961265 AND entry=0));
DROP TEMPORARY TABLE creature;
DROP TEMPORARY TABLE creature_addon;
DROP TEMPORARY TABLE waypoint_data;
SELECT JSON_OBJECT('persistent_path_unchanged',(SELECT $fingerprint FROM waypoint_data WHERE id IN(12961265,1467062100))=(SELECT $fingerprint FROM seat_seed_waypoints),'persistent_addons_unchanged',(SELECT GROUP_CONCAT(CONCAT(guid,':',path_id) ORDER BY guid) FROM creature_addon WHERE guid IN(14505201,342743,342793))=(SELECT GROUP_CONCAT(CONCAT(guid,':',path_id) ORDER BY guid) FROM seat_seed_addon));
"@
[IO.File]::WriteAllText("$out/shadow.sql",$prelude+"`n"+$migration+"`n"+$migration+"`n"+$tail)
$private="$out/client.cnf"
$conf=Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" -Raw
$match=[regex]::Match($conf,'(?m)^\s*WorldDatabaseInfo\s*=\s*"([^"]+)"');if(!$match.Success){throw 'Runtime World connection absent'}
$parts=$match.Groups[1].Value.Split(';');if($parts.Count -ne 5){throw 'Runtime World connection shape changed'}
function Quote-Client([string]$v){'"'+$v.Replace('\','\\').Replace('"','\"')+'"'}
try{
 [IO.File]::WriteAllText($private,"[client]`nhost=$(Quote-Client $parts[0])`nport=$($parts[1])`nuser=$(Quote-Client $parts[2])`npassword=$(Quote-Client $parts[3])`ndatabase=$(Quote-Client $parts[4])`n")
 & "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out) -Test {
  Get-Content "$out/shadow.sql" -Raw|& 'C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mariadb.exe' "--defaults-extra-file=$private" --batch --raw --skip-column-names *> "$out/readback.jsonl"
  if($LASTEXITCODE){throw "Seat path SQL fixture failed:$out/readback.jsonl"}
  $values=@(Get-Content "$out/readback.jsonl"|ForEach-Object {$_|ConvertFrom-Json})
  if($values.Count -ne 2 -or $values[0].seat_nodes -ne 6 -or $values[0].outdoor_nodes -ne 13 -or $values[0].foreign_seat_nodes -ne 0 -or $values[0].seat_addon -ne 1467062100 -or $values[0].outdoor_addons -ne 2 -or $values[0].outdoor_unchanged -ne 1 -or $values[1].persistent_path_unchanged -ne 1 -or $values[1].persistent_addons_unchanged -ne 1){throw 'Seat collision readback/persistent preservation failed'}
  'PASS exact migration twice in native-schema temporary shadows:Seat6/outdoor13;all retained outdoor fields and persistent source path/addons unchanged'
 }
}finally{if(Test-Path -LiteralPath $private){Remove-Item -LiteralPath $private -Force};if((Get-FileHash -LiteralPath $MigrationFile).Hash -ne $sealed){throw 'Path migration changed during test'}}
"PATH_FIXTURE $out"
