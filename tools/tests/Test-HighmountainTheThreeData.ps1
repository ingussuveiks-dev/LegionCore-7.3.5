param([switch]$Live)
$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$migration=Get-Content "$repo/sql/updates/world/2026_10_12_04_highmountain_the_three.sql" -Raw
$native=Get-Content "$repo/docs/audits/highmountain-the-three-native-2026-10-12.json" -Raw | ConvertFrom-Json
$rows=($native.objectives | ForEach-Object {"SELECT $($_.ID) ID,$($_.QuestID) QuestID,$($_.Type) Type,$($_.StorageIndex) StorageIndex,$($_.ObjectID) ObjectID,$($_.Amount) Amount,$($_.Flags) Flags,$($_.Flags2) Flags2"}) -join ' UNION ALL '
$sql=''
if(!$Live){
 $sql=(@('smart_scripts','quest_template_addon','spell_dummy_trigger') | ForEach-Object {"CREATE TEMPORARY TABLE saved_$_ LIKE $_; INSERT INTO saved_$_ SELECT * FROM $_; CREATE TEMPORARY TABLE $_ LIKE saved_$_; INSERT INTO $_ SELECT * FROM saved_$_;"}) -join "`n"
 $sql+="`n$migration`n$migration`n"
}
$sql+=@"
SELECT 'native-objectives',COUNT(*) FROM ($rows) a LEFT JOIN quest_objectives b USING(ID) WHERE b.ID IS NULL OR a.QuestID<>b.QuestID OR a.Type<>b.Type OR a.StorageIndex<>b.StorageIndex OR a.ObjectID<>b.ObjectID OR a.Amount<>b.Amount OR a.Flags<>b.Flags OR a.Flags2<>b.Flags2;
SELECT 'parallel',COUNT(*) FROM quest_template_addon WHERE ID IN(39321,39429) AND PrevQuestID=38910 AND NextQuestID=39322 AND ExclusiveGroup=-39321;
SELECT 'gate',COUNT(*) FROM quest_template_addon WHERE ID=39322 AND PrevQuestID=39321 AND NextQuestID=39387;
SELECT 'incoming-bypass',COUNT(*) FROM quest_template_addon WHERE NextQuestID=39322 AND ID NOT IN(39321,39429);
SELECT 'dummy-links',COUNT(*) FROM spell_dummy_trigger WHERE ((spell_id=194241 AND spell_trigger=215588) OR (spell_id=215591 AND spell_trigger=215592)) AND ``option``=6 AND effectmask=1 AND handlemask=8 AND target=0 AND caster=0 AND bp0=0 AND bp1=0 AND bp2=0 AND aura=0;
SELECT 'no-duplicate-dummy',COUNT(*) FROM spell_dummy_trigger WHERE spell_id IN(194241,215591);
SELECT 'hex-model',COUNT(*) FROM creature_template_wdb WHERE Entry=97277 AND Displayid1=21950;
SELECT 'combat',COUNT(*) FROM smart_scripts WHERE source_type=0 AND id=1 AND event_type=0 AND event_chance=100 AND action_type=11 AND target_type=1 AND event_param1=3000 AND event_param2=3000 AND ((entryorguid=95194 AND action_param1=192111 AND event_param3=15000 AND event_param4=15000) OR (entryorguid=95195 AND action_param1=194228 AND event_param3=12000 AND event_param4=12000));
SELECT 'idle-auras',COUNT(*) FROM smart_scripts WHERE source_type=0 AND id=0 AND event_type=1 AND action_type=11 AND target_type=1 AND ((entryorguid=95194 AND action_param1=213273) OR (entryorguid=95195 AND action_param1=194245));
SELECT 'ai',COUNT(*) FROM creature_template WHERE faction=16 AND ((entry IN(95194,95195) AND AIName='SmartAI' AND ScriptName='') OR (entry=95196 AND AIName='' AND ScriptName=''));
SELECT 'scaling',COUNT(*) FROM creature_template_scaling WHERE Entry IN(95194,95195,95196) AND LevelScalingMin=98 AND LevelScalingMax=110;
SELECT 'targets',COUNT(*) FROM creature WHERE map=1220 AND phaseId='' AND ((guid=282902 AND id=95194) OR (guid=282919 AND id=95195) OR (guid=282860 AND id=95196));
SELECT 'ugla-spell',COUNT(*) FROM creature_template_spell WHERE entry=95196 AND spell=191955 AND Difficulties='';
SELECT 'aliases',COUNT(*) FROM creature_template_wdb WHERE Entry IN(95265,105068) AND KillCredit1=95266 AND KillCredit2=0;
SELECT 'direct',COUNT(*) FROM creature_template_wdb WHERE Entry IN(95194,95195,95196,95266,95277) AND KillCredit1=0 AND KillCredit2=0;
SELECT 'harpies',COUNT(*)>=8 FROM creature WHERE map=1220 AND areaId=7728 AND id IN(95265,95266) AND phaseId='';
SELECT 'loyalists',COUNT(*)>=6 FROM creature WHERE map=1220 AND areaId=7728 AND id=95277 AND phaseId='';
SELECT 'starters',COUNT(*) FROM creature_queststarter WHERE id=95410 AND quest IN(39321,39429,39322);
SELECT 'enders',COUNT(*) FROM creature_questender WHERE id=95410 AND quest IN(39321,39429,39322);
SELECT 'turn-in-actor',COUNT(*) FROM creature WHERE id=95410 AND guid=340578 AND map=1220 AND ABS(position_x-5238.11)<0.02 AND ABS(position_y-4209.42)<0.02 AND phaseId='5978 5975 4979';
SELECT 'turn-in-phase',COUNT(*) FROM phase_definitions WHERE zoneId=7503 AND entry=24 AND phaseId='5978 5975 4979';
SELECT 'phase-conditions',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=23 AND SourceGroup=7503 AND SourceEntry=24;
SELECT 'enabled',COUNT(*) FROM disables WHERE sourceType=1 AND entry IN(39321,39429,39322);
"@
$expected=@("native-objectives`t0","parallel`t2","gate`t1","incoming-bypass`t0","dummy-links`t2","no-duplicate-dummy`t2","hex-model`t1","combat`t2","idle-auras`t2","ai`t3","scaling`t3","targets`t3","ugla-spell`t1","aliases`t2","direct`t5","harpies`t1","loyalists`t1","starters`t3","enders`t3","turn-in-actor`t1","turn-in-phase`t1","phase-conditions`t0","enabled`t0")
if(!$Live){
 $sql+=@"
SELECT 'other-smart',COUNT(*) FROM saved_smart_scripts a LEFT JOIN smart_scripts b USING(entryorguid,source_type,id,link) WHERE NOT(a.entryorguid IN(95194,95195) AND a.source_type=0 AND a.id=1) AND (b.entryorguid IS NULL OR a.event_type<>b.event_type OR a.event_flags<>b.event_flags OR a.action_type<>b.action_type OR a.action_param1<>b.action_param1 OR a.target_type<>b.target_type);
SELECT 'other-dummy',COUNT(*) FROM saved_spell_dummy_trigger a LEFT JOIN spell_dummy_trigger b ON a.spell_id=b.spell_id AND a.spell_trigger=b.spell_trigger AND a.option=b.option AND a.aura=b.aura WHERE a.spell_id NOT IN(194241,215591) AND (b.spell_id IS NULL OR a.effectmask<>b.effectmask OR a.handlemask<>b.handlemask OR a.target<>b.target OR a.caster<>b.caster OR a.bp0<>b.bp0 OR a.bp1<>b.bp1 OR a.bp2<>b.bp2);
SELECT 'other-links',COUNT(*) FROM saved_quest_template_addon a JOIN quest_template_addon b USING(ID) WHERE a.ID NOT IN(39321,39429,39322) AND (a.PrevQuestID<>b.PrevQuestID OR a.NextQuestID<>b.NextQuestID OR a.ExclusiveGroup<>b.ExclusiveGroup);
"@
 $expected+=@("other-smart`t0","other-dummy`t0","other-links`t0")
}
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$actual=& 'C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe' --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --skip-column-names --execute=$sql;if($LASTEXITCODE){throw 'The Three SQL test failed'}}finally{$env:MYSQL_PWD=$previous}
if(($actual -join '|') -ne ($expected -join '|')){throw "The Three SQL mismatch: $($actual -join '|')"}
'PASS: native objectives/aliases, parallel Haglands pair and both-reward gate, native combat casts and retained idle auras, all targets/scaling, Ugla automatic spell, accessible turn-in and repeated migration isolation.'
