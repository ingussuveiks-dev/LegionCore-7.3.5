param([switch]$Live)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$migration=Get-Content "$repo/sql/updates/world/2026_10_14_06_stormheim_will.sql" -Raw
$sql=''
if(!$Live){
 $tables=@{creature_template='entry IN(96139,96144,96129,108263,96067,96122)';gameobject_template='entry IN(243814,243802,243808)';quest_objectives='QuestID IN(39591,39592,39593)';quest_template_addon='ID IN(39591,39592,39593)';quest_template='ID IN(39591,39592,39593)';quest_start_scripts='id=39592 OR datalong=39592'}
 foreach($table in $tables.Keys){$sql+="CREATE TEMPORARY TABLE saved_$table AS SELECT * FROM $table WHERE $($tables[$table]); CREATE TEMPORARY TABLE $table AS SELECT * FROM saved_$table;`n"}
 $sql+="UPDATE quest_objectives SET Type=0,ObjectID=96144 WHERE ID=279721; UPDATE quest_template_addon SET SpecialFlags=2 WHERE ID=39592; UPDATE quest_template SET StartScript=39592 WHERE ID=39592; DELETE FROM quest_start_scripts WHERE id=39592; INSERT INTO quest_start_scripts VALUES(39592,0,7,39592,0,0,0,0,0,0),(39592,0,7,39592,0,0,0,0,0,0),(39592,0,7,39592,0,0,0,0,0,0),(39592,0,7,39592,0,0,0,0,0,0);`n$migration`n$migration`n"
}
$sql+=@'
SELECT 'native-record-objective',COUNT(*) FROM quest_objectives WHERE ID=279721 AND QuestID=39592 AND Type=2 AND ObjectID=243802 AND StorageIndex=3 AND Amount=1;
SELECT 'normal-completion',COUNT(*) FROM quest_template_addon WHERE ID=39592 AND SpecialFlags=0 AND PrevQuestID=39590 AND NextQuestID=39593;
SELECT 'normal-quest',COUNT(*) FROM quest_template WHERE ID=39592 AND QuestType=2;
SELECT 'head-starter',COUNT(*) FROM gameobject_queststarter WHERE id=243836 AND quest=39592;
SELECT 'head-handin',COUNT(*) FROM gameobject_questender WHERE id=243836 AND quest=39592;
SELECT 'console-bindings',COUNT(*) FROM gameobject_template WHERE entry IN(243814,243802) AND ScriptName='go_stormheim_will_console';
SELECT 'actor-bindings',COUNT(*) FROM creature_template WHERE entry IN(96139,96144,96129,108263) AND ScriptName='npc_stormheim_will_actor';
SELECT 'access-spell',COUNT(*) FROM gameobject_template WHERE entry=243802 AND Data0=1691 AND Data10=190707;
SELECT 'record-data',COUNT(*) FROM conversation_data WHERE entry IN(773,774,775);
SELECT 'record-actors',COUNT(*) FROM conversation_actor WHERE entry IN(773,774,775) AND actorId=50369 AND creatureId=96257 AND displayId=25217 AND duration>0;
SELECT 'native-unpowered-location',COUNT(*) FROM gameobject WHERE guid=114812 AND id=243814 AND map=1220 AND PhaseId='' AND ABS(position_x-3333.12)<0.02 AND ABS(position_y-2209.12)<0.02 AND ABS(position_z-314.554)<0.02;
SELECT 'native-console-event',COUNT(*) FROM event_scripts WHERE id=243814 AND command=10 AND datalong=96139 AND datalong2=300000;
SELECT 'native-raider-counts',COUNT(*) FROM smart_scripts WHERE entryorguid=96139 AND source_type=9 AND action_type=12 AND action_param1=96129;
SELECT 'native-warmonger',COUNT(*) FROM smart_scripts WHERE entryorguid=96139 AND source_type=9 AND id=11 AND event_param1=5000 AND action_type=12 AND action_param1=108263;
SELECT 'active-enemy-scaling',COUNT(*) FROM creature_template_scaling WHERE Entry IN(96129,108263) AND LevelScalingMin=98 AND LevelScalingMax=110;
SELECT 'no-accept-completion',COUNT(*) FROM quest_start_scripts WHERE id=39592 AND command=7 AND datalong=39592;
SELECT 'detached-empty-start',COUNT(*) FROM quest_template WHERE ID=39592 AND StartScript=0;
'@
$expected=@("native-record-objective`t1","normal-completion`t1","normal-quest`t1","head-starter`t1","head-handin`t1","console-bindings`t2","actor-bindings`t4","access-spell`t1","record-data`t3","record-actors`t3","native-unpowered-location`t1","native-console-event`t1","native-raider-counts`t5","native-warmonger`t1","active-enemy-scaling`t2","no-accept-completion`t0","detached-empty-start`t1")
if(!$Live){
 $sql+=@"
SELECT 'preserved-actor-flags',COUNT(*) FROM creature_template a JOIN saved_creature_template b USING(entry) WHERE a.npcflag<>b.npcflag OR a.unit_flags<>b.unit_flags OR a.VehicleId<>b.VehicleId OR a.lootid<>b.lootid;
SELECT 'preserved-object-flags',COUNT(*) FROM gameobject_template a JOIN saved_gameobject_template b USING(entry) WHERE a.flags<>b.flags OR a.Data0<>b.Data0 OR a.Data10<>b.Data10;
SELECT 'other-objectives',COUNT(*) FROM quest_objectives a JOIN saved_quest_objectives b USING(ID) WHERE a.QuestID<>39592 AND (a.Type<>b.Type OR a.ObjectID<>b.ObjectID OR a.StorageIndex<>b.StorageIndex);
UPDATE creature_template SET ScriptName='custom' WHERE entry=96129;
UPDATE gameobject_template SET ScriptName='custom' WHERE entry=243802;
UPDATE quest_objectives SET Type=0,ObjectID=999 WHERE ID=279721;
UPDATE quest_template_addon SET SpecialFlags=9 WHERE ID=39592;
$migration
SELECT 'custom-actor',COUNT(*) FROM creature_template WHERE entry=96129 AND ScriptName='custom';
SELECT 'custom-object',COUNT(*) FROM gameobject_template WHERE entry=243802 AND ScriptName='custom';
SELECT 'custom-objective',COUNT(*) FROM quest_objectives WHERE ID=279721 AND Type=0 AND ObjectID=999;
SELECT 'custom-flags',COUNT(*) FROM quest_template_addon WHERE ID=39592 AND SpecialFlags=9;
INSERT INTO quest_start_scripts VALUES(39592,1,7,39592,0,0,0,0,0,0);
UPDATE quest_template SET StartScript=39592 WHERE ID=39592;
UPDATE quest_template_addon SET SpecialFlags=2 WHERE ID=39592;
$migration
SELECT 'custom-start-action',COUNT(*) FROM quest_start_scripts WHERE id=39592 AND delay=1 AND command=7;
SELECT 'custom-start-hook',COUNT(*) FROM quest_template WHERE ID=39592 AND StartScript=39592;
SELECT 'custom-explore-flag',COUNT(*) FROM quest_template_addon WHERE ID=39592 AND SpecialFlags=2;
"@
 $expected+=@("preserved-actor-flags`t0","preserved-object-flags`t0","other-objectives`t0","custom-actor`t1","custom-object`t1","custom-objective`t1","custom-flags`t1","custom-start-action`t1","custom-start-hook`t1","custom-explore-flag`t1")
}
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$actual=$sql | & 'C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe' --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --skip-column-names;if($LASTEXITCODE){throw 'Will SQL failed'}}finally{$env:MYSQL_PWD=$previous}
if(($actual -join '|') -ne ($expected -join '|')){throw "Will SQL mismatch: $($actual -join '|')"}
'PASS: migration twice, original slot/completion flag, physical offer/hand-in, native 2+3 raiders and warmonger, enemy scaling, recording actors, scoped bindings and custom/unrelated preservation.'
