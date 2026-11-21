param(
    [Parameter(Mandatory)][string]$ClientOptions,
    [Parameter(Mandatory)][string]$FixtureDataDirectory,
    [Parameter(Mandatory)][string]$OutputDirectory,
    [string]$MariaDb = 'C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mariadb.exe'
)
$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path "$PSScriptRoot/../..").Path
$out = [IO.Path]::GetFullPath($OutputDirectory)
$options = (Resolve-Path -LiteralPath $ClientOptions).Path
$fixture = (Resolve-Path -LiteralPath $FixtureDataDirectory).Path
New-Item -ItemType Directory -Force -Path $out | Out-Null

function Query([string]$Sql) {
    $rows = $Sql | & $MariaDb "--defaults-extra-file=$options" --batch --raw --skip-column-names --database=legion_world
    if ($LASTEXITCODE) { throw 'Quest object loot fixture query failed' }
    $rows
}
$dataDirectory = Query 'SELECT @@datadir;'
$realDatabase = (Resolve-Path -LiteralPath (Join-Path (Split-Path (Split-Path $MariaDb -Parent) -Parent) 'data')).Path
if ([IO.Path]::GetFullPath($dataDirectory).TrimEnd('\','/') -ne $fixture -or $fixture -eq $realDatabase) {
    throw 'Regression requires the specified disposable fixture, away from the production DB'
}

$readback = @'
SELECT 'template', entry, type, Data0, Data1, flags, questItem1 FROM gameobject_template
WHERE entry IN (1617,1618,1619,1620,1621,1622,3724,3725,3726,3727,3729,3730,171938,194100,202552,202553,202554,203188,203385)
ORDER BY entry;
SELECT 'drop', Entry, Item, Currency, Reference, Chance, QuestRequired, LootMode, GroupId, MinCount, MaxCount
FROM gameobject_loot_template
WHERE Entry IN (1414,1415,1417,1418,1419,1617,1618,1619,1620,1621,1622,2512,3725,3726,3727,3729,3730,171938,194100,202552,202553,202554,203188,203385)
ORDER BY Entry, Item, Currency, Reference;
SELECT 'zone', SourceGroup, SourceEntry, ElseGroup, ConditionTypeOrReference, ConditionTarget, ConditionValue1
FROM conditions WHERE SourceTypeOrReferenceId=4 AND SourceEntry IN (5056,44850,52483,11583,55241,56225)
ORDER BY SourceGroup, SourceEntry, ElseGroup, ConditionTypeOrReference, ConditionValue1;
SELECT 'spawn', id, COUNT(*) FROM gameobject
WHERE map=1 AND zoneId=17 AND id IN (1620,1622,3724,3725,3726,3727,3729) GROUP BY id ORDER BY id;
SELECT 'objective', QuestID, ObjectID, Amount FROM quest_objectives
WHERE Type=1 AND QuestID IN (866,4402,13526,25110,25136,25735,25937) ORDER BY QuestID, ID;
'@
$readback | Set-Content -LiteralPath "$out/readback.sql" -Encoding utf8NoBOM
$migrations = foreach ($name in '29_root_samples_herb_loot','30_low_level_quest_object_loot') {
    Get-Content -Raw -LiteralPath "$repo/sql/updates/world/2026_11_21_$name.sql"
}
$before = @(Query $readback)
$before | Set-Content -LiteralPath "$out/before.tsv" -Encoding utf8NoBOM
if ("template`t3724`t3`t29`t2512`t278528`t5056" -notin $before -or
    "drop`t3727`t5056`t0`t0`t1.4`t0`t1`t0`t1`t1" -notin $before -or
    "drop`t194100`t44850`t0`t0`t100`t0`t1`t0`t1`t1" -notin $before) {
    throw 'Observed broken loot paths were not reproduced'
}
$sql = $migrations -join "`n"
Query "START TRANSACTION;`n$sql`nCOMMIT;" | Out-Null
$after = @(Query $readback)
$after | Set-Content -LiteralPath "$out/after.tsv" -Encoding utf8NoBOM
Query "START TRANSACTION;`n$sql`nCOMMIT;" | Out-Null
$repeat = @(Query $readback)
if (($after -join "`n") -cne ($repeat -join "`n")) { throw 'Fixture repair failed idempotence' }
$repeat | Set-Content -LiteralPath "$out/repeated.tsv" -Encoding utf8NoBOM

# An exact comparison also checks unchanged normal herb and second Kaja’Cola rows,
# rates, spawn counts, objectives, amounts, and unrelated loot in the same entries.
$expected = $before | ForEach-Object {
    $row = $_.Split("`t")
    if ($row[0] -eq 'template') {
        switch ($row[1]) {
            '1620' { $row[4] = '1620' }
            '1622' { $row[4] = '1622' }
            '3724' { $row[4] = '1618' }
        }
    } elseif ($row[0] -eq 'drop' -and "$($row[1])/$($row[2])" -in @(
        '3727/5056','3729/5056','171938/11583','194100/44850','202552/52483','202553/52483',
        '202554/52483','203188/55241','203385/56225')) {
        $row[6] = '1'
    }
    $row -join "`t"
}
$expected += 1618,1622,3726 | ForEach-Object { "drop`t$_`t5056`t0`t0`t100`t1`t1`t0`t1`t1" }
$expected += 1618,1620,1622,3725,3726,3727,3729 | ForEach-Object { "zone`t$_`t5056`t0`t4`t0`t17" }
$delta = Compare-Object $expected $after -CaseSensitive
if ($delta) {
    $delta | Format-Table
    throw 'Loot repair changed unexpected fixture rows'
}
'PASS fixture baseline, seven Barrens herb sources, source-4 zone conditions, seven quest loot gates, and idempotent replay'
