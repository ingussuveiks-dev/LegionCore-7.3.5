param([switch]$Live)
$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$migration=Get-Content "$repo/sql/updates/world/2026_10_12_05_highmountain_witchqueen.sql" -Raw
$sql=''
if(!$Live){
 $sql=(@('creature_template','smart_scripts','spell_dummy_trigger','areatrigger_actions','areatrigger_scripts') | ForEach-Object {"CREATE TEMPORARY TABLE saved_$_ LIKE $_; INSERT INTO saved_$_ SELECT * FROM $_; CREATE TEMPORARY TABLE $_ LIKE saved_$_; INSERT INTO $_ SELECT * FROM saved_$_;"}) -join "`n"
 $sql+="`n$migration`n$migration`n"
}
$sql+=@"
SELECT 'objective',COUNT(*) FROM quest_objectives WHERE ID=279206 AND QuestID=39322 AND Type=0 AND StorageIndex=0 AND ObjectID=95153 AND Amount=1 AND Flags=1 AND Flags2=0;
SELECT 'gate',COUNT(*) FROM quest_template_addon WHERE (ID=39322 AND PrevQuestID=39321 AND NextQuestID=39387) OR (ID=39387 AND PrevQuestID=39322);
SELECT 'parallel',COUNT(*) FROM quest_template_addon WHERE ID IN(39321,39429) AND ExclusiveGroup=-39321 AND NextQuestID=39322;
SELECT 'reward-next',COUNT(*) FROM quest_template WHERE ID=39322 AND RewardNextQuest=39387;
SELECT 'starters',COUNT(*) FROM creature_queststarter WHERE id=95410 AND quest IN(39322,39387);
SELECT 'ender',COUNT(*) FROM creature_questender WHERE id=95410 AND quest=39322;
SELECT 'ai',COUNT(*) FROM creature_template WHERE entry=95153 AND AIName='SmartAI' AND ScriptName='' AND faction=16 AND unit_flags=0;
SELECT 'scaling',COUNT(*) FROM creature_template_scaling WHERE Entry=95153 AND LevelScalingMin=98 AND LevelScalingMax=110;
SELECT 'target',COUNT(*) FROM creature WHERE guid=282952 AND id=95153 AND map=1220 AND phaseId='' AND ABS(position_x-5079.16)<0.02 AND ABS(position_y-4411.78)<0.02;
SELECT 'direct-credit',COUNT(*) FROM creature_template_wdb WHERE Entry=95153 AND KillCredit1=0 AND KillCredit2=0;
SELECT 'turn-in',COUNT(*) FROM creature WHERE guid=340584 AND id=95410 AND map=1220 AND phaseId='5978 5975 4980';
SELECT 'turn-in-phase',COUNT(*) FROM phase_definitions WHERE zoneId=7503 AND entry=25 AND phaseId='5978 5975 4980';
SELECT 'phase-conditions',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=23 AND SourceGroup=7503 AND SourceEntry=25;
SELECT 'enabled',COUNT(*) FROM disables WHERE sourceType=1 AND entry IN(39322,39387);
SELECT 'combat',COUNT(*) FROM smart_scripts WHERE entryorguid=95153 AND source_type=0 AND event_type=0 AND event_chance=100 AND action_type=11 AND event_flags=0 AND ((id=0 AND action_param1=192173 AND target_type=1 AND event_param1=4000 AND event_param3=15000) OR (id=1 AND action_param1=192177 AND target_type=1 AND event_param1=8000 AND event_param3=18000) OR (id=2 AND action_param1=192287 AND target_type=2 AND event_param1=12000 AND event_param3=12000));
SELECT 'only-three-events',COUNT(*) FROM smart_scripts WHERE entryorguid=95153 AND source_type=0;
SELECT 'dummy',COUNT(*) FROM spell_dummy_trigger WHERE spell_id=192177 AND spell_trigger BETWEEN 192179 AND 192183 AND ``option``=6 AND effectmask=(1 << (spell_trigger-192179)) AND handlemask=8 AND target=0 AND caster=0 AND aura=0 AND bp0=0 AND bp1=0 AND bp2=0;
SELECT 'only-five-links',COUNT(*) FROM spell_dummy_trigger WHERE spell_id=192177;
SELECT 'contact-actions',COUNT(*) FROM areatrigger_actions WHERE entry IN(2736,4889) AND customEntry=0 AND id=0 AND moment=1 AND actionType=0 AND targetFlags=4194306 AND spellId=192184;
SELECT 'script-binding',COUNT(*) FROM areatrigger_scripts WHERE entry IN(7345,9605) AND ScriptName='at_highmountain_shredding_cyclone';
SELECT 'native-templates',COUNT(*) FROM areatrigger_template WHERE ((entry=4889 AND spellId=192179 AND customEntry=9605) OR (entry=2736 AND spellId BETWEEN 192180 AND 192183 AND customEntry=7345)) AND VisualID=49781 AND Radius=2.5 AND isMoving=1 AND Speed>0;
SELECT 'old-templates',COUNT(*) FROM areatrigger_template WHERE entry=2736 AND spellId IN(166311,166327,166332,166344,166345) AND customEntry=7345;
"@
$expected=@("objective`t1","gate`t2","parallel`t2","reward-next`t1","starters`t2","ender`t1","ai`t1","scaling`t1","target`t1","direct-credit`t1","turn-in`t1","turn-in-phase`t1","phase-conditions`t0","enabled`t0","combat`t3","only-three-events`t3","dummy`t5","only-five-links`t5","contact-actions`t2","script-binding`t2","native-templates`t5","old-templates`t5")
if(!$Live){
 $sql+=@"
SELECT 'other-ai',COUNT(*) FROM saved_creature_template a JOIN creature_template b USING(entry) WHERE a.entry<>95153 AND (a.AIName<>b.AIName OR a.ScriptName<>b.ScriptName);
SELECT 'other-smart',COUNT(*) FROM saved_smart_scripts a LEFT JOIN smart_scripts b USING(entryorguid,source_type,id,link) WHERE a.entryorguid<>95153 AND (b.entryorguid IS NULL OR a.event_type<>b.event_type OR a.action_type<>b.action_type OR a.action_param1<>b.action_param1 OR a.target_type<>b.target_type);
SELECT 'other-dummy',COUNT(*) FROM saved_spell_dummy_trigger a LEFT JOIN spell_dummy_trigger b ON a.spell_id=b.spell_id AND a.spell_trigger=b.spell_trigger AND a.option=b.option AND a.aura=b.aura WHERE a.spell_id<>192177 AND (b.spell_id IS NULL OR a.effectmask<>b.effectmask OR a.handlemask<>b.handlemask);
SELECT 'other-actions',COUNT(*) FROM saved_areatrigger_actions a LEFT JOIN areatrigger_actions b USING(entry,id,customEntry) WHERE a.entry NOT IN(2736,4889) AND (b.entry IS NULL OR a.spellId<>b.spellId OR a.targetFlags<>b.targetFlags OR a.moment<>b.moment);
SELECT 'other-scripts',COUNT(*) FROM saved_areatrigger_scripts a LEFT JOIN areatrigger_scripts b USING(entry) WHERE a.entry NOT IN(7345,9605) AND (b.entry IS NULL OR a.ScriptName<>b.ScriptName);
"@
 $expected+=@("other-ai`t0","other-smart`t0","other-dummy`t0","other-actions`t0","other-scripts`t0")
}
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$actual=& 'C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe' --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --skip-column-names --execute=$sql;if($LASTEXITCODE){throw 'Witchqueen SQL test failed'}}finally{$env:MYSQL_PWD=$previous}
if(($actual -join '|') -ne ($expected -join '|')){throw "Witchqueen SQL mismatch: $($actual -join '|')"}
'PASS: native objective/credit/spawn/scaling/handoff; explicit combat targets; five exact dummy links; gated contact actions, retained shared visuals and templates; repeated migration isolation.'
