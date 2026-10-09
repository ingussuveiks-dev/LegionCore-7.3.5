$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$migration=(6..7 | ForEach-Object {Get-Content (Get-ChildItem "$repo/sql/updates/world/2026_10_09_0$($_)_*.sql").FullName -Raw}) -join "`n"
$tables=@('creature_template','quest_template','quest_template_addon','instance_template','creature','gameobject','conditions','spell_script_names')
$fixture=($tables | ForEach-Object {"CREATE TEMPORARY TABLE saved_$_ LIKE $_; INSERT INTO saved_$_ SELECT * FROM $_; CREATE TEMPORARY TABLE $_ LIKE saved_$_; INSERT INTO $_ SELECT * FROM saved_$_;"}) -join "`n"
$fixture+=@"
DELETE FROM instance_template WHERE map=1191;
DELETE FROM creature WHERE map=0 AND id IN(115414,115027,115037,114641);
DELETE FROM gameobject WHERE id=266290 AND map=1220;
UPDATE quest_template_addon SET PrevQuestID=ID WHERE ID IN(38923,39090);
$migration
$migration
SELECT 'instance',script FROM instance_template WHERE map=1191;
SELECT 'weeklies',COUNT(*) FROM quest_template q JOIN quest_template_addon a USING(ID) WHERE ID IN(38923,38925,39090,39096) AND (q.Flags & 32768)<>0 AND (a.SpecialFlags & 1)<>0;
SELECT 'self-links',COUNT(*) FROM quest_template_addon WHERE ID IN(38923,39090) AND PrevQuestID=ID;
SELECT 'expiry',COUNT(*) FROM quest_template WHERE ID IN(38923,38925) AND (FlagsEx & 2097152)<>0 AND (FlagsEx & 16777216)<>0;
SELECT 'nodes',GROUP_CONCAT(CONCAT(id,':',n) ORDER BY id) FROM (SELECT id,COUNT(*) n FROM creature WHERE map=0 AND id IN(115414,115027,115037,114641) GROUP BY id) t;
SELECT 'portal',COUNT(*) FROM gameobject WHERE id=266290 AND map=1220;
SELECT 'bindings',COUNT(*) FROM spell_script_names WHERE ScriptName IN('spell_karazhan_collect_sample','spell_karazhan_disable_node');
SELECT 'native-clicks',GROUP_CONCAT(CONCAT(npc_entry,':',spell_id) ORDER BY npc_entry) FROM npc_spellclick_spells WHERE npc_entry IN(115027,115037,115414);
SELECT 'historical-link',PrevQuestID FROM quest_template_addon WHERE ID=44556;
SELECT 'unrelated-links',COUNT(*) FROM quest_template_addon a JOIN saved_quest_template_addon b USING(ID) WHERE a.ID NOT IN(38923,38925,39090,39096) AND (a.PrevQuestID<>b.PrevQuestID OR a.NextQuestID<>b.NextQuestID OR a.SpecialFlags<>b.SpecialFlags);
SELECT 'unrelated-quests',COUNT(*) FROM quest_template a JOIN saved_quest_template b USING(ID) WHERE a.ID NOT IN(38923,38925,39090,39096) AND (a.Flags<>b.Flags OR a.FlagsEx<>b.FlagsEx OR a.RewardNextQuest<>b.RewardNextQuest);
SELECT 'original-spawns',COUNT(*) FROM saved_creature a LEFT JOIN creature b USING(guid) WHERE NOT(a.map=0 AND a.id IN(115414,115027,115037,114641)) AND (b.guid IS NULL OR a.id<>b.id OR a.map<>b.map OR a.position_x<>b.position_x OR a.position_y<>b.position_y OR a.position_z<>b.position_z);
UPDATE instance_template SET script='custom_ashran' WHERE map=1191;
UPDATE quest_template_addon SET PrevQuestID=999 WHERE ID=38923;
$migration
SELECT 'custom-instance',script FROM instance_template WHERE map=1191;
SELECT 'custom-link',PrevQuestID FROM quest_template_addon WHERE ID=38923;
"@
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$mysql=Get-ChildItem C:/wamp64/bin/mysql -Recurse -Filter mysql.exe | Select-Object -First 1 -ExpandProperty FullName
$previous=$env:MYSQL_PWD
try {
 $env:MYSQL_PWD=$connection[3]
 $rows=& $mysql --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --skip-column-names --execute=$fixture
 if($LASTEXITCODE){throw 'Ashran/Karazhan SQL fixture failed'}
 $expected=@("instance`tinstance_ashran","weeklies`t4","self-links`t0","expiry`t2","nodes`t114641:1,115027:1,115037:1,115414:4","portal`t1","bindings`t4","native-clicks`t115027:228208,115037:231458,115414:229466","historical-link`t44944","unrelated-links`t0","unrelated-quests`t0","original-spawns`t0","custom-instance`tcustom_ashran","custom-link`t999")
 if(($rows -join '|') -ne ($expected -join '|')){throw "SQL mismatch: $($rows -join '|')"}
} finally {$env:MYSQL_PWD=$previous}
'PASS: both production migrations are idempotent, preserve unrelated/custom records and restore the native node bindings and weekly flags.'
