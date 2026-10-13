param([switch]$Live)
$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$migration=Get-Content "$repo/sql/updates/world/2026_10_13_03_highmountain_lair.sql" -Raw
$sql=''
if(!$Live){
 foreach($table in @('quest_objectives','conditions')){
  $sql+="CREATE TEMPORARY TABLE saved_$table LIKE $table; INSERT INTO saved_$table SELECT * FROM $table; CREATE TEMPORARY TABLE $table LIKE saved_$table; INSERT INTO $table SELECT * FROM saved_$table;`n"
 }
 $sql+=@"
DELETE FROM quest_objectives WHERE ID=279938;
DELETE FROM conditions WHERE SourceTypeOrReferenceId=23 AND SourceGroup=7546 AND SourceEntry=30;
INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,ElseGroup,ConditionTypeOrReference,ConditionValue1,NegativeCondition) VALUES(23,7546,30,0,8,39781,0),(23,7546,30,0,28,39781,0),(23,7546,30,1,8,42454,1);
"@
 $sql+="`n$migration`n$migration`n"
}
$sql+=@"
SELECT 'required',COUNT(*) FROM quest_objectives WHERE ID=279938 AND QuestID=39781 AND Type=1 AND StorageIndex=0 AND ObjectID=128765 AND Amount=1 AND Flags=1 AND Flags2=1;
SELECT 'optional',COUNT(*) FROM quest_objectives WHERE ID=279973 AND QuestID=39781 AND Type=3 AND StorageIndex=1 AND ObjectID=97304 AND Amount=1 AND Flags=4;
SELECT 'loot',COUNT(*) FROM creature_loot_template WHERE Entry=91007 AND Item=128765 AND Chance=100 AND QuestRequired=1 AND LootMode=7 AND MinCount=1 AND MaxCount=1;
SELECT 'boss',COUNT(*) FROM creature_template WHERE entry=91007 AND lootid=91007 AND ScriptName='boss_dargrul_the_underking';
SELECT 'hammer',COUNT(*) FROM gameobject WHERE guid=2167781 AND id=250548 AND map=1458 AND PhaseId='5645' AND spawnMask=8388870;
SELECT 'ender',COUNT(*) FROM gameobject_questender WHERE id=250548 AND quest=39781;
SELECT 'next',COUNT(*) FROM gameobject_queststarter WHERE id=250548 AND quest=42454;
SELECT 'chain',COUNT(*) FROM quest_template_addon WHERE (ID=39781 AND PrevQuestID=39780 AND NextQuestID=42454) OR (ID=42454 AND PrevQuestID=39781);
SELECT 'navarrogg',COUNT(*) FROM smart_scripts WHERE entryorguid=97304 AND source_type=0 AND event_type=64 AND action_type=33 AND action_param1=97304 AND target_type=7;
CREATE TEMPORARY TABLE stages(stage INT);INSERT INTO stages VALUES(0),(1),(2),(3);
SELECT CONCAT('phase-',lair,'-',followup),MAX(met) FROM (
 SELECT lair,followup,ElseGroup,MIN(IF(NegativeCondition,NOT raw_met,raw_met)) met FROM (
  SELECT a.stage lair,b.stage followup,c.*,CASE ConditionTypeOrReference
   WHEN 8 THEN IF(ConditionValue1=39781,a.stage=3,b.stage=3)
   WHEN 28 THEN a.stage=2 ELSE 0 END raw_met
  FROM stages a CROSS JOIN stages b CROSS JOIN conditions c
  WHERE SourceTypeOrReferenceId=23 AND SourceGroup=7546 AND SourceEntry=30
 ) evaluated GROUP BY lair,followup,ElseGroup
) grouped GROUP BY lair,followup ORDER BY lair,followup;
"@
$expected=@("required`t1","optional`t1","loot`t1","boss`t1","hammer`t1","ender`t1","next`t1","chain`t2","navarrogg`t1")
foreach($lair in 0..3){foreach($follow in 0..3){$value=[int]($lair -ge 2 -and $follow -ne 3);$expected+="phase-$lair-$follow`t$value"}}
if(!$Live){
 $sql+=@"
SELECT 'other-objectives',(SELECT COUNT(*) FROM saved_quest_objectives WHERE ID<>279938)-(SELECT COUNT(*) FROM quest_objectives WHERE ID<>279938);
SELECT 'other-conditions',(SELECT COUNT(*) FROM saved_conditions WHERE NOT(SourceTypeOrReferenceId=23 AND SourceGroup=7546 AND SourceEntry=30))-(SELECT COUNT(*) FROM conditions WHERE NOT(SourceTypeOrReferenceId=23 AND SourceGroup=7546 AND SourceEntry=30));
UPDATE quest_objectives SET Amount=7 WHERE ID=279938;
"@
 $sql+="$migration`nSELECT 'custom-amount',COUNT(*) FROM quest_objectives WHERE ID=279938 AND Amount=7;"
 $expected+=@("other-objectives`t0","other-conditions`t0","custom-amount`t1")
}
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try {
 $env:MYSQL_PWD=$c[3]
 $actual=$sql | & 'C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe' --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --skip-column-names
 if($LASTEXITCODE){throw 'Lair SQL test failed'}
} finally {$env:MYSQL_PWD=$previous}
if(($actual -join '|') -ne ($expected -join '|')){throw "Lair SQL mismatch: $($actual -join '|')"}
'PASS: Lair required collection, optional talk, native loot and quest givers, all 16 phase states, idempotence and custom-data preservation.'
