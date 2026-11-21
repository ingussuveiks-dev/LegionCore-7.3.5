param(
    [string]$OutputDirectory = "$PSScriptRoot/../../.codex/level-quest-audit",
    [string]$MariaDb = 'C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mariadb.exe',
    [int]$SnapshotPort = 0
)

$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path "$PSScriptRoot/../..").Path
$output = [IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Force -Path $output | Out-Null
$conf = Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"

function Export-Database($setting, $tables) {
    $line = $conf | Where-Object { $_ -match "^$setting\s*=" }
    $connection = $line.Split('"')[1].Split(';')
    $port = $connection[1]
    if ($SnapshotPort) { $port = $SnapshotPort }
    $arguments = @('--no-defaults', '--batch', '--default-character-set=utf8mb4',
        "--host=$($connection[0])", "--port=$port", "--user=$($connection[2])",
        "--database=$($connection[4])")
    $sql = [Collections.Generic.List[string]]::new()
    $sql.Add('SET TRANSACTION READ ONLY; START TRANSACTION WITH CONSISTENT SNAPSHOT;')
    foreach ($name in $tables.Keys) {
        $sql.Add("SELECT '__AUDIT_TABLE__$name';")
        $sql.Add("$($tables[$name]);")
    }
    $sql.Add('COMMIT;')
    $previousPassword = $env:MYSQL_PWD
    $previousEncoding = [Console]::OutputEncoding
    $writer = $null
    try {
        [Console]::OutputEncoding = [Text.UTF8Encoding]::new($false)
        $env:MYSQL_PWD = $connection[3]
        # The marker SELECT has the same text as its column header and its only row.
        $skipMarkerRow = $false
        & $MariaDb @arguments "--execute=$($sql -join ' ')" | ForEach-Object {
            if ($_ -match '^(?:__AUDIT_TABLE__|''__AUDIT_TABLE__)([a-z_]+)''?$') {
                if ($skipMarkerRow) {
                    $skipMarkerRow = $false
                } else {
                    if ($null -ne $writer) { $writer.Dispose() }
                    $writer = [IO.StreamWriter]::new("$output/$($Matches[1]).tsv", $false, [Text.UTF8Encoding]::new($false))
                    $skipMarkerRow = $true
                }
            } elseif ($null -ne $writer) {
                $writer.WriteLine($_)
            }
        }
        if ($LASTEXITCODE) { throw "Read-only export failed for $setting." }
    } finally {
        if ($null -ne $writer) { $writer.Dispose() }
        $env:MYSQL_PWD = $previousPassword
        [Console]::OutputEncoding = $previousEncoding
    }
}

$world = [ordered]@{
    database_version = 'SELECT VERSION() AS version'
    quests = 'SELECT ID,LogTitle,LogDescription,QuestLevel,MinLevel,QuestMaxScalingLevel,QuestType,Expansion,QuestSortID,QuestInfoID,SuggestedGroupNum,AllowableRaces,Flags,FlagsEx,StartItem,StartScript,CompleteScript,RewardNextQuest,RewardSpell,TimeAllowed,RewardItem1,RewardItem2,RewardItem3,RewardItem4,ItemDrop1,ItemDrop2,ItemDrop3,ItemDrop4,ItemDropQuantity1,ItemDropQuantity2,ItemDropQuantity3,ItemDropQuantity4 FROM quest_template ORDER BY ID'
    addons = 'SELECT * FROM quest_template_addon ORDER BY ID'
    objectives = 'SELECT * FROM quest_objectives ORDER BY QuestID,ID'
    disabled = 'SELECT entry,comment FROM disables WHERE sourceType=1 ORDER BY entry'
    relations = "SELECT 'npc_start' AS kind,id,quest FROM creature_queststarter UNION ALL SELECT 'npc_end',id,quest FROM creature_questender UNION ALL SELECT 'go_start',id,quest FROM gameobject_queststarter UNION ALL SELECT 'go_end',id,quest FROM gameobject_questender UNION ALL SELECT 'area_start',id,quest FROM area_queststart UNION ALL SELECT 'event_npc_start',id,quest FROM game_event_creature_quest UNION ALL SELECT 'event_go_start',id,quest FROM game_event_gameobject_quest ORDER BY quest,kind,id"
    creatures = 'SELECT w.entry,w.Name1,w.KillCredit1,w.KillCredit2,w.Displayid1,w.Displayid2,w.Displayid3,w.Displayid4,w.FlagQuest,c.gossip_menu_id,c.npcflag,c.minlevel,c.maxlevel,c.unit_flags,c.lootid,c.pickpocketloot,c.skinloot,c.AIName,c.VehicleId,c.ScriptName,s.LevelScalingMin,s.LevelScalingMax FROM creature_template_wdb w LEFT JOIN creature_template c ON c.entry=w.entry LEFT JOIN creature_template_scaling s ON s.Entry=w.entry ORDER BY w.entry'
    creature_spawns = 'SELECT guid,id,map,zoneId,areaId,spawnMask,PhaseId,phaseMask,modelid,npcflag,unit_flags,AiID,position_x,position_y,position_z FROM creature ORDER BY guid'
    objects = 'SELECT entry,type,name,displayId,flags,AiName,ScriptName,questItem1,questItem2,questItem3,questItem4,questItem5,questItem6,Data0,Data1,Data2,Data3,Data4,Data5,Data6,Data7,Data8,Data9,Data10,Data11,Data12,Data13,Data14,Data15,Data16,Data17,Data18,Data19,Data20,Data21,Data22,Data23,Data24,Data25,Data26,Data27,Data28,Data29,Data30,Data31,Data32 FROM gameobject_template ORDER BY entry'
    object_spawns = 'SELECT guid,id,map,zoneId,areaId,spawnMask,PhaseId,phaseMask,spawntimesecs,AiID,position_x,position_y,position_z FROM gameobject ORDER BY guid'
    object_actions = 'SELECT * FROM gameobject_action ORDER BY entry,idx'
    object_visuals = 'SELECT * FROM gameobject_quest_visual ORDER BY goID,questID'
    event_objects = 'SELECT s.guid,s.id,s.map,s.zoneId,s.areaId,s.PhaseId,t.ScriptName,t.SpellID FROM eventobject s JOIN eventobject_template t ON s.id=t.entry ORDER BY s.guid'
    creature_events = 'SELECT guid,eventEntry FROM game_event_creature ORDER BY guid,eventEntry'
    object_events = 'SELECT guid,eventEntry FROM game_event_gameobject ORDER BY guid,eventEntry'
    npc_quest_events = 'SELECT id,quest,eventEntry FROM game_event_creature_quest ORDER BY quest,id,eventEntry'
    go_quest_events = 'SELECT id,quest,eventEntry FROM game_event_gameobject_quest ORDER BY quest,id,eventEntry'
    creature_pools = 'SELECT guid,pool_entry,chance FROM pool_creature ORDER BY guid,pool_entry'
    object_pools = 'SELECT guid,pool_entry,chance FROM pool_gameobject ORDER BY guid,pool_entry'
    loot_creature = 'SELECT * FROM creature_loot_template'
    loot_object = 'SELECT * FROM gameobject_loot_template'
    loot_fishing = 'SELECT * FROM fishing_loot_template'
    loot_item = 'SELECT * FROM item_loot_template'
    loot_reference = 'SELECT * FROM reference_loot_template'
    loot_pickpocket = 'SELECT * FROM pickpocketing_loot_template'
    loot_skinning = 'SELECT * FROM skinning_loot_template'
    loot_spell = 'SELECT * FROM spell_loot_template'
    loot_zone = 'SELECT * FROM zone_loot_template'
    loot_world = 'SELECT * FROM world_loot_template'
    loot_mail = 'SELECT * FROM mail_loot_template'
    vendor_items = 'SELECT entry,item,ExtendedCost,type,PlayerConditionID FROM npc_vendor ORDER BY entry,slot'
    conditions = 'SELECT * FROM conditions ORDER BY SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1'
    phase_definitions = 'SELECT * FROM phase_definitions ORDER BY zoneId,entry'
    spell_phase = 'SELECT * FROM spell_phase ORDER BY id'
    spell_area = 'SELECT * FROM spell_area ORDER BY spell,area'
    spell_click = 'SELECT * FROM npc_spellclick_spells ORDER BY npc_entry,spell_id'
    spell_scripts = 'SELECT * FROM spell_script_names ORDER BY spell_id,ScriptName'
    spell_checks = 'SELECT * FROM spell_check_cast ORDER BY spellId'
    spell_destinations = 'SELECT * FROM spell_target_position ORDER BY ID'
    smart = 'SELECT * FROM smart_scripts ORDER BY source_type,entryorguid,id'
    summon_groups = 'SELECT * FROM creature_summon_groups ORDER BY summonerType,summonerId,groupId,entry'
    db_quest_start = 'SELECT * FROM quest_start_scripts ORDER BY id,delay,command'
    db_quest_end = 'SELECT * FROM quest_end_scripts ORDER BY id,delay,command'
    db_object_scripts = 'SELECT * FROM gameobject_scripts ORDER BY id,delay,command'
    db_event_scripts = 'SELECT * FROM event_scripts ORDER BY id,delay,command'
    area_scripts = 'SELECT * FROM areatrigger_scripts ORDER BY entry'
    gossip_options = 'SELECT * FROM gossip_menu_option ORDER BY MenuID,OptionID'
    quest_poi = 'SELECT * FROM quest_poi ORDER BY QuestID'
    updates = 'SELECT name,hash,state,timestamp FROM updates ORDER BY timestamp,name'
}
$hotfix = [ordered]@{
    hotfix_item = 'SELECT ID,ClassID,SubclassID,VerifiedBuild FROM item ORDER BY ID'
    hotfix_item_sparse = 'SELECT ID,Display,StartQuestID,RequiredLevel,VerifiedBuild FROM item_sparse ORDER BY ID'
    hotfix_item_effect = 'SELECT ID,ItemID,SpellID,TriggerType,Charges,VerifiedBuild FROM item_effect ORDER BY ID'
    hotfix_spell = 'SELECT ID,VerifiedBuild FROM spell ORDER BY ID'
    hotfix_spell_effect = 'SELECT ID,SpellID,Effect,EffectIndex,DifficultyID,EffectItemType,EffectTriggerSpell,EffectMiscValue1,EffectMiscValue2,ImplicitTarget1,ImplicitTarget2,VerifiedBuild FROM spell_effect ORDER BY ID'
    hotfix_quest_objective = 'SELECT * FROM quest_objective ORDER BY ID'
    hotfix_gameobjects = 'SELECT * FROM gameobjects ORDER BY ID'
    hotfix_area = 'SELECT ID,AreaName,VerifiedBuild FROM area_table ORDER BY ID'
    hotfix_criteria_tree = 'SELECT ID,VerifiedBuild FROM criteria_tree ORDER BY ID'
    hotfix_lookup = 'SELECT TableHash,RecordID,Deleted FROM hotfix_data ORDER BY Id'
}
Export-Database WorldDatabaseInfo $world
Export-Database HotfixDatabaseInfo $hotfix
'Exported world and hotfix data in read-only transactions.'
