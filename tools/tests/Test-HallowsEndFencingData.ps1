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
    if ($LASTEXITCODE) { throw 'Fencing the Goods data query failed' }
    $rows
}

$actual = [IO.Path]::GetFullPath((Query 'SELECT @@datadir;')).TrimEnd('\','/')
$production = (Resolve-Path -LiteralPath (Join-Path (Split-Path (Split-Path $MariaDb -Parent) -Parent) 'data')).Path
if ($actual -ne $fixture -or $actual -eq $production) { throw 'A verified isolated world copy is required' }
$null = New-Item -ItemType Directory -Force -Path $out

$readback = @'
SELECT 'spider',c.map,c.id,COUNT(*),SUM(c.MovementType=1),SUM(e.eventEntry=12)
FROM creature c LEFT JOIN game_event_creature e USING(guid) WHERE c.id IN (53965,54148) GROUP BY c.map,c.id ORDER BY c.map,c.id;
SELECT 'template',t.entry,COALESCE(a.auras,''),t.unit_flags,t.AIName,t.ScriptName
FROM creature_template t LEFT JOIN creature_template_addon a ON a.entry=t.entry WHERE t.entry IN(53965,54148) ORDER BY t.entry;
SELECT 'detection',spell,area,quest_start,quest_start_status,autocast,active_event FROM spell_area WHERE spell IN(100618,100619) ORDER BY spell,area;
SELECT 'credit',entryorguid,id,event_type,event_param1,event_param2,action_type,action_param1,target_type FROM smart_scripts
WHERE (entryorguid,id,source_type) IN ((8719,0,0),(44866,0,0)) ORDER BY entryorguid;
SELECT 'quest',QuestID,Type,ObjectID,Amount FROM quest_objectives WHERE QuestID IN(29398,29416) ORDER BY QuestID,ID;
'@
$readback | Set-Content -LiteralPath "$out/readback.sql" -Encoding utf8NoBOM
$before = @(Query $readback)
$before | Set-Content -LiteralPath "$out/before.tsv" -Encoding utf8NoBOM
if ($before -match '^(spider|detection)\t' -or "template`t53965`t`t0`t`t" -notin $before -or
    "template`t54148`t`t0`t`t" -notin $before) { throw 'Expected missing Fencing trail baseline was not reproduced' }
Query "START TRANSACTION;`n$(Get-Content -Raw -LiteralPath $Migration)`nCOMMIT;" | Out-Null
$after = @(Query $readback)
$after | Set-Content -LiteralPath "$out/after.tsv" -Encoding utf8NoBOM
$expected = @($before | Where-Object { $_ -notmatch '^template\t' }) + @(
    "spider`t0`t53965`t16`t16`t16", "spider`t1`t54148`t16`t16`t16",
    "template`t53965`t100512`t33554434`t`t", "template`t54148`t100617`t33554434`t`t",
    "detection`t100618`t1519`t29398`t8`t1`t12", "detection`t100619`t14`t29416`t8`t1`t12", "detection`t100619`t1637`t29416`t8`t1`t12"
)
$delta = Compare-Object $expected $after -CaseSensitive
if ($delta) { $delta | Format-Table; throw 'Unexpected Fencing the Goods data changes' }
'PASS 32 event-bound spider markers, native quest visibility auras and unchanged auctioneer credit'
