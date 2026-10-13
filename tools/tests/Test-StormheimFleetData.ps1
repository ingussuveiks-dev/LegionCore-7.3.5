param([switch]$Live)
$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$migration=Get-Content "$repo/sql/updates/world/2026_10_13_15_stormheim_fleet.sql" -Raw
$sql=''
if(!$Live){
 foreach($table in @('instance_template','creature_template','gameobject_template','npc_spellclick_spells','spell_script_names','creature_template_spell','creature_template_scaling')){
  $sql+="CREATE TEMPORARY TABLE saved_$table LIKE $table; INSERT INTO saved_$table SELECT * FROM $table; CREATE TEMPORARY TABLE $table LIKE saved_$table; INSERT INTO $table SELECT * FROM saved_$table;`n"
 }
 $sql+="UPDATE creature_template_scaling SET LevelScalingMin=0,LevelScalingMax=0 WHERE Entry=94318;`n$migration`n$migration`n"
}
$sql+=@"
SELECT 'instance',COUNT(*) FROM instance_template WHERE map=1475 AND script='instance_stormheim_fleet';
SELECT 'actors',COUNT(*) FROM creature_template WHERE entry IN(92930,92957,93113,93173,93433,93479,93486,93490,93496,93506,93560,93563,93603,93960,93961,93962,94318,94420,94517,94550,94576,94737,94809) AND ScriptName='npc_stormheim_fleet_actor';
SELECT 'objects',COUNT(*) FROM gameobject_template WHERE (entry IN(242380,243244) AND ScriptName='go_stormheim_fleet_flare') OR(entry=240210 AND ScriptName='go_stormheim_fleet_parachute');
SELECT 'bindings',COUNT(*) FROM spell_script_names WHERE (spell_id=43671 AND ScriptName='spell_stormheim_fleet_board') OR(spell_id IN(185614,185616) AND ScriptName='spell_stormheim_fleet_finale') OR(spell_id=186010 AND ScriptName='spell_stormheim_fleet_flare');
SELECT 'rides',COUNT(*) FROM npc_spellclick_spells WHERE npc_entry IN(94517,94576) AND spell_id=43671 AND cast_flags=1 AND user_type=0 AND add_npc_flag=0;
SELECT 'gun-spells',COUNT(*) FROM creature_template_spell WHERE entry=92957 AND spell IN(184420,186276);
SELECT 'vehicle-data',COUNT(*) FROM creature_template WHERE (entry=92957 AND VehicleId=4151) OR(entry=94550 AND VehicleId=4285) OR(entry=93433 AND VehicleId=1439) OR(entry=94517 AND VehicleId=4283) OR(entry=94576 AND VehicleId=4286);
SELECT 'skyfire',COUNT(*) FROM gameobject_template WHERE entry=241630 AND type=15 AND displayId=28177 AND Data0=5130 AND Data6=1476;
SELECT 'genn-active-scaling',COUNT(*) FROM creature_template_scaling WHERE Entry=94318 AND LevelScalingMin=98 AND LevelScalingMax=110;
SELECT 'native-objectives',COUNT(*) FROM quest_objectives WHERE QuestID IN(39800,39801) AND Type=0 AND StorageIndex=0 AND ObjectID=113320 AND Amount=1;
SELECT 'final-destinations',COUNT(*) FROM spell_target_position WHERE (id=185613 AND target_map=1220 AND ABS(target_position_x-3209.48)<0.01 AND ABS(target_position_y-3072.73)<0.01) OR(id=185615 AND target_map=1220 AND ABS(target_position_x-4110.7)<0.01 AND ABS(target_position_y-2944.49)<0.01);
"@
$expected=@("instance`t1","actors`t23","objects`t3","bindings`t4","rides`t2","gun-spells`t2","vehicle-data`t5","skyfire`t1","genn-active-scaling`t1","native-objectives`t2","final-destinations`t2")
if(!$Live){
 $sql+=@"
SELECT 'other-actors',COUNT(*) FROM creature_template a JOIN saved_creature_template b USING(entry) WHERE a.entry NOT IN(92930,92957,93113,93173,93433,93479,93486,93490,93496,93506,93560,93563,93603,93960,93961,93962,94318,94420,94517,94550,94576,94737,94809) AND a.ScriptName<>b.ScriptName;
SELECT 'other-scaling',COUNT(*) FROM creature_template_scaling a JOIN saved_creature_template_scaling b USING(Entry) WHERE a.Entry<>94318 AND (a.LevelScalingMin<>b.LevelScalingMin OR a.LevelScalingMax<>b.LevelScalingMax);
UPDATE creature_template SET ScriptName='custom' WHERE entry=93560;
UPDATE gameobject_template SET ScriptName='custom' WHERE entry=242380;
UPDATE instance_template SET script='custom' WHERE map=1475;
"@
 $sql+="$migration`nSELECT 'custom-actor',COUNT(*) FROM creature_template WHERE entry=93560 AND ScriptName='custom'; SELECT 'custom-go',COUNT(*) FROM gameobject_template WHERE entry=242380 AND ScriptName='custom'; SELECT 'custom-instance',COUNT(*) FROM instance_template WHERE map=1475 AND script='custom';"
 $expected+=@("other-actors`t0","other-scaling`t0","custom-actor`t1","custom-go`t1","custom-instance`t1")
}
# Replay the actual prepared UPDATE in a temporary checkpoint table. Old
# asynchronous writes must not undo later actions or a fresh acceptance.
$table=Get-Content "$repo/sql/updates/characters/2026_10_13_01_stormheim_mission.sql" -Raw
$sql+="`n"+$table.Replace('CREATE TABLE IF NOT EXISTS','CREATE TEMPORARY TABLE IF NOT EXISTS')+"`nINSERT INTO character_stormheim_mission VALUES(1,39800,11,0,0,0,0,0,0),(2,39801,22,0,0,0,0,0,0);`n"
$source=Get-Content "$repo/src/server/database/Database/Implementation/CharacterDatabase.cpp" -Raw
$update=[regex]::Match($source,'CHAR_UPD_STORMHEIM_MISSION, "([^"]+)"').Groups[1].Value
if(!$update){throw 'Prepared checkpoint UPDATE not found'}
foreach($values in @(@(5,30,7,127,2,1,1,39800,11),@(1,10,0,0,1,0,1,39800,11))){$parts=$update.Split('?');$statement=$parts[0];for($i=0;$i -lt 9;$i++){$statement+=$values[$i].ToString()+$parts[$i+1]};$sql+=$statement+";`n"}
$sql+="SELECT 'monotonic',COUNT(*) FROM character_stormheim_mission WHERE guid=1 AND quest=39800 AND stage=5 AND air=30 AND captains=7 AND flares=127 AND frost=2 AND final=1;`n"
$sql+="UPDATE character_stormheim_mission SET serial=12,stage=0,air=0,captains=0,flares=0,frost=0,final=0 WHERE guid=1;`n$statement;`nSELECT 'old-serial',COUNT(*) FROM character_stormheim_mission WHERE guid=1 AND serial=12 AND stage=0 AND final=0; SELECT 'other-owner',COUNT(*) FROM character_stormheim_mission WHERE guid=2 AND serial=22 AND stage=0 AND final=0;"
$expected+=@("monotonic`t1","old-serial`t1","other-owner`t1")
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';');$old=$env:MYSQL_PWD
try {$env:MYSQL_PWD=$c[3];$actual=$sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --skip-column-names;if($LASTEXITCODE){throw 'Fleet SQL regression failed'}}finally{$env:MYSQL_PWD=$old}
if(($actual -join '|') -ne ($expected -join '|')){throw "Fleet SQL mismatch: $($actual -join '|')"}
'PASS: real migration replay twice, complete instance/actor/GO/click/spell graph, native vehicles/transport/endings, active Genn scaling, custom preservation and reordered/stale prepared checkpoint writes.'
