$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$migration=Get-Content "$repo/sql/updates/world/2026_10_09_10_azurewing_repose.sql" -Raw
$tables=@('quest_objectives','creature_template','creature_template_wdb','smart_scripts','gameobject_template','creature','spell_area','conditions','spell_script_names','npc_spellclick_spells')
$fixture=($tables | ForEach-Object {"CREATE TEMPORARY TABLE saved_$_ LIKE $_; INSERT INTO saved_$_ SELECT * FROM $_; CREATE TEMPORARY TABLE $_ LIKE saved_$_; INSERT INTO $_ SELECT * FROM saved_$_;"}) -join "`n"
$native=Get-Content "$repo/docs/audits/azurewing-native-2026-10-09.json" -Raw | ConvertFrom-Json
$nativeRows=($native.objectives | ForEach-Object {"SELECT $($_.ID) ID,$($_.QuestID) QuestID,$($_.Type) Type,$($_.ObjectID) ObjectID,$($_.Amount) Amount,$($_.Flags) Flags,$($_.Flags2) Flags2"}) -join ' UNION ALL '
$fixture+=@"
$migration
$migration
SELECT 'native-objectives',COUNT(*) FROM ($nativeRows) a LEFT JOIN quest_objectives b USING(ID) WHERE b.ID IS NULL OR a.QuestID<>b.QuestID OR a.Type<>b.Type OR a.ObjectID<>b.ObjectID OR a.Amount<>b.Amount OR a.Flags<>b.Flags OR a.Flags2<>b.Flags2;
SELECT 'pre-runas-phase',NegativeCondition FROM conditions WHERE SourceTypeOrReferenceId=23 AND SourceGroup=7334 AND SourceEntry=15 AND ConditionTypeOrReference=14 AND ConditionValue1=37957;
SELECT 'loot-links',COUNT(*) FROM gameobject_template WHERE entry IN(240033,240267) AND Data1=entry;
SELECT 'items',COUNT(*) FROM quest_objectives WHERE ID IN(277011,277144,277268,284014,284016) AND Flags2=1;
SELECT 'optional',Flags FROM quest_objectives WHERE ID=284778;
SELECT 'bindings',COUNT(*) FROM spell_script_names WHERE ScriptName LIKE 'spell_azurewing_%';
SELECT 'pylons',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=13 AND SourceEntry=179825 AND ConditionTypeOrReference=31 AND ConditionValue2 IN(90263,100383,100384,100385);
SELECT 'prerequisites',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=19 AND SourceEntry=37857 AND ElseGroup=0 AND ConditionTypeOrReference=8 AND ConditionValue1 IN(37856,37859) AND NegativeCondition=0;
SELECT 'old-follower',COUNT(*) FROM spell_area WHERE spell=180066 AND area=7338 AND quest_start=37957;
SELECT 'actors',COUNT(*) FROM creature_template WHERE ScriptName IN('npc_azurewing_runas_duel','npc_azurewing_runas_follower','npc_azurewing_runas_guide','npc_azurewing_runas_start','npc_azurewing_stellagosa_return','npc_azurewing_final_enemy') AND AIName='';
SELECT 'aliases',SUM(KillCredit1) FROM creature_template_wdb WHERE Entry IN(91155,108721);
SELECT 'hostile-boarding',COUNT(*) FROM npc_spellclick_spells WHERE npc_entry=91155;
SELECT 'shared-finale',COUNT(*) FROM creature WHERE guid=338484;
SELECT 'unrelated-spawns',COUNT(*) FROM saved_creature a LEFT JOIN creature b USING(guid) WHERE a.guid<>338484 AND (b.guid IS NULL OR a.id<>b.id OR a.map<>b.map OR a.PhaseId<>b.PhaseId OR a.position_x<>b.position_x OR a.position_y<>b.position_y OR a.position_z<>b.position_z);
SELECT 'saved-progress',COUNT(*) FROM saved_quest_objectives a JOIN quest_objectives b USING(ID) WHERE a.StorageIndex<>b.StorageIndex OR a.ObjectID<>b.ObjectID;
SELECT 'native-guide',COUNT(*) FROM waypoints WHERE entry=90406;
SELECT 'native-flight',COUNT(*) FROM waypoints WHERE entry=107995;
SELECT 'loot',COUNT(*) FROM gameobject_template g JOIN gameobject_loot_template l ON l.Entry=g.Data1 WHERE (g.entry=240033 AND l.Item=122188) OR (g.entry=240267 AND l.Item=122306);
UPDATE creature_template SET ScriptName='custom_runas' WHERE entry=90372;
$migration
SELECT 'custom',ScriptName FROM creature_template WHERE entry=90372;
"@
$mysql=Get-ChildItem C:/wamp64/bin/mysql -Recurse -Filter mysql.exe | Select-Object -First 1 -ExpandProperty FullName
function Invoke-Fixture($kind,$sql) {
 $connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match "^${kind}DatabaseInfo\s*="}).Split('"')[1].Split(';')
 $previous=$env:MYSQL_PWD
 try {
  $env:MYSQL_PWD=$connection[3]
  $rows=& $mysql --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --skip-column-names --execute=$sql
  if($LASTEXITCODE){throw 'Azurewing SQL fixture failed'}
  return $rows
 } finally {$env:MYSQL_PWD=$previous}
}
$rows=Invoke-Fixture World $fixture
$expected=@("native-objectives`t0","pre-runas-phase`t0","loot-links`t2","items`t5","optional`t28","bindings`t5","pylons`t4","prerequisites`t2","old-follower`t0","actors`t7","aliases`t0","hostile-boarding`t0","shared-finale`t0","unrelated-spawns`t0","saved-progress`t0","native-guide`t10","native-flight`t10","loot`t2","custom`tcustom_runas")
if(($rows -join '|') -ne ($expected -join '|')){throw "World SQL mismatch: $($rows -join '|')"}
$schema=(Get-Content "$repo/sql/updates/characters/2026_10_09_10_azurewing_interactions.sql" -Raw).Replace('CREATE TABLE IF NOT EXISTS','CREATE TEMPORARY TABLE IF NOT EXISTS')
$rows=Invoke-Fixture Character "$schema $schema INSERT INTO character_azurewing_interactions VALUES (1,42271,338304,1),(2,42271,338304,1),(1,37859,266388,1); INSERT IGNORE INTO character_azurewing_interactions VALUES (1,42271,338304,2); SELECT COUNT(*) FROM character_azurewing_interactions; DELETE FROM character_azurewing_interactions WHERE guid=1 AND quest=42271 AND ordinal>0; SELECT COUNT(*) FROM character_azurewing_interactions;"
if(($rows -join '|') -ne '3|2'){throw 'Character ledger isolation failed'}
'PASS: Azurewing migrations are repeatable, preserve saved progress/custom actors, link actual loot, retain optional flight, separate enemies and isolate player interactions.'
