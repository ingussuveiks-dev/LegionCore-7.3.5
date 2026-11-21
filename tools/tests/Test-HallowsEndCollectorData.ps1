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
    if ($LASTEXITCODE) { throw 'Collector quest data query failed' }
    $rows
}

$actual = [IO.Path]::GetFullPath((Query 'SELECT @@datadir;')).TrimEnd('\','/')
$production = (Resolve-Path -LiteralPath (Join-Path (Split-Path (Split-Path $MariaDb -Parent) -Parent) 'data')).Path
if ($actual -ne $fixture -or $actual -eq $production) { throw 'A verified isolated world copy is required' }
$null = New-Item -ItemType Directory -Force -Path $out

$readback = @'
SELECT 'object',g.id,g.map,COALESCE(e.eventEntry,0),t.type,t.ScriptName,COUNT(*) FROM gameobject g
JOIN gameobject_template t ON t.entry=g.id LEFT JOIN game_event_gameobject e USING (guid)
WHERE g.id IN (209072,209076,209094,209095) GROUP BY g.id,g.map,e.eventEntry,t.type,t.ScriptName ORDER BY g.id,g.map,e.eventEntry;
SELECT 'actor',c.guid,c.id,c.map,COALESCE(e.eventEntry,0),COALESCE(a.bytes1,0),t.ScriptName FROM creature c
JOIN creature_template t ON t.entry=c.id LEFT JOIN game_event_creature e USING (guid) LEFT JOIN creature_addon a USING (guid)
WHERE c.id IN (54072,54118,54170,54205) ORDER BY c.guid;
SELECT 'void',t.faction,t.minlevel,t.maxlevel,t.ScriptName,COALESCE(s.LevelScalingMin,0),COALESCE(s.LevelScalingMax,0)
FROM creature_template t LEFT JOIN creature_template_scaling s ON s.Entry=t.entry WHERE t.entry=54114;
SELECT 'quest',q.ID,a.PrevQuestID,q.QuestType,q.StartItem,q.FlagsEx,q.RewardSpell,q.RewardNextQuest,q.RewardItem1,q.RewardAmount1
FROM quest_template q JOIN quest_template_addon a USING (ID) WHERE q.ID IN (29403,29411,29413,29427,29428,29429) ORDER BY q.ID;
SELECT 'objective',QuestID,Type,ObjectID,Amount FROM quest_objectives WHERE QuestID IN (29403,29411,29413,29427,29428,29429) ORDER BY QuestID,ID;
SELECT 'relation','GO start',id,quest FROM gameobject_queststarter WHERE id IN (209072,209076,209094,209095)
UNION ALL SELECT 'relation','GO end',id,quest FROM gameobject_questender WHERE id IN (209072,209076,209094,209095)
UNION ALL SELECT 'relation','NPC end',id,quest FROM creature_questender WHERE quest IN (29411,29428) ORDER BY 2,3,4;
SELECT 'season',SourceEntry,ConditionTypeOrReference,ConditionValue1 FROM conditions
WHERE SourceTypeOrReferenceId=19 AND SourceEntry IN (29413,29429) ORDER BY SourceEntry,ConditionTypeOrReference;
'@
$readback | Set-Content -LiteralPath "$out/readback.sql" -Encoding utf8NoBOM
$before = @(Query $readback)
$before | Set-Content -LiteralPath "$out/before.tsv" -Encoding utf8NoBOM
if ($before -match '^(object|actor|season)\t' -or "void`t35`t1`t1`t`t0`t0" -notin $before) { throw 'Collector counterfactual not reproduced' }
$sql = Get-Content -Raw -LiteralPath $Migration
Query "START TRANSACTION;`n$sql`nCOMMIT;" | Out-Null
$after = @(Query $readback)
$after | Set-Content -LiteralPath "$out/after.tsv" -Encoding utf8NoBOM

$expected = $before | ForEach-Object {
    if ($_ -match '^quest\t(29413|29429)\t') {
        $row = $_.Split("`t")
        $row[2] = if ($row[1] -eq '29413') { '29411' } else { '29428' }
        $row -join "`t"
    } elseif ($_ -match '^void\t') {
        "void`t14`t10`t10`tnpc_hallows_end_unleashed_void`t10`t110"
    } else { $_ }
}
$expected += @(
    "object`t209072`t0`t12`t2`tgo_hallows_end_crate`t1", "object`t209076`t0`t12`t2`tgo_hallows_end_crate`t2",
    "object`t209094`t0`t12`t2`tgo_hallows_end_crate`t1", "object`t209095`t1`t12`t2`tgo_hallows_end_crate`t2",
    "actor`t202611270`t54072`t0`t12`t0`tnpc_hallows_end_sanath", "actor`t202611271`t54072`t0`t12`t0`tnpc_hallows_end_sanath",
    "actor`t202611272`t54118`t0`t12`t0`t", "actor`t202611273`t54170`t1`t12`t0`t",
    "actor`t202611274`t54205`t0`t12`t7`t", "actor`t202611275`t54205`t0`t12`t7`t",
    "relation`tGO start`t209076`t29413", "relation`tGO start`t209095`t29429",
    "season`t29413`t12`t12", "season`t29429`t12`t12"
)
$delta = Compare-Object $expected $after -CaseSensitive
if ($delta) { $delta | Format-Table; throw 'Unexpected Collector data changes' }
'PASS both encounter routes, six seasonal crates, both pairs of receivers, native kill/turn-in objectives and one-time pet item reward links'
