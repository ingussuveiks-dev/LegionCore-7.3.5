param([switch]$Live)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$migration=Get-Content "$repo/sql/updates/world/2026_10_14_07_stormheim_watcher.sql" -Raw
$sql=''
if(!$Live){
 $tables=@{gameobject_template='entry IN(243819,243820,243822,243823,243802)';creature='guid IN(338528,12864348)';phase_definitions='zoneId=7541 AND entry IN(15,16,650)';conditions='SourceTypeOrReferenceId=23 AND SourceGroup=7541 AND SourceEntry IN(15,16,650)'}
 foreach($table in $tables.Keys){$sql+="CREATE TEMPORARY TABLE saved_$table AS SELECT * FROM $table WHERE $($tables[$table]); CREATE TEMPORARY TABLE $table AS SELECT * FROM saved_$table;`n"}
 $sql+="UPDATE gameobject_template SET Data1=0 WHERE entry IN(243819,243820,243822,243823); DELETE FROM phase_definitions WHERE zoneId=7541 AND entry=650; DELETE FROM conditions WHERE SourceTypeOrReferenceId=23 AND SourceGroup=7541 AND SourceEntry=650; UPDATE creature SET PhaseId='6754' WHERE guid=338528;`n$migration`n$migration`n"
}
$sql+=@'
SELECT 'fragment-loot-bindings',COUNT(*) FROM gameobject_template WHERE entry IN(243819,243820,243822,243823) AND type=3 AND Data0=1691 AND Data1=entry;
SELECT 'four-native-fragments',COUNT(*) FROM quest_objectives WHERE QuestID=39593 AND Type=1 AND Amount=1 AND ((ID=279722 AND ObjectID=128493 AND StorageIndex=3) OR(ID=279724 AND ObjectID=128495 AND StorageIndex=1) OR(ID=279725 AND ObjectID=128496 AND StorageIndex=2) OR(ID=279726 AND ObjectID=128497 AND StorageIndex=4));
SELECT 'native-loot-rows',COUNT(*) FROM gameobject_loot_template WHERE Chance=100 AND QuestRequired=1 AND LootMode=1 AND MinCount=1 AND MaxCount=1 AND ((Entry=243819 AND Item=128493) OR(Entry=243820 AND Item=128495) OR(Entry=243822 AND Item=128496) OR(Entry=243823 AND Item=128497));
SELECT 'head-offer',COUNT(*) FROM gameobject_queststarter WHERE id=243836 AND quest=39593;
SELECT 'head-handin',COUNT(*) FROM gameobject_questender WHERE id=243836 AND quest=39593;
SELECT 'reward-gates',COUNT(*) FROM quest_template_addon WHERE (ID=39593 AND PrevQuestID=39590 AND NextQuestID=39594) OR (ID=39594 AND PrevQuestID=39593);
SELECT 'next-npc-offer',COUNT(*) FROM creature_queststarter WHERE id=96257 AND quest=39594;
SELECT 'body-phase',COUNT(*) FROM creature WHERE guid=338528 AND id=96257 AND map=1220 AND PhaseId='6665';
SELECT 'supported-definition',COUNT(*) FROM phase_definitions WHERE zoneId=7541 AND entry=650 AND phaseId='6665' AND flags=16 AND phasemask=0;
SELECT 'rewarded-condition',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=23 AND SourceGroup=7541 AND SourceEntry=650 AND ConditionTypeOrReference=8 AND ConditionValue1=39593 AND NegativeCondition=0;
SELECT 'night-eyes-preserved',COUNT(*) FROM creature WHERE guid=12864348 AND id=106897 AND PhaseId='6754';
SELECT 'original-shared-phase',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=23 AND SourceGroup=7541 AND SourceEntry=16 AND ConditionTypeOrReference=8 AND ConditionValue1=39592;
SELECT 'head-still-available',COUNT(*) FROM gameobject WHERE guid=114802 AND id=243836 AND PhaseId='6532';
SELECT 'no-instant-completion',COUNT(*) FROM quest_start_scripts WHERE id=39593 AND command=7 AND datalong=39593;
SELECT 'no-script-shortcut',COUNT(*) FROM quest_template WHERE ID=39593 AND StartScript=0 AND CompleteScript=0 AND QuestType=2;
'@
$expected=@("fragment-loot-bindings`t4","four-native-fragments`t4","native-loot-rows`t4","head-offer`t1","head-handin`t1","reward-gates`t2","next-npc-offer`t1","body-phase`t1","supported-definition`t1","rewarded-condition`t1","night-eyes-preserved`t1","original-shared-phase`t1","head-still-available`t1","no-instant-completion`t0","no-script-shortcut`t1")
if(!$Live){
 $sql+=@"
SELECT 'preserved-fragment-data',COUNT(*) FROM gameobject_template a JOIN saved_gameobject_template b USING(entry) WHERE a.flags<>b.flags OR a.displayId<>b.displayId OR a.size<>b.size OR a.Data0<>b.Data0 OR a.Data3<>b.Data3 OR a.Data30<>b.Data30 OR a.ScriptName<>b.ScriptName;
SELECT 'preserved-npc-position',COUNT(*) FROM creature a JOIN saved_creature b USING(guid) WHERE a.position_x<>b.position_x OR a.position_y<>b.position_y OR a.position_z<>b.position_z OR a.npcflag<>b.npcflag;
UPDATE gameobject_template SET Data1=999 WHERE entry=243819;
UPDATE creature SET PhaseId='999' WHERE guid=338528;
$migration
SELECT 'custom-loot',COUNT(*) FROM gameobject_template WHERE entry=243819 AND Data1=999;
SELECT 'custom-spawn-phase',COUNT(*) FROM creature WHERE guid=338528 AND PhaseId='999';
DELETE FROM conditions WHERE SourceTypeOrReferenceId=23 AND SourceGroup=7541 AND SourceEntry=650;
INSERT INTO conditions (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment) VALUES(23,7541,650,0,0,8,0,12345,0,0,0,0,'','custom');
UPDATE creature SET PhaseId='6754' WHERE guid=338528;
$migration
SELECT 'custom-condition',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=23 AND SourceGroup=7541 AND SourceEntry=650 AND ConditionValue1=12345;
SELECT 'custom-condition-no-rephase',COUNT(*) FROM creature WHERE guid=338528 AND PhaseId='6754';
UPDATE conditions SET ConditionValue1=39593 WHERE SourceTypeOrReferenceId=23 AND SourceGroup=7541 AND SourceEntry=650;
UPDATE phase_definitions SET comment='custom' WHERE zoneId=7541 AND entry=650;
$migration
SELECT 'custom-definition-no-rephase',COUNT(*) FROM creature WHERE guid=338528 AND PhaseId='6754';
"@
 $expected+=@("preserved-fragment-data`t0","preserved-npc-position`t0","custom-loot`t1","custom-spawn-phase`t1","custom-condition`t1","custom-condition-no-rephase`t1","custom-definition-no-rephase`t1")
}
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$actual=$sql | & 'C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe' --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --skip-column-names;if($LASTEXITCODE){throw 'Watcher SQL failed'}}finally{$env:MYSQL_PWD=$previous}
if(($actual -join '|') -ne ($expected -join '|')){throw "Watcher SQL mismatch: $($actual -join '|')"}
'PASS: migration twice, four native loot/slot definitions, physical hand-in/next offer, restored-only Yotnar phase, retained head/shared Nighteyes phase, no auto-completion and custom/unrelated preservation.'
