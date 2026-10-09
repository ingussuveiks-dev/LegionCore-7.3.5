$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$migration=Get-Content "$repo/sql/updates/world/2026_10_09_08_azsuna_scythe.sql" -Raw
$tables=@('quest_objectives','creature_template','smart_scripts','gameobject_template','gameobject')
$fixture=($tables | ForEach-Object {"CREATE TEMPORARY TABLE saved_$_ LIKE $_; INSERT INTO saved_$_ SELECT * FROM $_; CREATE TEMPORARY TABLE $_ LIKE saved_$_; INSERT INTO $_ SELECT * FROM saved_$_;"}) -join "`n"
$fixture+=@"
DELETE FROM quest_objectives WHERE QuestID=37660;
DELETE FROM gameobject WHERE id=237017 AND map=1220;
UPDATE gameobject SET phaseMask=2 WHERE guid=127745 AND id=240012;
$migration
$migration
SELECT 'objectives',COUNT(*),COUNT(DISTINCT StorageIndex),SUM(Amount) FROM quest_objectives WHERE QuestID=37660;
SELECT 'targets',GROUP_CONCAT(CONCAT(ID,':',Type,':',StorageIndex,':',ObjectID,':',Flags) ORDER BY StorageIndex) FROM quest_objectives WHERE QuestID=37660;
SELECT 'premature-return',COUNT(*) FROM smart_scripts WHERE entryorguid=89398 AND source_type=0 AND event_type=60 AND action_type=33 AND action_param1=89398;
SELECT 'guide',AIName,ScriptName FROM creature_template WHERE entry=90401;
SELECT 'gems',COUNT(*) FROM gameobject_template WHERE entry IN(237017,240012,239338,239332) AND ScriptName='go_azsuna_soul_gem';
SELECT 'entry',COUNT(*) FROM gameobject WHERE id=237017 AND map=1220 AND phaseMask=1 AND PhaseId='4264';
SELECT 'exit',phaseMask FROM gameobject WHERE guid=127745 AND id=240012;
SELECT 'unrelated-objectives',COUNT(*) FROM quest_objectives a JOIN saved_quest_objectives b USING(ID) WHERE a.QuestID<>37660 AND (a.Type<>b.Type OR a.StorageIndex<>b.StorageIndex OR a.ObjectID<>b.ObjectID OR a.Amount<>b.Amount OR a.Flags<>b.Flags);
SELECT 'unrelated-spawns',COUNT(*) FROM saved_gameobject a LEFT JOIN gameobject b USING(guid) WHERE a.id NOT IN(237017,240012) AND (b.guid IS NULL OR a.id<>b.id OR a.map<>b.map OR a.phaseMask<>b.phaseMask OR a.PhaseId<>b.PhaseId OR a.position_x<>b.position_x OR a.position_y<>b.position_y);
UPDATE gameobject_template SET ScriptName='custom_gem' WHERE entry=237017;
UPDATE quest_objectives SET Description='custom description' WHERE ID=277151;
$migration
SELECT 'custom-binding',ScriptName FROM gameobject_template WHERE entry=237017;
SELECT 'custom-objective',Description FROM quest_objectives WHERE ID=277151;
"@
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$mysql=Get-ChildItem C:/wamp64/bin/mysql -Recurse -Filter mysql.exe | Select-Object -First 1 -ExpandProperty FullName
$previous=$env:MYSQL_PWD
try {
 $env:MYSQL_PWD=$connection[3]
 $rows=& $mysql --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --skip-column-names --execute=$fixture
 if($LASTEXITCODE){throw 'Azsuna SQL fixture failed'}
 $expected=@("objectives`t10`t10`t10",
 "targets`t277151:0:0:90403:0,277152:2:1:239338:2,277153:0:2:90402:2,277154:0:3:90401:28,277157:0:4:89276:2,277156:2:5:239332:2,277158:2:6:237017:2,277159:0:7:89673:3,277160:2:8:240012:6,286117:0:9:89398:0",
 "premature-return`t0","guide`t`tnpc_azsuna_allari_q37660","gems`t4","entry`t1","exit`t1","unrelated-objectives`t0","unrelated-spawns`t0","custom-binding`tcustom_gem","custom-objective`tcustom description")
 if(($rows -join '|') -ne ($expected -join '|')){throw "SQL mismatch: $($rows -join '|')"}
} finally {$env:MYSQL_PWD=$previous}
'PASS: production Azsuna migration twice in temporary tables; exact original objectives, entry/exit, bindings, unrelated and custom data preserved.'
