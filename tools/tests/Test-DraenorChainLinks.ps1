$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path "$PSScriptRoot/../..").Path
$migration = Get-Content "$repo/sql/updates/world/2026_10_09_01_fix_gearing_up_chain.sql" -Raw
$fixture = @"
CREATE TEMPORARY TABLE quest_link_original LIKE quest_template;
INSERT INTO quest_link_original SELECT * FROM quest_template;
CREATE TEMPORARY TABLE quest_addon_original LIKE quest_template_addon;
INSERT INTO quest_addon_original SELECT * FROM quest_template_addon;
CREATE TEMPORARY TABLE quest_template LIKE quest_link_original;
INSERT INTO quest_template SELECT * FROM quest_link_original;
CREATE TEMPORARY TABLE quest_template_addon LIKE quest_addon_original;
INSERT INTO quest_template_addon SELECT * FROM quest_addon_original;
UPDATE quest_template SET RewardNextQuest=34315 WHERE ID=34315;
UPDATE quest_template_addon SET PrevQuestID=34315,NextQuestID=34315 WHERE ID=34315;
$migration
SELECT ROW_COUNT();
$migration
SELECT ROW_COUNT();
SELECT q.ID,q.RewardNextQuest,a.PrevQuestID,a.NextQuestID FROM quest_template q JOIN quest_template_addon a USING(ID) WHERE q.ID=34315;
SELECT COUNT(*) FROM quest_template q JOIN quest_link_original o USING(ID) WHERE q.ID<>34315 AND q.RewardNextQuest<>o.RewardNextQuest;
SELECT COUNT(*) FROM quest_template_addon a JOIN quest_addon_original o USING(ID) WHERE a.ID<>34315 AND (a.PrevQuestID<>o.PrevQuestID OR a.NextQuestID<>o.NextQuestID);
UPDATE quest_template SET RewardNextQuest=34315 WHERE ID=34315;
UPDATE quest_template_addon SET PrevQuestID=999,NextQuestID=34315 WHERE ID=34315;
$migration
SELECT ROW_COUNT();
SELECT q.RewardNextQuest,a.PrevQuestID,a.NextQuestID FROM quest_template q JOIN quest_template_addon a USING(ID) WHERE q.ID=34315;
"@
$connection = (Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object { $_ -match '^WorldDatabaseInfo\s*=' }).Split('"')[1].Split(';')
$mysql = Get-ChildItem C:/wamp64/bin/mysql -Recurse -Filter mysql.exe | Select-Object -First 1 -ExpandProperty FullName
$previous = $env:MYSQL_PWD
try {
    $env:MYSQL_PWD = $connection[3]
    $rows = & $mysql --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --skip-column-names --execute=$fixture
    if ($LASTEXITCODE) { throw 'Draenor chain SQL fixture failed' }
    if (($rows -join '|') -ne "2|0|34315`t34316`t34314`t34316|0|0|0|34315`t999`t34315") {
        throw "Draenor chain idempotence/isolation regression: $($rows -join '|')"
    }
} finally { $env:MYSQL_PWD = $previous }
'PASS: production migration repairs all three self-links, is idempotent, preserves other quest links and does not partially overwrite a customized prerequisite.'
