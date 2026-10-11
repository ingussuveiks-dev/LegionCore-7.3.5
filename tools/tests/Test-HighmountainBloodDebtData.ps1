param([switch]$Live)
$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$migration=Get-Content "$repo/sql/updates/world/2026_10_12_00_highmountain_blood_debt.sql" -Raw
$sql=''
if(!$Live){
 $sql=(@('smart_scripts','creature_template','quest_template_addon','conditions') | ForEach-Object {"CREATE TEMPORARY TABLE saved_$_ LIKE $_; INSERT INTO saved_$_ SELECT * FROM $_; CREATE TEMPORARY TABLE $_ LIKE saved_$_; INSERT INTO $_ SELECT * FROM saved_$_;"}) -join "`n"
 $sql+="`n$migration`n$migration`n"
}
$sql+=@"
SELECT 'objective',COUNT(*) FROM quest_objectives WHERE ID=279393 AND QuestID=39426 AND Type=0 AND ObjectID=95707 AND Amount=1 AND StorageIndex=1 AND Flags=1 AND Flags2=0;
SELECT 'boss-smart',COUNT(*) FROM creature_template WHERE entry=95707 AND AIName='SmartAI' AND ScriptName='';
SELECT 'combat',COUNT(*) FROM smart_scripts WHERE entryorguid=95707 AND source_type=0 AND event_type=0 AND event_chance=100 AND event_flags=0 AND action_type=11 AND action_param2=0 AND ((id=1 AND action_param1=197427 AND target_type=2 AND event_param1=2000 AND event_param3=8000) OR (id=2 AND action_param1=197450 AND target_type=1 AND event_param1=8000 AND event_param3=15000) OR (id=3 AND action_param1=197500 AND target_type=1 AND event_param1=15000 AND event_param3=45000) OR (id=4 AND action_param1=197520 AND target_type=1 AND event_param1=20000 AND event_param3=20000));
SELECT 'death-notification',COUNT(*) FROM smart_scripts WHERE entryorguid=95707 AND source_type=0 AND id=0 AND event_type=6 AND action_type=45 AND action_param1=0 AND action_param2=1 AND target_type=11 AND target_param1=96453;
SELECT 'companion-binding',COUNT(*) FROM creature_template WHERE entry=96453 AND AIName='' AND ScriptName='npc_blood_debt_navarrogg';
SELECT 'personal-follow',COUNT(*) FROM smart_scripts WHERE entryorguid=96453 AND source_type=0 AND event_type=54 AND target_type=23 AND action_type IN(207,29);
SELECT 'valid-response',COUNT(*) FROM smart_scripts s JOIN creature_text t ON t.CreatureID=s.entryorguid AND t.GroupID=s.action_param1 WHERE s.entryorguid=96453 AND s.source_type=0 AND s.event_type=38 AND s.action_type=1;
SELECT 'invalid-response',COUNT(*) FROM smart_scripts s LEFT JOIN creature_text t ON t.CreatureID=s.entryorguid AND t.GroupID=s.action_param1 WHERE s.entryorguid=96453 AND s.source_type=0 AND s.event_type=38 AND s.action_type=1 AND t.CreatureID IS NULL;
SELECT 'handoff-actions',COUNT(*) FROM smart_scripts WHERE entryorguid=96453 AND source_type=0 AND ((id=3 AND event_type=19 AND event_param1=40229 AND action_type=11 AND action_param1=195048 AND link=4) OR (id=4 AND event_type=61 AND action_type=11 AND action_param1=195056 AND link=5) OR (id=5 AND event_type=61 AND action_type=41));
SELECT 'companion-ender',COUNT(*) FROM creature_questender WHERE id=96453 AND quest=39426;
SELECT 'handoff-starter',COUNT(*) FROM creature_queststarter WHERE id=96453 AND quest=40229;
SELECT 'grotto-ender',COUNT(*) FROM creature_questender WHERE id=95392 AND quest=40229;
SELECT 'grotto-spawn',COUNT(*) FROM creature WHERE guid=340565 AND id=95392 AND map=1220 AND phaseId='';
SELECT 'boss-spawn',COUNT(*) FROM creature WHERE guid=283112 AND id=95707 AND map=1220 AND phaseId='';
SELECT 'unexpected-allies',COUNT(*) FROM quest_template_addon WHERE ID=40229 AND PrevQuestID=39426 AND NextQuestID=39456;
SELECT 'reward-required',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=19 AND SourceGroup=0 AND SourceEntry=40229 AND SourceId=0 AND ElseGroup=0 AND ConditionTypeOrReference=8 AND ConditionTarget=0 AND ConditionValue1=39426 AND NegativeCondition=0;
SELECT 'no-condition-bypass',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=19 AND SourceEntry=40229 AND ElseGroup<>0;
SELECT 'enabled',COUNT(*) FROM disables WHERE sourceType=1 AND entry IN(39426,40229);
"@
$expected=@("objective`t1","boss-smart`t1","combat`t4","death-notification`t1","companion-binding`t1","personal-follow`t2","valid-response`t1","invalid-response`t0","handoff-actions`t3","companion-ender`t1","handoff-starter`t1","grotto-ender`t1","grotto-spawn`t1","boss-spawn`t1","unexpected-allies`t1","reward-required`t1","no-condition-bypass`t0","enabled`t0")
if(!$Live){
 $sql+=@"
SELECT 'other-ai',COUNT(*) FROM saved_creature_template a JOIN creature_template b USING(entry) WHERE a.entry<>96453 AND (a.AIName<>b.AIName OR a.ScriptName<>b.ScriptName);
SELECT 'other-links',COUNT(*) FROM saved_quest_template_addon a JOIN quest_template_addon b USING(ID) WHERE (a.ID<>40229 AND a.NextQuestID<>b.NextQuestID) OR a.PrevQuestID<>b.PrevQuestID OR a.ExclusiveGroup<>b.ExclusiveGroup;
SELECT 'old-smart-preserved',COUNT(*) FROM saved_smart_scripts a LEFT JOIN smart_scripts b USING(entryorguid,source_type,id,link) WHERE NOT(a.entryorguid=96453 AND a.source_type=0 AND a.id=2) AND (b.entryorguid IS NULL OR a.event_type<>b.event_type OR a.action_type<>b.action_type OR a.action_param1<>b.action_param1 OR a.target_type<>b.target_type);
"@
 $expected+=@("other-ai`t0","other-links`t0","old-smart-preserved`t0")
}
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$actual=& 'C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe' --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --skip-column-names --execute=$sql;if($LASTEXITCODE){throw 'Blood Debt SQL test failed'}}finally{$env:MYSQL_PWD=$previous}
if(($actual -join '|') -ne ($expected -join '|')){throw "Blood Debt SQL mismatch: $($actual -join '|')"}
'PASS: four combat casts, native objective, valid completion dialogue, companion and grotto handoff, corrected next link, reward gate and repeated migration isolation.'
