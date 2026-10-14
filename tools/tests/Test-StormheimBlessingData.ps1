param([switch]$Live)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$migration=Get-Content "$repo/sql/updates/world/2026_10_14_11_stormheim_blessing.sql" -Raw
$sql=''
if(!$Live){
 $tables=@{creature_template='entry IN(96257,96258,96175)';quest_template='ID IN(39597,38473)';conditions='SourceTypeOrReferenceId=15 AND SourceGroup=18672 AND SourceEntry=0';spell_script_names='spell_id=190905'}
 foreach($table in $tables.Keys){$sql+="CREATE TEMPORARY TABLE saved_$table AS SELECT * FROM $table WHERE $($tables[$table]); CREATE TEMPORARY TABLE $table AS SELECT * FROM saved_$table;`n"}
 $sql+="UPDATE creature_template SET npcflag=2,gossip_menu_id=0,AIName='SmartAI',ScriptName='' WHERE entry=96257; UPDATE creature_template SET npcflag=3,gossip_menu_id=18672,AIName='',ScriptName='' WHERE entry=96258; UPDATE quest_template SET RewardItem1=0,RewardAmount1=0 WHERE ID=39597; DELETE FROM conditions; DELETE FROM spell_script_names;`n$migration`n$migration`n"
}
$sql+=@'
SELECT 'ready-actors',COUNT(*) FROM creature_template WHERE entry IN(96257,96258) AND npcflag=3 AND gossip_menu_id=18672 AND AIName='' AND ScriptName='npc_stormheim_blessing';
SELECT 'native-ready-option',COUNT(*) FROM gossip_menu_option WHERE MenuID=18672 AND OptionID=0 AND OptionBroadcastTextID=97814 AND OptionNpc=0 AND ActionMenuID=0;
SELECT 'ready-condition',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=15 AND SourceGroup=18672 AND SourceEntry=0 AND ConditionTypeOrReference=9 AND ConditionValue1=39597 AND NegativeCondition=0;
SELECT 'aura-expiry-hook',COUNT(*) FROM spell_script_names WHERE spell_id=190905 AND ScriptName='aura_stormheim_blessing_finish';
SELECT 'native-objective',COUNT(*) FROM quest_objectives WHERE QuestID=39597 AND ID=279738 AND Type=0 AND StorageIndex=5 AND ObjectID=96285 AND Amount=1;
SELECT 'native-reward',COUNT(*) FROM quest_template WHERE ID=39597 AND RewardItem1=141398 AND RewardAmount1=1 AND RewardSpell=0 AND RewardArtifactXP=0;
SELECT 'no-instant-completion',COUNT(*) FROM quest_template WHERE ID=39597 AND StartScript=0 AND CompleteScript=0 AND QuestType=2;
SELECT 'reward-gates',COUNT(*) FROM quest_template_addon WHERE (ID=39597 AND PrevQuestID=39594 AND NextQuestID=38473) OR (ID=38473 AND PrevQuestID=39597);
SELECT 'handoff',COUNT(*) FROM creature_questender WHERE id=96254 AND quest=39597;
SELECT 'next-offer',COUNT(*) FROM creature_queststarter WHERE id=96254 AND quest=38473;
SELECT 'physical-source',COUNT(*) FROM creature WHERE guid=338528 AND id=96257 AND map=1220 AND PhaseId='6665';
SELECT 'retained-dialogue',COUNT(DISTINCT GroupID) FROM creature_text WHERE CreatureID=96257 AND GroupID BETWEEN 0 AND 3;
SELECT 'retained-smart-fallback',COUNT(*) FROM smart_scripts WHERE source_type=9 AND entryorguid=9625700 AND id=4 AND action_type=33 AND action_param1=96285;
'@
$expected=@("ready-actors`t2","native-ready-option`t1","ready-condition`t1","aura-expiry-hook`t1","native-objective`t1","native-reward`t1","no-instant-completion`t1","reward-gates`t2","handoff`t1","next-offer`t1","physical-source`t1","retained-dialogue`t4","retained-smart-fallback`t1")
if(!$Live){
 $sql+=@"
SELECT 'preserved-actor-data',COUNT(*) FROM creature_template a JOIN saved_creature_template b USING(entry) WHERE a.unit_flags<>b.unit_flags OR a.faction<>b.faction OR a.minlevel<>b.minlevel OR a.maxlevel<>b.maxlevel OR (a.entry=96175 AND (a.AIName<>b.AIName OR a.ScriptName<>b.ScriptName));
SELECT 'preserved-quest-data',COUNT(*) FROM quest_template a JOIN saved_quest_template b USING(ID) WHERE a.Flags<>b.Flags OR a.FlagsEx<>b.FlagsEx OR a.RewardNextQuest<>b.RewardNextQuest OR a.RewardSpell<>b.RewardSpell OR (a.ID=38473 AND (a.RewardItem1<>b.RewardItem1 OR a.RewardAmount1<>b.RewardAmount1));
UPDATE creature_template SET ScriptName='custom' WHERE entry=96258;
UPDATE creature_template SET ScriptName='',AIName='custom' WHERE entry=96257;
UPDATE quest_template SET RewardItem1=999,RewardAmount1=2 WHERE ID=39597;
DELETE FROM conditions;
INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment) VALUES(15,18672,0,0,0,9,0,12345,0,0,0,0,'','custom');
$migration
SELECT 'custom-script',COUNT(*) FROM creature_template WHERE entry=96258 AND ScriptName='custom';
SELECT 'custom-ai',COUNT(*) FROM creature_template WHERE entry=96257 AND AIName='custom' AND ScriptName='';
SELECT 'custom-reward',COUNT(*) FROM quest_template WHERE ID=39597 AND RewardItem1=999 AND RewardAmount1=2;
SELECT 'custom-condition',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=15 AND SourceGroup=18672 AND SourceEntry=0 AND ConditionValue1=12345;
"@
 $expected+=@("preserved-actor-data`t0","preserved-quest-data`t0","custom-script`t1","custom-ai`t1","custom-reward`t1","custom-condition`t1")
}
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$actual=$sql | & 'C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe' --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --skip-column-names;if($LASTEXITCODE){throw 'Blessing SQL failed'}}finally{$env:MYSQL_PWD=$previous}
if(($actual -join '|') -ne ($expected -join '|')){throw "Blessing SQL mismatch: $($actual -join '|')"}
'PASS: migration twice, native localized readiness/menu/condition, actor and expiry-hook bindings, native objective/item reward, physical handoff/next offer, preserved dialogue/SmartAI and custom/unrelated data.'
