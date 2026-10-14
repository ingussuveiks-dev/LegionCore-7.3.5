param([switch]$Live)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$migration=Get-Content "$repo/sql/updates/world/2026_10_14_16_stormheim_another_way.sql" -Raw
$sql=''
if(!$Live){
 $tables=@{quest_objectives='QuestID IN(38342,38617)';creature_template='entry IN(91240,91558,92569,98255,92566)';smart_scripts='source_type=0 AND entryorguid IN(91240,91553,91558,92569,98255,92566)';quest_template='ID IN(38342,38617,38412,38618)';quest_start_scripts='id=38617'}
 foreach($table in $tables.Keys){$sql+="CREATE TEMPORARY TABLE saved_$table AS SELECT * FROM $table WHERE $($tables[$table]); CREATE TEMPORARY TABLE $table AS SELECT * FROM saved_$table;`n"}
 $sql+=@"
UPDATE quest_objectives SET Flags=0 WHERE ID IN(277721,278227);
UPDATE creature_template SET AIName='SmartAI',ScriptName='' WHERE entry IN(91558,92569);
UPDATE quest_template SET StartScript=38617 WHERE ID=38617;
DELETE FROM quest_start_scripts WHERE id=38617;
DELETE FROM smart_scripts WHERE entryorguid=98255 AND source_type=0 AND id=0;
INSERT INTO smart_scripts(entryorguid,source_type,id,link,Difficulties,event_type,event_phase_mask,event_chance,event_flags,event_param1,event_param2,event_param3,event_param4,event_param5,action_type,action_param1,action_param2,action_param3,action_param4,action_param5,action_param6,target_type,target_param1,target_param2,target_param3,target_param4,target_x,target_y,target_z,target_o,comment)
VALUES(98255,0,0,0,'',60,0,100,0,1000,1000,1000,1000,0,33,98255,0,0,0,0,0,18,10,0,0,0,0,0,0,0,'legacy tower credit');
$migration
$migration
"@
}
$sql+=@'
SELECT 'native-objectives',COUNT(*) FROM quest_objectives WHERE (ID=277721 AND QuestID=38342 AND ObjectID=91240 AND StorageIndex=0 AND Flags=1) OR(ID=278227 AND QuestID=38617 AND ObjectID=91240 AND StorageIndex=0 AND Flags=1) OR(ID=277825 AND QuestID=38342 AND ObjectID=98255 AND StorageIndex=1 AND Flags=2) OR(ID=278228 AND QuestID=38617 AND ObjectID=98255 AND StorageIndex=1 AND Flags=2);
SELECT 'mandatory-counts',COUNT(*) FROM quest_objectives WHERE QuestID IN(38342,38617) AND Type=0 AND Amount=1 AND (Flags&4)=0;
SELECT 'guide-bindings',COUNT(*) FROM creature_template WHERE entry IN(91558,92569) AND AIName='' AND ScriptName='npc_stormheim_another_way_guide' AND (npcflag&2)=2;
SELECT 'guide-native-actions',COUNT(*) FROM smart_scripts WHERE source_type=0 AND entryorguid IN(91558,92569) AND ((id=0 AND event_type=54 AND link=1 AND action_type=207 AND target_type=7) OR(id=1 AND event_type=61 AND action_type=29 AND target_type=7) OR(id=2 AND event_type=38 AND event_param1=1 AND event_param2=1 AND action_type=1 AND target_type=23));
SELECT 'native-alliance-summon',COUNT(*) FROM smart_scripts WHERE entryorguid=91553 AND source_type=0 AND event_type=19 AND event_param1=38342 AND action_type=85 AND action_param1=205485 AND target_type=7;
SELECT 'no-blind-tower-credit',COUNT(*) FROM smart_scripts WHERE entryorguid=98255 AND source_type=0 AND id=0 AND event_type=60 AND action_type=33 AND action_param1=98255 AND target_type=18 AND target_param1=10;
SELECT 'empty-start-detached',COUNT(*) FROM quest_template WHERE ID=38617 AND StartScript=0;
SELECT 'unchanged-normal-flags',COUNT(*) FROM quest_template_addon WHERE ID IN(38342,38617) AND SpecialFlags=0;
SELECT 'next-links',COUNT(*) FROM quest_template WHERE (ID=38342 AND RewardNextQuest=38412) OR(ID=38617 AND RewardNextQuest=38618);
SELECT 'next-prerequisites',COUNT(*) FROM quest_template_addon WHERE (ID=38412 AND PrevQuestID=38342) OR(ID=38618 AND PrevQuestID=38617);
SELECT 'next-hook-preserved',COUNT(*) FROM quest_template WHERE ID=38618 AND StartScript=38618;
SELECT 'boss-ai-preserved',COUNT(*) FROM smart_scripts WHERE source_type=0 AND entryorguid=91240 AND id IN(0,1,2,3);
SELECT 'boss-alias-preserved',COUNT(*) FROM creature_template_wdb WHERE Entry=91240 AND KillCredit1=113889 AND KillCredit2=0;
SELECT 'no-static-tower',COUNT(*) FROM creature WHERE id=98255;
SELECT 'native-guide-relations',COUNT(*) FROM creature_questender WHERE (id=91558 AND quest=38342) OR(id=92569 AND quest=38617);
SELECT 'native-next-relations',COUNT(*) FROM creature_queststarter WHERE (id=91558 AND quest=38412) OR(id=92569 AND quest=38618);
SELECT 'fallback-script-preserved',COUNT(*) FROM creature_template WHERE entry=92566 AND ScriptName='npc_stormheim_grapple_supply';
SELECT 'fallback-next',COUNT(*) FROM creature_queststarter WHERE id=92566 AND quest=38618;
SELECT 'fallback-hand-in',COUNT(*) FROM creature_questender WHERE id=92566 AND quest=38617;
SELECT 'native-tower-poi',COUNT(*) FROM quest_poi a JOIN quest_poi_points b ON a.QuestID=b.QuestID AND a.Idx1=b.Idx1 WHERE a.QuestID IN(38342,38617) AND a.QuestObjectID=98255 AND a.ObjectiveIndex=1 AND a.MapID=1220 AND b.X=2348 AND b.Y=2380;
'@
$expected=@("native-objectives`t4","mandatory-counts`t4","guide-bindings`t2","guide-native-actions`t6","native-alliance-summon`t1","no-blind-tower-credit`t0","empty-start-detached`t1","unchanged-normal-flags`t2","next-links`t2","next-prerequisites`t2","next-hook-preserved`t1","boss-ai-preserved`t4","boss-alias-preserved`t1","no-static-tower`t0","native-guide-relations`t2","native-next-relations`t2","fallback-script-preserved`t1","fallback-next`t1","fallback-hand-in`t1","native-tower-poi`t2")
if(!$Live){
 $sql+=@"
SELECT 'preserved-quest-fields',COUNT(*) FROM quest_template a JOIN saved_quest_template b USING(ID) WHERE a.Flags<>b.Flags OR a.FlagsEx<>b.FlagsEx OR a.RewardNextQuest<>b.RewardNextQuest OR a.AllowableRaces<>b.AllowableRaces;
SELECT 'preserved-guide-fields',COUNT(*) FROM creature_template a JOIN saved_creature_template b USING(entry) WHERE a.npcflag<>b.npcflag OR a.faction<>b.faction OR a.unit_flags<>b.unit_flags;
SELECT 'preserved-native-ai',COUNT(*) FROM smart_scripts a JOIN saved_smart_scripts b USING(entryorguid,source_type,id) WHERE a.entryorguid<>98255 AND (a.link<>b.link OR a.event_type<>b.event_type OR a.action_type<>b.action_type OR a.action_param1<>b.action_param1 OR a.target_type<>b.target_type);
UPDATE creature_template SET ScriptName='custom' WHERE entry=92569;
UPDATE quest_objectives SET Flags=5 WHERE ID=278227;
UPDATE quest_template SET StartScript=38617 WHERE ID=38617;
INSERT INTO quest_start_scripts VALUES(38617,1,0,123,0,0,0,0,0,0);
INSERT INTO smart_scripts(entryorguid,source_type,id,link,Difficulties,event_type,event_phase_mask,event_chance,event_flags,event_param1,event_param2,event_param3,event_param4,event_param5,action_type,action_param1,action_param2,action_param3,action_param4,action_param5,action_param6,target_type,target_param1,target_param2,target_param3,target_param4,target_x,target_y,target_z,target_o,comment) VALUES(98255,0,9,0,'',60,0,100,0,1000,1000,1000,1000,0,33,98255,0,0,0,0,0,18,10,0,0,0,0,0,0,0,'custom id'),(98255,0,0,0,'',60,0,100,0,1000,1000,1000,1000,0,33,98255,0,0,0,0,0,18,9,0,0,0,0,0,0,0,'custom range');
$migration
SELECT 'custom-binding',COUNT(*) FROM creature_template WHERE entry=92569 AND ScriptName='custom';
SELECT 'custom-objective',COUNT(*) FROM quest_objectives WHERE ID=278227 AND Flags=5;
SELECT 'custom-start',COUNT(*) FROM quest_template WHERE ID=38617 AND StartScript=38617;
SELECT 'custom-action',COUNT(*) FROM quest_start_scripts WHERE id=38617 AND delay=1;
SELECT 'custom-tower-row',COUNT(*) FROM smart_scripts WHERE entryorguid=98255 AND ((id=9 AND target_param1=10) OR(id=0 AND target_param1=9));
"@
 $expected+=@("preserved-quest-fields`t0","preserved-guide-fields`t0","preserved-native-ai`t0","custom-binding`t1","custom-objective`t1","custom-start`t1","custom-action`t1","custom-tower-row`t2")
}
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$actual=$sql | & 'C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe' --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --skip-column-names;if($LASTEXITCODE){throw 'Another Way SQL failed'}}finally{$env:MYSQL_PWD=$previous}
if(($actual -join '|') -ne ($expected -join '|')){throw "Another Way SQL mismatch: $($actual -join '|')"}
'PASS: migration twice, both mandatory native stages, personal guide bindings/native actions, native kill alias/combat, tower POI, dangling start cleanup, next relations and custom preservation.'
