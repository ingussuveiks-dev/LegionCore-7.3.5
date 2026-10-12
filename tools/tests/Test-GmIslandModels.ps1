$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path "$PSScriptRoot/../..").Path
$migration = Get-Content "$repo/sql/updates/world/2026_10_12_12_gm_island_missing_models.sql" -Raw
# Connection-local tables shadow the live tables; never mutate live NPCs here.
$fixture = @"
CREATE TEMPORARY TABLE fixture_template_shape LIKE creature_template;
INSERT INTO fixture_template_shape SELECT * FROM creature_template WHERE entry IN(230001,230002,230005);
CREATE TEMPORARY TABLE fixture_wdb_shape LIKE creature_template_wdb;
CREATE TEMPORARY TABLE creature_template LIKE fixture_template_shape;
CREATE TEMPORARY TABLE creature_template_wdb LIKE fixture_wdb_shape;
INSERT INTO creature_template SELECT * FROM fixture_template_shape;
INSERT INTO creature_template_wdb (Entry,Name1,Displayid1) VALUES (230005,'Preserved neighbour',308);
$migration
SELECT 'created',COUNT(*) FROM creature_template_wdb WHERE Entry IN(230001,230002) AND Displayid1=308 AND Type=7 AND HpMulti=1 AND PowerMulti=1 AND VerifiedBuild=0 AND Name1=CONCAT('GM Island Custom NPC ',Entry);
$migration
SELECT 'repeat',COUNT(*) FROM creature_template_wdb;
UPDATE creature_template_wdb SET Name1='Existing custom name',Displayid1=0,Displayid2=5233,HpMulti=72,VerifiedBuild=123 WHERE Entry=230001;
UPDATE creature_template_wdb SET Name1='Existing model-less NPC',Displayid1=0,Type=10,HpMulti=9 WHERE Entry=230002;
$migration
SELECT 'preserved',COUNT(*) FROM creature_template_wdb WHERE Entry=230001 AND Name1='Existing custom name' AND Displayid1=0 AND Displayid2=5233 AND HpMulti=72 AND VerifiedBuild=123;
SELECT 'repaired',COUNT(*) FROM creature_template_wdb WHERE Entry=230002 AND Name1='Existing model-less NPC' AND Displayid1=308 AND Type=10 AND HpMulti=9;
SELECT 'neighbour',COUNT(*) FROM creature_template_wdb WHERE Entry=230005 AND Name1='Preserved neighbour' AND Displayid1=308;
DELETE FROM creature_template_wdb WHERE Entry=230002;
DELETE FROM creature_template WHERE entry=230002;
$migration
SELECT 'missing-template',COUNT(*) FROM creature_template_wdb WHERE Entry=230002;
"@
$connection = (Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object { $_ -match '^WorldDatabaseInfo\s*=' }).Split('"')[1].Split(';')
$mysql = Get-ChildItem C:/wamp64/bin/mysql -Recurse -Filter mysql.exe | Select-Object -First 1 -ExpandProperty FullName
$previous = $env:MYSQL_PWD
try {
    $env:MYSQL_PWD = $connection[3]
    $rows = & $mysql --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --skip-column-names --execute=$fixture
    if ($LASTEXITCODE) { throw 'GM Island model fixture failed' }
    $expected = @("created`t2", "repeat`t3", "preserved`t1", "repaired`t1", "neighbour`t1", "missing-template`t0")
    if (($rows -join '|') -ne ($expected -join '|')) { throw "Wrong migration result: $($rows -join '|')" }
} finally { $env:MYSQL_PWD = $previous }
'PASS: missing definitions/models repaired; repeat application, existing alternate models/metadata, neighbours and absent templates preserved.'
