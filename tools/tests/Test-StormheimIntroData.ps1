param([switch]$Live)
$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$migration=Get-Content "$repo/sql/updates/world/2026_10_13_12_stormheim_introduction.sql" -Raw
$sql=''
if(!$Live){
 foreach($table in @('quest_start_scripts','quest_objectives','quest_template_addon','smart_scripts','creature_template','gameobject_template')){
  $sql+="CREATE TEMPORARY TABLE saved_$table LIKE $table; INSERT INTO saved_$table SELECT * FROM $table; CREATE TEMPORARY TABLE $table LIKE saved_$table; INSERT INTO $table SELECT * FROM saved_$table;`n"
 }
 $sql+=@"
UPDATE quest_template_addon SET SpecialFlags=SpecialFlags | 2 WHERE ID IN(39735,39864,38035);
UPDATE quest_objectives SET ObjectID=97549 WHERE ID=280241;
INSERT INTO quest_start_scripts(id,delay,command,datalong,datalong2,dataint,x,y,z,o) VALUES(39735,0,7,39735,0,0,0,0,0,0),(39864,0,7,39864,0,0,0,0,0,0);
UPDATE creature_template SET ScriptName='' WHERE entry IN(96644,96683);
UPDATE gameobject_template SET ScriptName='' WHERE entry IN(244567,244571);
DELETE FROM smart_scripts WHERE source_type=0 AND ((entryorguid=96644 AND id=3) OR (entryorguid=96683 AND id=0));
INSERT INTO smart_scripts(entryorguid,source_type,id,event_type,event_chance,action_type,action_param1,comment) VALUES(96644,0,3,60,100,15,38035,'original shortcut'),(96683,0,0,10,100,15,39864,'original shortcut');
"@
 $sql+="`n$migration`n$migration`n"
}
$sql+=@"
SELECT 'event-locks',COUNT(*) FROM quest_template_addon WHERE ID IN(39735,39864,38035) AND (SpecialFlags & 2)=0;
SELECT 'actors',COUNT(*) FROM creature_template WHERE entry IN(96644,96683) AND AIName='SmartAI' AND ScriptName='npc_stormheim_introduction';
SELECT 'portals',COUNT(*) FROM gameobject_template WHERE (entry=244567 AND Data0=192465 OR entry=244571 AND Data0=192477) AND type=22 AND ScriptName='go_stormheim_summons_portal';
SELECT 'shortcut',COUNT(*) FROM smart_scripts WHERE source_type=0 AND action_type=15 AND ((entryorguid=96644 AND id=3 AND action_param1=38035) OR (entryorguid=96683 AND id=0 AND action_param1=39864));
SELECT 'accept-shortcut',COUNT(*) FROM quest_start_scripts WHERE id IN(39735,39864) AND delay=0 AND command=7 AND datalong=id AND datalong2=0 AND dataint=0;
SELECT 'intro-objectives',COUNT(*) FROM quest_objectives WHERE ((ID=280133 AND QuestID=39735) OR (ID=282101 AND QuestID=39864)) AND Type=0 AND StorageIndex=0 AND ObjectID=97377 AND Amount=1 AND Flags=0 AND Flags2=0;
SELECT 'letter-objectives',COUNT(*) FROM quest_objectives WHERE Type=0 AND Amount=1 AND Flags2=0 AND ((ID=279833 AND QuestID=38035 AND StorageIndex=0 AND ObjectID=96644 AND Flags=0) OR (ID=280241 AND QuestID=38035 AND StorageIndex=1 AND ObjectID=96663 AND Flags=2) OR (ID=279847 AND QuestID=38307 AND StorageIndex=0 AND ObjectID=96683 AND Flags=0) OR (ID=280234 AND QuestID=38307 AND StorageIndex=1 AND ObjectID=96686 AND Flags=2));
SELECT 'items',COUNT(*) FROM quest_template q JOIN quest_template_addon a USING(ID) WHERE a.ProvidedItemCount=1 AND ((q.ID=38035 AND q.StartItem=128687) OR(q.ID=38307 AND q.StartItem=128692));
SELECT 'starters',COUNT(*) FROM creature_queststarter WHERE (id=96644 AND quest IN(38035,38206)) OR (id=96683 AND quest=38307) OR (id=96686 AND quest=39698);
SELECT 'enders',COUNT(*) FROM creature_questender WHERE (id=96644 AND quest IN(39735,38035)) OR (id=96683 AND quest IN(39864,38307));
SELECT 'reward-prerequisites',COUNT(*) FROM quest_template_addon WHERE (ID=38035 AND PrevQuestID=39735) OR (ID=38307 AND PrevQuestID=39864) OR (ID=38206 AND PrevQuestID=38035) OR (ID=39698 AND PrevQuestID=38307);
SELECT 'phase-definitions',COUNT(*) FROM phase_definitions WHERE zoneId=7502 AND ((entry=35 AND phaseId=6648) OR (entry=38 AND phaseId=6638));
SELECT 'phase-conditions',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=23 AND SourceGroup=7502 AND ElseGroup=0 AND ((SourceEntry=35 AND ((ConditionTypeOrReference=8 AND ConditionValue1=38035 AND NegativeCondition=1) OR (ConditionTypeOrReference=14 AND ConditionValue1=39735 AND NegativeCondition=1))) OR (SourceEntry=38 AND ((ConditionTypeOrReference=8 AND ConditionValue1=38307 AND NegativeCondition=1) OR (ConditionTypeOrReference=14 AND ConditionValue1=39864 AND NegativeCondition=1))));
SELECT 'portal-spawns',COUNT(*) FROM gameobject WHERE map=1220 AND zoneId=7502 AND ABS(position_x+830.957)<0.01 AND ABS(position_y-4655.49)<0.01 AND ABS(position_z-749.611)<0.01 AND ((guid=127656 AND id=244567 AND PhaseId='6648') OR (guid=127742 AND id=244571 AND PhaseId='6638'));
SELECT 'destinations',COUNT(*) FROM spell_target_position WHERE (id=192465 AND target_map=0 AND ABS(target_position_x+8479.7)<0.01 AND ABS(target_position_y-1402.06)<0.01 AND ABS(target_position_z-205)<0.01) OR(id=192477 AND target_map=1 AND ABS(target_position_x-1496.85)<0.01 AND ABS(target_position_y+5199.62)<0.01 AND ABS(target_position_z-11.24)<0.01);
SELECT 'next-paths',COUNT(*) FROM smart_scripts WHERE source_type=0 AND event_type=54 AND ((entryorguid=96644 AND id=4) OR(entryorguid=96686 AND id=4));
"@
$expected=@("event-locks`t3","actors`t2","portals`t2","shortcut`t0","accept-shortcut`t0","intro-objectives`t2","letter-objectives`t4","items`t2","starters`t4","enders`t4","reward-prerequisites`t4","phase-definitions`t2","phase-conditions`t4","portal-spawns`t2","destinations`t2","next-paths`t2")
if(!$Live){
 $sql+=@"
SELECT 'other-locks',COUNT(*) FROM quest_template_addon a JOIN saved_quest_template_addon b USING(ID) WHERE a.ID NOT IN(39735,39864,38035) AND a.SpecialFlags<>b.SpecialFlags;
SELECT 'other-objectives',COUNT(*) FROM quest_objectives a JOIN saved_quest_objectives b USING(ID) WHERE a.ID<>280241 AND a.ObjectID<>b.ObjectID;
SELECT 'other-start-scripts',(SELECT COUNT(*) FROM saved_quest_start_scripts WHERE NOT(id IN(39735,39864) AND delay=0 AND command=7 AND datalong=id AND datalong2=0 AND dataint=0))-(SELECT COUNT(*) FROM quest_start_scripts);
SELECT 'other-actors',COUNT(*) FROM creature_template a JOIN saved_creature_template b USING(entry) WHERE a.entry NOT IN(96644,96683) AND a.ScriptName<>b.ScriptName;
SELECT 'other-portals',COUNT(*) FROM gameobject_template a JOIN saved_gameobject_template b USING(entry) WHERE a.entry NOT IN(244567,244571) AND a.ScriptName<>b.ScriptName;
UPDATE creature_template SET ScriptName='custom' WHERE entry=96644;
UPDATE gameobject_template SET ScriptName='custom' WHERE entry=244567;
UPDATE quest_template_addon SET SpecialFlags=64 WHERE ID=39735;
UPDATE quest_objectives SET ObjectID=97549,Amount=7 WHERE ID=280241;
"@
 $sql+="$migration`nSELECT 'custom-script',COUNT(*) FROM creature_template WHERE entry=96644 AND ScriptName='custom'; SELECT 'custom-portal',COUNT(*) FROM gameobject_template WHERE entry=244567 AND ScriptName='custom'; SELECT 'other-bits',COUNT(*) FROM quest_template_addon WHERE ID=39735 AND SpecialFlags=64; SELECT 'custom-amount',COUNT(*) FROM quest_objectives WHERE ID=280241 AND ObjectID=97549 AND Amount=7;"
 $expected+=@("other-locks`t0","other-objectives`t0","other-start-scripts`t0","other-actors`t0","other-portals`t0","custom-script`t1","custom-portal`t1","other-bits`t1","custom-amount`t1")
}
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try {
 $env:MYSQL_PWD=$c[3]
 $actual=$sql | & 'C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe' --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --skip-column-names
 if($LASTEXITCODE){throw 'Stormheim SQL test failed'}
} finally {$env:MYSQL_PWD=$previous}
if(($actual -join '|') -ne ($expected -join '|')){throw "Stormheim SQL mismatch: $($actual -join '|')"}
'PASS: faction objectives/items/handoff, native portal/spawn/destination data, removed shortcuts/event locks, existing rounds paths; temporary replay twice and preservation checks or live validation.'
