param([switch]$Live)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$migration=Get-Content "$repo/sql/updates/world/2026_10_14_08_stormheim_might.sql" -Raw
$sql=''
if(!$Live){
 foreach($table in @('creature_template','gameobject_template')){
  $filter=if($table -eq 'creature_template'){'entry IN(96175,96176,35845,96257,96258)'}else{'entry IN(243817,243802)'}
  $sql+="CREATE TEMPORARY TABLE saved_$table AS SELECT * FROM $table WHERE $filter; CREATE TEMPORARY TABLE $table AS SELECT * FROM saved_$table;`n"
 }
 $sql+="UPDATE creature_template SET AIName='SmartAI',ScriptName='' WHERE entry IN(96175,96176,35845); UPDATE gameobject_template SET AIName='',ScriptName='' WHERE entry=243817;`n$migration`n$migration`n"
}
$sql+=@'
SELECT 'private-actors',COUNT(*) FROM creature_template WHERE entry IN(96175,96176,35845) AND AIName='' AND ScriptName='npc_stormheim_might_actor';
SELECT 'console-packet-guard',COUNT(*) FROM gameobject_template WHERE entry=243817 AND type=10 AND Data0=1691 AND Data1=39594 AND Data2=243817 AND Data10=0 AND AIName='' AND ScriptName='go_stormheim_might_console';
SELECT 'original-objectives',COUNT(*) FROM quest_objectives WHERE QuestID=39594 AND Type=0 AND Amount=1 AND ((ID=279739 AND StorageIndex=0 AND ObjectID=96290 AND Flags=0) OR(ID=279732 AND StorageIndex=5 AND ObjectID=96175 AND Flags=2));
SELECT 'no-auto-completion',COUNT(*) FROM quest_template WHERE ID=39594 AND QuestType=2 AND StartScript=0 AND CompleteScript=0;
SELECT 'reward-gates',COUNT(*) FROM quest_template_addon WHERE (ID=39594 AND PrevQuestID=39593 AND NextQuestID=39597) OR(ID=39597 AND PrevQuestID=39594);
SELECT 'active-scaling',COUNT(*) FROM creature_template_scaling WHERE Entry=96175 AND LevelScalingMin=98 AND LevelScalingMax=110;
SELECT 'native-console',COUNT(*) FROM gameobject WHERE guid=114842 AND id=243817 AND map=1220 AND ABS(position_x-3546.82)<0.01 AND ABS(position_y-2163.42)<0.01 AND ABS(position_z-231.647)<0.01;
SELECT 'reachable-hand-in',COUNT(*) FROM creature c JOIN creature_questender q ON c.id=q.id WHERE c.guid=338528 AND c.id=96257 AND c.PhaseId='6665' AND q.quest=39594;
SELECT 'next-offer',COUNT(*) FROM creature_queststarter WHERE id=96257 AND quest=39597;
SELECT 'retained-native-dialogue',COUNT(DISTINCT GroupID) FROM creature_text WHERE CreatureID=96175 AND GroupID BETWEEN 0 AND 4;
SELECT 'retained-surrender-fallback',COUNT(*) FROM smart_scripts WHERE entryorguid=96175 AND source_type=0 AND id=6 AND event_type=2 AND action_type=11 AND action_param1=68442;
SELECT 'shared-helper-preserved',COUNT(*) FROM smart_scripts WHERE entryorguid=35845 AND source_type=0 AND id=12 AND action_type=12 AND action_param1=96175;
'@
$expected=@("private-actors`t3","console-packet-guard`t1","original-objectives`t2","no-auto-completion`t1","reward-gates`t2","active-scaling`t1","native-console`t1","reachable-hand-in`t1","next-offer`t1","retained-native-dialogue`t5","retained-surrender-fallback`t1","shared-helper-preserved`t1")
if(!$Live){
 $sql+=@"
SELECT 'preserved-actor-data',COUNT(*) FROM creature_template a JOIN saved_creature_template b USING(entry) WHERE a.npcflag<>b.npcflag OR a.unit_flags<>b.unit_flags OR a.faction<>b.faction OR a.minlevel<>b.minlevel OR a.maxlevel<>b.maxlevel OR a.StateWorldEffectID<>b.StateWorldEffectID OR (a.entry IN(96257,96258) AND (a.AIName<>b.AIName OR a.ScriptName<>b.ScriptName));
SELECT 'preserved-console-data',COUNT(*) FROM gameobject_template a JOIN saved_gameobject_template b USING(entry) WHERE a.flags<>b.flags OR a.displayId<>b.displayId OR a.size<>b.size OR a.Data0<>b.Data0 OR a.Data1<>b.Data1 OR a.Data2<>b.Data2 OR a.Data3<>b.Data3 OR a.Data10<>b.Data10 OR (a.entry=243802 AND a.ScriptName<>b.ScriptName);
UPDATE creature_template SET ScriptName='custom' WHERE entry=35845;
UPDATE creature_template SET AIName='custom',ScriptName='' WHERE entry=96175;
UPDATE gameobject_template SET ScriptName='custom' WHERE entry=243817;
$migration
SELECT 'custom-actor-script',COUNT(*) FROM creature_template WHERE entry=35845 AND ScriptName='custom';
SELECT 'custom-actor-ai',COUNT(*) FROM creature_template WHERE entry=96175 AND AIName='custom' AND ScriptName='';
SELECT 'custom-console-script',COUNT(*) FROM gameobject_template WHERE entry=243817 AND ScriptName='custom';
UPDATE gameobject_template SET ScriptName='',AIName='custom' WHERE entry=243817;
$migration
SELECT 'custom-console-ai',COUNT(*) FROM gameobject_template WHERE entry=243817 AND AIName='custom' AND ScriptName='';
UPDATE gameobject_template SET AIName='',Data1=999 WHERE entry=243817;
$migration
SELECT 'custom-console-quest',COUNT(*) FROM gameobject_template WHERE entry=243817 AND Data1=999 AND ScriptName='';
"@
 $expected+=@("preserved-actor-data`t0","preserved-console-data`t0","custom-actor-script`t1","custom-actor-ai`t1","custom-console-script`t1","custom-console-ai`t1","custom-console-quest`t1")
}
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$actual=$sql | & 'C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe' --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --skip-column-names;if($LASTEXITCODE){throw 'Might SQL failed'}}finally{$env:MYSQL_PWD=$previous}
if(($actual -join '|') -ne ($expected -join '|')){throw "Might SQL mismatch: $($actual -join '|')"}
'PASS: migration twice, private packet/actor bindings, preserved real objective slots/sequencing/scaling/dialogue/SmartAI, reward hand-in/next offer and custom/unrelated guards.'
