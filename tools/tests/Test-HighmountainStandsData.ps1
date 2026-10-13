param([switch]$Live)
$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$migration=Get-Content "$repo/sql/updates/world/2026_10_12_21_highmountain_stands.sql" -Raw
$sql=''
if(!$Live){
 foreach($table in @('quest_objectives','smart_scripts','spell_scene')){
  $sql+="CREATE TEMPORARY TABLE saved_$table LIKE $table; INSERT INTO saved_$table SELECT * FROM $table; CREATE TEMPORARY TABLE $table LIKE saved_$table; INSERT INTO $table SELECT * FROM saved_$table;`n"
 }
 $sql+="$migration`n$migration`n"
}
$sql+=@"
SELECT 'native-objective',COUNT(*) FROM quest_objectives WHERE ID=284367 AND QuestID=42512 AND Type=0 AND StorageIndex=0 AND ObjectID=107704 AND Amount=1 AND Flags=1;
SELECT 'accept-scene',COUNT(*) FROM smart_scripts WHERE entryorguid=93846 AND source_type=0 AND event_type=19 AND event_param1=42512 AND action_type=85 AND action_param1=213725;
SELECT 'mayla-other-events',COUNT(*) FROM smart_scripts WHERE entryorguid=93846 AND source_type=0 AND (id=1 AND event_type=20 AND event_param1=39780 AND action_type=205 AND action_param1=27);
SELECT 'scene',COUNT(*) FROM spell_scene WHERE MiscValue=1287 AND SceneScriptPackageID=1638 AND PlaybackFlags=25 AND CustomDuration=0 AND ScriptName='scene_highmountain_stands';
SELECT 'unsafe-sql-events',COUNT(*) FROM spell_scene_event WHERE MiscValue=1287;
SELECT 'start',COUNT(*) FROM creature_queststarter WHERE id=93846 AND quest=42512;
SELECT 'end',COUNT(*) FROM creature_questender WHERE id=97407 AND quest=42512;
SELECT 'lasan',COUNT(*) FROM creature WHERE guid=340650 AND id=97407 AND map=1220 AND phaseId='' AND ABS(position_x-3926.54)<0.01 AND ABS(position_y-3924.1)<0.01;
SELECT 'teleport',COUNT(*) FROM spell_target_position WHERE id=197461 AND target_map=1220 AND ABS(target_position_x-3931.76)<0.01 AND ABS(target_position_y-3922.34)<0.01 AND ABS(target_position_z-905.17)<0.01;
SELECT 'poi',COUNT(*) FROM quest_poi_points WHERE QuestID=42512 AND Idx1=1 AND X=3864 AND Y=3888;
SELECT 'links',COUNT(*) FROM quest_template_addon WHERE (ID=42512 AND PrevQuestID=42088 AND NextQuestID=40594) OR (ID=40594 AND PrevQuestID=42512);
SELECT 'terrain-definitions',COUNT(*) FROM phase_definitions WHERE zoneId=7503 AND ((entry=6 AND PreloadMapID=1527) OR (entry=7 AND PreloadMapID=1532));
CREATE TEMPORARY TABLE stages(stage VARCHAR(20)); INSERT INTO stages VALUES('none'),('incomplete'),('complete'),('rewarded');
-- Evaluate the actual OR-of-AND condition rows for the normal story path:
-- Evacuate Snowmane rewarded; the independent quest 41800 absent.
SELECT CONCAT('terrain-',stage,'-',SourceEntry),MAX(met) FROM (
 SELECT stage,SourceEntry,ElseGroup,MIN(IF(NegativeCondition,NOT raw_met,raw_met)) AS met FROM (
  SELECT s.stage,c.*,CASE ConditionTypeOrReference
   WHEN 8 THEN (ConditionValue1=42088 OR (ConditionValue1=42512 AND stage='rewarded'))
   WHEN 28 THEN (ConditionValue1=42512 AND stage='complete')
   WHEN 9 THEN (ConditionValue1=42512 AND stage='incomplete') ELSE 0 END AS raw_met
  FROM stages s CROSS JOIN conditions c WHERE SourceTypeOrReferenceId=23 AND SourceGroup=7503 AND SourceEntry IN(6,7)
 ) checks GROUP BY stage,SourceEntry,ElseGroup
) groups_checked GROUP BY stage,SourceEntry ORDER BY FIELD(stage,'none','incomplete','complete','rewarded'),SourceEntry;
"@
$expected=@("native-objective`t1","accept-scene`t0","mayla-other-events`t1","scene`t1","unsafe-sql-events`t0","start`t1","end`t1","lasan`t1","teleport`t1","poi`t1","links`t2","terrain-definitions`t2")
$expected+=@("terrain-none-6`t0","terrain-none-7`t1","terrain-incomplete-6`t0","terrain-incomplete-7`t1","terrain-complete-6`t1","terrain-complete-7`t0","terrain-rewarded-6`t1","terrain-rewarded-7`t0")
if(!$Live){
 $sql+=@"
SELECT 'other-objectives',COUNT(*) FROM saved_quest_objectives a LEFT JOIN quest_objectives b ON a.ID=b.ID AND a.QuestID=b.QuestID AND a.Type=b.Type AND a.ObjectID=b.ObjectID AND a.Amount=b.Amount AND a.Flags=b.Flags WHERE a.ID<>284367 AND b.ID IS NULL;
SELECT 'other-scenes',COUNT(*) FROM saved_spell_scene a LEFT JOIN spell_scene b ON a.MiscValue=b.MiscValue AND a.SceneScriptPackageID=b.SceneScriptPackageID AND a.ScriptName<=>b.ScriptName WHERE a.MiscValue<>1287 AND b.MiscValue IS NULL;
SELECT 'other-actions',(SELECT COUNT(*) FROM saved_smart_scripts WHERE NOT(entryorguid=93846 AND source_type=0 AND id=2))-(SELECT COUNT(*) FROM smart_scripts WHERE NOT(entryorguid=93846 AND source_type=0 AND id=2));
UPDATE quest_objectives SET Amount=7 WHERE ID=284367;
UPDATE spell_scene SET ScriptName='custom_scene' WHERE MiscValue=1287;
"@
 $sql+="`n$migration`nSELECT 'custom-objective',COUNT(*) FROM quest_objectives WHERE ID=284367 AND Amount=7; SELECT 'custom-scene',COUNT(*) FROM spell_scene WHERE MiscValue=1287 AND ScriptName='custom_scene';"
 $expected+=@("other-objectives`t0","other-scenes`t0","other-actions`t0","custom-objective`t1","custom-scene`t1")
}
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try {
 $env:MYSQL_PWD=$c[3]
 $actual=$sql | & 'C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe' --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --skip-column-names
 if($LASTEXITCODE){throw 'Highmountain Stands SQL test failed'}
} finally {$env:MYSQL_PWD=$previous}
if(($actual -join '|') -ne ($expected -join '|')){throw "Highmountain Stands SQL mismatch: $($actual -join '|')"}
if($Live){'PASS: deployed confrontation objective, scene binding, safe teleport, Lasan handoff and intact/destroyed terrain matrix.'}
else{'PASS: migration twice, native confrontation data, scene binding, terrain matrix, unrelated rows and custom values preserved.'}
