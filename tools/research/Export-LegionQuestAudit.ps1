param([string]$OutputDirectory = "$PSScriptRoot/../../.codex/legion-quests")
$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path "$PSScriptRoot/../..").Path
$output = [IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Force -Path $output | Out-Null
$connection = (Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object { $_ -match '^WorldDatabaseInfo\s*=' }).Split('"')[1].Split(';')
$mysql = Get-ChildItem C:/wamp64/bin/mysql -Recurse -Filter mysql.exe | Select-Object -First 1 -ExpandProperty FullName
function Export($sql, $name) {
    $previous = $env:MYSQL_PWD
    try {
        $env:MYSQL_PWD = $connection[3]
        $rows = & $mysql --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --execute=$sql
        if ($LASTEXITCODE) { throw "Export failed: $name" }
        [IO.File]::WriteAllText("$output/$name.tsv", ($rows -join "`n") + "`n", [Text.UTF8Encoding]::new($false))
    } finally { $env:MYSQL_PWD = $previous }
}
Export "SELECT q.ID,COALESCE(l.LogTitle,q.LogTitle,'') AS Title,q.QuestType,q.QuestLevel,q.MinLevel,q.QuestMaxScalingLevel,q.Expansion,q.QuestSortID,q.QuestInfoID,q.AllowableRaces,q.Flags,q.FlagsEx,q.RewardNextQuest,q.RewardSpell,q.StartItem,q.StartScript,q.CompleteScript FROM quest_template q LEFT JOIN quest_template_locale l ON l.ID=q.ID AND l.locale='enUS'" 'quests'
Export 'SELECT ID,PrevQuestID,NextQuestID,ExclusiveGroup,AllowableClasses,SpecialFlags,SourceSpellID FROM quest_template_addon' 'addons'
Export 'SELECT * FROM quest_objectives' 'objectives'
Export 'SELECT entry,comment FROM disables WHERE sourceType=1' 'disabled'
Export "SELECT 'npc_start' AS kind,id,quest FROM creature_queststarter UNION ALL SELECT 'npc_end',id,quest FROM creature_questender UNION ALL SELECT 'go_start',id,quest FROM gameobject_queststarter UNION ALL SELECT 'go_end',id,quest FROM gameobject_questender UNION ALL SELECT 'area_start',id,quest FROM area_queststart" 'relations'
Export 'SELECT id,COUNT(*) AS spawns FROM creature GROUP BY id' 'creature-spawns'
Export 'SELECT id,COUNT(*) AS spawns FROM gameobject GROUP BY id' 'gameobject-spawns'
Export 'SELECT * FROM conditions WHERE SourceTypeOrReferenceId IN (19,20)' 'conditions'
Export 'SELECT eventEntry,id,quest FROM game_event_creature_quest' 'event-quests'
Export 'SELECT entry FROM creature_template_wdb' 'creature-templates'
Export 'SELECT entry FROM gameobject_template' 'gameobject-templates'
Export 'SELECT entryorguid,source_type,id,action_type,action_param1,action_param2,event_type,event_param1,event_param2,comment FROM smart_scripts WHERE action_type IN (7,15,26,33,56,80,85,134,211,223) OR event_type IN (19,20)' 'smart-quests'
'Exported quest metadata, prerequisites, sources and conditions read-only; no character data exported.'
