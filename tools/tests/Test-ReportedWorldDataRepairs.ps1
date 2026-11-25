param([switch]$Live, [string]$EvidenceDirectory = "$PSScriptRoot/../../build-extractors/runtime-test/reported-world-data")

$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path "$PSScriptRoot/../..").Path
$migrations = @(
    "$repo/sql/updates/world/2026_11_25_10_smart_terminal_links.sql",
    "$repo/sql/updates/world/2026_11_25_11_great_sea_legion_graveyards.sql"
)

& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $EvidenceDirectory -Test {
    $checks = @'
SELECT 'FIRE', GROUP_CONCAT(CONCAT(id, ':', link, ':', event_type) ORDER BY id)
FROM smart_scripts WHERE entryorguid=104841 AND source_type=0;
SELECT 'VELEN', GROUP_CONCAT(CONCAT(id, ':', link, ':', action_param1) ORDER BY id)
FROM smart_scripts WHERE entryorguid=123232 AND source_type=0;
SELECT 'SCENE', GROUP_CONCAT(CONCAT(id, ':', link, ':', action_param1) ORDER BY id)
FROM smart_scripts WHERE entryorguid=101 AND source_type=13;
SELECT 'TIMED', COUNT(*) FROM smart_scripts WHERE entryorguid=12323200 AND source_type=9;
SELECT 'COAST', COUNT(*), SUM(id=5890), SUM(faction<>0)
FROM game_graveyard_zone WHERE ghost_zone=7656;
SELECT 'CHEST', g.Data1, COUNT(l.Entry)
FROM gameobject_template g JOIN gameobject_loot_template l ON l.Entry=g.Data1 WHERE g.entry=271554 GROUP BY g.Data1;
SELECT 'TOKEN', Entry, COUNT(*) FROM item_loot_template WHERE Entry IN(147213,147220) GROUP BY Entry ORDER BY Entry;
'@
    $sql = 'START TRANSACTION READ ONLY;' + "`n" + $checks
    if (!$Live) {
        $shadow = @'
CREATE TEMPORARY TABLE smart_seed LIKE smart_scripts;
INSERT INTO smart_seed SELECT * FROM smart_scripts WHERE entryorguid IN(101,104841,123232,12323200);
CREATE TEMPORARY TABLE smart_scripts LIKE smart_seed;
INSERT INTO smart_scripts SELECT * FROM smart_seed;
UPDATE smart_scripts SET link=1 WHERE entryorguid=101 AND source_type=13 AND id=0;
UPDATE smart_scripts SET link=1 WHERE entryorguid=104841 AND source_type=0 AND id=0;
UPDATE smart_scripts SET link=2 WHERE entryorguid=123232 AND source_type=0 AND id=1;
CREATE TEMPORARY TABLE grave_seed LIKE game_graveyard_zone;
INSERT INTO grave_seed SELECT * FROM game_graveyard_zone WHERE ghost_zone IN(7334,7503,7541,7543,7558,7637);
CREATE TEMPORARY TABLE game_graveyard_zone LIKE grave_seed;
INSERT INTO game_graveyard_zone SELECT * FROM grave_seed;
'@
        $repairs = ($migrations | ForEach-Object { Get-Content -LiteralPath $_ -Raw }) -join "`n"
        $sql = ($shadow, $repairs, $repairs, $checks) -join "`n"
    }

    $config = Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" -Raw
    $connection = [regex]::Match($config, '(?m)^\s*WorldDatabaseInfo\s*=\s*"([^"\r\n]+)"').Groups[1].Value.Split(';')
    if ($connection.Length -ne 5) { throw 'Could not read the world database settings' }
    $previous = $env:MYSQL_PWD
    try {
        $env:MYSQL_PWD = $connection[3]
        $rows = @($sql | & C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mariadb.exe --no-defaults --protocol=TCP --batch --raw --skip-column-names -h $connection[0] -P $connection[1] -u $connection[2] $connection[4])
        if ($LASTEXITCODE) { throw 'World data check failed' }
    } finally {
        $env:MYSQL_PWD = $previous
    }
    $expected = @(
        "FIRE`t0:0:10,1:2:8,2:3:61,3:0:61",
        "VELEN`t0:1:12323200,1:0:119535",
        "SCENE`t0:0:92",
        "TIMED`t10",
        "COAST`t25`t1`t0",
        "CHEST`t271554`t72",
        "TOKEN`t147213`t7",
        "TOKEN`t147220`t4"
    )
    $rows
    if (($rows -join "`n") -cne ($expected -join "`n")) { throw 'SmartScript, graveyard or loot data differs' }
    "PASS world data (live=$Live)"
}
