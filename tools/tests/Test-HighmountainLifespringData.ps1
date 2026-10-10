param([switch]$Live)
$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$migration=Get-Content "$repo/sql/updates/world/2026_10_10_08_highmountain_lifespring.sql" -Raw
$sql=''
if(!$Live){
 $sql=(@('creature_template','creature_queststarter','spell_script_names','spell_area','gameobject') | ForEach-Object {"CREATE TEMPORARY TABLE saved_$_ LIKE $_; INSERT INTO saved_$_ SELECT * FROM $_; CREATE TEMPORARY TABLE $_ LIKE saved_$_; INSERT INTO $_ SELECT * FROM saved_$_;"}) -join "`n"
 $sql+="`n$migration`n$migration`n"
}
$sql+=@"
SELECT 'companion-binding',COUNT(*) FROM creature_template WHERE entry=96038 AND AIName='' AND ScriptName='npc_lifespring_companion';
SELECT 'cast-guard',COUNT(*) FROM spell_script_names WHERE spell_id=190370 AND ScriptName='spell_lifespring_companion';
SELECT 'obsolete-area',COUNT(*) FROM spell_area WHERE spell=190370 AND area=7786 AND quest_start=39488 AND quest_end=39498;
SELECT 'existing-phase',COUNT(*) FROM spell_area WHERE spell=100044 AND area=7786 AND quest_start=39661 AND quest_end=39488 AND quest_start_status=66 AND quest_end_status=74;
SELECT 'offers',COUNT(*) FROM creature_queststarter WHERE id=96038 AND quest IN(39488,39489,39487,39498);
SELECT 'enders',COUNT(*) FROM creature_questender WHERE id=96038 AND quest IN(39488,39489,39487);
SELECT 'crystals',COUNT(*),COUNT(DISTINCT CONCAT(position_x,':',position_y,':',position_z)) FROM gameobject WHERE id=243639 AND map=1220 AND areaId=7786 AND zoneId=7503 AND spawnMask=1 AND phaseMask=1 AND PhaseId='' AND spawntimesecs=120 AND state=1 AND rotation3=1;
SELECT 'native-object',COUNT(*) FROM gameobject_template WHERE entry=243639 AND type=3 AND displayId=28054 AND Data0=1691 AND Data1=243639 AND QuestItem1=128393;
SELECT 'quest-only-loot',COUNT(*) FROM gameobject_loot_template WHERE Entry=243639 AND Item=128393 AND Chance=100 AND QuestRequired=1 AND LootMode=1 AND MinCount=1 AND MaxCount=1;
SELECT 'ten-required',COUNT(*) FROM quest_objectives WHERE QuestID=39488 AND ObjectID=128393 AND Amount=10 AND Type=1;
SELECT 'stationary-preserved',COUNT(*) FROM creature WHERE guid=12728656 AND id=96038 AND map=1220;
"@
$expected=@("companion-binding`t1","cast-guard`t1","obsolete-area`t0","existing-phase`t1","offers`t4","enders`t3","crystals`t12`t12","native-object`t1","quest-only-loot`t1","ten-required`t1","stationary-preserved`t1")
if(!$Live){
 $sql+=@"
SELECT 'unrelated-npcs',COUNT(*) FROM saved_creature_template a LEFT JOIN creature_template b USING(entry) WHERE a.entry<>96038 AND (b.entry IS NULL OR a.ScriptName<>b.ScriptName OR a.AIName<>b.AIName);
SELECT 'unrelated-area',COUNT(*) FROM saved_spell_area a LEFT JOIN spell_area b ON a.spell=b.spell AND a.area=b.area AND a.quest_start=b.quest_start AND a.quest_end=b.quest_end AND a.quest_start_status=b.quest_start_status AND a.quest_end_status=b.quest_end_status WHERE a.spell<>190370 AND b.spell IS NULL;
SELECT 'existing-objects',COUNT(*) FROM saved_gameobject a LEFT JOIN gameobject b USING(guid) WHERE b.guid IS NULL OR a.id<>b.id OR a.map<>b.map OR a.position_x<>b.position_x OR a.position_y<>b.position_y OR a.position_z<>b.position_z OR a.PhaseId<>b.PhaseId;
UPDATE creature_template SET ScriptName='unrelated_custom_script',AIName='custom_ai' WHERE entry=96038;
$migration
SELECT 'custom-script-preserved',COUNT(*) FROM creature_template WHERE entry=96038 AND ScriptName='unrelated_custom_script' AND AIName='custom_ai';
"@
 $expected+=@("unrelated-npcs`t0","unrelated-area`t0","existing-objects`t0","custom-script-preserved`t1")
}
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$rows=& 'C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe' --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --skip-column-names --execute=$sql;if($LASTEXITCODE){throw 'Lifespring SQL test failed'}}finally{$env:MYSQL_PWD=$previous}
if(($rows -join '|') -ne ($expected -join '|')){throw "Lifespring SQL mismatch: $($rows -join '|')"}
'PASS: repeatable migration, native quest-only crystal loot, twelve unique spawns, companion spell/NPC/quest bindings, preserved phase, stationary NPC and unrelated/custom rows.'
