param([switch]$Live)
$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$migration=Get-Content "$repo/sql/updates/world/2026_10_12_03_highmountain_rocs_eagles.sql" -Raw
$native=Get-Content "$repo/docs/audits/highmountain-rocs-eagles-native-2026-10-12.json" -Raw | ConvertFrom-Json
$rows=($native.objectives | ForEach-Object {"SELECT $($_.ID) ID,$($_.QuestID) QuestID,$($_.Type) Type,$($_.StorageIndex) StorageIndex,$($_.ObjectID) ObjectID,$($_.Amount) Amount,$($_.Flags) Flags,$($_.Flags2) Flags2"}) -join ' UNION ALL '
$sql=''
if(!$Live){
 $sql=(@('smart_scripts','conditions') | ForEach-Object {"CREATE TEMPORARY TABLE saved_$_ LIKE $_; INSERT INTO saved_$_ SELECT * FROM $_; CREATE TEMPORARY TABLE $_ LIKE saved_$_; INSERT INTO $_ SELECT * FROM saved_$_;"}) -join "`n"
 $sql+="`n$migration`n$migration`n"
}
$sql+=@"
SELECT 'native-objectives',COUNT(*) FROM ($rows) a LEFT JOIN quest_objectives b USING(ID) WHERE b.ID IS NULL OR a.QuestID<>b.QuestID OR a.Type<>b.Type OR a.StorageIndex<>b.StorageIndex OR a.ObjectID<>b.ObjectID OR a.Amount<>b.Amount OR a.Flags<>b.Flags OR a.Flags2<>b.Flags2;
SELECT 'link',COUNT(*) FROM quest_template_addon WHERE ID=38910 AND PrevQuestID=39318 AND NextQuestID=39321;
SELECT 'release-event',COUNT(*) FROM smart_scripts WHERE entryorguid=94991 AND source_type=0 AND id=0 AND event_type=8 AND event_param1=188432 AND event_flags=1 AND link=2 AND action_type=80 AND action_param1=9499100 AND action_param2=2 AND target_type=1;
SELECT 'release-links',COUNT(*) FROM smart_scripts WHERE entryorguid=94991 AND source_type=0 AND ((id=2 AND link=3 AND event_type=61 AND action_type=28 AND action_param1=188434 AND target_type=1) OR (id=3 AND link=0 AND event_type=61 AND action_type=83 AND action_param1=16777216 AND target_type=1));
SELECT 'reset',COUNT(*) FROM smart_scripts WHERE entryorguid=94991 AND source_type=0 AND ((id=1 AND event_type=25 AND link=4 AND action_type=11 AND action_param1=188434) OR (id=4 AND event_type=61 AND action_type=82 AND action_param1=16777216 AND target_type=1));
SELECT 'timed-list',COUNT(*) FROM smart_scripts WHERE entryorguid=9499100 AND source_type=9 AND ((id=0 AND event_param1=0 AND action_type=33 AND action_param1=94991 AND target_type=7) OR (id=1 AND event_param1=1000 AND event_param2=1000 AND action_type=202 AND action_param1=0 AND action_param2=0 AND action_param3=15 AND action_param4=0 AND target_type=1) OR (id=2 AND action_type=41 AND action_param1=5000 AND target_type=1));
SELECT 'click',COUNT(*) FROM npc_spellclick_spells WHERE npc_entry=94991 AND spell_id=188432 AND cast_flags=1 AND add_npc_flag=1;
SELECT 'quest-guards',COUNT(*) FROM conditions WHERE ConditionTypeOrReference=9 AND ConditionTarget=0 AND ConditionValue1=38910 AND NegativeCondition=0 AND ElseGroup=0 AND SourceId=0 AND ((SourceTypeOrReferenceId=18 AND SourceGroup=94991 AND SourceEntry=188432) OR (SourceTypeOrReferenceId=22 AND SourceGroup=1 AND SourceEntry=94991));
SELECT 'spawns',COUNT(*) FROM creature WHERE id=94991 AND map=1220 AND areaId=7755 AND phaseId='' AND spawntimesecs=180;
SELECT 'ai',COUNT(*) FROM creature_template WHERE entry=94991 AND AIName='SmartAI' AND ScriptName='' AND (npcflag & 16777216)<>0;
SELECT 'aura',COUNT(*) FROM creature_template_addon WHERE entry=94991 AND auras='188434';
SELECT 'starter',COUNT(*) FROM creature_queststarter WHERE id=93841 AND quest=38910;
SELECT 'ender',COUNT(*) FROM creature_questender WHERE id=95410 AND quest=38910;
SELECT 'handoff-actor',COUNT(*) FROM creature WHERE id=95410 AND guid=340573 AND map=1220 AND ABS(position_x-5246.3)<0.02 AND ABS(position_y-3989.95)<0.02 AND phaseId='5978 5975 4983 4978';
SELECT 'handoff-phase',COUNT(*) FROM phase_definitions WHERE zoneId=7503 AND entry=23 AND phaseId='5978 5975 4983 4978';
SELECT 'handoff-phase-unconditional',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=23 AND SourceGroup=7503 AND SourceEntry=23;
SELECT 'next-starter',COUNT(*) FROM creature_queststarter WHERE id=95410 AND quest=39321;
SELECT 'enabled',COUNT(*) FROM disables WHERE sourceType=1 AND entry=38910;
"@
$expected=@("native-objectives`t0","link`t1","release-event`t1","release-links`t2","reset`t2","timed-list`t3","click`t1","quest-guards`t2","spawns`t13","ai`t1","aura`t1","starter`t1","ender`t1","handoff-actor`t1","handoff-phase`t1","handoff-phase-unconditional`t0","next-starter`t1","enabled`t0")
if(!$Live){
 $sql+=@"
SELECT 'other-smart',COUNT(*) FROM saved_smart_scripts a LEFT JOIN smart_scripts b USING(entryorguid,source_type,id,link) WHERE NOT((a.entryorguid=94991 AND a.source_type=0) OR (a.entryorguid=9499100 AND a.source_type=9)) AND (b.entryorguid IS NULL OR a.event_flags<>b.event_flags OR a.action_param1<>b.action_param1 OR a.action_param3<>b.action_param3);
SELECT 'other-conditions',COUNT(*) FROM saved_conditions a LEFT JOIN conditions b ON a.SourceTypeOrReferenceId=b.SourceTypeOrReferenceId AND a.SourceGroup=b.SourceGroup AND a.SourceEntry=b.SourceEntry AND a.SourceId=b.SourceId AND a.ElseGroup=b.ElseGroup AND a.ConditionTypeOrReference=b.ConditionTypeOrReference AND a.ConditionTarget=b.ConditionTarget AND a.ConditionValue1=b.ConditionValue1 AND a.ConditionValue2=b.ConditionValue2 AND a.ConditionValue3=b.ConditionValue3 AND a.NegativeCondition=b.NegativeCondition WHERE NOT((a.SourceTypeOrReferenceId=18 AND a.SourceGroup=94991 AND a.SourceEntry=188432) OR (a.SourceTypeOrReferenceId=22 AND a.SourceGroup=1 AND a.SourceEntry=94991 AND a.SourceId=0)) AND b.SourceTypeOrReferenceId IS NULL;
"@
 $expected+=@("other-smart`t0","other-conditions`t0")
}
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$actual=& 'C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe' --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --skip-column-names --execute=$sql;if($LASTEXITCODE){throw 'Rocs vs Eagles SQL test failed'}}finally{$env:MYSQL_PWD=$previous}
if(($actual -join '|') -ne ($expected -join '|')){throw "Rocs vs Eagles SQL mismatch: $($actual -join '|')"}
'PASS: native seven-eagle objective, single release/unclickable/untrapped/reset sequence, vertical departure, quest guards, 13 respawning captives, accessible turn-in and repeated migration isolation.'
