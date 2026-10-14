param([switch]$Live)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$migration=Get-Content "$repo/sql/updates/world/2026_10_14_17_stormheim_winter.sql" -Raw
$sql=''
if(!$Live){
 $tables=@{npc_spellclick_spells='npc_entry IN(91975,92017,92072)';eventobject_template='entry IN(611,612)';quest_template='ID IN(38412,38618,38414)';quest_start_scripts='id=38618'}
 foreach($table in $tables.Keys){$sql+="CREATE TEMPORARY TABLE saved_$table AS SELECT * FROM $table WHERE $($tables[$table]); CREATE TEMPORARY TABLE $table AS SELECT * FROM saved_$table;`n"}
 $sql+=@"
UPDATE npc_spellclick_spells SET spell_id=121975 WHERE npc_entry IN(92017,92072);
UPDATE eventobject_template SET ScriptName='SmartEventObject' WHERE entry IN(611,612);
UPDATE quest_template SET StartScript=38618 WHERE ID=38618;
DELETE FROM quest_start_scripts WHERE id=38618;
$migration
$migration
"@
}
$sql+=@'
SELECT 'gun-types',COUNT(*) FROM npc_spellclick_spells WHERE npc_entry IN(92017,92072) AND spell_id=182478 AND cast_flags=1 AND user_type=0 AND add_npc_flag=1;
SELECT 'no-keg',COUNT(*) FROM npc_spellclick_spells WHERE npc_entry IN(92017,92072) AND spell_id=121975;
SELECT 'events',COUNT(*) FROM eventobject_template WHERE entry IN(611,612) AND radius=IF(entry=611,5,10) AND Flags=0 AND SpellID=0 AND WorldSafeLocID=0 AND ScriptName='eventobject_stormheim_winter';
SELECT 'event-anchors',COUNT(*) FROM eventobject WHERE map=1220 AND zoneId=7541 AND areaId=7611 AND ((guid=410363 AND id=611 AND ABS(position_x-2299.91)<0.01 AND ABS(position_y-2343.38)<0.01 AND ABS(position_z-311.742)<0.01) OR(guid=410364 AND id=612 AND ABS(position_x-2174.72)<0.01 AND ABS(position_y-2454.51)<0.01 AND ABS(position_z-481.376)<0.01));
SELECT 'mandatory-stages',COUNT(*) FROM quest_objectives WHERE QuestID IN(38412,38618) AND Type=0 AND Amount=1 AND ((ObjectID=91728 AND StorageIndex=0 AND Flags=0) OR(ObjectID=91729 AND StorageIndex=1 AND Flags=2));
SELECT 'normal-flags',COUNT(*) FROM quest_template_addon WHERE ID IN(38412,38618) AND SpecialFlags=0;
SELECT 'dangling-hook-detached',COUNT(*) FROM quest_template WHERE ID=38618 AND StartScript=0;
SELECT 'native-next-links',COUNT(*) FROM quest_template WHERE ID IN(38412,38618) AND RewardNextQuest=38414;
SELECT 'loader-or-links',COUNT(*) FROM quest_template_addon WHERE ID IN(38412,38618) AND NextQuestID=38414;
SELECT 'native-next-prev',COUNT(*) FROM quest_template_addon WHERE ID=38414 AND PrevQuestID=38412;
SELECT 'vethir-spawn',COUNT(*) FROM creature WHERE guid=294030 AND id=91249 AND map=1220 AND zoneId=7541 AND ABS(position_z-485.375)<0.01;
SELECT 'vethir-hand-ins',COUNT(*) FROM creature_questender WHERE id=91249 AND quest IN(38412,38618);
SELECT 'vethir-next',COUNT(*) FROM creature_queststarter WHERE id=91249 AND quest=38414;
SELECT 'vethir-template',COUNT(*) FROM creature_template WHERE entry=91249 AND npcflag=2 AND AIName='SmartAI' AND ScriptName='';
SELECT 'native-range-model',COUNT(*) FROM creature_model_info WHERE DisplayID=62719 AND BoundingRadius=0.25 AND CombatReach=85;
SELECT 'first-gun-point',COUNT(*) FROM creature WHERE guid=294024 AND id=91975 AND map=1220 AND ABS(position_x-2299.91)<0.01 AND ABS(position_y-2343.66)<0.01 AND ABS(position_z-311.963)<0.01;
SELECT 'first-gun-binding',COUNT(*) FROM npc_spellclick_spells WHERE npc_entry=91975 AND spell_id=182478;
SELECT 'native-sai-retained',COUNT(*) FROM smart_scripts WHERE source_type=13 AND ((entryorguid=611 AND id=0 AND event_type=89 AND action_type=33 AND action_param1=91728) OR(entryorguid=612 AND id=0 AND event_type=89 AND action_type=33 AND action_param1=91729) OR(entryorguid=612 AND id=1 AND event_type=61 AND action_type=206 AND action_param1=2549));
SELECT 'conversation-data',COUNT(*) FROM conversation_data WHERE entry=2549 AND id=5378;
SELECT 'conversation-actor',COUNT(*) FROM conversation_creature WHERE entry=2549 AND creatureId=91249;
SELECT 'legacy-scene-conditions-retained',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=612 AND SourceId=10 AND ConditionValue1 IN(38412,38618);
SELECT 'next-hook-preserved',COUNT(*) FROM quest_template WHERE ID=38414 AND StartScript=38414;
'@
$expected=@("gun-types`t2","no-keg`t0","events`t2","event-anchors`t2","mandatory-stages`t4","normal-flags`t2","dangling-hook-detached`t1","native-next-links`t2","loader-or-links`t2","native-next-prev`t1","vethir-spawn`t1","vethir-hand-ins`t2","vethir-next`t1","vethir-template`t1","native-range-model`t1","first-gun-point`t1","first-gun-binding`t1","native-sai-retained`t3","conversation-data`t1","conversation-actor`t1","legacy-scene-conditions-retained`t2","next-hook-preserved`t1")
if(!$Live){
 $sql+=@"
SELECT 'preserved-quest-fields',COUNT(*) FROM quest_template a JOIN saved_quest_template b USING(ID) WHERE a.Flags<>b.Flags OR a.FlagsEx<>b.FlagsEx OR a.RewardNextQuest<>b.RewardNextQuest OR a.AllowableRaces<>b.AllowableRaces;
UPDATE eventobject_template SET ScriptName='custom' WHERE entry=611;
UPDATE eventobject_template SET Flags=1 WHERE entry=612;
UPDATE npc_spellclick_spells SET spell_id=121975,cast_flags=3 WHERE npc_entry=92017;
UPDATE quest_template SET StartScript=38618 WHERE ID=38618;
INSERT INTO quest_start_scripts VALUES(38618,1,0,123,0,0,0,0,0,0);
$migration
SELECT 'custom-event',COUNT(*) FROM eventobject_template WHERE entry=611 AND ScriptName='custom';
SELECT 'custom-flags',COUNT(*) FROM eventobject_template WHERE entry=612 AND Flags=1;
SELECT 'custom-gun-row',COUNT(*) FROM npc_spellclick_spells WHERE npc_entry=92017 AND spell_id=121975 AND cast_flags=3;
SELECT 'custom-start',COUNT(*) FROM quest_template WHERE ID=38618 AND StartScript=38618;
SELECT 'custom-action',COUNT(*) FROM quest_start_scripts WHERE id=38618 AND delay=1;
"@
 $expected+=@("preserved-quest-fields`t0","custom-event`t1","custom-flags`t1","custom-gun-row`t1","custom-start`t1","custom-action`t1")
}
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$actual=$sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --skip-column-names;if($LASTEXITCODE){throw 'Winter SQL failed'}}finally{$env:MYSQL_PWD=$previous}
if(($actual -join '|') -ne ($expected -join '|')){throw "Winter SQL mismatch: $($actual -join '|')"}
'PASS: migration twice, shared native gun and both event bindings/anchors/mandatory stages, native Vethir/conversation/next-loader relations, custom preservation.'
