param([string]$OutputDirectory = "$PSScriptRoot/../../.codex/progression-audit")
$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path "$PSScriptRoot/../..").Path
$output = [IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Force -Path $output | Out-Null
$config = Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"
$mysql = Get-ChildItem C:/wamp64/bin/mysql -Recurse -Filter mysql.exe | Select-Object -First 1 -ExpandProperty FullName
function Export($database, $sql, $name) {
    $connection = ($config | Where-Object { $_ -match "^${database}DatabaseInfo\s*=" } | Select-Object -First 1).Split('"')[1].Split(';')
    $previous = $env:MYSQL_PWD
    try {
        $env:MYSQL_PWD = $connection[3]
        $rows = & $mysql --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --execute=$sql
        if ($LASTEXITCODE) { throw "Export failed: $name" }
        [IO.File]::WriteAllText("$output/$name", ($rows -join "`n") + "`n", [Text.UTF8Encoding]::new($false))
    } finally { $env:MYSQL_PWD = $previous }
}
$tables = @('spell','spell_effect','spell_misc','spell_range','spell_radius','spell_cast_times','spell_duration','spell_target_restrictions','spell_interrupts','spell_aura_restrictions','spell_shapeshift','item','item_set','item_set_spell','item_effect','currency_types','skill_line','faction','scenario','scenario_step','criteria','criteria_tree')
foreach ($table in $tables) { Export Hotfix "SELECT * FROM $table" "mechanics-$table.tsv" }
Export Hotfix (($tables | ForEach-Object { "SELECT '$_' AS table_name, COUNT(*) AS rows_count FROM $_" }) -join ' UNION ALL ') 'counts.tsv'
Export Hotfix 'SELECT * FROM hotfix_data' 'mechanics-link-hotfixes.tsv'
$rewards = (1..4 | ForEach-Object { "RewardItem$_,RewardAmount$_,RewardCurrencyID$_,RewardCurrencyQty$_" }) -join ','
$choices = (1..6 | ForEach-Object { "RewardChoiceItemID$_,RewardChoiceItemQuantity$_" }) -join ','
Export World "SELECT ID,RewardNextQuest,RewardSpell,StartItem,RewardSkillLineID,$rewards,$choices FROM quest_template" 'quests.tsv'
Export World 'SELECT ID,PrevQuestID,NextQuestID,ExclusiveGroup,SourceSpellID,RequiredSkillID,RequiredSkillPoints FROM quest_template_addon' 'quest-addon.tsv'
Export World 'SELECT ID,QuestID,Type,ObjectID,Amount,StorageIndex FROM quest_objectives' 'objectives.tsv'
Export World 'SELECT entry FROM creature_template_wdb' 'creatures.tsv'
Export World 'SELECT entry FROM disables WHERE sourceType=1' 'disabled-quests.tsv'
Export World 'SELECT entry FROM gameobject_template' 'gameobjects.tsv'
Export World 'SELECT * FROM creature_queststarter UNION SELECT * FROM creature_questender' 'npc-quests.tsv'
Export World 'SELECT * FROM gameobject_queststarter UNION SELECT * FROM gameobject_questender' 'go-quests.tsv'
Export World 'SELECT * FROM scenario_data WHERE MapID IN (1460,1554,1557)' 'scenario-routes.tsv'
Export World 'SELECT * FROM instance_template WHERE map IN (1460,1554,1557)' 'instance-routes.tsv'
Export World 'SELECT * FROM spell_script_names WHERE spell_id=227058' 'departure-binding.tsv'
Export World 'SELECT id,point,action FROM waypoint_data_script WHERE action<>0' 'script-paths.tsv'
Export World 'SELECT DISTINCT action FROM waypoint_data' 'path-actions.tsv'
Export World 'SELECT DISTINCT id FROM waypoint_scripts' 'path-scripts.tsv'
'Exported read-only progression and spell/item data; no character rows or credentials exported.'
