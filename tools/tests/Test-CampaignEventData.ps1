$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$migration=(3..5 | ForEach-Object {Get-Content (Get-ChildItem "$repo/sql/updates/world/2026_10_09_0$($_)_*.sql").FullName -Raw}) -join "`n"
$tables=@('quest_template_addon','gameobject_template','creature_template','spell_scene','spell_script_names','npc_spellclick_spells','conditions','disables','instance_template','lfg_entrances')
$fixture=($tables | ForEach-Object {"CREATE TEMPORARY TABLE saved_$_ LIKE $_; INSERT INTO saved_$_ SELECT * FROM $_; CREATE TEMPORARY TABLE $_ LIKE saved_$_; INSERT INTO $_ SELECT * FROM saved_$_;"}) -join "`n"
$fixture+=@"
DELETE FROM spell_scene WHERE MiscValue IN(580,602,612);
UPDATE quest_template_addon SET PrevQuestID=0,ExclusiveGroup=0 WHERE ID IN(34318,34469,34031,34048,36168,36169);
UPDATE quest_template_addon SET ExclusiveGroup=0 WHERE ID IN(36164,36167);
UPDATE creature_template SET ScriptName='' WHERE entry IN(78003,77997,77958,77959,77965,77966,78386,78393,77225,77244,84368,84538,79434,84803,84364,84719,84814,84973,84974,84975);
UPDATE gameobject_template SET ScriptName='' WHERE entry IN(229026,227183,227172,227231,227270);
$migration
SELECT 'groups',GROUP_CONCAT(CONCAT(ID,':',PrevQuestID,':',ExclusiveGroup) ORDER BY ID) FROM quest_template_addon WHERE ID IN(34318,34469,34031,34048,36164,36167,36168,36169);
SELECT 'scenes',GROUP_CONCAT(CONCAT(MiscValue,':',SceneScriptPackageID,':',CustomDuration) ORDER BY MiscValue) FROM spell_scene WHERE MiscValue IN(580,602,612);
SELECT 'instance',script FROM instance_template WHERE map=1374;
SELECT 'entrance',name FROM lfg_entrances WHERE dungeonId=870;
SELECT 'spell-bindings',COUNT(*) FROM spell_script_names WHERE (spell_id=158278 AND ScriptName='spell_bloodmaul_purify_soul') OR (spell_id IN(158645,158317) AND ScriptName='aura_seismic_mole_ride');
SELECT 'corpse-conditions',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=13 AND SourceEntry=158278 AND Comment='Bloodmaul purification target';
SELECT 'totem-clicks',COUNT(*) FROM npc_spellclick_spells WHERE npc_entry IN(78386,78393) AND spell_id=158944;
SELECT 'objectives',GROUP_CONCAT(CONCAT(QuestID,':',ObjectID,':',Amount) ORDER BY QuestID,ObjectID) FROM quest_objectives WHERE QuestID IN(34031,34048,34318,34469,36169);
SELECT 'unchanged-links',COUNT(*) FROM quest_template_addon a JOIN saved_quest_template_addon b USING(ID) WHERE a.ID NOT IN(34318,34469,34031,34048,36164,36167,36168,36169) AND (a.PrevQuestID<>b.PrevQuestID OR a.ExclusiveGroup<>b.ExclusiveGroup);
SELECT 'native-dungeon',COUNT(*) FROM instance_template a JOIN saved_instance_template b USING(map) WHERE a.map=1182 AND a.script<>b.script;
$migration
SELECT 'repeat-scene-count',COUNT(*) FROM spell_scene WHERE MiscValue IN(580,602,612);
SELECT 'repeat-condition-count',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=13 AND SourceEntry=158278 AND Comment='Bloodmaul purification target';
UPDATE creature_template SET ScriptName='custom_encounter' WHERE entry=78003;
UPDATE gameobject_template SET ScriptName='custom_gate' WHERE entry=229026;
UPDATE quest_template_addon SET PrevQuestID=999,ExclusiveGroup=-999 WHERE ID=34318;
$migration
SELECT 'custom-creature',ScriptName FROM creature_template WHERE entry=78003;
SELECT 'custom-object',ScriptName FROM gameobject_template WHERE entry=229026;
SELECT 'custom-link',CONCAT(PrevQuestID,':',ExclusiveGroup) FROM quest_template_addon WHERE ID=34318;
"@
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$mysql=Get-ChildItem C:/wamp64/bin/mysql -Recurse -Filter mysql.exe | Select-Object -First 1 -ExpandProperty FullName
$previous=$env:MYSQL_PWD
try {
    $env:MYSQL_PWD=$connection[3]
    $rows=& $mysql --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --skip-column-names --execute=$fixture
    if($LASTEXITCODE){throw 'Campaign SQL fixture failed'}
    $expected=@(
        "groups`t34031:34030:-34032,34048:34030:-34032,34318:34381:-34319,34469:34381:-34319,36164:36163:-36169,36167:36163:-36169,36168:36163:-36169,36169:36164:0",
        "scenes`t580:773:60000,602:801:15000,612:788:45000",
        "instance`tinstance_exarch_trial_of_faith", "entrance`tThe Trial of Faith", "spell-bindings`t3", "corpse-conditions`t4", "totem-clicks`t2",
        "objectives`t34031:227183:4,34048:227172:5,34318:110378:5,34469:78431:10,36169:84974:1",
        "unchanged-links`t0", "native-dungeon`t0", "repeat-scene-count`t3", "repeat-condition-count`t4",
        "custom-creature`tcustom_encounter", "custom-object`tcustom_gate", "custom-link`t999:-999"
    )
    if(($rows -join '|') -ne ($expected -join '|')){throw "Campaign SQL mismatch: $($rows -join '|')"}
} finally {$env:MYSQL_PWD=$previous}
'PASS: production migrations run twice in isolated temporary tables; native objectives, parallel prerequisites, scene packages, script bindings and custom data are preserved.'
