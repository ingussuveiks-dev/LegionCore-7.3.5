$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path "$PSScriptRoot/../..").Path
$migration = Get-Content "$repo/sql/updates/world/2026_10_12_13_dalaran_spellstalker_ground.sql" -Raw
$fixture = @"
CREATE TEMPORARY TABLE saved_movement LIKE creature_template_movement;
INSERT INTO saved_movement SELECT * FROM creature_template_movement;
CREATE TEMPORARY TABLE creature_template_movement LIKE saved_movement;
INSERT INTO creature_template_movement SELECT * FROM saved_movement;
UPDATE creature_template_movement SET Ground=0,Flight=1 WHERE CreatureId=114407;
$migration
SELECT 'ground',COUNT(*) FROM creature_template_movement WHERE CreatureId=114407 AND Ground=1 AND Flight=0;
$migration
SELECT 'repeat',ROW_COUNT();
SELECT 'other-fields',COUNT(*) FROM creature_template_movement a JOIN saved_movement b USING(CreatureId) WHERE a.CreatureId=114407 AND a.Swim=b.Swim AND a.Rooted=b.Rooted AND a.Random=b.Random AND (a.InteractionPauseTimer <=> b.InteractionPauseTimer);
SELECT 'others-changed',COUNT(*) FROM creature_template_movement a JOIN saved_movement b USING(CreatureId) WHERE a.CreatureId<>114407 AND (a.Ground<>b.Ground OR a.Flight<>b.Flight);
UPDATE creature_template_movement SET Ground=1,Flight=2 WHERE CreatureId=114407;
$migration
SELECT 'custom-preserved',COUNT(*) FROM creature_template_movement WHERE CreatureId=114407 AND Ground=1 AND Flight=2;
"@
$connection = (Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object { $_ -match '^WorldDatabaseInfo\s*=' }).Split('"')[1].Split(';')
$mysql = Get-ChildItem C:/wamp64/bin/mysql -Recurse -Filter mysql.exe | Select-Object -First 1 -ExpandProperty FullName
$previous = $env:MYSQL_PWD
try {
    $env:MYSQL_PWD = $connection[3]
    $rows = & $mysql --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --skip-column-names --execute=$fixture
    if ($LASTEXITCODE) { throw 'Spellstalker movement fixture failed' }
    $expected = @("ground`t1", "repeat`t0", "other-fields`t1", "others-changed`t0", "custom-preserved`t1")
    if (($rows -join '|') -ne ($expected -join '|')) { throw "Wrong migration result: $($rows -join '|')" }
} finally { $env:MYSQL_PWD = $previous }
'PASS: ground movement restored; repeat application, other fields, unrelated NPC movement and custom flight preserved.'
