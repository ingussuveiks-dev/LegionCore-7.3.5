$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path "$PSScriptRoot/../..").Path
$migration = Get-Content "$repo/sql/updates/world/2026_10_09_02_bloodmaul_campaign_progress.sql" -Raw
$fixture = @"
CREATE TEMPORARY TABLE campaign_addon_original LIKE quest_template_addon;
INSERT INTO campaign_addon_original SELECT * FROM quest_template_addon;
CREATE TEMPORARY TABLE campaign_go_original LIKE gameobject_template;
INSERT INTO campaign_go_original SELECT * FROM gameobject_template;
CREATE TEMPORARY TABLE quest_template_addon LIKE campaign_addon_original;
INSERT INTO quest_template_addon SELECT * FROM campaign_addon_original;
CREATE TEMPORARY TABLE gameobject_template LIKE campaign_go_original;
INSERT INTO gameobject_template SELECT * FROM campaign_go_original;
UPDATE quest_template_addon SET PrevQuestID=0 WHERE ID IN (34314,34316,34381);
UPDATE gameobject_template SET ScriptName='' WHERE entry=229414;
$migration
SELECT GROUP_CONCAT(CONCAT(ID,':',PrevQuestID) ORDER BY ID) FROM quest_template_addon WHERE ID IN (34314,34316,34381);
SELECT ScriptName FROM gameobject_template WHERE entry=229414;
$migration
SELECT ROW_COUNT();
SELECT COUNT(*) FROM quest_template_addon a JOIN campaign_addon_original o USING(ID) WHERE a.ID NOT IN (34314,34316,34381) AND a.PrevQuestID<>o.PrevQuestID;
SELECT COUNT(*) FROM gameobject_template a JOIN campaign_go_original o USING(entry) WHERE a.entry<>229414 AND a.ScriptName<>o.ScriptName;
UPDATE quest_template_addon SET PrevQuestID=999 WHERE ID=34314;
UPDATE gameobject_template SET ScriptName='custom_shackle' WHERE entry=229414;
$migration
SELECT PrevQuestID FROM quest_template_addon WHERE ID=34314;
SELECT ScriptName FROM gameobject_template WHERE entry=229414;
SELECT COUNT(*) FROM quest_objectives WHERE QuestID=34314 AND Type=2 AND ObjectID=229414 AND Amount=1 AND Flags=2;
SELECT COUNT(*) FROM quest_objectives WHERE QuestID=34315 AND Type=1 AND ObjectID IN (110606,110607,110608) AND Amount=1;
SELECT COUNT(*) FROM creature_loot_template WHERE Entry=78778 AND Item=110664 AND QuestRequired=1 AND Chance>0;
SELECT COUNT(*) FROM creature_queststarter WHERE (id=78659 AND quest IN (34314,34315)) OR (id=78746 AND quest=34316);
SELECT COUNT(*) FROM creature_questender WHERE (id=78659 AND quest IN (34309,34314,34315)) OR (id=78785 AND quest=34316);
"@
$connection = (Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object { $_ -match '^WorldDatabaseInfo\s*=' }).Split('"')[1].Split(';')
$mysql = Get-ChildItem C:/wamp64/bin/mysql -Recurse -Filter mysql.exe | Select-Object -First 1 -ExpandProperty FullName
$previous = $env:MYSQL_PWD
try {
    $env:MYSQL_PWD = $connection[3]
    $rows = & $mysql --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --skip-column-names --execute=$fixture
    if ($LASTEXITCODE) { throw 'Bloodmaul data fixture failed' }
    $expected = '34314:34309,34316:34315,34381:34316|go_bwuja_shackle|0|0|0|999|custom_shackle|1|3|1|3|4'
    if (($rows -join '|') -ne $expected) { throw "Bloodmaul data regression: $($rows -join '|')" }
} finally { $env:MYSQL_PWD = $previous }
'PASS: production migration is idempotent, preserves custom bindings and unrelated prerequisites; mandatory shackle, key loot, gear goals and all four quest handoffs exist.'
