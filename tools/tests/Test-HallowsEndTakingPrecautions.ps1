param(
    [Parameter(Mandatory)][string]$ClientOptions,
    [Parameter(Mandatory)][string]$FixtureDataDirectory,
    [Parameter(Mandatory)][string]$OutputDirectory,
    [Parameter(Mandatory)][string]$Migration,
    [string]$MariaDb = 'C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mariadb.exe'
)
$ErrorActionPreference = 'Stop'
$options = (Resolve-Path -LiteralPath $ClientOptions).Path
$fixture = (Resolve-Path -LiteralPath $FixtureDataDirectory).Path
$out = [IO.Path]::GetFullPath($OutputDirectory)
$null = New-Item -ItemType Directory -Force -Path $out

function Query([string]$Sql) {
    $rows = $Sql | & $MariaDb "--defaults-extra-file=$options" --batch --raw --skip-column-names --database=legion_world
    if ($LASTEXITCODE) { throw 'Taking Precautions fixture query failed' }
    $rows
}

$actual = [IO.Path]::GetFullPath((Query 'SELECT @@datadir;')).TrimEnd('\','/')
$production = (Resolve-Path -LiteralPath (Join-Path (Split-Path (Split-Path $MariaDb -Parent) -Parent) 'data')).Path
if ($actual -ne $fixture -or $actual -eq $production) { throw 'A separate verified world fixture is required' }

$readback = @'
SELECT 'herb',entry,type,Data0,Data1,questItem1,flags FROM gameobject_template WHERE entry=209059;
SELECT 'plant',g.map,COUNT(*),SUM(e.eventEntry=12),SUM(g.spawntimesecs=120),SUM(ABS(g.rotation2*g.rotation2+g.rotation3*g.rotation3-1)<0.001)
FROM gameobject g LEFT JOIN game_event_gameobject e USING (guid) WHERE g.id=209059 GROUP BY g.map ORDER BY g.map;
SELECT 'loot',Entry,Item,Currency,Reference,Chance,QuestRequired,LootMode,MinCount,MaxCount
FROM gameobject_loot_template WHERE Entry=39847 OR Item=71035 ORDER BY Entry,Item,Currency;
SELECT 'actor',c.id,c.map,e.eventEntry,t.ScriptName,COALESCE(a.bytes1,0)
FROM creature c JOIN creature_template t ON t.entry=c.id LEFT JOIN game_event_creature e USING (guid)
LEFT JOIN creature_addon a USING (guid) WHERE c.id IN (53950,54142,54021,54146) ORDER BY c.id,c.guid;
SELECT 'requirement',QuestID,ObjectID,Amount FROM quest_objectives WHERE QuestID IN (29402,29426) AND Type=1 ORDER BY QuestID,ObjectID;
SELECT 'prerequisite',a.ID,a.PrevQuestID,a.NextQuestID,q.RewardSpell,q.StartItem,q.ItemDrop1,q.ItemDropQuantity1,q.FlagsEx
FROM quest_template_addon a JOIN quest_template q USING (ID) WHERE a.ID IN (29402,29426) ORDER BY a.ID;
SELECT 'offer', 'S', id,quest FROM creature_queststarter WHERE quest IN (29392,29398,29399,29402,29403,29415,29416,29425,29426,29427)
UNION ALL SELECT 'offer', 'E', id,quest FROM creature_questender WHERE quest IN (29392,29398,29399,29402,29415,29416,29425,29426) ORDER BY 2,4,3;
SELECT 'conditions',SourceTypeOrReferenceId,COUNT(*) FROM conditions WHERE
(SourceTypeOrReferenceId=4 AND SourceGroup=39847 AND SourceEntry=71035) OR
(SourceTypeOrReferenceId=13 AND SourceGroup=1 AND SourceEntry=100529) OR
(SourceTypeOrReferenceId=15 AND SourceGroup IN(12941,12949) AND SourceEntry=2) OR
(SourceTypeOrReferenceId=19 AND SourceEntry IN(29392,29398,29399,29402,29403,29411,29415,29416,29425,29426,29427,29428)) OR
(SourceTypeOrReferenceId=22 AND SourceEntry IN(8719,44866) AND SourceGroup=1 AND SourceId=0)
GROUP BY SourceTypeOrReferenceId ORDER BY SourceTypeOrReferenceId;
SELECT 'question',s.entryorguid,s.event_param1,s.event_param2,s.action_type,s.action_param1
FROM smart_scripts s JOIN gossip_menu_option g ON (g.MenuID,g.OptionID)=(s.event_param1,s.event_param2)
WHERE s.entryorguid IN(8719,44866) AND s.source_type=0 AND g.OptionNpc=1 ORDER BY s.entryorguid;
SELECT 'auction',MenuID,OptionID,OptionNpc FROM gossip_menu_option WHERE MenuID IN(12941,12949) AND OptionNpc=10 ORDER BY MenuID,OptionID;
'@
$readback | Set-Content -LiteralPath "$out/readback.sql" -Encoding utf8NoBOM
$before = @(Query $readback)
$before | Set-Content -LiteralPath "$out/before.tsv" -Encoding utf8NoBOM
if ($before -match '^(plant|loot|actor|question|conditions)\t') { throw 'Expected broken baseline is missing' }
$sql = Get-Content -Raw -LiteralPath $Migration
Query "START TRANSACTION;`n$sql`nCOMMIT;" | Out-Null
$after = @(Query $readback)
$after | Set-Content -LiteralPath "$out/after.tsv" -Encoding utf8NoBOM

$expected = $before | ForEach-Object {
    if ($_ -match '^prerequisite\t') {
        $row = $_.Split("`t")
        $row[6] = '71035'
        $row[7] = '5'
        $row -join "`t"
    } else { $_ }
}
$expected += @(
    "plant`t0`t20`t20`t20`t20", "plant`t1`t20`t20`t20`t20", "loot`t39847`t71035`t0`t0`t100`t1`t1`t1`t1",
    "actor`t53950`t0`t12`tnpc_hallows_end_hired_courier`t7", "actor`t54142`t1`t12`tnpc_hallows_end_hired_courier`t7",
    "actor`t54021`t0`t12`tnpc_hallows_end_investigator`t0", "actor`t54146`t1`t12`tnpc_hallows_end_investigator`t0",
    "conditions`t4`t1", "conditions`t13`t1", "conditions`t15`t8", "conditions`t19`t12", "conditions`t22`t8",
    "question`t8719`t12941`t2`t86`t100529", "question`t44866`t12949`t2`t86`t100529"
)
$delta = Compare-Object $expected $after -CaseSensitive
if ($delta) { $delta | Format-Table; throw 'Unexpected Taking Precautions rows' }
'PASS isolated baseline and migrated world rows for both factions, all 40 seasonal plants, native quantities, NPC registrations, auction options and quest links'
