param([switch]$Live)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/stormheim-overlook-rider"
New-Item -ItemType Directory -Force -Path $out | Out-Null
$migration=Get-Content "$repo/sql/updates/world/2026_10_17_16_stormheim_overlook_rider.sql" -Raw
$sql=''
if(!$Live){
 # Snapshot real schema/data, then replay the duplicate even after deployment.
 $sql=@'
CREATE TEMPORARY TABLE saved_creature LIKE creature;
INSERT INTO saved_creature SELECT * FROM creature WHERE guid IN(340420,341178,288730);
CREATE TEMPORARY TABLE creature LIKE saved_creature;
INSERT INTO creature SELECT * FROM saved_creature WHERE guid<>341178;
CREATE TEMPORARY TABLE duplicate_seed LIKE creature;
INSERT INTO duplicate_seed SELECT * FROM saved_creature WHERE guid=340420;
UPDATE duplicate_seed SET guid=341178;
INSERT INTO creature SELECT * FROM duplicate_seed;
'@
 # MySQL cannot reopen a temporary table for a self join: redirect only the
 # read-only original alias to the unchanged snapshot of the canonical actor.
 $temporary=$migration.Replace('JOIN creature original','JOIN saved_creature original')
 $sql+="$temporary`n$temporary`n"
}
$sql+=@'
SELECT 'canonical',COUNT(*) FROM creature WHERE guid=340420 AND id=97250 AND map=1220 AND phaseMask=1 AND PhaseId='' AND npcflag=0 AND ABS(position_x-3764.07)<0.001 AND ABS(position_y-2918.25)<0.001 AND ABS(position_z-431.166)<0.001;
SELECT 'duplicate',COUNT(*) FROM creature WHERE guid=341178;
SELECT 'quest-ender',COUNT(*) FROM creature_questender WHERE id=97250 AND quest=38317;
SELECT 'template',COUNT(*) FROM creature_template WHERE entry=97250 AND npcflag=3 AND faction=83 AND VehicleId=4387 AND gossip_menu_id=18762;
SELECT 'other-leader',COUNT(*) FROM creature WHERE guid=288730 AND id=91468;
'@
if(!$Live){
 $sql+=@"
INSERT INTO creature SELECT * FROM duplicate_seed;
UPDATE creature SET PhaseId='999' WHERE guid=341178;
$temporary
SELECT 'custom-phase',COUNT(*) FROM creature WHERE guid=341178 AND PhaseId='999';
UPDATE creature SET PhaseId='',position_z=position_z+1 WHERE guid=341178;
$temporary
SELECT 'custom-position',COUNT(*) FROM creature WHERE guid=341178;
UPDATE creature SET position_z=431.166,unit_flags3=1 WHERE guid=341178;
$temporary
SELECT 'custom-flags',COUNT(*) FROM creature WHERE guid=341178 AND unit_flags3=1;
DELETE FROM creature WHERE guid=341178;
INSERT INTO creature SELECT * FROM duplicate_seed;
"@
 # Shadow dependency tables separately, so no production membership changes.
 foreach($table in @('creature_addon','pool_creature','game_event_creature','linked_respawn','creature_formations','smart_scripts','conditions')){
  $sql+="CREATE TEMPORARY TABLE schema_$table LIKE $table; CREATE TEMPORARY TABLE $table LIKE schema_$table;`n"
 }
 $guards=@(
  @('creature_addon','INSERT INTO creature_addon(guid) VALUES(341178);'),
  @('pool_creature','INSERT INTO pool_creature(guid,pool_entry,chance,description) VALUES(341178,1,0,''fixture'');'),
  @('game_event_creature','INSERT INTO game_event_creature(eventEntry,guid) VALUES(1,341178);'),
  @('linked_respawn','INSERT INTO linked_respawn(guid,linkedGuid,linkType) VALUES(341178,1,0);'),
  @('creature_formations','INSERT INTO creature_formations(leaderGUID,memberGUID,dist,angle,groupAI) VALUES(341178,1,0,0,0);'),
  @('smart_scripts','INSERT INTO smart_scripts(entryorguid,source_type,id,comment) VALUES(-341178,0,0,''fixture'');'),
  @('conditions','INSERT INTO conditions(SourceTypeOrReferenceId,SourceEntry,ConditionTypeOrReference) VALUES(22,-341178,1);')
 )
 foreach($g in $guards){$sql+=$g[1]+"`n$temporary`nSELECT '$($g[0])-reference',COUNT(*) FROM creature WHERE guid=341178; DELETE FROM $($g[0]);`n"}
 $sql+="INSERT INTO smart_scripts(entryorguid,source_type,id,target_type,target_param1,comment) VALUES(97250,0,0,10,341178,'fixture');`n$temporary`nSELECT 'smart-target-reference',COUNT(*) FROM creature WHERE guid=341178; DELETE FROM smart_scripts;`n"
 # Canonical pooled/event actors must not be substituted by a permanent copy.
 $sql+="INSERT INTO game_event_creature(eventEntry,guid) VALUES(1,340420);`n$temporary`nSELECT 'canonical-event',COUNT(*) FROM creature WHERE guid=341178; DELETE FROM game_event_creature;`n"
 $sql+="DELETE FROM saved_creature WHERE guid=340420;`n$temporary`nSELECT 'missing-original',COUNT(*) FROM creature WHERE guid=341178;`n"
}
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';');$old=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Overlook rider query failed'}}finally{$env:MYSQL_PWD=$old}
$rows | Set-Content "$out/$(if($Live){'live'}else{'migration'}).tsv"
$expected=@("canonical`t1","duplicate`t0","quest-ender`t1","template`t1","other-leader`t1")
if(!$Live){$expected+=@("custom-phase`t1","custom-position`t1","custom-flags`t1");foreach($g in $guards){$expected+="$($g[0])-reference`t1"};$expected+=@("smart-target-reference`t1","canonical-event`t1","missing-original`t1")}
if(($rows -join '|') -ne ($expected -join '|')){throw "Rider contract mismatch: $($rows -join '|')"}
if($Live){'PASS: one canonical rider and38317 ender/template retained; duplicate absent.'}else{'PASS: duplicate removal twice; canonical actor/quest ender/unrelated leader retained; phase/position/flags and all seven dependency classes plus canonical event protect customized actors.'}
