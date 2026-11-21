param(
    [Parameter(Mandatory)][string]$ClientOptions,
    [Parameter(Mandatory)][string]$FixtureDataDirectory,
    [Parameter(Mandatory)][string]$Migration,
    [Parameter(Mandatory)][string]$OutputDirectory,
    [string]$MariaDb = 'C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mariadb.exe'
)
$ErrorActionPreference = 'Stop'
$options = (Resolve-Path -LiteralPath $ClientOptions).Path
$fixture = (Resolve-Path -LiteralPath $FixtureDataDirectory).Path
$out = [IO.Path]::GetFullPath($OutputDirectory)

function Query([string]$Sql) {
    $rows = $Sql | & $MariaDb "--defaults-extra-file=$options" --batch --raw --skip-column-names --database=legion_world
    if ($LASTEXITCODE) { throw 'Hallow End scene data query failed' }
    $rows
}

$actual = [IO.Path]::GetFullPath((Query 'SELECT @@datadir;')).TrimEnd('\','/')
$production = (Resolve-Path -LiteralPath (Join-Path (Split-Path (Split-Path $MariaDb -Parent) -Parent) 'data')).Path
if ($actual -ne $fixture -or $actual -eq $production) { throw 'A verified isolated world copy is required' }
$null = New-Item -ItemType Directory -Force -Path $out

$readback = @'
SELECT 'actor',entry,ScriptName,AIName,npcflag,gossip_menu_id FROM creature_template
WHERE entry IN (6986,53949,53991,54021,54118,54141,54146,54170) ORDER BY entry;
SELECT 'text',CreatureID,GroupID,Type,Text FROM creature_text
WHERE CreatureID IN (6986,53949,53991,54021,54118,54141,54146,54170) ORDER BY CreatureID,GroupID,ID;
SELECT 'quest',ID,Flags,FlagsEx,StartItem,RewardSpell,RewardNextQuest,RewardItem1,RewardAmount1 FROM quest_template
WHERE ID IN (29399,29411,29413,29425,29428,29429) ORDER BY ID;
SELECT 'objective',QuestID,Type,ObjectID,Amount FROM quest_objectives WHERE QuestID IN (29399,29411,29413,29425,29428,29429) ORDER BY QuestID,ID;
SELECT 'receiver',id,quest FROM creature_questender WHERE quest IN (29399,29411,29425,29428) ORDER BY quest,id;
SELECT 'reward',id,quest FROM gameobject_questender WHERE quest IN (29413,29429) ORDER BY quest,id;
'@
$readback | Set-Content -LiteralPath "$out/readback.sql" -Encoding utf8NoBOM
$before = @(Query $readback)
$before | Set-Content -LiteralPath "$out/before.tsv" -Encoding utf8NoBOM
if (@($before | Where-Object { $_ -match '^text\t(54021|54146)\t[01]\t' }).Count -ne 4 -or
    $before -match '^actor\t\d+\tnpc_hallows_end_crate_return\t') { throw 'Shopping scene baseline differs' }

$sql = Get-Content -Raw -LiteralPath $Migration
Query "START TRANSACTION;`n$sql`nCOMMIT;" | Out-Null
$after = @(Query $readback)
$after | Set-Content -LiteralPath "$out/after.tsv" -Encoding utf8NoBOM
$stable = Compare-Object @($before | Where-Object { $_ -notmatch '^(text|actor)\t' }) @($after | Where-Object { $_ -notmatch '^(text|actor)\t' })
if ($stable) { $stable | Format-Table; throw 'Quest objective, item, reward or receiver was altered by the presentation migration' }

$expectedActors = $before | Where-Object { $_ -match '^actor\t' } | ForEach-Object {
    $_ -replace '^actor\t(53949|54118|54141|54170)\t\t', "actor`t`$1`tnpc_hallows_end_crate_return`t"
}
$actorDelta = Compare-Object @($expectedActors) @($after | Where-Object { $_ -match '^actor\t' }) -CaseSensitive
if ($actorDelta) { $actorDelta | Format-Table; throw 'Wrong actor bindings or NPC flags' }

$expectedCues = @(
    '54021/10/12','54021/11/12','53991/12/12','53991/13/12','54021/14/12','53991/15/12','54021/16/12',
    '54146/10/12','6986/11/12','6986/12/12','54146/13/12','6986/14/12','54146/15/12','54146/16/12',
    '54118/10/15','54118/11/15','54118/12/15','54118/13/15','53949/10/15','53949/11/15','53949/12/15','53949/13/15','53949/14/15','53949/15/15',
    '54170/10/15','54170/11/15','54170/12/15','54170/13/15','54141/10/15','54141/11/15','54141/12/15','54141/13/15','54141/14/15'
)
$actualCues = $after | Where-Object { $_ -match '^text\t' } | ForEach-Object {
    $row = $_.Split("`t", 5)
    if (!$row[4]) { throw 'Empty scene speech' }
    $row[1..3] -join '/'
}
$cueDelta = Compare-Object $expectedCues @($actualCues)
if ($cueDelta) { $cueDelta | Format-Table; throw 'Missing or unexpected scene speech' }
'PASS seven alternating lines per faction, private ritual speech, four existing actors and unchanged native quest rewards'
