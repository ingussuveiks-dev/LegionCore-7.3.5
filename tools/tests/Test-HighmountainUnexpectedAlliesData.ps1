param([switch]$Live)
$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$migration=Get-Content "$repo/sql/updates/world/2026_10_12_01_highmountain_unexpected_allies.sql" -Raw
$native=Get-Content "$repo/docs/audits/highmountain-unexpected-allies-native-2026-10-12.json" -Raw | ConvertFrom-Json
$rows=($native.objectives | ForEach-Object {"SELECT $($_.ID) ID,$($_.QuestID) QuestID,$($_.Type) Type,$($_.StorageIndex) StorageIndex,$($_.ObjectID) ObjectID,$($_.Amount) Amount,$($_.Flags) Flags,$($_.Flags2) Flags2"}) -join ' UNION ALL '
$sql=''
if(!$Live){
 $sql=(@('smart_scripts','creature_template','conditions','phase_definitions','creature_text') | ForEach-Object {"CREATE TEMPORARY TABLE saved_$_ LIKE $_; INSERT INTO saved_$_ SELECT * FROM $_; CREATE TEMPORARY TABLE $_ LIKE saved_$_; INSERT INTO $_ SELECT * FROM saved_$_;"}) -join "`n"
 $sql+="`n$migration`n$migration`n"
}
$sql+=@"
SELECT 'native-objectives',COUNT(*) FROM ($rows) a LEFT JOIN quest_objectives b USING(ID) WHERE b.ID IS NULL OR a.QuestID<>b.QuestID OR a.Type<>b.Type OR a.StorageIndex<>b.StorageIndex OR a.ObjectID<>b.ObjectID OR a.Amount<>b.Amount OR a.Flags<>b.Flags OR a.Flags2<>b.Flags2;
SELECT 'step-objectives',COUNT(*) FROM quest_objectives WHERE QuestID=40229;
SELECT 'guide',COUNT(*) FROM creature_template WHERE entry=99619 AND AIName='' AND ScriptName='npc_unexpected_allies_guide';
SELECT 'ritual',COUNT(*) FROM creature_template WHERE entry=99624 AND AIName='' AND ScriptName='npc_unexpected_allies_ritual';
SELECT 'no-instant-credit',COUNT(*) FROM smart_scripts WHERE entryorguid=99619 AND source_type=0 AND action_type=33;
SELECT 'start-position',COUNT(*) FROM creature WHERE guid=340590 AND id=99619 AND map=1220 AND phaseId='6625' AND ABS(position_x-4123.89)<0.02 AND ABS(position_y-4475.31)<0.02 AND ABS(position_z-660.195)<0.02;
SELECT 'mayla-position',COUNT(*) FROM creature WHERE guid=11349875 AND id=93826 AND map=1220 AND phaseId='' AND ABS(position_x-4083.05)<0.02 AND ABS(position_y-4385.5)<0.02 AND ABS(position_z-670.709)<0.02;
SELECT 'models',COUNT(*) FROM creature_template_wdb WHERE Entry IN(99619,99624) AND Displayid1=69863;
SELECT 'native-phase',COUNT(*) FROM phase_definitions WHERE phaseId=6625 AND phasemask=0 AND ((zoneId=7503 AND entry=22) OR (zoneId=7731 AND entry=39456));
SELECT 'quest-phase',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=23 AND ((SourceGroup=7503 AND SourceEntry=22) OR (SourceGroup=7731 AND SourceEntry=39456)) AND ConditionValue1=39456 AND ConditionTarget=0 AND NegativeCondition=0 AND ((ElseGroup=0 AND ConditionTypeOrReference=9) OR (ElseGroup=1 AND ConditionTypeOrReference=28));
SELECT 'phase-no-bypass',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=23 AND ((SourceGroup=7503 AND SourceEntry=22) OR (SourceGroup=7731 AND SourceEntry=39456)) AND ElseGroup NOT IN(0,1);
SELECT 'gossip-condition',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=15 AND SourceGroup=18971 AND SourceEntry=0 AND ConditionTypeOrReference=9 AND ConditionValue1=39456 AND NegativeCondition=0;
SELECT 'native-dialogue',COUNT(*) FROM creature_text WHERE (CreatureID=99624 AND ((GroupID=0 AND BroadcastTextID=101325) OR (GroupID=1 AND BroadcastTextID=101326) OR (GroupID=2 AND BroadcastTextID=101327) OR (GroupID=3 AND BroadcastTextID=102109))) OR (CreatureID IN(93826,108434) AND ((GroupID=50 AND BroadcastTextID=102106) OR (GroupID=51 AND BroadcastTextID=102107) OR (GroupID=52 AND BroadcastTextID=102108)));
SELECT 'starters',COUNT(*) FROM creature_queststarter WHERE (id=96453 AND quest=40229) OR (id=95392 AND quest=39456);
SELECT 'enders',COUNT(*) FROM creature_questender WHERE (id=95392 AND quest=40229) OR (id IN(93826,108434) AND quest=39456);
SELECT 'grotto',COUNT(*) FROM creature WHERE guid=340565 AND id=95392 AND map=1220 AND phaseId='';
SELECT 'handoff-links',COUNT(*) FROM quest_template_addon WHERE (ID=40229 AND PrevQuestID=39426 AND NextQuestID=39456) OR (ID=39456 AND PrevQuestID=40229 AND NextQuestID=0);
SELECT 'native-end',COUNT(*) FROM quest_template WHERE (ID=40229 AND RewardNextQuest=39456) OR (ID=39456 AND RewardNextQuest=0);
SELECT 'enabled',COUNT(*) FROM disables WHERE sourceType=1 AND entry IN(40229,39456);
"@
$expected=@("native-objectives`t0","step-objectives`t0","guide`t1","ritual`t1","no-instant-credit`t0","start-position`t1","mayla-position`t1","models`t2","native-phase`t2","quest-phase`t4","phase-no-bypass`t0","gossip-condition`t1","native-dialogue`t10","starters`t2","enders`t3","grotto`t1","handoff-links`t2","native-end`t2","enabled`t0")
if(!$Live){
 $sql+=@"
SELECT 'other-ai',COUNT(*) FROM saved_creature_template a JOIN creature_template b USING(entry) WHERE a.entry NOT IN(99619,99624) AND (a.AIName<>b.AIName OR a.ScriptName<>b.ScriptName);
SELECT 'other-smart',COUNT(*) FROM saved_smart_scripts a LEFT JOIN smart_scripts b USING(entryorguid,source_type,id,link) WHERE NOT(a.entryorguid=99619 AND a.source_type=0 AND a.id IN(0,1,2)) AND (b.entryorguid IS NULL OR a.event_type<>b.event_type OR a.action_type<>b.action_type OR a.action_param1<>b.action_param1 OR a.target_type<>b.target_type);
SELECT 'other-text',COUNT(*) FROM saved_creature_text a LEFT JOIN creature_text b ON a.CreatureID=b.CreatureID AND a.GroupID=b.GroupID AND a.ID=b.ID AND a.BroadcastTextID=b.BroadcastTextID AND a.Text=b.Text WHERE NOT((a.CreatureID=99624 AND a.GroupID BETWEEN 0 AND 3) OR (a.CreatureID IN(93826,108434) AND a.GroupID BETWEEN 50 AND 52)) AND b.CreatureID IS NULL;
SELECT 'old-phases',COUNT(*) FROM saved_phase_definitions a LEFT JOIN phase_definitions b USING(zoneId,entry) WHERE NOT(a.zoneId=7731 AND a.entry=39456) AND (b.entry IS NULL OR a.phaseId<>b.phaseId OR a.flags<>b.flags OR a.phasemask<>b.phasemask);
"@
 $expected+=@("other-ai`t0","other-smart`t0","other-text`t0","old-phases`t0")
}
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$actual=& 'C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe' --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --skip-column-names --execute=$sql;if($LASTEXITCODE){throw 'Unexpected Allies SQL test failed'}}finally{$env:MYSQL_PWD=$previous}
if(($actual -join '|') -ne ($expected -join '|')){throw "Unexpected Allies SQL mismatch: $($actual -join '|')"}
'PASS: native objectives/models, no instant credits, both-zone quest phase, personal ritual binding/dialogue, actual route endpoints/turn-ins, branch links and repeated migration isolation.'
