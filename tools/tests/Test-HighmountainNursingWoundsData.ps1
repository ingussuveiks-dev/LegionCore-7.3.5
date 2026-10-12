param([switch]$Live)
$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$migration=Get-Content "$repo/sql/updates/world/2026_10_12_02_highmountain_nursing_wounds.sql" -Raw
$native=Get-Content "$repo/docs/audits/highmountain-nursing-wounds-native-2026-10-12.json" -Raw | ConvertFrom-Json
$rows=($native.objectives | ForEach-Object {"SELECT $($_.ID) ID,$($_.QuestID) QuestID,$($_.Type) Type,$($_.StorageIndex) StorageIndex,$($_.ObjectID) ObjectID,$($_.Amount) Amount,$($_.Flags) Flags,$($_.Flags2) Flags2"}) -join ' UNION ALL '
$sql=''
if(!$Live){
 $sql=(@('quest_template_addon','spell_area','conditions') | ForEach-Object {"CREATE TEMPORARY TABLE saved_$_ LIKE $_; INSERT INTO saved_$_ SELECT * FROM $_; CREATE TEMPORARY TABLE $_ LIKE saved_$_; INSERT INTO $_ SELECT * FROM saved_$_;"}) -join "`n"
 $sql+="`n$migration`n$migration`n"
}
$sql+=@"
SELECT 'native-objectives',COUNT(*) FROM ($rows) a LEFT JOIN quest_objectives b USING(ID) WHERE b.ID IS NULL OR a.QuestID<>b.QuestID OR a.Type<>b.Type OR a.StorageIndex<>b.StorageIndex OR a.ObjectID<>b.ObjectID OR a.Amount<>b.Amount OR a.Flags<>b.Flags OR a.Flags2<>b.Flags2;
SELECT 'links',COUNT(*) FROM quest_template_addon WHERE (ID=38913 AND PrevQuestID=38909 AND NextQuestID=39318) OR (ID=39318 AND PrevQuestID=38913 AND NextQuestID=38910) OR (ID=38910 AND PrevQuestID=39318 AND NextQuestID=39321);
SELECT 'incoming',COUNT(*) FROM quest_template_addon WHERE NextQuestID=39318 AND ID<>38913;
SELECT 'branches',COUNT(*) FROM quest_template_addon WHERE (ID=40229 AND NextQuestID=39456) OR (ID=40515 AND PrevQuestID=38909 AND NextQuestID=40167);
SELECT 'totem-area',COUNT(*) FROM spell_area WHERE spell=189182 AND area=7726 AND quest_start=0 AND quest_end=39318 AND quest_end_status=10 AND autocast=0;
SELECT 'totem-no-bypass',COUNT(*) FROM spell_area WHERE spell=189182;
SELECT 'gossip',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=15 AND SourceGroup=18894 AND SourceEntry=0 AND ElseGroup=0 AND ConditionTypeOrReference=9 AND ConditionTarget=0 AND ConditionValue1=39318 AND NegativeCondition=0;
SELECT 'julan-actions',COUNT(*) FROM smart_scripts WHERE entryorguid=95403 AND source_type=0 AND ((id=0 AND event_type=62 AND event_param1=18894 AND event_param2=0 AND action_type=33 AND action_param1=95403 AND target_type=7 AND link=1) OR (id=1 AND event_type=61 AND action_type=85 AND action_param1=189182 AND target_type=7 AND link=2) OR (id=2 AND event_type=61 AND action_type=72));
SELECT 'fire-hit',COUNT(*) FROM smart_scripts WHERE entryorguid=95261 AND source_type=0 AND event_type=8 AND event_param1=189168 AND event_param3=10000 AND event_param4=10000 AND action_type=80 AND action_param1=9526100;
SELECT 'fire-actions',COUNT(*) FROM smart_scripts WHERE entryorguid=9526100 AND source_type=9 AND ((id=0 AND action_type=33 AND action_param1=95261 AND target_type=7) OR (id=1 AND action_type=47 AND action_param1=0) OR (id=2 AND event_param1=10000 AND event_param2=10000 AND action_type=47 AND action_param1=1));
SELECT 'fire-spawns',COUNT(*)>=8 FROM creature WHERE id=95261 AND map=1220 AND areaId=7726 AND phaseId='';
SELECT 'julan-spawn',COUNT(*) FROM creature WHERE guid=282501 AND id=95403 AND map=1220 AND areaId=7726 AND phaseId='';
SELECT 'lasan-spawn',COUNT(*) FROM creature WHERE guid=282571 AND id=93841 AND map=1220 AND areaId=7726 AND phaseId='';
SELECT 'fire-aura',COUNT(*) FROM creature_template_addon WHERE entry=95261 AND auras='188911';
SELECT 'ai',COUNT(*) FROM creature_template WHERE entry IN(95261,95403) AND AIName='SmartAI' AND ScriptName='';
SELECT 'starters',COUNT(*) FROM creature_queststarter WHERE id=93841 AND quest IN(39318,38910);
SELECT 'enders',COUNT(*) FROM creature_questender WHERE id=93841 AND quest IN(38913,39318);
SELECT 'enabled',COUNT(*) FROM disables WHERE sourceType=1 AND entry IN(38913,39318,38910);
"@
$expected=@("native-objectives`t0","links`t3","incoming`t0","branches`t2","totem-area`t1","totem-no-bypass`t1","gossip`t1","julan-actions`t3","fire-hit`t1","fire-actions`t3","fire-spawns`t1","julan-spawn`t1","lasan-spawn`t1","fire-aura`t1","ai`t2","starters`t2","enders`t2","enabled`t0")
if(!$Live){
 $sql+=@"
SELECT 'other-links',COUNT(*) FROM saved_quest_template_addon a JOIN quest_template_addon b USING(ID) WHERE (a.ID<>38913 AND a.NextQuestID<>b.NextQuestID) OR (a.ID<>39318 AND a.PrevQuestID<>b.PrevQuestID) OR a.ExclusiveGroup<>b.ExclusiveGroup;
SELECT 'other-area',COUNT(*) FROM saved_spell_area a LEFT JOIN spell_area b ON a.spell=b.spell AND a.area=b.area AND a.quest_start=b.quest_start AND a.quest_end=b.quest_end AND a.aura_spell=b.aura_spell AND a.racemask=b.racemask AND a.gender=b.gender WHERE a.spell<>189182 AND (b.spell IS NULL OR a.quest_end_status<>b.quest_end_status OR a.autocast<>b.autocast);
SELECT 'other-conditions',COUNT(*) FROM saved_conditions a LEFT JOIN conditions b ON a.SourceTypeOrReferenceId=b.SourceTypeOrReferenceId AND a.SourceGroup=b.SourceGroup AND a.SourceEntry=b.SourceEntry AND a.SourceId=b.SourceId AND a.ElseGroup=b.ElseGroup AND a.ConditionTypeOrReference=b.ConditionTypeOrReference AND a.ConditionTarget=b.ConditionTarget AND a.ConditionValue1=b.ConditionValue1 AND a.ConditionValue2=b.ConditionValue2 AND a.ConditionValue3=b.ConditionValue3 AND a.NegativeCondition=b.NegativeCondition WHERE NOT(a.SourceTypeOrReferenceId=15 AND a.SourceGroup=18894 AND a.SourceEntry=0) AND b.SourceTypeOrReferenceId IS NULL;
"@
 $expected+=@("other-links`t0","other-area`t0","other-conditions`t0")
}
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$actual=& 'C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe' --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --skip-column-names --execute=$sql;if($LASTEXITCODE){throw 'Nursing the Wounds SQL test failed'}}finally{$env:MYSQL_PWD=$previous}
if(($actual -join '|') -ne ($expected -join '|')){throw "Nursing the Wounds SQL mismatch: $($actual -join '|')"}
'PASS: native objectives, independent Skyhorn links, active-quest totem mask/gossip, Julan/fire SmartAI actions, fire recovery timer, spawns/turn-ins and repeated migration isolation.'
