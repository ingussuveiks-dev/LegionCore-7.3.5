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
    $lines = $Sql | & $MariaDb "--defaults-extra-file=$options" --batch --raw --skip-column-names --database=legion_world
    if ($LASTEXITCODE) { throw 'Collector introduction data query failed' }
    $lines
}

$actual = [IO.Path]::GetFullPath((Query 'SELECT @@datadir;')).TrimEnd('\','/')
$production = (Resolve-Path -LiteralPath (Join-Path (Split-Path (Split-Path $MariaDb -Parent) -Parent) 'data')).Path
if ($actual -ne $fixture -or $actual -eq $production) { throw 'A verified isolated world copy is required' }
$null = New-Item -ItemType Directory -Force -Path $out

$readback = @'
SELECT 'actor',c.guid,c.id,c.map,e.eventEntry,COALESCE(a.bytes1,0),COALESCE(a.auras,''),t.ScriptName FROM creature c
JOIN creature_template t ON t.entry=c.id JOIN game_event_creature e USING (guid) LEFT JOIN creature_addon a USING (guid)
WHERE c.id IN (54072,54205) ORDER BY c.guid;
SELECT 'line',GroupID,ID,Type,Probability,comment,Text FROM creature_text WHERE CreatureID=54072 ORDER BY GroupID,ID;
SELECT 'cast',spell_id,ScriptName FROM spell_script_names WHERE spell_id IN (100668,100669) ORDER BY spell_id,ScriptName;
SELECT 'objective',QuestID,Type,ObjectID,Amount FROM quest_objectives WHERE QuestID IN (29403,29427,29411,29428) ORDER BY QuestID,ID;
SELECT 'crate',g.guid,g.id,g.map,e.eventEntry FROM gameobject g JOIN game_event_gameobject e USING (guid)
WHERE g.id IN (209072,209094,209076,209095) ORDER BY g.guid;
'@
[IO.File]::WriteAllText("$out/readback.sql",$readback)
$before = @(Query $readback)
$before | Set-Content -LiteralPath "$out/before.tsv" -Encoding utf8NoBOM
if (($before -match '^cast\t').Count -or ($before -match '^line\t').Count -ne 1 -or
    "actor`t202611274`t54205`t0`t12`t7`t`t" -notin $before -or "actor`t202611275`t54205`t0`t12`t7`t`t" -notin $before) {
    throw 'Earlier Collector intro data did not reproduce'
}
$sql = Get-Content -Raw -LiteralPath $Migration
Query "START TRANSACTION;`n$sql`nCOMMIT;" | Out-Null
$after = @(Query $readback)
$after | Set-Content -LiteralPath "$out/after.tsv" -Encoding utf8NoBOM
$expected = @($before | Where-Object { $_ -notmatch '^line\t' } | ForEach-Object {
    if ($_ -match '^actor\t20261127[45]\t54205\t') {
        $fields = $_.Split("`t")
        $fields[7] = 'npc_hallows_end_ren'
        $fields -join "`t"
    } else { $_ }
})
$expected += @(
    "line`t0`t0`t12`t100`tHallow End - Sanath denies the payment`tMy master owes you nothing. You will leave here empty-handed.",
    "line`t1`t0`t12`t100`tHallow End - Sanath notices the witness`tSomeone followed you. Time to find out what this crate is capable of.",
    "line`t2`t0`t12`t100`tHallow End - Sanath retreats`tNo! I cannot control it!",
    "cast`t100668`tspell_hallows_end_summon_void", "cast`t100669`tspell_hallows_end_void_explosion"
)
$delta = Compare-Object $expected $after -CaseSensitive
if ($delta) { $delta | Format-Table; throw 'Unexpected Collector introduction row changes' }
'PASS three private dialogue cues, original Ren placements and corpse pose, native explosion bindings, unchanged objectives and crates'
