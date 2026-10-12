param([switch]$Live)
$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$migration=Get-Content "$repo/sql/updates/world/2026_10_12_06_highmountain_skyhorn_ritual.sql" -Raw
$native=Get-Content "$repo/docs/audits/highmountain-skyhorn-ritual-native-2026-10-12.json" -Raw | ConvertFrom-Json
$rows=($native.objectives | ForEach-Object {"SELECT $($_.ID) ID,$($_.QuestID) QuestID,$($_.Type) Type,$($_.StorageIndex) StorageIndex,$($_.ObjectID) ObjectID,$($_.Amount) Amount,$($_.Flags) Flags,$($_.Flags2) Flags2"}) -join ' UNION ALL '
$sql=''
if(!$Live){
 $sql=(@('smart_scripts','creature_template','conditions','quest_objectives','creature_text') | ForEach-Object {"CREATE TEMPORARY TABLE saved_$_ LIKE $_; INSERT INTO saved_$_ SELECT * FROM $_; CREATE TEMPORARY TABLE $_ LIKE saved_$_; INSERT INTO $_ SELECT * FROM saved_$_;"}) -join "`n"
 $sql+="`n$migration`n$migration`n"
}
$sql+=@"
SELECT 'native-objectives',COUNT(*) FROM ($rows) a LEFT JOIN quest_objectives b USING(ID) WHERE b.ID IS NULL OR a.QuestID<>b.QuestID OR a.Type<>b.Type OR a.StorageIndex<>b.StorageIndex OR a.ObjectID<>b.ObjectID OR a.Amount<>b.Amount OR a.Flags<>b.Flags OR a.Flags2<>b.Flags2;
SELECT 'guide',COUNT(*) FROM creature_template WHERE entry=98773 AND AIName='' AND ScriptName='npc_skyhorn_ritual_guide' AND gossip_menu_id=18944;
SELECT 'no-old-ai',COUNT(*) FROM smart_scripts WHERE entryorguid=98773 AND source_type=0;
SELECT 'start',COUNT(*) FROM creature WHERE guid=14641573 AND id=98773 AND map=1220 AND phaseId='6626' AND ABS(position_x-4171.37)<0.02 AND ABS(position_y-4398.75)<0.02 AND ABS(position_z-660.976)<0.02;
SELECT 'mayla',COUNT(*) FROM creature WHERE guid=11349875 AND id=93826 AND map=1220 AND phaseId='' AND ABS(position_x-4083.05)<0.02 AND ABS(position_y-4385.5)<0.02 AND ABS(position_z-670.709)<0.02;
SELECT 'destination',COUNT(*) FROM waypoints WHERE entry=98773 AND pointid=12 AND ABS(position_x-4087.64)<0.02 AND ABS(position_y-4379.32)<0.02 AND ABS(position_z-670.626)<0.02;
SELECT 'phases',COUNT(*) FROM phase_definitions WHERE phaseId=6626 AND phasemask=0 AND ((zoneId=7503 AND entry=26) OR (zoneId=7731 AND entry=4));
SELECT 'quest-phase',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=23 AND ((SourceGroup=7503 AND SourceEntry=26) OR (SourceGroup=7731 AND SourceEntry=4)) AND ConditionValue1=39387 AND ConditionTarget=0 AND NegativeCondition=0 AND ((ElseGroup=0 AND ConditionTypeOrReference=9) OR (ElseGroup=1 AND ConditionTypeOrReference=28));
SELECT 'phase-total',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=23 AND ((SourceGroup=7503 AND SourceEntry=26) OR (SourceGroup=7731 AND SourceEntry=4));
SELECT 'gossip',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=15 AND SourceGroup=18944 AND SourceEntry=0 AND ConditionTypeOrReference=9 AND ConditionValue1=39387 AND NegativeCondition=0;
SELECT 'dialogue',COUNT(*) FROM creature_text WHERE CreatureID=98773 AND ((GroupID=0 AND BroadcastTextID=100850) OR (GroupID=1 AND BroadcastTextID=100852) OR (GroupID=2 AND BroadcastTextID=100853) OR (GroupID=3 AND BroadcastTextID=100851));
SELECT 'starter',COUNT(*) FROM creature_queststarter WHERE id=95410 AND quest=39387;
SELECT 'enders',COUNT(*) FROM creature_questender WHERE id IN(93826,108434) AND quest=39387;
SELECT 'previous',COUNT(*) FROM quest_template_addon WHERE ID=39387 AND PrevQuestID=39322;
SELECT 'reward-next',COUNT(*) FROM quest_template WHERE ID=39387 AND RewardNextQuest=0;
SELECT 'enabled',COUNT(*) FROM disables WHERE sourceType=1 AND entry=39387;
"@
$expected=@("native-objectives`t0","guide`t1","no-old-ai`t0","start`t1","mayla`t1","destination`t1","phases`t2","quest-phase`t4","phase-total`t4","gossip`t1","dialogue`t4","starter`t1","enders`t2","previous`t1","reward-next`t1","enabled`t0")
if(!$Live){
 $sql+=@"
SELECT 'other-ai',COUNT(*) FROM saved_creature_template a JOIN creature_template b USING(entry) WHERE a.entry<>98773 AND (a.AIName<>b.AIName OR a.ScriptName<>b.ScriptName);
SELECT 'other-smart',COUNT(*) FROM saved_smart_scripts a LEFT JOIN smart_scripts b USING(entryorguid,source_type,id,link) WHERE NOT(a.entryorguid=98773 AND a.source_type=0) AND (b.entryorguid IS NULL OR a.event_type<>b.event_type OR a.action_type<>b.action_type OR a.action_param1<>b.action_param1 OR a.target_type<>b.target_type);
SELECT 'other-objectives',COUNT(*) FROM saved_quest_objectives a JOIN quest_objectives b USING(ID) WHERE a.ID NOT IN(280508,280509) AND (a.Flags<>b.Flags OR a.Flags2<>b.Flags2 OR a.ObjectID<>b.ObjectID OR a.Amount<>b.Amount);
SELECT 'other-conditions',COUNT(*) FROM saved_conditions a LEFT JOIN conditions b USING(SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition) WHERE NOT(a.SourceTypeOrReferenceId=23 AND ((a.SourceGroup=7503 AND a.SourceEntry=26) OR (a.SourceGroup=7731 AND a.SourceEntry=4))) AND b.SourceGroup IS NULL;
"@
 $expected+=@("other-ai`t0","other-smart`t0","other-objectives`t0","other-conditions`t0")
}
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$actual=& 'C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe' --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --skip-column-names --execute=$sql;if($LASTEXITCODE){throw 'Skyhorn ritual SQL test failed'}}finally{$env:MYSQL_PWD=$previous}
if(($actual -join '|') -ne ($expected -join '|')){throw "Skyhorn ritual SQL mismatch: $($actual -join '|')"}
'PASS: native objectives/flags, owner ritual binding, retry phase, verified route endpoints/dialogue/enders, repeated migration and unrelated-row preservation.'
