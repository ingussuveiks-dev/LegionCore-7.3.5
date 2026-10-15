param([switch]$Live)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$migration=Get-Content "$repo/sql/updates/world/2026_10_15_19_stormheim_dragons.sql" -Raw
$sql=''
if(!$Live){
 $tables=@{creature_template='entry IN(92302,96465,92218)';creature='guid IN(146706156,146706157)';smart_scripts='source_type=0 AND entryorguid IN(92302,96465,92218,-146706156,-146706157)'}
 foreach($table in $tables.Keys){$sql+="CREATE TEMPORARY TABLE saved_$table AS SELECT * FROM $table WHERE $($tables[$table]); CREATE TEMPORARY TABLE $table AS SELECT * FROM saved_$table;`n"}
 $sql+="DELETE FROM smart_scripts WHERE entryorguid=-146706157 AND comment LIKE '%Cry Thunder38624%'; UPDATE creature_template SET ScriptName='',AIName=IF(entry=92302,'','SmartAI');`n$migration`n$migration`n"
}
$sql+=@'
SELECT 'ride',COUNT(*) FROM creature_template WHERE entry=92302 AND ScriptName='npc_stormheim_dragons' AND VehicleId=4133;
SELECT 'givers',COUNT(*) FROM creature_template WHERE entry IN(96465,92218) AND AIName='' AND ScriptName='npc_stormheim_dragons_giver';
SELECT 'source-vehicle',COUNT(*) FROM creature_template WHERE entry=96465 AND VehicleId=4367 AND npcflag=3 AND gossip_menu_id=18693;
SELECT 'spawns',COUNT(*) FROM creature WHERE (guid=146706156 AND id=96465 AND map=1220 AND ABS(position_x-1977.46)<0.01 AND ABS(position_y-2551.44)<0.01 AND ABS(position_z-489.014)<0.01) OR(guid=146706157 AND id=92218 AND map=1220 AND ABS(position_x-2042.73)<0.01 AND ABS(position_y-2662.03)<0.01 AND ABS(position_z-666.41)<0.01);
SELECT 'stages',COUNT(*) FROM quest_objectives WHERE QuestID=39652 AND Type=0 AND Amount=1 AND ((ID=279773 AND ObjectID=96466 AND StorageIndex=0 AND Flags=0) OR(ID=279774 AND ObjectID=96467 AND StorageIndex=1 AND Flags=2));
SELECT 'reward-next',COUNT(*) FROM quest_template WHERE ID=39652 AND RewardNextQuest=38624 AND RewardItem1=0 AND RewardAmount1=0;
SELECT 'alternative',COUNT(*) FROM quest_template_addon WHERE ID=39652 AND PrevQuestID=40568 AND NextQuestID=38624;
SELECT 'option',COUNT(*) FROM gossip_menu_option WHERE MenuID=18693 AND OptionID=0 AND OptionNpc=0 AND OptionBroadcastTextID=98282;
SELECT 'condition',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=15 AND SourceGroup=18693 AND SourceEntry=0 AND ConditionTypeOrReference=9 AND ConditionValue1=39652;
SELECT 'source-ai',COUNT(*) FROM smart_scripts WHERE entryorguid=96465 AND source_type=0 AND ((id=0 AND event_type=62 AND event_param1=18693 AND action_type=33 AND action_param1=96466 AND link=1) OR(id=1 AND event_type=61 AND action_type=62 AND action_param1=1220 AND ABS(target_z-676.92)<0.01));
SELECT 'summit-ai',COUNT(*) FROM smart_scripts WHERE entryorguid=92218 AND source_type=0 AND ((id=0 AND event_type=64 AND action_type=33 AND action_param1=96467) OR(id=1 AND event_type=19 AND event_param1=38624 AND action_type=62 AND action_param1=1220) OR(id=3 AND event_type=19 AND event_param1=43560 AND action_type=12 AND action_param1=110990));
SELECT 'next-relation',COUNT(*) FROM creature_queststarter WHERE id=92218 AND quest=38624;
SELECT 'hand-in',COUNT(*) FROM creature_questender WHERE id=92218 AND quest=39652;
'@
$expected=@("ride`t1","givers`t2","source-vehicle`t1","spawns`t2","stages`t2","reward-next`t1","alternative`t1","option`t1","condition`t1","source-ai`t2","summit-ai`t3","next-relation`t1","hand-in`t1")
if(!$Live){
 $sql+=@"
UPDATE creature_template SET ScriptName='',AIName=IF(entry=92302,'','SmartAI');
UPDATE creature_template SET ScriptName='custom' WHERE entry=92302;
$migration
SELECT 'custom-ride',COUNT(*) FROM creature_template WHERE entry=92302 AND ScriptName='custom';
SELECT 'source-skipped',COUNT(*) FROM creature_template WHERE entry=96465 AND ScriptName='' AND AIName='SmartAI';
UPDATE creature_template SET ScriptName='',AIName=IF(entry=92302,'','SmartAI');
UPDATE smart_scripts SET action_param1=96499 WHERE entryorguid=96465 AND id=0;
$migration
SELECT 'custom-action',COUNT(*) FROM smart_scripts WHERE entryorguid=96465 AND id=0 AND action_param1=96499;
SELECT 'action-binding-skipped',COUNT(*) FROM creature_template WHERE entry=96465 AND ScriptName='' AND AIName='SmartAI';
UPDATE smart_scripts SET action_param1=96466 WHERE entryorguid=96465 AND id=0;
UPDATE creature_template SET ScriptName='',AIName=IF(entry=92302,'','SmartAI');
CREATE TEMPORARY TABLE custom_ai AS SELECT * FROM smart_scripts WHERE source_type=0 AND ((entryorguid=96465 AND id=0) OR(entryorguid=92218 AND id=0));
UPDATE custom_ai SET entryorguid=IF(entryorguid=96465,-146706156,-146706157),id=42,comment='custom GUID';
INSERT INTO smart_scripts SELECT * FROM custom_ai;
$migration
SELECT 'guid-bindings-skipped',COUNT(*) FROM creature_template WHERE entry IN(96465,92218) AND ScriptName='' AND AIName='SmartAI';
SELECT 'guid-actions-preserved',COUNT(*) FROM smart_scripts WHERE entryorguid IN(-146706156,-146706157) AND id=42 AND comment='custom GUID';
SELECT 'fields-preserved',COUNT(*) FROM creature_template a JOIN saved_creature_template b USING(entry) WHERE a.VehicleId<>b.VehicleId OR a.npcflag<>b.npcflag OR a.gossip_menu_id<>b.gossip_menu_id OR a.unit_flags<>b.unit_flags;
"@
 $expected+=@("custom-ride`t1","source-skipped`t1","custom-action`t1","action-binding-skipped`t1","guid-bindings-skipped`t2","guid-actions-preserved`t2","fields-preserved`t0")
}
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$actual=$sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --skip-column-names;if($LASTEXITCODE){throw 'Dragons SQL failed'}}finally{$env:MYSQL_PWD=$previous}
if(($actual -join '|') -ne ($expected -join '|')){throw "Dragons SQL mismatch: $($actual -join '|')"}
if($Live){'PASS: installed bindings, quest/stages/next/rewards, vehicles/menu/conditions and original SmartAI lists.'}else{'PASS: migration twice and custom actor/action/GUID preservation; quest/stages/next/rewards, vehicles/menu/conditions and SmartAI retained.'}
