$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$migration=@('12_narthalas_entry','13_narthalas_wanding','14_narthalas_lessons') | ForEach-Object {Get-Content "$repo/sql/updates/world/2026_10_09_$_.sql" -Raw}
$migration=$migration -join "`n"
$tables=@('gameobject_template','gameobject','quest_objectives','creature_template','smart_scripts','spell_script_names','spell_loot_template','creature_loot_template','quest_template_addon','spell_scene','areatrigger_scripts')
$fixture=($tables | ForEach-Object {"CREATE TEMPORARY TABLE saved_$_ LIKE $_; INSERT INTO saved_$_ SELECT * FROM $_; CREATE TEMPORARY TABLE $_ LIKE saved_$_; INSERT INTO $_ SELECT * FROM saved_$_;"}) -join "`n"
$native=Get-Content "$repo/docs/audits/narthalas-native-2026-10-09.json" -Raw | ConvertFrom-Json
$nativeRows=($native.objectives | ForEach-Object {"SELECT $($_.ID) ID,$($_.QuestID) QuestID,$($_.Type) Type,$($_.ObjectID) ObjectID,$($_.Amount) Amount,$($_.Flags) Flags,$($_.Flags2) Flags2"}) -join ' UNION ALL '
$fixture+=@"
$migration
$migration
SELECT 'native-objectives',COUNT(*) FROM ($nativeRows) a LEFT JOIN quest_objectives b USING(ID) WHERE b.ID IS NULL OR a.QuestID<>b.QuestID OR a.Type<>b.Type OR a.ObjectID<>b.ObjectID OR a.Amount<>b.Amount OR a.Flags<>b.Flags OR a.Flags2<>b.Flags2;
SELECT 'saved-progress',COUNT(*) FROM saved_quest_objectives a JOIN quest_objectives b USING(ID) WHERE a.StorageIndex<>b.StorageIndex OR a.ObjectID<>b.ObjectID;
SELECT 'loot-links',COUNT(*) FROM gameobject_template WHERE entry IN(239744,239745,250372,250373,250374,239341,245483,245484,245485,245486) AND Data1=entry;
SELECT 'bookshelf',Data1 FROM gameobject_template WHERE entry=238940;
SELECT 'new-books',COUNT(*) FROM gameobject WHERE id IN(250372,250373,250374) AND map=1220;
SELECT 'unrelated-spawns',COUNT(*) FROM saved_gameobject a LEFT JOIN gameobject b USING(guid) WHERE b.guid IS NULL OR a.id<>b.id OR a.PhaseId<>b.PhaseId OR a.position_x<>b.position_x OR a.position_y<>b.position_y OR a.position_z<>b.position_z;
SELECT 'old-escort',COUNT(*) FROM smart_scripts WHERE (entryorguid=88889 AND source_type=0) OR (entryorguid=8888900 AND source_type=9);
SELECT 'shared-hide',COUNT(*) FROM smart_scripts WHERE entryorguid=88867 AND source_type=0 AND id IN(2,3) AND action_type=47;
SELECT 'start-link',link FROM smart_scripts WHERE entryorguid=88867 AND source_type=0 AND id=1 AND action_param1=177816;
SELECT 'unrelated-farondis',COUNT(*) FROM smart_scripts WHERE entryorguid=88867 AND source_type=0 AND id=0 AND action_type=206;
SELECT 'escort-path',COUNT(*) FROM waypoints WHERE entry=88889;
SELECT 'spell-bindings',COUNT(*) FROM spell_script_names WHERE ScriptName LIKE 'spell_narthalas_%';
SELECT 'actors',COUNT(*) FROM creature_template WHERE ScriptName IN('npc_narthalas_farondis_walk','npc_narthalas_instructor','npc_narthalas_drawing');
SELECT 'runes',COUNT(*) FROM spell_scene WHERE MiscValue IN(935,936,937) AND ScriptName='scene_narthalas_rune';
SELECT 'wand',ScriptName FROM areatrigger_scripts WHERE entry=11511;
SELECT 'podium',ScriptName FROM gameobject_template WHERE entry=250362;
SELECT 'events',SUM(SpecialFlags & 2) FROM quest_template_addon WHERE ID IN(37729,42370,42371);
SELECT 'prerequisites',COUNT(*) FROM quest_template_addon WHERE (ID=37729 AND PrevQuestID=42371) OR (ID=37730 AND PrevQuestID=37729);
SELECT 'robes',QuestRequired FROM spell_loot_template WHERE Entry=179185 AND Item=120948;
SELECT 'key',LootMode,QuestRequired FROM creature_loot_template WHERE Entry=88859 AND Item=120169;
SELECT 'key-source',COUNT(*) FROM creature_template WHERE entry=88859 AND LootID=88859 AND AIName='SmartAI';
UPDATE creature_template SET ScriptName='custom_farondis' WHERE entry=88889;
$migration
SELECT 'custom',ScriptName FROM creature_template WHERE entry=88889;
"@
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try {
 $env:MYSQL_PWD=$connection[3]
 $rows=& 'C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe' --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --skip-column-names --execute=$fixture
 if($LASTEXITCODE){throw 'Nar thalas SQL fixture failed'}
} finally {$env:MYSQL_PWD=$previous}
$expected=@("native-objectives`t0","saved-progress`t0","loot-links`t10","bookshelf`t59536","new-books`t3","unrelated-spawns`t0","old-escort`t0","shared-hide`t0","start-link`t0","unrelated-farondis`t1","escort-path`t10","spell-bindings`t4","actors`t5","runes`t3","wand`tat_narthalas_wand","podium`tgo_narthalas_podium","events`t0","prerequisites`t2","robes`t1","key`t1`t1","key-source`t1","custom`tcustom_farondis")
if(($rows -join '|') -ne ($expected -join '|')){throw "SQL mismatch: $($rows -join '|')"}
'PASS: three repeatable migrations, 21 native objectives, saved progress/unrelated spawns/custom bindings, ten loot links, three books, native scenes, proper prerequisites and loot mode.'
