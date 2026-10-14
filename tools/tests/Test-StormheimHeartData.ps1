param([switch]$Live)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$migration=Get-Content "$repo/sql/updates/world/2026_10_14_18_stormheim_heart.sql" -Raw
$sql=''
if(!$Live){
 $tables=@{eventobject_template='entry=613';creature_template='entry IN(91570,91737,96465,91249)';smart_scripts='source_type=0 AND entryorguid IN(-294116,91570,91737,96465)';quest_objectives='QuestID=38414';quest_template='ID IN(38414,39652)';quest_start_scripts='id=38414'}
 foreach($table in $tables.Keys){$sql+="CREATE TEMPORARY TABLE saved_$table AS SELECT * FROM $table WHERE $($tables[$table]); CREATE TEMPORARY TABLE $table AS SELECT * FROM saved_$table;`n"}
 $sql+=@"
UPDATE eventobject_template SET ScriptName='SmartEventObject' WHERE entry=613;
UPDATE creature_template SET AIName='SmartAI',ScriptName='' WHERE entry=91570;
UPDATE smart_scripts SET action_param1=0 WHERE entryorguid=91570 AND id=2;
DELETE FROM smart_scripts WHERE entryorguid=-294116 AND source_type=0;
UPDATE quest_objectives SET Flags=2 WHERE ID=277831;
UPDATE quest_template SET StartScript=38414 WHERE ID=38414;
DELETE FROM quest_start_scripts WHERE id=38414;
$migration
$migration
"@
}
$sql+=@'
SELECT 'approach-event',COUNT(*) FROM eventobject_template WHERE entry=613 AND radius=10 AND Flags=0 AND SpellID=0 AND WorldSafeLocID=0 AND ScriptName='eventobject_stormheim_heart';
SELECT 'approach-anchor',COUNT(*) FROM eventobject WHERE guid=410365 AND id=613 AND map=1220 AND zoneId=7541 AND ABS(position_x-2009.2)<0.01 AND ABS(position_y-2578.84)<0.01 AND ABS(position_z-486.069)<0.01;
SELECT 'mandatory-objectives',COUNT(*) FROM quest_objectives WHERE QuestID=38414 AND Type=0 AND Amount=1 AND ((ID=277830 AND ObjectID=91734 AND StorageIndex=0 AND Flags=0) OR(ID=277831 AND ObjectID=91737 AND StorageIndex=1 AND Flags=3));
SELECT 'mother-binding',COUNT(*) FROM creature_template WHERE entry=91570 AND AIName='' AND ScriptName='npc_stormheim_heart_mother' AND faction=190 AND unit_flags=33536;
SELECT 'mother-sleep-pose',COUNT(*) FROM creature_template_addon WHERE entry=91570 AND bytes1=3;
SELECT 'mother-spawn',COUNT(*) FROM creature WHERE guid=294116 AND id=91570 AND map=1220 AND zoneId=7541 AND ABS(position_z-467.371)<0.01;
SELECT 'boss-preserved',COUNT(*) FROM creature_template WHERE entry=91737 AND AIName='SmartAI' AND ScriptName='' AND faction=14 AND unit_flags=32768;
SELECT 'boss-spawn',COUNT(*) FROM creature WHERE guid=294111 AND id=91737 AND map=1220 AND zoneId=7541 AND ABS(position_z-467.236)<0.01;
SELECT 'boss-native-credit',COUNT(*) FROM creature_template_wdb WHERE Entry=91737 AND KillCredit1=0 AND KillCredit2=0;
SELECT 'boss-native-actions',COUNT(*) FROM smart_scripts WHERE source_type=0 AND entryorguid=91737 AND id IN(0,1,2,3,4,5);
SELECT 'mother-native-dialogue',COUNT(*) FROM smart_scripts WHERE source_type=0 AND entryorguid=91570 AND ((id=0 AND event_type=38 AND event_param1=1 AND event_param2=1 AND action_type=1 AND action_param1=3 AND action_param2=6000) OR(id=1 AND event_type=52 AND event_param1=3 AND action_type=1 AND action_param1=2 AND action_param2=6000));
SELECT 'scoped-guid-list',COUNT(*) FROM smart_scripts WHERE source_type=0 AND entryorguid=-294116 AND id IN(0,1,2);
SELECT 'entry-aura-action-preserved',COUNT(*) FROM smart_scripts WHERE source_type=0 AND entryorguid=91570 AND id=2 AND action_type=28 AND action_param1=0;
SELECT 'scoped-aura-removal',COUNT(*) FROM smart_scripts WHERE source_type=0 AND entryorguid=-294116 AND id=2 AND event_type=61 AND action_type=28 AND action_param1=182091 AND target_type=1;
SELECT 'native-corruption-target',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=13 AND SourceEntry=182091 AND ConditionTypeOrReference=31 AND ConditionValue1=3 AND ConditionValue2=91570;
SELECT 'native-corruption-addon',COUNT(*) FROM creature_template_addon WHERE entry=91737 AND auras='182091';
SELECT 'normal-flags',COUNT(*) FROM quest_template_addon WHERE ID=38414 AND SpecialFlags=0;
SELECT 'empty-start-detached',COUNT(*) FROM quest_template WHERE ID=38414 AND StartScript=0;
SELECT 'native-next-link',COUNT(*) FROM quest_template WHERE ID=38414 AND RewardNextQuest=39652;
SELECT 'native-loader-link',COUNT(*) FROM quest_template_addon WHERE ID=38414 AND NextQuestID=39652;
SELECT 'next-alternative-preserved',COUNT(*) FROM quest_template_addon WHERE ID=39652 AND PrevQuestID=40568;
SELECT 'actual-hand-in-next',COUNT(*) FROM creature WHERE guid=146706156 AND id=96465 AND map=1220;
SELECT 'hand-in-relation',COUNT(*) FROM creature_questender WHERE id=96465 AND quest=38414;
SELECT 'next-relation',COUNT(*) FROM creature_queststarter WHERE id=96465 AND quest=39652;
SELECT 'next-sai-preserved',COUNT(*) FROM smart_scripts WHERE source_type=0 AND entryorguid=96465 AND id IN(0,1);
'@
$expected=@("approach-event`t1","approach-anchor`t1","mandatory-objectives`t2","mother-binding`t1","mother-sleep-pose`t1","mother-spawn`t1","boss-preserved`t1","boss-spawn`t1","boss-native-credit`t1","boss-native-actions`t6","mother-native-dialogue`t2","scoped-guid-list`t3","entry-aura-action-preserved`t1","scoped-aura-removal`t1","native-corruption-target`t1","native-corruption-addon`t1","normal-flags`t1","empty-start-detached`t1","native-next-link`t1","native-loader-link`t1","next-alternative-preserved`t1","actual-hand-in-next`t1","hand-in-relation`t1","next-relation`t1","next-sai-preserved`t2")
if(!$Live){
 $sql+=@"
SELECT 'preserved-quest-fields',COUNT(*) FROM quest_template a JOIN saved_quest_template b USING(ID) WHERE a.Flags<>b.Flags OR a.FlagsEx<>b.FlagsEx OR a.RewardNextQuest<>b.RewardNextQuest OR a.AllowableRaces<>b.AllowableRaces;
UPDATE eventobject_template SET ScriptName='custom' WHERE entry=613;
UPDATE creature_template SET ScriptName='custom' WHERE entry=91570;
DELETE FROM smart_scripts WHERE entryorguid=-294116 AND source_type=0;
UPDATE smart_scripts SET action_param1=0 WHERE entryorguid=91570 AND id=2;
DELETE FROM smart_scripts WHERE entryorguid=-294116 AND source_type=0;
UPDATE quest_objectives SET Flags=7 WHERE ID=277831;
UPDATE quest_template SET StartScript=38414 WHERE ID=38414;
INSERT INTO quest_start_scripts VALUES(38414,1,0,123,0,0,0,0,0,0);
$migration
SELECT 'custom-event',COUNT(*) FROM eventobject_template WHERE entry=613 AND ScriptName='custom';
SELECT 'custom-mother',COUNT(*) FROM creature_template WHERE entry=91570 AND ScriptName='custom';
SELECT 'custom-guid-list-not-created',COUNT(*) FROM smart_scripts WHERE entryorguid=-294116 AND source_type=0;
SELECT 'custom-mother-aura-action',COUNT(*) FROM smart_scripts WHERE entryorguid=91570 AND id=2 AND action_param1=0;
SELECT 'custom-objective',COUNT(*) FROM quest_objectives WHERE ID=277831 AND Flags=7;
SELECT 'custom-start',COUNT(*) FROM quest_template WHERE ID=38414 AND StartScript=38414;
SELECT 'custom-action',COUNT(*) FROM quest_start_scripts WHERE id=38414 AND delay=1;
UPDATE creature_template SET ScriptName='',AIName='SmartAI' WHERE entry=91570;
INSERT INTO smart_scripts(entryorguid,source_type,id,link,Difficulties,event_type,event_phase_mask,event_chance,event_flags,event_param1,event_param2,event_param3,event_param4,event_param5,action_type,action_param1,action_param2,action_param3,action_param4,action_param5,action_param6,target_type,target_param1,target_param2,target_param3,target_param4,target_x,target_y,target_z,target_o,comment)
VALUES(-294116,0,42,0,'',61,0,100,0,0,0,0,0,0,1,99,0,0,0,0,0,1,0,0,0,0,0,0,0,0,'custom GUID');
$migration
SELECT 'custom-guid-binding-preserved',COUNT(*) FROM creature_template WHERE entry=91570 AND ScriptName='' AND AIName='SmartAI';
SELECT 'custom-guid-action-preserved',COUNT(*) FROM smart_scripts WHERE entryorguid=-294116 AND id=42 AND action_param1=99;
SELECT 'no-guid-list-merge',COUNT(*) FROM smart_scripts WHERE entryorguid=-294116 AND id IN(0,1,2);
"@
 $expected+=@("preserved-quest-fields`t0","custom-event`t1","custom-mother`t1","custom-guid-list-not-created`t0","custom-mother-aura-action`t1","custom-objective`t1","custom-start`t1","custom-action`t1","custom-guid-binding-preserved`t1","custom-guid-action-preserved`t1","no-guid-list-merge`t0")
}
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$actual=$sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --skip-column-names;if($LASTEXITCODE){throw 'Heart SQL failed'}}finally{$env:MYSQL_PWD=$previous}
if(($actual -join '|') -ne ($expected -join '|')){throw "Heart SQL mismatch: $($actual -join '|')"}
'PASS: migration twice, native mandatory stages/minimap, grounded event binding, scoped mother/pose/corruption/dialogue, unchanged boss/combat/next and custom preservation.'
