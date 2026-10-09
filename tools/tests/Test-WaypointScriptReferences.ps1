$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path "$PSScriptRoot/../..").Path
$source = Get-Content "$repo/src/server/database/Database/Implementation/WorldDatabase.cpp" -Raw
$match = [regex]::Match($source, 'PrepareStatement\(WOLRD_SEL_WAYPOINT_DATA_ACTION, "([^"]+)"')
if (!$match.Success) { throw 'Production waypoint-reference query not found' }
$query = $match.Groups[1].Value
# Connection-local tables shadow both live tables. Include duplicate actions,
# a scripted-only action (the real 347 case), zero and an unreferenced script.
$fixture = @"
CREATE TEMPORARY TABLE waypoint_data (action INT);
CREATE TEMPORARY TABLE waypoint_data_script (action INT);
INSERT INTO waypoint_data VALUES (0),(10),(10);
INSERT INTO waypoint_data_script VALUES (10),(347),(347);
SELECT action FROM ($query) references_in_both_tables ORDER BY action;
"@
$connection = (Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object { $_ -match '^WorldDatabaseInfo\s*=' }).Split('"')[1].Split(';')
$mysql = Get-ChildItem C:/wamp64/bin/mysql -Recurse -Filter mysql.exe | Select-Object -First 1 -ExpandProperty FullName
$previous = $env:MYSQL_PWD
try {
    $env:MYSQL_PWD = $connection[3]
    $rows = & $mysql --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --skip-column-names --execute=$fixture
    if ($LASTEXITCODE) { throw 'Waypoint-reference SQL fixture failed' }
    if (($rows -join ',') -ne '0,10,347') { throw "Wrong referenced actions: $($rows -join ',')" }
    $unreferenced = @(10,347,999 | Where-Object { $_ -notin $rows })
    if (($unreferenced -join ',') -ne '999') { throw 'Unreferenced script detection changed' }
} finally { $env:MYSQL_PWD = $previous }
'PASS: production query includes both path tables, deduplicates actions and retains genuine orphan detection.'
