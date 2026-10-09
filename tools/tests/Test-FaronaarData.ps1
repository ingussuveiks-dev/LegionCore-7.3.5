$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$migration=Get-Content "$repo/sql/updates/world/2026_10_09_09_faronaar_chain.sql" -Raw
$tables=@('quest_objectives','creature_template','smart_scripts','gameobject_template','creature','spell_area')
$fixture=($tables | ForEach-Object {"CREATE TEMPORARY TABLE saved_$_ LIKE $_; INSERT INTO saved_$_ SELECT * FROM $_; CREATE TEMPORARY TABLE $_ LIKE saved_$_; INSERT INTO $_ SELECT * FROM saved_$_;"}) -join "`n"
$fixture+=@"
$migration
$migration
SELECT 'objects',COUNT(*) FROM gameobject_template WHERE entry IN(239455,240075,240121,240122,240123) AND ScriptName='go_faronaar_objective';
SELECT 'personal',COUNT(*) FROM creature_template WHERE entry IN(90546,90578) AND AIName='' AND ScriptName='npc_faronaar_personal_actor';
SELECT 'shared-spawns',COUNT(*) FROM creature WHERE (guid=338233 AND id=90546) OR (guid IN(269444,269445,269470) AND id=90578);
SELECT 'shared-credit',COUNT(*) FROM smart_scripts WHERE entryorguid=9048700 AND source_type=9 AND action_type=33 AND action_param1=90487;
SELECT 'key',Flags2 FROM quest_objectives WHERE ID=277456;
SELECT 'old-companion',COUNT(*) FROM spell_area WHERE spell=178860 AND quest_start=36920;
SELECT 'board-start',event_type,event_flags,action_type,action_param2,target_type FROM smart_scripts WHERE entryorguid=90982 AND source_type=0 AND id=1;
SELECT 'arrival',event_type,event_param1,link,action_type,action_param1,target_type FROM smart_scripts WHERE entryorguid=90982 AND source_type=0 AND id=3;
SELECT 'board-credit',event_type,action_type,action_param1,target_type FROM smart_scripts WHERE entryorguid=90982 AND source_type=0 AND id=5;
SELECT 'unboard',event_type,action_type,action_param1 FROM smart_scripts WHERE entryorguid=90982 AND source_type=0 AND id=6;
SELECT 'premature-return',COUNT(*) FROM smart_scripts WHERE entryorguid=90982 AND action_type=33 AND action_param1=112175;
SELECT 'unrelated-spawns',COUNT(*) FROM saved_creature a LEFT JOIN creature b USING(guid) WHERE a.guid NOT IN(338233,269444,269445,269470) AND (b.guid IS NULL OR a.id<>b.id OR a.map<>b.map OR a.PhaseId<>b.PhaseId OR a.position_x<>b.position_x OR a.position_y<>b.position_y OR a.position_z<>b.position_z);
SELECT 'unrelated-objectives',COUNT(*) FROM saved_quest_objectives a JOIN quest_objectives b USING(ID) WHERE a.ID<>277456 AND (a.Amount<>b.Amount OR a.Flags<>b.Flags OR a.Flags2<>b.Flags2 OR a.ObjectID<>b.ObjectID);
SELECT 'key-loot',Chance,QuestRequired FROM creature_loot_template WHERE Entry=86535 AND Item=120359;
SELECT 'both-quests',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=19 AND SourceEntry=37449 AND ElseGroup=0 AND ConditionTypeOrReference=8 AND ConditionValue1 IN(37450,37656) AND NegativeCondition=0;
SELECT 'native-route',COUNT(*) FROM waypoints WHERE entry=90982;
UPDATE creature_template SET ScriptName='custom_dragon' WHERE entry=90546;
UPDATE gameobject_template SET ScriptName='custom_lock' WHERE entry=239455;
$migration
SELECT 'custom-dragon',ScriptName FROM creature_template WHERE entry=90546;
SELECT 'custom-lock',ScriptName FROM gameobject_template WHERE entry=239455;
"@
$mysql=Get-ChildItem C:/wamp64/bin/mysql -Recurse -Filter mysql.exe | Select-Object -First 1 -ExpandProperty FullName
function Invoke-Fixture($kind,$sql) {
 $connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match "^${kind}DatabaseInfo\s*="}).Split('"')[1].Split(';')
 $previous=$env:MYSQL_PWD
 try {
  $env:MYSQL_PWD=$connection[3]
  $rows=& $mysql --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --skip-column-names --execute=$sql
  if($LASTEXITCODE){throw 'Faronaar SQL fixture failed'}
  return $rows
 } finally {$env:MYSQL_PWD=$previous}
}
$rows=Invoke-Fixture World $fixture
$expected=@("objects`t5","personal`t2","shared-spawns`t0","shared-credit`t0","key`t1","old-companion`t0","board-start`t27`t1`t53`t90982`t7","arrival`t40`t7`t0`t41`t1000`t1","board-credit`t27`t33`t90982`t7","unboard`t28`t41`t1000","premature-return`t0","unrelated-spawns`t0","unrelated-objectives`t0","key-loot`t100`t1","both-quests`t2","native-route`t7","custom-dragon`tcustom_dragon","custom-lock`tcustom_lock")
if(($rows -join '|') -ne ($expected -join '|')){throw "World SQL mismatch: $($rows -join '|')"}
$characterMigration=(Get-Content "$repo/sql/updates/characters/2026_10_09_09_faronaar_interactions.sql" -Raw).Replace('CREATE TABLE IF NOT EXISTS','CREATE TEMPORARY TABLE IF NOT EXISTS')
$characterFixture=@"
$characterMigration
$characterMigration
INSERT INTO character_faronaar_interactions VALUES (1,37450,109169,1),(2,37450,109169,1),(1,37656,109119,1);
INSERT IGNORE INTO character_faronaar_interactions VALUES (1,37450,109169,2);
SELECT 'distinct',COUNT(*),SUM(ordinal) FROM character_faronaar_interactions;
DELETE FROM character_faronaar_interactions WHERE guid=1 AND quest=37450 AND ordinal>0;
SELECT 'isolated-reset',COUNT(*) FROM character_faronaar_interactions;
"@
$rows=Invoke-Fixture Character $characterFixture
if(($rows -join '|') -ne "distinct`t3`t3|isolated-reset`t2"){throw 'Character ledger isolation failed'}
'PASS: world migration twice; scoped personal actors, native objectives/loot/route and custom data. Character schema tested only in temporary tables; player/quest use isolation holds.'
