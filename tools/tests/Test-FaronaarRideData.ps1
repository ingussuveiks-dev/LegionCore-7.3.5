param([string]$MigrationPath="$PSScriptRoot/../../sql/updates/world/2026_10_26_10_faronaar_stellagosa_ride.sql")
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/faronaar-ride-data"
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -Test {
 $migration=Get-Content $MigrationPath -Raw
 $sql=(@('creature_template','vehicle_template_accessory','spell_script_names','smart_scripts')|ForEach-Object {"CREATE TEMPORARY TABLE saved_$_ LIKE $_; INSERT INTO saved_$_ SELECT * FROM $_; CREATE TEMPORARY TABLE $_ LIKE saved_$_; INSERT INTO $_ SELECT * FROM saved_$_;"}) -join "`n"
 $sql+=@"

UPDATE creature_template SET ScriptName='' WHERE entry=90982;
DELETE FROM spell_script_names WHERE spell_id=180768;
REPLACE INTO vehicle_template_accessory VALUES(90982,90983,1,0,0,0,0,0,'Drake',8,300);
SELECT 'broken-player-seat',COUNT(*) FROM vehicle_template_accessory WHERE EntryOrAura=90982 AND seat_id=1;
$migration
$migration
SELECT 'ai',AIName,ScriptName FROM creature_template WHERE entry=90982;
SELECT 'spell',COUNT(*) FROM spell_script_names WHERE spell_id=180768 AND ScriptName='spell_faronaar_stellagosa_ride';
SELECT 'player-seat',COUNT(*) FROM vehicle_template_accessory WHERE EntryOrAura=90982 AND seat_id=1;
SELECT 'korvas',COUNT(*) FROM vehicle_template_accessory a JOIN saved_vehicle_template_accessory b USING(EntryOrAura,seat_id) WHERE a.EntryOrAura=90982 AND a.seat_id=0 AND a.accessory_entry=b.accessory_entry AND a.offsetX=b.offsetX AND a.offsetZ=b.offsetZ AND a.minion=b.minion AND a.summontimer=b.summontimer;
SELECT 'native-smart',COUNT(*) FROM smart_scripts a JOIN saved_smart_scripts b USING(entryorguid,source_type,id,link) WHERE a.entryorguid=90982 AND a.event_type=b.event_type AND a.event_flags=b.event_flags AND a.action_type=b.action_type AND a.action_param1=b.action_param1 AND a.action_param2=b.action_param2 AND a.target_type=b.target_type;
SELECT 'native-route',COUNT(*) FROM waypoints WHERE entry=90982;
SELECT 'click',spell_id,cast_flags FROM npc_spellclick_spells WHERE npc_entry=90982;
SELECT 'unrelated',COUNT(*) FROM vehicle_template_accessory a LEFT JOIN saved_vehicle_template_accessory b USING(EntryOrAura,seat_id) WHERE a.EntryOrAura<>90982 AND (b.EntryOrAura IS NULL OR a.accessory_entry<>b.accessory_entry OR a.offsetX<>b.offsetX OR a.offsetY<>b.offsetY OR a.offsetZ<>b.offsetZ OR a.offsetO<>b.offsetO OR a.minion<>b.minion OR a.description<>b.description OR a.summontype<>b.summontype OR a.summontimer<>b.summontimer);
REPLACE INTO vehicle_template_accessory VALUES(90982,90983,1,0,0,0,0,0,'Drake',8,300);
UPDATE creature_template SET ScriptName='custom_ride' WHERE entry=90982;
$migration
SELECT 'custom-ai',ScriptName FROM creature_template WHERE entry=90982;
SELECT 'refused-seat',COUNT(*) FROM vehicle_template_accessory WHERE EntryOrAura=90982 AND seat_id=1;
UPDATE creature_template SET ScriptName='' WHERE entry=90982;
INSERT INTO spell_script_names VALUES(180768,'custom_spell');
$migration
SELECT 'custom-spell',ScriptName FROM creature_template WHERE entry=90982;
SELECT 'refused-seat',COUNT(*) FROM vehicle_template_accessory WHERE EntryOrAura=90982 AND seat_id=1;
DELETE FROM spell_script_names WHERE spell_id=180768 AND ScriptName='custom_spell';
UPDATE vehicle_template_accessory SET accessory_entry=1 WHERE EntryOrAura=90982 AND seat_id=1;
$migration
SELECT 'custom-accessory',accessory_entry FROM vehicle_template_accessory WHERE EntryOrAura=90982 AND seat_id=1;
"@
 $connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $previous=$env:MYSQL_PWD
 try{$env:MYSQL_PWD=$connection[3];$rows=@($sql|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Faronaar ride TEMP migration failed'}}finally{$env:MYSQL_PWD=$previous}
 $expected=@("broken-player-seat`t1","ai`tSmartAI`tnpc_faronaar_stellagosa_ride","spell`t1","player-seat`t0","korvas`t1","native-smart`t6","native-route`t7","click`t46598`t1","unrelated`t0","custom-ai`tcustom_ride","refused-seat`t1","custom-spell`t","refused-seat`t1","custom-accessory`t1")
 if(($rows -join '|') -ne ($expected -join '|')){throw "Ride data mismatch: $($rows -join '|')"}
 [IO.File]::WriteAllLines("$out/rows.tsv",[string[]]$rows)
 'PASS TEMP-only migration twice: duplicate player-seat NPC removed, original Korvas/native SmartAI/route/click retained; custom AI/spell/accessory preserved. No persistent SQL applied.'
}
