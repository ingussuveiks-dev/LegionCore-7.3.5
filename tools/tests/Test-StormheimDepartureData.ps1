param([switch]$Live)
$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$migration=Get-Content "$repo/sql/updates/world/2026_10_13_14_stormheim_departure.sql" -Raw
$sql=''
if(!$Live){
 foreach($table in @('smart_scripts','creature_template','quest_template','quest_template_addon','scenario_data','conditions','quest_start_scripts')){
  $sql+="CREATE TEMPORARY TABLE saved_$table LIKE $table; INSERT INTO saved_$table SELECT * FROM $table; CREATE TEMPORARY TABLE $table LIKE saved_$table; INSERT INTO $table SELECT * FROM saved_$table;`n"
 }
 $sql+="INSERT INTO quest_start_scripts(id,delay,command,datalong,datalong2,dataint,x,y,z,o) VALUES(39800,0,7,39800,0,0,0,0,0,0),(39801,0,7,39801,0,0,0,0,0,0);`n"
 $sql+="UPDATE quest_template SET Flags=Flags | 65544 WHERE ID=39801; UPDATE quest_template_addon SET SpecialFlags=SpecialFlags | 2 WHERE ID IN(39800,39801); UPDATE scenario_data SET DifficultyID=0,Team=0,LfgDungeonID=0 WHERE MapID=1475 AND ScenarioID IN(824,826);`n$migration`n$migration`n"
}
$sql+=@"
SELECT 'quest-flags',COUNT(*) FROM quest_template WHERE ID IN(39800,39801) AND Flags=34603008;
SELECT 'event-locks',COUNT(*) FROM quest_template_addon WHERE ID IN(39800,39801) AND (SpecialFlags & 2)=0;
SELECT 'native-objectives',COUNT(*) FROM quest_objectives WHERE Type=0 AND StorageIndex=0 AND ObjectID=113320 AND Amount=1 AND Flags=0 AND Flags2=0 AND ((QuestID=39800 AND ID=286405) OR(QuestID=39801 AND ID=286401));
SELECT 'scenario-rows',COUNT(*) FROM scenario_data WHERE MapID=1475 AND DifficultyID=12 AND Class=0 AND ((ScenarioID=824 AND Team=67 AND LfgDungeonID=998) OR(ScenarioID=826 AND Team=469 AND LfgDungeonID=969));
SELECT 'native-entrances',COUNT(*) FROM lfg_entrances WHERE (dungeonId=969 AND ABS(position_x-4342.5)<0.01 AND ABS(position_y-2592.85)<0.01 AND ABS(position_z-39.1087)<0.01) OR(dungeonId=998 AND ABS(position_x-4657.47)<0.01 AND ABS(position_y-2859.09)<0.01 AND ABS(position_z-23.5792)<0.01);
SELECT 'departure-flags',COUNT(*) FROM creature_template WHERE entry IN(96644,96686) AND ScriptName='npc_stormheim_introduction' AND (npcflag & 3)=3;
SELECT 'report',COUNT(*) FROM creature_template WHERE entry=94419 AND ScriptName='npc_stormheim_fleet_report' AND AIName='' AND (npcflag & 1)<>0;
SELECT 'unsafe-departures',COUNT(*) FROM smart_scripts WHERE source_type=0 AND ((entryorguid=96663 AND event_type=19 AND event_param1=39800 AND action_type=62) OR(entryorguid=96686 AND ((event_type=19 AND event_param1=39801) OR(event_type=62 AND event_param1=19342) OR(event_type=61 AND action_type=85 AND action_param1=228560))));
SELECT 'unsafe-report',COUNT(*) FROM smart_scripts WHERE entryorguid=94419 AND ((source_type=0 AND id BETWEEN 0 AND 2) OR(source_type=9 AND id BETWEEN 0 AND 6));
SELECT 'menu-conditions',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=15 AND SourceEntry=0 AND SourceId=0 AND ElseGroup=0 AND ConditionTypeOrReference=9 AND ConditionTarget=0 AND NegativeCondition=0 AND ((SourceGroup=19355 AND ConditionValue1=39800) OR(SourceGroup=19342 AND ConditionValue1=39801));
SELECT 'quest-links',COUNT(*) FROM quest_template_addon WHERE (ID=39800 AND PrevQuestID=38206) OR(ID=39801 AND PrevQuestID=39698);
SELECT 'accept-shortcuts',COUNT(*) FROM quest_start_scripts WHERE id IN(39800,39801) AND delay=0 AND command=7 AND datalong=id AND datalong2=0 AND dataint=0 AND x=0 AND y=0 AND z=0 AND o=0;
"@
$expected=@("quest-flags`t2","event-locks`t2","native-objectives`t2","scenario-rows`t2","native-entrances`t2","departure-flags`t2","report`t1","unsafe-departures`t0","unsafe-report`t0","menu-conditions`t2","quest-links`t2","accept-shortcuts`t0")
if(!$Live){
 $sql+=@"
SELECT 'other-flags',COUNT(*) FROM quest_template a JOIN saved_quest_template b USING(ID) WHERE a.ID<>39801 AND a.Flags<>b.Flags;
SELECT 'other-event-locks',COUNT(*) FROM quest_template_addon a JOIN saved_quest_template_addon b USING(ID) WHERE a.ID NOT IN(39800,39801) AND a.SpecialFlags<>b.SpecialFlags;
SELECT 'other-npcs',COUNT(*) FROM creature_template a JOIN saved_creature_template b USING(entry) WHERE a.entry NOT IN(94419,96686) AND (a.npcflag<>b.npcflag OR a.ScriptName<>b.ScriptName OR a.AIName<>b.AIName);
SELECT 'other-smart-count',COUNT(*) FROM saved_smart_scripts a LEFT JOIN smart_scripts b USING(entryorguid,source_type,id,link) WHERE a.entryorguid NOT IN(94419,96663,96686) AND b.entryorguid IS NULL;
SELECT 'other-start-count',(SELECT COUNT(*) FROM saved_quest_start_scripts WHERE NOT(id IN(39800,39801) AND delay=0 AND command=7 AND datalong=id AND datalong2=0 AND dataint=0 AND x=0 AND y=0 AND z=0 AND o=0))-(SELECT COUNT(*) FROM quest_start_scripts);
UPDATE creature_template SET ScriptName='custom' WHERE entry=94419;
"@
 $sql+="$migration`nSELECT 'custom-report',COUNT(*) FROM creature_template WHERE entry=94419 AND ScriptName='custom';"
 $expected+=@("other-flags`t0","other-event-locks`t0","other-npcs`t0","other-smart-count`t0","other-start-count`t0","custom-report`t1")
}
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try {
 $env:MYSQL_PWD=$c[3]
 $actual=$sql | & 'C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe' --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --skip-column-names
 if($LASTEXITCODE){throw 'Departure SQL test failed'}
} finally {$env:MYSQL_PWD=$previous}
if(($actual -join '|') -ne ($expected -join '|')){throw "Departure SQL mismatch: $($actual -join '|')"}
'PASS: native objectives/entrances and faction scenarios, completion locks removed, unsafe shortcuts removed, visible conditioned departure menus, idempotent migration and unrelated/custom data preserved.'
