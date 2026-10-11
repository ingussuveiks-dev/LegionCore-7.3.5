param([switch]$Live)
$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$migration=Get-Content "$repo/sql/updates/world/2026_10_11_01_highmountain_dargrul.sql" -Raw
$native=Get-Content "$repo/docs/audits/highmountain-dargrul-native-2026-10-11.json" -Raw | ConvertFrom-Json
$rows=($native.objectives | ForEach-Object {"SELECT $($_.ID) ID,$($_.QuestID) QuestID,$($_.Type) Type,$($_.ObjectID) ObjectID,$($_.Amount) Amount,$($_.Flags) Flags,$($_.Flags2) Flags2"}) -join ' UNION ALL '
$sql=''
if(!$Live){
 $sql=(@('creature_template','smart_scripts','quest_objectives','conditions','eventobject_template','spell_script_names') | ForEach-Object {"CREATE TEMPORARY TABLE saved_$_ LIKE $_; INSERT INTO saved_$_ SELECT * FROM $_; CREATE TEMPORARY TABLE $_ LIKE saved_$_; INSERT INTO $_ SELECT * FROM saved_$_;"}) -join "`n"
 $sql+="`n$migration`n$migration`n"
}
$sql+=@"
SELECT 'native-objectives',COUNT(*) FROM ($rows) a LEFT JOIN quest_objectives b USING(ID) WHERE b.ID IS NULL OR a.QuestID<>b.QuestID OR a.Type<>b.Type OR a.ObjectID<>b.ObjectID OR a.Amount<>b.Amount OR a.Flags<>b.Flags OR a.Flags2<>b.Flags2;
SELECT 'save-slots',COUNT(*) FROM quest_objectives WHERE QuestID=39027 AND ((ObjectID=97871 AND StorageIndex=0) OR (ObjectID=94610 AND StorageIndex=1) OR (ObjectID=97781 AND StorageIndex=2) OR (ObjectID=93805 AND StorageIndex=3));
SELECT 'ai',COUNT(*) FROM creature_template WHERE AIName='' AND ((entry=97552 AND ScriptName='npc_dargrul_escort') OR (entry=97781 AND ScriptName='npc_dargrul_grasp' AND VehicleId=4404));
SELECT 'old-events',COUNT(*) FROM smart_scripts WHERE (entryorguid=97552 AND source_type=0) OR (entryorguid=604 AND source_type=13) OR (entryorguid=97553 AND source_type=0 AND event_type=19 AND event_param1=39027 AND action_type=85 AND action_param1=205051);
SELECT 'old-event-binding',COUNT(*) FROM eventobject_template WHERE entry=604 AND ScriptName='';
SELECT 'escape-spell',COUNT(*) FROM spell_script_names WHERE spell_id=193017 AND ScriptName='spell_dargrul_escape';
SELECT 'click',COUNT(*) FROM npc_spellclick_spells WHERE npc_entry=97781 AND spell_id=193017 AND cast_flags=1;
SELECT 'exit-event',COUNT(*) FROM smart_scripts WHERE entryorguid=543 AND source_type=13 AND id=1 AND event_type=89 AND action_type=33 AND action_param1=93805 AND target_type=7;
SELECT 'exit-gates',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=2 AND SourceEntry=543 AND SourceId=13 AND ConditionTarget=0 AND ElseGroup=0 AND ConditionValue1=39027 AND ((ConditionTypeOrReference=9 AND NegativeCondition=0) OR (ConditionTypeOrReference=41 AND ConditionValue2 IN(97871,94610,97781) AND ConditionValue3=1 AND NegativeCondition=0) OR (ConditionTypeOrReference=41 AND ConditionValue2=93805 AND ConditionValue3=1 AND NegativeCondition=1));
SELECT 'other-quest-scene',COUNT(*) FROM smart_scripts WHERE entryorguid=543 AND source_type=13 AND id=0 AND event_type=89 AND action_type=206 AND action_param1=6409;
SELECT 'other-quest-gate',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=1 AND SourceEntry=543 AND SourceId=13 AND ConditionTypeOrReference=9 AND ConditionValue1=41764;
SELECT 'wrong-source',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry IN(543,604) AND SourceId=10;
SELECT 'phases',COUNT(*) FROM phase_definitions WHERE zoneId=7503 AND ((entry=80 AND phaseId='5040') OR (entry=81 AND phaseId='5041') OR (entry=82 AND phaseId='5042'));
SELECT 'chain',COUNT(*) FROM quest_template_addon WHERE (ID=39027 AND PrevQuestID=39043 AND NextQuestID=38909) OR (ID=38909 AND PrevQuestID=39027 AND NextQuestID=38912);
SELECT 'endpoints', (SELECT COUNT(*) FROM creature_queststarter WHERE id=97553 AND quest=39027),(SELECT COUNT(*) FROM creature_questender WHERE id=93805 AND quest=39027);
SELECT 'actor-timelines',COUNT(*) FROM smart_scripts WHERE source_type=9 AND entryorguid IN(94610,97827);
"@
$expected=@("native-objectives`t0","save-slots`t4","ai`t2","old-events`t0","old-event-binding`t1","escape-spell`t1","click`t1","exit-event`t1","exit-gates`t5","other-quest-scene`t1","other-quest-gate`t1","wrong-source`t0","phases`t3","chain`t2","endpoints`t1`t1","actor-timelines`t9")
if(!$Live){
 $sql+=@"
SELECT 'unrelated-objectives',COUNT(*) FROM saved_quest_objectives a LEFT JOIN quest_objectives b USING(ID) WHERE a.ID NOT IN(280130,280971) AND (b.ID IS NULL OR a.StorageIndex<>b.StorageIndex OR a.ObjectID<>b.ObjectID OR a.Amount<>b.Amount OR a.Flags<>b.Flags OR a.Flags2<>b.Flags2);
SELECT 'unrelated-ai',COUNT(*) FROM saved_creature_template a JOIN creature_template b USING(entry) WHERE a.entry NOT IN(97552,97781) AND (a.AIName<>b.AIName OR a.ScriptName<>b.ScriptName);
SELECT 'unrelated-smart',COUNT(*) FROM saved_smart_scripts a LEFT JOIN smart_scripts b ON a.entryorguid=b.entryorguid AND a.source_type=b.source_type AND a.id=b.id WHERE b.id IS NULL AND NOT ((a.entryorguid=97552 AND a.source_type=0) OR (a.entryorguid=604 AND a.source_type=13) OR (a.entryorguid=97553 AND a.source_type=0 AND a.event_type=19 AND a.event_param1=39027 AND a.action_type=85 AND a.action_param1=205051));
SELECT 'unrelated-event',COUNT(*) FROM saved_eventobject_template a JOIN eventobject_template b USING(entry) WHERE a.entry<>604 AND a.ScriptName<>b.ScriptName;
SELECT 'unrelated-conditions',COUNT(*) FROM saved_conditions a LEFT JOIN conditions b ON a.SourceTypeOrReferenceId=b.SourceTypeOrReferenceId AND a.SourceGroup=b.SourceGroup AND a.SourceEntry=b.SourceEntry AND a.SourceId=b.SourceId AND a.ElseGroup=b.ElseGroup AND a.ConditionTypeOrReference=b.ConditionTypeOrReference AND a.ConditionTarget=b.ConditionTarget AND a.ConditionValue1=b.ConditionValue1 AND a.ConditionValue2=b.ConditionValue2 AND a.ConditionValue3=b.ConditionValue3 AND a.NegativeCondition=b.NegativeCondition WHERE b.SourceEntry IS NULL AND NOT(a.SourceTypeOrReferenceId=22 AND ((a.SourceEntry=604 AND a.SourceId IN(10,13)) OR (a.SourceEntry=543 AND a.SourceGroup=1 AND a.SourceId=10 AND a.ConditionValue1=41764) OR (a.SourceEntry=543 AND a.SourceGroup=2 AND a.SourceId=13)));
"@
 $expected+=@("unrelated-objectives`t0","unrelated-ai`t0","unrelated-smart`t0","unrelated-event`t0","unrelated-conditions`t0")
}
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$actual=& 'C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe' --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --skip-column-names --execute=$sql;if($LASTEXITCODE){throw 'Dargrul SQL test failed'}}finally{$env:MYSQL_PWD=$previous}
if(($actual -join '|') -ne ($expected -join '|')){throw "Dargrul SQL mismatch: $($actual -join '|')"}
'PASS: native objectives/retained save slots, repeatable migration, owner script/vehicle/spell bindings, sequential exit gates, retained 41764 scene/actor timelines/phases/chain and unrelated data.'
