$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$migration=Get-Content "$repo/sql/updates/world/2026_10_10_01_valsharah_chain.sql" -Raw
$tables=@('quest_objectives','gameobject_template','quest_start_scripts','quest_template','quest_template_addon','smart_scripts','creature_queststarter','creature_questender','conditions')
$fixture=($tables|ForEach-Object {"CREATE TEMPORARY TABLE saved_$_ LIKE $_; INSERT INTO saved_$_ SELECT * FROM $_; CREATE TEMPORARY TABLE $_ LIKE saved_$_; INSERT INTO $_ SELECT * FROM saved_$_;"}) -join "`n"
$native=Get-Content "$repo/docs/audits/valsharah-chain-native-2026-10-10.json" -Raw|ConvertFrom-Json
$nativeRows=($native.objectives|ForEach-Object {"SELECT $($_.ID) ID,$($_.QuestID) QuestID,$($_.Type) Type,$($_.StorageIndex) StorageIndex,$($_.ObjectID) ObjectID,$($_.Amount) Amount,$($_.Flags) Flags,$($_.Flags2) Flags2"}) -join ' UNION ALL '
$fixture+=@"
$migration
$migration
SELECT 'native',COUNT(*) FROM ($nativeRows) a LEFT JOIN quest_objectives b USING(ID) WHERE b.ID IS NULL OR a.QuestID<>b.QuestID OR a.Type<>b.Type OR a.StorageIndex<>b.StorageIndex OR a.ObjectID<>b.ObjectID OR a.Amount<>b.Amount OR a.Flags<>b.Flags OR a.Flags2<>b.Flags2;
SELECT 'collision',COUNT(*) FROM (SELECT QuestID,StorageIndex FROM quest_objectives WHERE QuestID IN(38582,39384,40573) GROUP BY QuestID,StorageIndex HAVING COUNT(*)>1) a;
SELECT 'shortcuts',COUNT(*) FROM quest_start_scripts WHERE id IN(38142,38381,38384,38225,38235) AND command=7;
SELECT 'event-locks',COUNT(*) FROM quest_template_addon WHERE ID IN(38142,38381,38382,38384,38225,38235,38147,39384) AND (SpecialFlags&2)<>0;
SELECT 'wall',COUNT(*) FROM gameobject_template WHERE entry=242279 AND ScriptName='go_valsharah_bramble_wall';
SELECT 'koda-return',COUNT(*) FROM creature_queststarter WHERE id=91223 AND quest=38148;
SELECT 'wrong-enders',COUNT(*) FROM creature_questender WHERE (id=94179 AND quest=38142) OR (id=97140 AND quest=43576);
SELECT 'regroup',QuestSortID,StartItem,ItemDrop1,ItemDropQuantity1 FROM quest_template WHERE ID=43576;
SELECT 'tears-prerequisite',PrevQuestID FROM quest_template_addon WHERE ID=40890;
SELECT 'reward',ID,RewardItem1,RewardAmount1 FROM quest_template WHERE ID IN(38377,38743,38753) ORDER BY ID;
SELECT 'placement',QuestPackageID,StartItem,ItemDrop1,ItemDropQuantity1 FROM quest_template WHERE ID=40890;
SELECT 'proximity-credit',COUNT(*) FROM smart_scripts WHERE entryorguid=106815 AND action_type=33 AND action_param1=109750;
SELECT 'ride-kept',COUNT(*) FROM smart_scripts WHERE entryorguid=91109 AND source_type=0 AND id=1 AND action_type=15 AND action_param1=40122;
SELECT 'barrow-and',COUNT(DISTINCT ElseGroup),COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=19 AND SourceEntry=38147 AND ConditionTypeOrReference=8 AND ConditionValue1 IN(38144,38145);
SELECT 'mutual-gossip',COUNT(*) FROM (SELECT ElseGroup FROM conditions WHERE SourceTypeOrReferenceId=15 AND SourceGroup=19419 AND SourceEntry=1 GROUP BY ElseGroup HAVING COUNT(DISTINCT ConditionValue1)>1) a;
SELECT 'unrelated-scripts',COUNT(*) FROM gameobject_template a JOIN saved_gameobject_template b USING(entry) WHERE a.entry<>242279 AND a.ScriptName<>b.ScriptName;
CREATE TEMPORARY TABLE completed_quests(id INT PRIMARY KEY);
INSERT INTO completed_quests VALUES (41890),(38675),(38684),(41749);
SELECT 'horde-shared',COUNT(DISTINCT SourceEntry) FROM (SELECT SourceEntry,ElseGroup FROM conditions WHERE SourceTypeOrReferenceId=19 AND SourceEntry IN(43576,38684,43702) GROUP BY SourceEntry,ElseGroup HAVING SUM(ConditionValue1 NOT IN(SELECT id FROM completed_quests))=0) passed;
DELETE FROM completed_quests;
INSERT INTO completed_quests VALUES (41708),(41724),(38684),(41893);
SELECT 'alliance-shared',COUNT(DISTINCT SourceEntry) FROM (SELECT SourceEntry,ElseGroup FROM conditions WHERE SourceTypeOrReferenceId=19 AND SourceEntry IN(43576,38684,43702) GROUP BY SourceEntry,ElseGroup HAVING SUM(ConditionValue1 NOT IN(SELECT id FROM completed_quests))=0) passed;
DELETE FROM completed_quests;
SELECT 'fresh-player',COUNT(*) FROM (SELECT SourceEntry,ElseGroup FROM conditions WHERE SourceTypeOrReferenceId=19 AND SourceEntry IN(43576,38684,43702) GROUP BY SourceEntry,ElseGroup HAVING SUM(ConditionValue1 NOT IN(SELECT id FROM completed_quests))=0) passed;
UPDATE gameobject_template SET ScriptName='custom_wall' WHERE entry=242279;
$migration
SELECT 'custom',ScriptName FROM gameobject_template WHERE entry=242279;
"@
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$old=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$rows=& 'C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe' --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --skip-column-names --execute=$fixture;if($LASTEXITCODE){throw 'Valsharah SQL fixture failed'}}finally{$env:MYSQL_PWD=$old}
$expected=@("native`t0","collision`t0","shortcuts`t0","event-locks`t0","wall`t1","koda-return`t1","wrong-enders`t0","regroup`t7558`t0`t0`t0","tears-prerequisite`t38743","reward`t38377`t141387`t1","reward`t38743`t141383`t1","reward`t38753`t141390`t1","placement`t665`t139043`t139043`t1","proximity-credit`t0","ride-kept`t1","barrow-and`t1`t2","mutual-gossip`t0","unrelated-scripts`t0","horde-shared`t3","alliance-shared`t3","fresh-player`t0","custom`tcustom_wall")
if(($rows -join '|') -ne ($expected -join '|')){throw "SQL mismatch: $($rows -join '|')"}
'PASS: migration twice in temporary tables; 58 native objectives, isolated counters, actual actions, faction branches, rewards, provided/removed Tears, custom scripts and existing Malfurion ride preserved.'
