param([switch]$RequirePlayable)
$ErrorActionPreference = 'Stop'
$repo = [IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out = "$repo/build-extractors/runtime-test/rockaway-audit"
New-Item -ItemType Directory -Force $out | Out-Null

function Query([string]$sql, [string]$database = 'World') {
    $connection = (Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" |
        Where-Object { $_ -match "^${database}DatabaseInfo\s*=" } |
        Select-Object -First 1).Split('"')[1].Split(';')
    $previousPassword = $env:MYSQL_PWD
    try {
        $env:MYSQL_PWD = $connection[3]
        $result = @($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names)
        if ($LASTEXITCODE) { throw 'Rockaway read-only database audit failed' }
    } finally { $env:MYSQL_PWD = $previousPassword }
    $result
}

$counts = @(Query @'
SELECT 'objective',COUNT(*) FROM quest_objectives WHERE QuestID=44746 AND Type=0 AND ObjectID=115168 AND Amount=1;
SELECT 'rewardConversation',COUNT(*) FROM quest_template WHERE ID=44746 AND RewardSpell=230020;
SELECT 'spawns',COUNT(*) FROM creature WHERE id IN(115132,115133);
SELECT 'groupMembers',COUNT(*) FROM creature_summon_groups WHERE entry IN(115132,115133);
SELECT 'configuredActors',COUNT(*) FROM creature_template WHERE entry IN(115132,115133) AND (AIName<>'' OR ScriptName<>'');
SELECT 'smartRows',COUNT(*) FROM smart_scripts WHERE source_type=0 AND entryorguid IN(115132,115133);
SELECT 'smartCredit',COUNT(*) FROM smart_scripts WHERE (action_type=33 AND action_param1=115168) OR (action_type IN(11,85) AND action_param1=229113);
SELECT 'creditSpellLoot',COUNT(*) FROM spell_loot_template WHERE Entry=229113;
SELECT 'miscIdSpellLoot',COUNT(*) FROM spell_loot_template WHERE Entry=70762;
SELECT 'worldQuestItems',COUNT(*) FROM world_quest_item WHERE QuestID=44746;
'@)
$values = [ordered]@{}
foreach ($row in $counts) { $pair = $row.Split("`t"); $values[$pair[0]] = [int]$pair[1] }
if ($values.objective -ne 1 -or $values.rewardConversation -ne 1) {
    throw 'Reinspect the native objective/reward link before using this audit'
}
$counts | Set-Content "$out/world-counts.tsv"

$overlays = @(Query @'
SELECT 'spellEffects',COUNT(*) FROM spell_effect WHERE SpellID IN(229113,230020,231295,231296,231298,231328,231329,231333,231340,231351,231353,231354,231356,231359,231386,231459,232681,232679);
SELECT 'task',COUNT(*) FROM quest_v2_cli_task WHERE ID=44746;
SELECT 'condition',COUNT(*) FROM player_condition WHERE ID=47902;
SELECT 'broadcast',ID,SoundEntriesID2 FROM broadcast_text WHERE ID IN(123587,123609) ORDER BY ID;
'@ 'Hotfix')
foreach ($tag in @('spellEffects', 'task', 'condition')) {
    if ($overlays -notcontains "$tag`t0") { throw "Inspect new $tag hotfix overrides" }
}
$overlays | Set-Content "$out/hotfix.tsv"

# A literal search is a discovery check, not proof that an arbitrary future script runs.
$cppReferences = @()
Get-ChildItem "$repo/src/server/scripts" -Recurse -File -Include *.cpp,*.h | ForEach-Object {
    $matches = Select-String -LiteralPath $_.FullName -Pattern '\b(115132|115133|115168|229113)\b'
    foreach ($match in $matches) {
        $cppReferences += "$($_.FullName.Substring($repo.Length + 1)):$($match.LineNumber):$($match.Line.Trim())"
    }
}
$cppReferences | Set-Content "$out/script-references.txt"
& "$repo/tools/tests/Test-RockawayCatalog.ps1" *> "$out/catalog.log"
if (-not (Select-String -LiteralPath "$out/catalog.log" -SimpleMatch 'PASS: typed native catalog.')) {
    throw 'Native catalog result missing'
}

$missing = @()
if (!$values.spawns -and !$values.groupMembers -and !$cppReferences.Count) {
    $missing += 'No DB actor placement/summon group or discovered C++ encounter route'
}
if (!$values.configuredActors -and !$values.smartRows) { $missing += 'Both principal actors have no configured AI' }
if (!$values.smartCredit -and !$cppReferences.Count) { $missing += 'No discovered completion credit invocation' }
if (!$values.creditSpellLoot) { $missing += 'No spell loot for the current EffectBonusLoot selector229113' }
$report = [ordered]@{
    Quest = 44746
    NativeCatalog = 'passed; content records only'
    Counts = $values
    CppReferences = $cppReferences
    Missing = $missing
    Playability = $(if ($missing.Count) { 'incomplete' } else { 'requires executable encounter and client checks' })
    ClientVerified = $false
}
$report | ConvertTo-Json -Depth 5 | Set-Content "$out/report.json"
$report | ConvertTo-Json -Depth 5
if ($RequirePlayable -and $missing.Count) { throw "44746 is incomplete: $($missing -join '; ')" }
