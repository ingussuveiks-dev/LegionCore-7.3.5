$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path "$PSScriptRoot/../..").Path
$migration = Get-Content "$repo/sql/updates/world/2026_10_09_00_fix_academic_exploration_chain.sql" -Raw
$fixture = @"
CREATE TEMPORARY TABLE quest_chain_snapshot LIKE quest_template_addon;
INSERT INTO quest_chain_snapshot SELECT * FROM quest_template_addon;
CREATE TEMPORARY TABLE quest_template_addon LIKE quest_chain_snapshot;
INSERT INTO quest_template_addon SELECT * FROM quest_chain_snapshot;
UPDATE quest_template_addon SET PrevQuestID=0,NextQuestID=41183 WHERE ID=41183;
$migration
SELECT ROW_COUNT();
$migration
SELECT ROW_COUNT();
SELECT ID,PrevQuestID,NextQuestID,RequiredSkillID,RequiredSkillPoints FROM quest_template_addon WHERE ID IN(41183,41184,41185) ORDER BY ID;
SELECT COUNT(*) FROM quest_template_addon a JOIN quest_chain_snapshot b USING(ID) WHERE a.ID<>41183 AND NOT(a.NextQuestID <=> b.NextQuestID);
UPDATE quest_template_addon SET PrevQuestID=999,NextQuestID=41183 WHERE ID=41183;
$migration
SELECT ROW_COUNT();
"@
$connection = (Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object { $_ -match '^WorldDatabaseInfo\s*=' }).Split('"')[1].Split(';')
$mysql = Get-ChildItem C:/wamp64/bin/mysql -Recurse -Filter mysql.exe | Select-Object -First 1 -ExpandProperty FullName
$previous = $env:MYSQL_PWD
try {
    $env:MYSQL_PWD = $connection[3]
    $rows = & $mysql --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --skip-column-names --execute=$fixture
    if ($LASTEXITCODE) { throw 'Quest migration SQL fixture failed' }
    $expected = "1|0|41183`t0`t41184`t794`t700|41184`t41183`t41185`t794`t700|41185`t41184`t0`t794`t700|0|0"
    if (($rows -join '|') -ne $expected) { throw "Quest chain idempotence/isolation regression: $($rows -join '|')" }
} finally { $env:MYSQL_PWD = $previous }
'PASS: migration repairs the self-link once, preserves downstream/skill requirements and other quests, and respects a changed prerequisite.'
